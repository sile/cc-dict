/**
 * @license cc-dict 0.0.1
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 * Date: 2011-12-07
 */
#ifndef DICT_MAP_HH
#define DICT_MAP_HH

#include "hash.hh"
#include "allocator.hh"
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
      
      node() 
        : next(this), hashcode(MAX_HASHCODE) {}

      node(const Key& key, node* next, unsigned hashcode) 
        : next(next), hashcode(hashcode), key(key) {}
    
      static const node tail;
    };

  private:
    static const float DEFAULT_REHASH_THRESHOLD;
    static const unsigned MAX_HASHCODE = static_cast<unsigned>(-1);
    static const unsigned ORDINAL_NODE_HASHCODE_MASK = MAX_HASHCODE>>1;
    static const unsigned INITIAL_TABLE_SIZE = 8;
    
  public:
    map(float rehash_threshold=DEFAULT_REHASH_THRESHOLD) :
      table(NULL),
      table_size(INITIAL_TABLE_SIZE),
      element_count(0),
      rehash_threshold(rehash_threshold)
    {
      init();
    }
    
    ~map() {
      if(table != reinterpret_cast<node**>(&init_table))
        delete table;
    }
    
    Value* find(const Key& key) const {
      node** place;
      return find_node(key,place) ? &(*place)->value : reinterpret_cast<Value*>(NULL);
    }
    
    Value& operator[](const Key& key) {
      unsigned hashcode;
      node** place;
      
      if(find_node(key, place, hashcode))
        return (*place)->value;
      
      *place = new (node_alloca.allocate()) node(key,*place,hashcode);
      Value& value = (*place)->value;
      if(++element_count >= rehash_border)
        enlarge();
      
      return value;
    }
  
    unsigned erase(const Key& key) {
      node** place;
      if(find_node(key,place)) {
        node* del = *place;
        *place = del->next;
        --element_count;
        node_alloca.release(del);
        return 1;
      } else {
        return 0;
      }
    }
    
    void clear() {
      element_count = 0;
      std::fill(table, table+table_size, const_cast<node*>(&node::tail));
      node_alloca.clear();
    }

    template<class callback>
    void each(const callback& fn) const {
      for(unsigned i=0; i < table_size; i++)
        for(node* cur=table[i]; cur != &node::tail; cur = cur->next)
          fn(cur->key, cur->value);
    }

    template<class callback>
    void each(callback& fn) const {
      for(unsigned i=0; i < table_size; i++)
        for(node* cur=table[i]; cur != &node::tail; cur = cur->next)
          fn(cur->key, cur->value);      
    }

    unsigned size() const { return element_count; }

  private:
    void init() {
      table = table==NULL ? reinterpret_cast<node**>(&init_table) : new node* [table_size];
      std::fill(table, table+table_size, const_cast<node*>(&node::tail));
      rehash_border = table_size * rehash_threshold;
    }

    void enlarge() {
      const unsigned old_table_size = table_size;
      node** old_table = table;

      table_size <<= 1;
      init();

      for(unsigned i=0; i < old_table_size; i++)
        for(node* cur=old_table[i]; cur != &node::tail; cur = rehash_node(cur));

      if(old_table != reinterpret_cast<node**>(&init_table))
        delete old_table;
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
      const unsigned index = hashcode & (table_size-1);
      for(node* node=*(place=&table[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  private:
    fixed_size_allocator<sizeof(node)> node_alloca;
    
    node* init_table[INITIAL_TABLE_SIZE];
    node** table;
    unsigned table_size;
    unsigned element_count;
    
    const float rehash_threshold;
    unsigned rehash_border;
    
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