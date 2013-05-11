#ifndef __IHT_HASHTRIE_HH__
#define __IHT_HASHTRIE_HH__

#include "string.hh"
#include "ipc/shared_memory.hh"
#include "trie/hashtrie_impl.hh"
#include <string>
#include <sys/types.h>

namespace iht {
  class HashTrie {
  public:
    HashTrie(size_t shm_size)
      : shm_(shm_size),
        impl_(shm_)
    {
      init();
    }

    HashTrie(size_t shm_size, const std::string & filepath, mode_t mode=0660)
      : shm_(filepath, shm_size, mode),
        impl_(shm_)
    {
      if(*this) {
        impl_.initOnce();
      }
    }

    operator bool() const { return shm_ && impl_; }

    void init() {
      if(*this) {
        impl_.init();
      }
    }

    void store(const String & key, const String & value) {
      // TODO: acquire lock
      impl_.store(key, value);
    }

    void view() const {
      // TODO
    }

    bool find(const String & key, std::string & value) const {
      trie::HashTrieImpl::View view = impl_.view();
      const String & v = view.find(key);
      if(! v) {
        return false;
      } else {
        value.assign(v.data(), v.size());
        return true;
      }
    }
    
    bool isMember(const String & key) const {
      return impl_.isMember(key);
    }

    size_t size() const {
      return impl_.size();
    }

    template <class Callback>
    void foreach(Callback & callback) const {
      trie::HashTrieImpl::View view = impl_.view();
      view.foreach(callback);
    }

  private:
    ipc::SharedMemory shm_;
    trie::HashTrieImpl impl_;
  };
}

#endif
