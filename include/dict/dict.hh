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

    Node() : next(this), hashcode(-1) {}
    static const Node TAIL;
  };
  template<typename Key, typename Value>
  const Node<Key,Value> Node<Key,Value>::TAIL;

  template<typename Key, typename Value, class HASH = dict::hash<Key>, class Alloca = CachedAllocator<Node<Key,Value>* > >
  class dict {
  public:
    dict() :
      buckets(NULL),
      bitlen(3),
      bucket_size(1 << bitlen),
      bitmask(bucket_size-1),
      count(0),
      rehash_threshold(0.8),
      rehash_border(bucket_size * rehash_threshold),
      alc(bucket_size), // XXX:
      acc(Alloca::instance()) // TODO: 引数でも指定可能に
    {
      buckets = new (acc.allocate(bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, const_cast<BucketNode*>(&BucketNode::TAIL));
    }

    ~dict() {
      acc.free(buckets);
    }

    bool contains(const Key& key) const {
      return find_node(key);
    }

    unsigned size() const { return count; }
    
    bool put(const Key& key, const Value& value) {
      unsigned hashcode;
      BucketNode** place;
      bool exists = find_node(key, place, hashcode);
      if(exists) {
        (*place)->value = value;
      } else {
        BucketNode* new_node = new (alc.allocate()) BucketNode;
        new_node->key = key;
        new_node->value = value;
        new_node->hashcode = hashcode;
        new_node->next = (*place);
        *place = new_node;

        count++;
        if(count >= rehash_border)
          enlarge();
      }

      return exists;
    }
    
    Value& get(const Key& key) const {
      BucketNode** place;
      if(find_node(key, place)) 
        return (*place)->value;
      throw "TODO: error";
    }
    
  private:
    typedef Node<Key,Value> BucketNode;

  private:
    void enlarge() {
      BucketNode** old_buckets = buckets;
      unsigned old_bucket_size = bucket_size;

      bitlen++;
      bucket_size = 1 << bitlen;
      bitmask = (1 << bitlen)-1;

      buckets = new (acc.allocate(bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, const_cast<BucketNode*>(&BucketNode::TAIL));
      rehash_border = bucket_size * rehash_threshold;

      for(unsigned i=0; i < old_bucket_size; i++)
        for(BucketNode* node=old_buckets[i]; node != &BucketNode::TAIL;)
          node = rehash_node(node);

      acc.free(old_buckets);
    }
    
    BucketNode* rehash_node(BucketNode* node) {
      BucketNode* next = node->next;
      BucketNode** place;
      
      find_candidate(node->hashcode, place);
      node->next = *place;
      *place = node;
      
      return next;
    }

    bool find_node(const Key& key) const {
      BucketNode** place;
      return find_node(key, place);
    }

    bool find_node(const Key& key, BucketNode**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, BucketNode**& place, unsigned& hashcode) const {
      hashcode = hash(key) & 0x7FFFFFFF; // XXX: sizeof(unsigned)==32 以外の環境に対応
      find_candidate(hashcode, place);

      for(BucketNode* node=*place;; node=*(place=&node->next)) {
        if(hashcode != node->hashcode)
          return false;
        if(key == node->key)
          return true;
      }
    }

    void find_candidate(const unsigned hashcode, BucketNode**& place) const {
      const unsigned index = hashcode & bitmask;
      for(BucketNode* node=*(place=&buckets[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  private:
    BucketNode** buckets;
    unsigned bitlen;
    unsigned bucket_size; // => buckets_size
    unsigned bitmask;
    unsigned count;
    
    float rehash_threshold;
    unsigned rehash_border;
    
    Allocator<BucketNode> alc;
    Alloca& acc;

    static HASH hash;
  };
}

#endif
