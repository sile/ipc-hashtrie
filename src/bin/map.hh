#ifndef __MAP_HH__
#define __MAP_HH__

#include <string>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <iht/hashtrie.hh>

class View;

class Map {
public:
  virtual ~Map() {}
  
  virtual void store(const std::string & key, const std::string & value) = 0;
  virtual bool find(const std::string & key, std::string & value) = 0;
  virtual bool member(const std::string & key) = 0;
  virtual size_t size() = 0;
  virtual unsigned totalValueLength() = 0;
  
  virtual View * createView() = 0;
};

class View {
public:
  virtual ~View() {}

  virtual bool find(const std::string & key, std::string & value) = 0;
  virtual bool member(const std::string & key) = 0;
  virtual size_t size() = 0;
  virtual unsigned totalValueLength() = 0;
};

typedef std::tr1::unordered_map<std::string, std::string> hashmap_t;

class MutexMap : public Map {
public:
  MutexMap() {
    int ret = pthread_mutex_init(&mtx_, NULL);
    assert(ret == 0);
  }

  ~MutexMap() {
    int ret = pthread_mutex_destroy(&mtx_);
    assert(ret == 0);
  }

  virtual void store(const std::string & key, const std::string & value) {
    pthread_mutex_lock(&mtx_);
    impl_[key] = value;
    pthread_mutex_unlock(&mtx_);
  }

  virtual bool find(const std::string & key, std::string & value) {
    pthread_mutex_lock(&mtx_);
    hashmap_t::const_iterator it = impl_.find(key);
    
    bool exists;
    if(it == impl_.end()) {
      exists = false;
    } else {
      value = it->second;
      exists =  true;
    }
    pthread_mutex_unlock(&mtx_);

    return exists;
  }
  
  virtual bool member(const std::string & key) {
    pthread_mutex_lock(&mtx_);
    bool exists = impl_.find(key) != impl_.end();
    pthread_mutex_unlock(&mtx_);

    return exists;
  }

  virtual size_t size() { 
    pthread_mutex_lock(&mtx_);
    size_t size = impl_.size(); 
    pthread_mutex_unlock(&mtx_);
    return size;
  }

  virtual unsigned totalValueLength() {
    unsigned total = 0;

    pthread_mutex_lock(&mtx_);
    hashmap_t::const_iterator it = impl_.begin();
    for(; it != impl_.end(); ++it) {
      total += it->second.size();
    }
    pthread_mutex_unlock(&mtx_);
    
    return total;
  }
  
  virtual View * createView();
  
private:
  hashmap_t impl_;
  pthread_mutex_t mtx_;
};

class MutexView : public View {
public:
  MutexView(MutexMap & map) : map_(map) {}
  
  virtual bool find(const std::string & key, std::string & value) { return map_.find(key, value); }
  virtual bool member(const std::string & key) { return map_.member(key); }
  virtual size_t size() { return map_.size(); }
  virtual unsigned totalValueLength() { return map_.totalValueLength(); }
  
private:
  MutexMap & map_;
};

View * MutexMap::createView() {
  return new MutexView(*this);
}

class RWLockMap : public Map {
public:
  RWLockMap() {
    int ret = pthread_rwlock_init(&lock_, NULL);
    assert(ret == 0);
  }

  ~RWLockMap() {
    int ret = pthread_rwlock_destroy(&lock_);
    assert(ret == 0);
  }

  virtual void store(const std::string & key, const std::string & value) {
    pthread_rwlock_wrlock(&lock_);
    impl_[key] = value;
    pthread_rwlock_unlock(&lock_);
  }

  virtual bool find(const std::string & key, std::string & value) {
    pthread_rwlock_rdlock(&lock_);
    hashmap_t::const_iterator it = impl_.find(key);
    
    bool exists;
    if(it == impl_.end()) {
      exists = false;
    } else {
      value = it->second;
      exists =  true;
    }
    pthread_rwlock_unlock(&lock_);

    return exists;
  }
  
  virtual bool member(const std::string & key) {
    pthread_rwlock_rdlock(&lock_);
    bool exists = impl_.find(key) != impl_.end();
    pthread_rwlock_unlock(&lock_);

    return exists;
  }

  virtual size_t size() { 
    pthread_rwlock_rdlock(&lock_);
    size_t size = impl_.size(); 
    pthread_rwlock_unlock(&lock_);
    return size;
  }

  virtual unsigned totalValueLength() {
    unsigned total = 0;

    pthread_rwlock_rdlock(&lock_);
    hashmap_t::const_iterator it = impl_.begin();
    for(; it != impl_.end(); ++it) {
      total += it->second.size();
    }
    pthread_rwlock_unlock(&lock_);
    
    return total;
  }
  
  virtual View * createView();
  
private:
  hashmap_t impl_;
  pthread_rwlock_t lock_;
};

class RWLockView : public View {
public:
  RWLockView(RWLockMap & map) : map_(map) {}
  
  virtual bool find(const std::string & key, std::string & value) { return map_.find(key, value); }
  virtual bool member(const std::string & key) { return map_.member(key); }
  virtual size_t size() { return map_.size(); }
  virtual unsigned totalValueLength() { return map_.totalValueLength(); }
  
private:
  RWLockMap & map_;
};

View * RWLockMap::createView() {
  return new RWLockView(*this);
}

class PersistentMap : public Map {
public:
  PersistentMap() : impl_(1024*1024*250) {
    int ret = pthread_mutex_init(&mtx_, NULL);
    assert(ret == 0);
  }

  ~PersistentMap() {
    int ret = pthread_mutex_destroy(&mtx_);
    assert(ret == 0);
  }
  
  virtual void store(const std::string & key, const std::string & value) {
    pthread_mutex_lock(&mtx_);
    impl_.store(key, value);
    pthread_mutex_unlock(&mtx_);
  }

  virtual bool find(const std::string & key, std::string & value) {
    iht::View v(impl_);
    iht::String s = v.find(key);
    if(s) {
      value.assign(s.data(), s.size());
      return true;
    } else {
      return false;
    }
  }

  virtual bool member(const std::string & key) {
    iht::View v(impl_);
    return v.find(key);
  }
  
  virtual size_t size() { return impl_.size(); }
  
  virtual unsigned totalValueLength() { return 0; /* TODO */ }

  virtual View * createView();
  
public:
  iht::HashTrie & getTrie() { return impl_; }
  
private:
  iht::HashTrie impl_;
  pthread_mutex_t mtx_;
};

class PersistentView : public View {
public:
  PersistentView(PersistentMap & map) : map_(map), v_(map.getTrie()) {}
  
  virtual bool find(const std::string & key, std::string & value) {
    v_.updateIfNeed();
    iht::String s = v_.find(key);
    if(s) {
      value.assign(s.data(), s.size());
      return true;
    } else {
      return false;
    }
  }
  virtual bool member(const std::string & key) {
    v_.updateIfNeed();
    return v_.find(key);
  }
  virtual size_t size() {
    return map_.size();
  }
  virtual unsigned totalValueLength() {
    return 0; // TODO:
  }
  
private:
  PersistentMap & map_;
  iht::View v_;
};

View * PersistentMap::createView() {
  return new PersistentView(*this);
}


#endif
