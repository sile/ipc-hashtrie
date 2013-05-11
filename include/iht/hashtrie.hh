#ifndef __IHT_HASHTRIE_HH__
#define __IHT_HASHTRIE_HH__

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
        impl_.init_once();
      }
    }

    operator bool() const { return shm_ && impl_; }

    void init() {
      if(*this) {
        impl_.init();
      }
    }

  private:
    ipc::SharedMemory shm_;
    trie::HashTrieImpl impl_;
  };
}

#endif
