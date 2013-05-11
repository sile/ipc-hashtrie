#ifndef __IHT_TRIE_HASHTRIE_IMPL_HH__
#define __IHT_TRIE_HASHTRIE_IMPL_HH__

#include "../allocator/fixed_allocator.hh"
#include "../ipc/shared_memory.hh"
#include <inttypes.h>
#include <algorithm>

namespace iht {
  namespace trie {
    class HashTrieImpl {
    public:
      struct Header {
      };
      static const uint32_t HEADER_SIZE = sizeof(Header);
      
    public:
      HashTrieImpl(ipc::SharedMemory & shm)
        : shm_size_(shm.size()),
          header_(shm.ptr<Header>()),
          alc_(shm.ptr<void>(HEADER_SIZE), std::max(0, static_cast<int32_t>(shm.size() - HEADER_SIZE)))
      {
      }

      operator bool() const { return alc_ && header_; }

      void init() {
      }
      
      void init_once() {
      }

    private:
      const size_t shm_size_;
      Header * header_;
      allocator::FixedAllocator alc_;
    };
  }
}

#endif
