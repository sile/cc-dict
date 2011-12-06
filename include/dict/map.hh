/**
 * @license cc-dict 0.0.1
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 * Date: 2011-12-07
 */
#ifndef DICT_MAP_HH
#define DICT_MAP_HH

#include "allocator.hh"
#include "hash.hh"

#include <cstddef>
#include <algorithm>

namespace dict {
  template<class Key, class Value, class Hash = dict::hash_functor<Key> >
  class map { 
  private:
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
    map(float rehash_threshold=DEFAULT_REHASH_THRESHOLD) :
      buckets(NULL),
      bitlen(INITIAL_BITLEN),
      count(0),
      rehash_threshold(rehash_threshold),
      node_alloca()
    {
      init();
    }

    ~map() {
      if(buckets != reinterpret_cast<node**>(&init_nodes))
        delete buckets;
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
      buckets = buckets==NULL ? reinterpret_cast<node**>(&init_nodes) : new node* [bucket_size];
      std::fill(buckets, buckets+bucket_size, const_cast<node*>(&node::tail));
      rehash_border = bucket_size * rehash_threshold;
    }

    void enlarge() {
      const unsigned old_bucket_size = bucket_size;
      node** old_buckets = buckets;

      bitlen++;
      init();

      for(unsigned i=0; i < old_bucket_size; i++)
        for(node* cur=old_buckets[i]; cur != &node::tail; cur = rehash_node(cur));

      if(old_buckets != reinterpret_cast<node**>(&init_nodes))
        delete old_buckets;
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
    
    fixed_size_allocator<sizeof(node)> node_alloca;

    node* init_nodes[1 << INITIAL_BITLEN];
    
    static const Hash hash;
  };

  template<class Key, class Value, class Hash>
  const typename map<Key,Value,Hash>::node map<Key,Value,Hash>::node::tail;

  template<class Key, class Value, class Hash>
  const float map<Key,Value,Hash>::DEFAULT_REHASH_THRESHOLD = 0.75;

  template<class Key, class Value, class Hash>
  const Hash map<Key,Value,Hash>::hash;
}

#endif
