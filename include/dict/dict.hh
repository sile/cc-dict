#ifndef DICT_DICT_HH
#define DICT_DICT_HH

#include "allocator.hh"
#include "hash.hh"

#include <cstddef>
#include <algorithm>

namespace dict {
  template<class Key, class Value, class HASH = dict::hash<Key>, class Alloca = CachedAllocator<void*> > // XXX: void*
  class dict {
  public:
    struct node {
      node* next;
      unsigned hashcode;
      Key key;
      Value value;
      
      node() : next(this), hashcode(MAX_HASHCODE) {}

      node(const Key& key, const Value& value, node* next, unsigned hashcode) 
      : next(next), hashcode(hashcode), key(key), value(value) {}
    
      static const node tail;
    };
    
  public:
    dict() :
      buckets(NULL),
      bitlen(INITIAL_BITLEN),
      count(0),
      rehash_threshold(DEFAULT_REHASH_THRESHOLD),
      node_alloca(1 << bitlen),
      acc(Alloca::instance()) // TODO: 引数でも指定可能に
    {
      init();
    }

    dict(float rehash_threshold) :
      buckets(NULL),
      bitlen(INITIAL_BITLEN),
      count(0),
      rehash_threshold(rehash_threshold),
      node_alloca(1 << bitlen),
      acc(Alloca::instance()) // TODO: 引数でも指定可能に
    {
      init();
    }

    ~dict() {
      acc.free(buckets);
    }

    Value* find(const Key& key) const {
      node** place;
      return find_node(key,place) ? &(*place)->value : const_cast<Value*>(end());
    }

    unsigned size() const { return count; }

    Value& operator[](const Key& key) {
      unsigned hashcode;
      node** place;

      if(find_node(key, place, hashcode))
        return (*place)->value;
      
      *place = new (node_alloca.allocate()) node(key,Value(),*place,hashcode);
      Value& value = (*place)->value;
      if(++count >= rehash_border)
        enlarge();

      return value;
    }

    static const Value* end() {
      return reinterpret_cast<const Value*>(&node::tail);
    }

    unsigned erase(const Key& key) {
      node** place;
      if(find_node(key,place)) {
        node* del = *place;
        *place = del->next;
        --count;
        node_alloca.release(del);
        return 1;
      } else {
        return 0;
      }
    }
    
    void clear() {
      count = 0;
      std::fill(buckets, buckets+bucket_size, const_cast<node*>(&node::tail));
      node_alloca.clear();
    }

    template<class callback>
    void each(const callback& fn) const {
      for(unsigned i=0; i < bucket_size; i++)
        for(node* cur=buckets[i]; cur != &node::tail; cur = cur->next)
          fn(cur->key, cur->value);
    }

    template<class callback>
    void each(callback& fn) const {
      for(unsigned i=0; i < bucket_size; i++)
        for(node* cur=buckets[i]; cur != &node::tail; cur = cur->next)
          fn(cur->key, cur->value);      
    }

  private:
    void init() {
      bucket_size = 1 << bitlen;
      bitmask = bucket_size-1;

      buckets = new (acc.allocate(bucket_size)) node* [bucket_size];
      std::fill(buckets, buckets+bucket_size, const_cast<node*>(&node::tail));
      rehash_border = bucket_size * rehash_threshold;
    }
    
    void enlarge() {
      node** old_buckets = buckets;
      unsigned old_bucket_size = bucket_size;

      bitlen++;
      init();

      for(unsigned i=0; i < old_bucket_size; i++)
        for(node* cur=old_buckets[i]; cur != &node::tail;)
          cur = rehash_node(cur);

      acc.free(old_buckets);
    }
    
    node* rehash_node(node* cur) {
      node* next = cur->next;
      node** place;
      
      find_candidate(cur->hashcode, place);
      cur->next = *place;
      *place = cur;
      
      return next;
    }

    bool find_node(const Key& key, node**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, node**& place, unsigned& hashcode) const {
      hashcode = hash(key) & ORDINAL_NODE_HASHCODE_MASK;
      find_candidate(hashcode, place);

      for(node* node=*place; hashcode==node->hashcode; node=*(place=&node->next))
        if(key == node->key)
          return true;
      return false;
    }

    void find_candidate(const unsigned hashcode, node**& place) const {
      const unsigned index = hashcode & bitmask;
      for(node* node=*(place=&buckets[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  public:
    static const float DEFAULT_REHASH_THRESHOLD;
    
  private:
    static const unsigned MAX_HASHCODE = static_cast<unsigned>(-1);
    static const unsigned ORDINAL_NODE_HASHCODE_MASK = MAX_HASHCODE>>1;
    static const unsigned INITIAL_BITLEN = 3;

  private:
    node** buckets;
    unsigned bitlen;
    unsigned bucket_size; // => buckets_size
    unsigned bitmask;
    unsigned count;
    
    const float rehash_threshold;
    unsigned rehash_border;
    
    Allocator<node> node_alloca;
    Alloca& acc;

    static const HASH hash;
  };

  template<class Key, class Value, class HASH, class Alloca>
  const typename dict<Key,Value,HASH,Alloca>::node dict<Key,Value,HASH,Alloca>::node::tail;

  template<class Key, class Value, class HASH, class Alloca>
  const float dict<Key,Value,HASH,Alloca>::DEFAULT_REHASH_THRESHOLD = 0.75;
}

#endif
