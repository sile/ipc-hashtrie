#ifndef __IHT_TRIE_REF_HH__
#define __IHT_TRIE_REF_HH__

#include "../atomic/atomic.hh"
#include "../allocator/fixed_allocator.hh"
#include <inttypes.h>

namespace iht {
  namespace trie {
    template <class T>
    class Ref {
    public:
      Ref(allocator::FixedAllocator & alc)
        : md_(0),
          alc_(alc)
      {
        md_ = alc_.allocate(sizeof(T));
      }
      
      Ref(uint32_t md, allocator::FixedAllocator & alc)
        : md_(0),
          alc_(alc)
      {
        if(md != 0 && alc.dup(md)) {
          md_ = md;
        }
      }

      ~Ref() {
        if(md_) {
          if(! alc_.release(md_)) {
            // TODO: throw exception
          }
        }
      }

      operator bool() const { return md_ != 0; }
      
      T fetch() const { return atomic::fetch(ptr()); }
      T* ptr() const { return alc_.ptr<T>(md_); }
      uint32_t md() const { return md_; }

    private:
      uint32_t md_;
      allocator::FixedAllocator & alc_;
    };
  }
}

#endif
