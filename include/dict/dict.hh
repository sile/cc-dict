#ifndef DICT_DICT_HH
#define DICT_DICT_HH

#include <cstddef>
#include <algorithm>

namespace dict {
  // TODO: 別ファイルに分ける
  // TODO: lisp/dictのcommit and push (inline展開指定ミス)
  template<typename Key>
  unsigned hash(const Key& key) {
    return key.hash();
  }

  template <>
  unsigned hash(const int& key) {
    return key;
  }

  template <>
  unsigned hash(const unsigned& key) {
    return key;
  }

  template<typename Key, typename Value>
  struct Node {
    unsigned hashcode;
    Key key;
    Value value;
    
    Node* next;
  };

  
  template<typename Key, typename Value>
  class dict {
  public:
    dict() :
      buckets(NULL),
      bitlen(3),
      rehash_threshold(1.0)
    {
      bucket_size = 1 << bitlen;
      buckets = new BucketNode*[bucket_size];
      std::fill(buckets, buckets+bucket_size, &TAIL);

      bitmask = (1 << bitlen)-1;
      recalc_rehash_border();

      TAIL.next = &TAIL;
      TAIL.hashcode = -1;
    }

    ~dict() {
      delete [] buckets;
    }

    bool contains(const Key& key) const {
      Place p;
      return find_node(key, p);
    }

    bool put(const Key& key, const Value& value) {
      Place p;
      bool exists = find_node(key, p);
      return exists;
    }

  private:
    typedef Node<Key,Value> BucketNode;

    struct Candidate {
      unsigned index;   // XXX: 不要？
      BucketNode* pred;
      BucketNode* node;
    };

    struct Place {
      BucketNode** place;
      BucketNode* pred;
      BucketNode* node;
    };

  private:
    void recalc_rehash_border() {
      rehash_border = bucket_size * rehash_threshold;
    }

    bool find_node(const Key& key, Place& p) const {
      const unsigned hashcode = hash(key) & 0x7FFFFFFF; // XXX: sizeof(unsigned)==32 以外の環境に対応

      Candidate ca;
      find_candidate(hashcode, ca);

      for(;;) {
        if(hashcode != ca.node->hashcode) {
          p.node = ca.node;
          p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? &buckets[ca.index] : &ca.pred;
            
          return false;
        } 
        if(key == ca.node->key) {
          p.node = ca.node;
          p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? &buckets[ca.index] : &ca.pred;
          
          return true;
        }
        ca.pred = ca.node;
        ca.node = ca.node->next;
      }
    }

    void find_candidate(const unsigned hashcode, Candidate& ca) const {
      const unsigned index = hashcode & bitmask;
      
      BucketNode* pred = const_cast<BucketNode*>(&TAIL);
      BucketNode* node = buckets[index];
      for(; node->hashcode < hashcode; pred=node, node=node->next)
        if(hashcode <= node->hashcode)
          break;

      ca.index = index;
      ca.pred = pred;
      ca.node = node;
    }

  private:
    BucketNode** buckets;
    unsigned bucket_size;
    unsigned bitlen;
    unsigned bitmask;
    unsigned count;
    
    unsigned rehash_border;
    float rehash_threshold;
    
    BucketNode TAIL;
  };
}

#endif
