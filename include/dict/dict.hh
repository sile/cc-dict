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
      std::fill(buckets, buckets+bucket_size, tail());

      bitmask = (1 << bitlen)-1;
      recalc_rehash_border();
    }

    ~dict() {
      delete [] buckets;
    }

    bool contains(const Key& key) const {
      return true;
    }

  private:
    typedef Node<Key,Value> BucketNode;

    struct Candidate {
      unsigned index;
      unsigned pred;
      unsigned node;
    };

    // XXX: => sentinel
    BucketNode* tail() const {
      return reinterpret_cast<BucketNode*>(NULL);
    }

  private:
    void recalc_rehash_border() {
      rehash_border = bucket_size * rehash_threshold;
    }

    void find_node(const Key& key) const {
      const unsigned hashcode = hash(key);

      Candidate ca;
      find_candidate(hashcode, ca);
    }

    void find_candidate(const unsigned hashcode, Candidate& ca) const {
      const unsigned index = hashcode & bitmask;
      
      //unsigned pred = TAIL;
      //unsigned node = buckets[index];
      BucketNode* pred = tail();
      BucketNode* node = buckets[index];
      for(; node->hashcode < hashcode; pred=node, node=node->next)
        if(hashcode <= node->hash)
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
  };
}

#endif
