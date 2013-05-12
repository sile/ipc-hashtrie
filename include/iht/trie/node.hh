#ifndef __IHT_TRIE_NODE_HH__
#define __IHT_TRIE_NODE_HH__

#include "ref.hh"
#include "../allocator/fixed_allocator.hh"
#include "../string.hh"
#include <inttypes.h>
#include <string.h>

// TODO:
#include <iostream>

namespace iht {
  namespace trie {
    class Cons {
      typedef uint32_t md_t;
      typedef allocator::FixedAllocator Alc;

    public:
      static md_t cons(Alc & alc, const String & key, const String & value, md_t cdr) {
        md_t md = alc.allocate(sizeof(Cons) + key.size() + value.size());
        assert(md != 0);

        Cons * c = alc.ptr<Cons>(md);
        c->next_ = cdr;
        c->key_size_ = key.size();
        c->val_size_ = value.size();
        memcpy(c->data_, key.data(), key.size());
        memcpy(c->data_ + key.size(), value.data(), value.size());
        
        return md;
      }

      void release(Alc & alc) {
        if(next_) {
          if(alc.undup(next_)) {
            alc.ptr<Cons>(next_)->release(alc);
            alc.release_no_undup(next_);
          }
        }
      }

      String key() const {
        return String(data_, key_size_);
      }

      String value() const {
        return String(data_+key_size_, val_size_);
      }

      md_t cdr() const { return next_; }

    private:
      md_t next_;
      uint32_t key_size_;
      uint32_t val_size_;
      char data_[0];
    };

    class List {
      typedef uint32_t md_t;
      typedef allocator::FixedAllocator Alc;
      
    public:
      static md_t insert(md_t list, const String & key, const String & value, bool & new_key, Alc & alc) {
        if(list == 0) {
          new_key = true;
          // TODO: key が存在するかをチェックして、存在しない場合は先頭に入れるようにした方が、無駄が少なくて済む
          return Cons::cons(alc, key, value, 0);
        }
        
        Cons * c = alc.ptr<Cons>(list);
        if(c->key() == key) {
          new_key = false;
          md_t cdr = c->cdr();
          if(cdr != 0) {
            bool dup_rlt = alc.dup(cdr);
            assert(dup_rlt != false);
          }
          
          return Cons::cons(alc, key, value, cdr);
        } else {
          // XXX: 毎回 key と value のコピーが走るのは無駄。ポインタにした方が良いかも。
          return Cons::cons(alc, c->key(), c->value(), insert(c->cdr(), key, value, new_key, alc));
        }
      }

      static String find(uint32_t list, const String & key, const Alc & alc) {
        if(list == 0) {
          return String::invalid();
        }

        const Cons * c = alc.ptr<Cons>(list);
        if(c->key() == key) {
          return c->value();
        } else {
          return find(c->cdr(), key, alc);
        }
      }

    private:
      
    };

    class Node {
      typedef uint32_t md_t;
      typedef allocator::FixedAllocator Alc;
      
    public:
      void init(Alc & alc) {
        memset(nodes_, 0, sizeof(nodes_));
      }

      void release(Alc & alc, uint32_t depth) {
        /*
        for(int i=0; i < 16; i++) {
          std::cerr << "[" << i << "] " << nodes_[i] << std::endl;
        }
        */

        for(int i=0; i < 16; i++) {
          if(nodes_[i] == 0) {
            continue;
          }
          
          if(alc.undup(nodes_[i])) {
            if(depth > 0) {
              alc.ptr<Node>(nodes_[i])->release(alc, depth-1);
            } else {
              alc.ptr<Cons>(nodes_[i])->release(alc);
            }
            alc.release_no_undup(nodes_[i]);
          }
        }
      }

      md_t store(const String & key, const String & value, uint32_t hash, uint32_t depth, bool & new_key, Alc & alc) {
        uint32_t idx = index(hash);
        if(depth == 0) {
          md_t list = getList(alc, idx);
          md_t new_list = List::insert(list, key, value, new_key, alc);
          return setList(alc, idx, new_list);
        } else {
          md_t new_sub_node = getSubNode(alc, idx)->store(key, value, next(hash), depth-1, new_key, alc);
          return setSubNode(alc, idx, new_sub_node);
        }
      }

      String find(const String & key, uint32_t hash, uint32_t depth, const Alc & alc) const {
        uint32_t idx = index(hash);
        if(depth == 0) {
          return List::find(getList(alc, idx), key, alc);
        } else {
          return getSubNode(alc, idx)->find(key, next(hash), depth-1, alc);
        }
      }

      md_t getList(const Alc & alc, uint32_t index) const {
        return nodes_[index];
      }

      md_t setList(Alc & alc, uint32_t index, md_t list) {
        return setSubNode(alc, index, list);
      }


      Node * getSubNode(const Alc & alc, uint32_t index) const {
        return alc.ptr<Node>(nodes_[index]);
      }

      md_t setSubNode(Alc & alc, uint32_t index, md_t sub_node) {
        md_t md = alc.allocate(sizeof(Node));
        Node * new_node = alc.ptr<Node>(md);
        
        memcpy(new_node, this, sizeof(Node));
        new_node->nodes_[index] = sub_node;

        for(uint32_t i=0; i < 16; i++) {
          if(i != index && nodes_[i]) {
            bool dup_rlt = alc.dup(nodes_[i]);
            assert(dup_rlt);
          }
        }
        
        return md;
      }
      
      static uint32_t index(uint32_t hash) {
        return hash & 15;
      }
      
      static uint32_t next(uint32_t hash) {
        return hash >> 4;
      }
      
    private:
      md_t nodes_[16];
    };

    class RootNode {
      typedef uint32_t md_t;
      typedef allocator::FixedAllocator Alc;
      
    public:
      RootNode(allocator::FixedAllocator & alc)
        : count_(0),
          next_resize_trigger_(16 * 4),
          root_depth_(0),
          root_(alc.allocate(sizeof(Node)))
      {
        if(root_) {
          Node * node = alc.ptr<Node>(root_);
          node->init(alc);
        }
      }

      void release(allocator::FixedAllocator & alc) {
        if(root_) {
          if(alc.undup(root_)) {
            Node * node = alc.ptr<Node>(root_);
            node->release(alc, root_depth_);
            
            alc.release_no_undup(root_);
          }
        }
      }

      operator bool() const { return root_ != 0; }
      
      uint32_t count() const { return count_; }

      static void releaseNode(md_t md, Alc & alc) {
        if(alc.undup(md)) {
          alc.ptr<RootNode>(md)->release(alc);
          alc.release_no_undup(md);
        }
      }

      md_t store(const String & key, const String & value, Alc & alc) {
        if(count_ >= next_resize_trigger_) {
          // TODO:
        }
        
        bool new_key;
        md_t new_node = alc.ptr<Node>(root_)->store(key, value, key.hash(), root_depth_, new_key, alc);
        assert(new_node != 0);
        
        md_t new_root = alc.allocate(sizeof(RootNode));
        assert(new_root != 0);

        uint32_t new_count = (new_key ? count_+1 : count_);
        new (alc.ptr<RootNode>(new_root)) RootNode(new_count, next_resize_trigger_, root_depth_, new_node);
        
        // NOTE: 呼び出し元で解放している: release(alc);
        
        return new_root;
      }

      String find(const String & key, const Alc & alc) const {
        return alc.ptr<Node>(root_)->find(key, key.hash(), root_depth_, alc);
      }

    private:
      RootNode(uint32_t count, uint32_t next_resize_trigger, uint32_t root_depth, md_t root)
        : count_(count),
          next_resize_trigger_(next_resize_trigger),
          root_depth_(root_depth),
          root_(root)
      {
      }

    private:
      const uint32_t count_;
      const uint32_t next_resize_trigger_;
      const uint32_t root_depth_;
      const md_t root_;
    };
  }
}

#endif
