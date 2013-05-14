#ifndef __MAP_HH__
#define __MAP_HH__

#include <string>
#include <sys/types.h>
#include <iht/hashtrie.hh>
#include <tr1/unordered_map>

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

class HashMap : public Map {
public:
  virtual void store(const std::string & key, const std::string & value) {
    impl_[key] = value;
  }

  virtual bool find(const std::string & key, std::string & value) {
    hashmap_t::const_iterator it = impl_.find(key);
    
    bool exists;
    if(it == impl_.end()) {
      exists = false;
    } else {
      value = it->second;
      exists =  true;
    }

    return exists;
  }
  
  virtual bool member(const std::string & key) {
    return impl_.find(key) != impl_.end();
  }

  virtual size_t size() { 
    return impl_.size(); 
  }

  virtual unsigned totalValueLength() {
    unsigned total = 0;

    hashmap_t::const_iterator it = impl_.begin();
    for(; it != impl_.end(); ++it) {
      total += it->second.size();
    }
    
    return total;
  }
  
  virtual View * createView();
  
private:
  hashmap_t impl_;
};

class HashView : public View {
public:
  HashView(HashMap & map) : map_(map) {}
  
  virtual bool find(const std::string & key, std::string & value) { return map_.find(key, value); }
  virtual bool member(const std::string & key) { return map_.member(key); }
  virtual size_t size() { return map_.size(); }
  virtual unsigned totalValueLength() { return map_.totalValueLength(); }
  
private:
  HashMap & map_;
};

View * HashMap::createView() {
  return new HashView(*this);
}

class TrieMap : public Map {
public:
  TrieMap() : impl_(1024*1024*250) {
  }
  
  virtual void store(const std::string & key, const std::string & value) {
    impl_.store(key, value);
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
};

class TrieView : public View {
public:
  TrieView(TrieMap & map) : map_(map), v_(map.getTrie()) {}
  
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

  struct Callback {
    Callback() : sum(0) {}
    void operator()(const iht::String & key, const iht::String & val) {
      sum += val.size();
    }
    unsigned sum;
  };
  
  virtual unsigned totalValueLength() {
    v_.updateIfNeed();
    Callback callback;
    v_.foreach(callback);
    return callback.sum;
  }
  
private:
  TrieMap & map_;
  iht::View v_;
};

View * TrieMap::createView() {
  return new TrieView(*this);
}

#endif
