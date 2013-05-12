#ifndef __IHT_TRIE_HASHTRIE_IMPL_HH__
#define __IHT_TRIE_HASHTRIE_IMPL_HH__

#include "node.hh"
#include "ref.hh"
#include "../string.hh"
#include "../allocator/fixed_allocator.hh"
#include "../ipc/shared_memory.hh"
#include <inttypes.h>
#include <algorithm>
#include <string.h>
#include <assert.h>

namespace iht {
  namespace trie {
    static const char MAGIC[] = "IHT-0.0.1";
    
    typedef uint32_t md_t;

    class HashTrieImpl {
    public:
      // TODO: HashTrieImplのaliasにする？
      class View {
      public:
        String find(const String & key) const {
          return String();
        }

        template <class Callback>
        void foreach(Callback & callback) const {
        }
      };

    private:
      struct Header {
        char magic[sizeof(MAGIC)];
        uint32_t shm_size;
        md_t root;
      };
      static const uint32_t HEADER_SIZE = sizeof(Header);
      
    public:
      HashTrieImpl(ipc::SharedMemory & shm)
        : shm_size_(shm.size()),
          h_(shm.ptr<Header>()),
          alc_(shm.ptr<void>(HEADER_SIZE), std::max(0, static_cast<int32_t>(shm.size() - HEADER_SIZE)))
      {
      }

      operator bool() const { return alc_ && h_; }

      void init() {
        if(*this) {
          alc_.init();
          
          memcpy(h_->magic, MAGIC, sizeof(MAGIC));
          h_->shm_size = shm_size_;
          h_->root = alc_.allocate(sizeof(RootNode));
          if(h_->root == 0) {
            h_ = NULL;
            return;
          }

          RootNode * node = new (alc_.ptr<RootNode>(h_->root)) RootNode(alc_);
          if(! *node) {
            h_ = NULL;
            return;
          }
        }
      }
      
      void initOnce() {
        if(*this) {
          if(memcmp(h_->magic, MAGIC, sizeof(MAGIC)) != 0 ||
             shm_size_ != h_->shm_size) {
            init();
          }
        }
      }

      void store(const String & key, const String & value) {
        md_t old;
        for(;;) {
          Ref<RootNode> root(h_->root, alc_);
          if(! root) {
            continue;
          }
          
          old = h_->root;
          h_->root = root.ptr()->store(key, value, alc_);
          assert(h_->root != 0);
          break;
        }
        RootNode::releaseNode(old, alc_);
      }

      View view() const {
        return View();
      }

      bool isMember(const String & key) const {
        return false;
      }

      size_t size() {
        for(;;) {
          Ref<RootNode> root(h_->root, alc_);
          if(root) {
            return root.ptr()->count();
          }
        }
      }

    private:
      const size_t shm_size_;
      Header * h_;
      allocator::FixedAllocator alc_;
    };
  }
}

#endif
