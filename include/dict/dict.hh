#ifndef DICT_DICT_HH
#define DICT_DICT_HH

#include "allocator.hh"
#include "hash.hh"

#include <cstddef>
#include <algorithm>

/*
 * TODO: 
 * - アロケータ自作 (ハッシュテーブル単位でも。template引数でmalloc版と独自版を選べるようにする)
 * - operator[]
 * - remove
 * - each
 * - clear
 * - Node::TAIL
 */
namespace dict {
  template<typename Key, typename Value>
  struct Node {
    Node* next;
    unsigned hashcode;
    Key key;
    Value value;
  };

  template<typename Key, typename Value, class HASH = dict::hash<Key>, class Alloca = CachedAllocator<Node<Key,Value>* > >
  class dict {
  public:
    dict() :
      buckets(NULL),
      bitlen(3),
      count(0),
      rehash_threshold(0.8),
      alc(8), // XXX:
      acc(Alloca::instance()) // TODO: 引数でも指定可能に
    {
      bucket_size = 1 << bitlen;

      buckets = new (acc.allocate(bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, &TAIL);

      bitmask = (1 << bitlen)-1;
      recalc_rehash_border();

      TAIL.next = &TAIL;
      TAIL.hashcode = -1;
    }

    ~dict() {
      acc.free(buckets);
    }

    bool contains(const Key& key) const {
      Place p;
      return find_node(key, p);
    }

    unsigned size() const { return count; }
    
    bool put(const Key& key, const Value& value) {
      Place p;
      bool exists = find_node(key, p);
      if(exists) {
        p.node->value = value;
      } else {
        BucketNode* new_node = new (alc.allocate()) BucketNode;
        new_node->key = key;
        new_node->value = value;
        new_node->hashcode = p.hashcode;
        new_node->next = p.node;
        *p.place = new_node;

        count++;
        if(count >= rehash_border)
          enlarge();
      }

      return exists;
    }
    
    Value& get(const Key& key) const {
      Place p;
      if(find_node(key, p)) 
        return p.node->value;
      throw "TODO: error";
    }
    
  private:
    typedef Node<Key,Value> BucketNode;

    struct Candidate {
      BucketNode** place; // => pred
      BucketNode* pred;
      BucketNode* node;
    };

    struct Place {
      BucketNode** place; // => pred
      //BucketNode* pred;  // XXX: 不要？ 
      BucketNode* node;
      unsigned hashcode;
    };

  private:
    void enlarge() {
      BucketNode** old_buckets = buckets;
      unsigned old_bucket_size = bucket_size;

      bitlen++;
      bucket_size = 1 << bitlen;
      bitmask = (1 << bitlen)-1;


      buckets = new (acc.allocate(bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, &TAIL);
      recalc_rehash_border();

      for(unsigned i=0; i < old_bucket_size; i++)
        for(BucketNode* node=old_buckets[i]; node != &TAIL;)
          node = rehash_node(node);

      acc.free(old_buckets);
    }
    
    BucketNode* rehash_node(BucketNode* node) {
      BucketNode* next = node->next;

      Candidate ca;
      find_candidate(node->hashcode, ca);
      node->next = ca.node;
      *ca.place = node;

      return next;
    }

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
          //p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? ca.place : &ca.pred->next;
          p.hashcode = hashcode;            
          return false;
        } 
        if(key == ca.node->key) {
          p.node = ca.node;
          //p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? ca.place : &ca.pred->next;
          p.hashcode = hashcode;
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
      for(; node->hashcode < hashcode; pred=node, node=node->next);

      ca.place = pred==&TAIL ? &buckets[index] : &pred->next;
      ca.pred = pred;
      ca.node = node;
    }

  private:
    BucketNode** buckets;
    unsigned bucket_size; // => buckets_size
    unsigned bitlen;
    unsigned bitmask;
    unsigned count;
    
    unsigned rehash_border;
    float rehash_threshold;
    
    BucketNode TAIL;

    Allocator<BucketNode> alc;
    Alloca& acc;

    static HASH hash;
  };
}

#endif
