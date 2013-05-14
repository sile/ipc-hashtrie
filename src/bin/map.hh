#ifndef __MAP_HH__
#define __MAP_HH__

#include <string>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

class Map {
public:
  virtual ~Map() {}

  virtual void store(const std::string & key, const std::string & value) = 0;
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
  
private:
  hashmap_t impl_;
  pthread_mutex_t mtx_;
};


#endif
