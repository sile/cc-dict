#ifndef DICT_DICT_HH
#define DICT_DICT_HH

#include "allocator.hh"
#include "hash.hh"

#include <cstddef>
#include <algorithm>

/*
 * TODO: 
 * - remove
 * - each
 * - clear
 */
namespace dict {
  template<typename Key, typename Value>
  struct Node {
    Node* next;
    unsigned hashcode;
    Key key;
    Value value;

    Node() : next(this), hashcode(-1) {}
    Node(const Key& key, const Value& value, Node* next, unsigned hashcode) 
      : next(next), hashcode(hashcode), key(key), value(value) {}
    
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
      count(0),
      rehash_threshold(0.75), 
      alc(8), // XXX:
      acc(Alloca::instance()) // TODO: 引数でも指定可能に
    {
      init();
    }

    ~dict() {
      acc.free(buckets);
    }

    Value* find(const Key& key) const {
      BucketNode** place;
      if(find_node(key,place))
        return &(*place)->value;
      else
        return const_cast<Value*>(end());
    }

    unsigned size() const { return count; }

    Value& operator[](const Key& key) {
      unsigned hashcode;
      BucketNode** place;

      bool exists = find_node(key, place, hashcode);
      Value& value = (*place)->value;
      if(exists==false) {
        *place = new (alc.allocate()) BucketNode(key,Value(),*place,hashcode);
        if(++count >= rehash_border)
          enlarge();
      }
      return value;
    }

    static const Value* end() {
      return reinterpret_cast<const Value*>(&BucketNode::TAIL);
    }

    unsigned erase(const Key& key) {
      BucketNode** place;
      if(find_node(key,place)) {
        BucketNode* del = *place;
        *place = del->next;
        --count;
        alc.release(del);
        return 1;
      } else {
        return 0;
      }
    }
    
    /*
      void each(Callback fn) {
      
      }
     */
    
  private:
    typedef Node<Key,Value> BucketNode;

  private:
    void init() {
      bucket_size = 1 << bitlen;
      bitmask = bucket_size-1;

      buckets = new (acc.allocate(bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, const_cast<BucketNode*>(&BucketNode::TAIL));
      rehash_border = bucket_size * rehash_threshold;
    }
    
    void enlarge() {
      BucketNode** old_buckets = buckets;
      unsigned old_bucket_size = bucket_size;

      bitlen++;
      init();

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

    bool find_node(const Key& key, BucketNode**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, BucketNode**& place, unsigned& hashcode) const {
      hashcode = hash(key) & ORDINAL_HASHCODE_MASK;
      find_candidate(hashcode, place);

      for(BucketNode* node=*place; hashcode==node->hashcode; node=*(place=&node->next))
        if(key == node->key)
          return true;
      return false;
    }

    void find_candidate(const unsigned hashcode, BucketNode**& place) const {
      const unsigned index = hashcode & bitmask;
      for(BucketNode* node=*(place=&buckets[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  private:
    static const unsigned ORDINAL_HASHCODE_MASK = -1>>1;

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
