/**
 * @license cc-dict 0.0.6
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_MAP_HH
#define DICT_MAP_HH

#include "hash.hh"
#include "eql.hh"
#include "allocator.hh"
#include <cstddef>
#include <algorithm>

namespace dict {
  template<class Key, class Value, class Hash=dict::hash_functor<Key>, class Eql=dict::eql_functor<Key> >
  class map { 
  private:
    struct node {
      typedef typename fixed_size_allocator<node>::index_t ptr;
      ptr next; 
      unsigned hashcode;
      Key key;
      Value value;
      
      node() : next(NULL), hashcode(MAX_HASHCODE) {}
      node(const Key& key, ptr next, unsigned hashcode) : next(next), hashcode(hashcode), key(key) {}
      ~node() {}
    };

    typedef typename node::ptr node_ptr;

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
      rehash_threshold(rehash_threshold),
      tail(node_alloca)
    {
      init();
    }
    
    ~map() {
      call_all_node_destructor(); 
      delete [] table;
    }
    
    Value* find(const Key& key) const {
      node_ptr* place;
      return find_node(key,place) ? &(*place).ref(node_alloca)->value : reinterpret_cast<Value*>(NULL);
    }

    Value& operator[](const Key& key) {
      unsigned hashcode;
      node_ptr* place;
      
      if(find_node(key, place, hashcode))
        return (*place).ref(node_alloca)->value;
      
      node_ptr tmp(node_alloca);
      node_ptr next = *place;
      *place = tmp; 
      node& n = *tmp.ref(node_alloca);
      n.key = key; // XXX:
      n.next = next;
      n.hashcode = hashcode;

      Value& value = n.value;
      if(++element_count >= rehash_border) {
        enlarge();
        return operator[](key);
      }
      
      return value;
    }
  
    bool erase(const Key& key) {
      node_ptr* place;
      if(find_node(key,place)) {
        node_ptr del = *place;
        *place = del.ref(node_alloca)->next;
        --element_count;
        del.ref(node_alloca)->~node();
        node_alloca.release(del.ref(node_alloca));
        return true;
      } else {
        return false;
      }
    }
    
    void clear() {
      call_all_node_destructor();
      element_count = 0;
      std::fill(table, table+table_size, tail);
      node_alloca.clear();
    }

    template<class Callback>
    void each(Callback& fn) const {
      for(unsigned i=0; i < table_size; i++)
        for(node_ptr cur=table[i]; cur != tail; cur = cur.ref(node_alloca)->next)
          fn(cur.ref(node_alloca)->key, cur.ref(node_alloca)->value);      
    }

    unsigned size() const { return element_count; }

  private:
    // TODO: 展開
    void init() {
      table = new node_ptr[table_size];
      std::fill(table, table+table_size, tail);
      rehash_border = table_size * rehash_threshold;
      index_mask = table_size-1;
    }

    // TODO: 整理
    void enlarge() {
      const unsigned old_table_size = table_size;

      table_size <<= 1;
      table = reinterpret_cast<node_ptr*>(std::realloc(table, sizeof(node_ptr)*table_size));
      rehash_border = table_size * rehash_threshold;
      index_mask = table_size-1;

      for(unsigned i=0; i < old_table_size; i++)
        rehash_node(i, old_table_size);
    }
    
    void rehash_node(unsigned old_index, unsigned old_table_size) {
      const unsigned i0 = old_index;
      const unsigned i1 = old_index | old_table_size;
      
      node_ptr cur = table[old_index];
      node_ptr n0 = table[i0] = tail;
      node_ptr n1 = table[i1] = tail;

      for(; cur != tail; cur = cur.ref(node_alloca)->next)
        if(cur.ref(node_alloca)->hashcode & old_table_size)
          if(table[i1]==tail)
            n1 = table[i1] = cur;
          else
            n1 = n1.ref(node_alloca)->next = cur;
        else
          if(table[i0]==tail)
            n0 = table[i0] = cur;
          else
            n0 = n0.ref(node_alloca)->next = cur;

      n0.ref(node_alloca)->next = tail;
      n1.ref(node_alloca)->next = tail;
    }

    bool find_node(const Key& key, node_ptr*& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, node_ptr*& place, unsigned& hashcode) const {
      hashcode = hash(key) & ORDINAL_NODE_HASHCODE_MASK;
      const unsigned index = hashcode & index_mask;
      
      for(place=&table[index]; (*place).ref(node_alloca)->hashcode < hashcode; place=&(*place).ref(node_alloca)->next);

      for(; (*place).ref(node_alloca)->hashcode == hashcode; place=&(*place).ref(node_alloca)->next)
        if(eql((*place).ref(node_alloca)->key, key))
          return true;
      
      return false;
    }

    void call_all_node_destructor() {
      for(unsigned i=0; i < table_size; i++)
        for(node_ptr cur=table[i]; cur != tail; cur = cur.ref(node_alloca)->next)
          cur.ref(node_alloca)->~node();
    }

  private:
    fixed_size_allocator<node> node_alloca;
    
    node_ptr* table;
    unsigned table_size;
    unsigned index_mask;
    unsigned element_count;
    
    const float rehash_threshold;
    unsigned rehash_border;
    
    static const Hash hash;
    static const Eql eql;
    
    node_ptr tail;
  };

  //  template<class Key, class Value, class Hash, class Eql>
  //  typename map<Key,Value,Hash,Eql>::node map<Key,Value,Hash,Eql>::node::tail;

  template<class Key, class Value, class Hash, class Eql>
  const float map<Key,Value,Hash,Eql>::DEFAULT_REHASH_THRESHOLD = 0.75;
}

#endif
