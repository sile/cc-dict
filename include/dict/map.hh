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
      node* next; // TODO: 32bitになるようにしてみる？
      unsigned hashcode;
      Key key;
      Value value;
      
      node() : next(this), hashcode(MAX_HASHCODE) {}
      node(const Key& key, node* next, unsigned hashcode) : next(next), hashcode(hashcode), key(key) {}
      ~node() {}

      static node tail;
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
      call_all_node_destructor();
      delete [] table;
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
  
    bool erase(const Key& key) {
      node** place;
      if(find_node(key,place)) {
        node* del = *place;
        *place = del->next;
        --element_count;
        del->~node();
        node_alloca.release(del);
        return true;
      } else {
        return false;
      }
    }
    
    void clear() {
      call_all_node_destructor();
      element_count = 0;
      std::fill(table, table+table_size, &node::tail);
      node_alloca.clear();
    }

    template<class Callback>
    void each(Callback& fn) const {
      for(unsigned i=0; i < table_size; i++)
        for(node* cur=table[i]; cur != &node::tail; cur = cur->next)
          fn(cur->key, cur->value);      
    }

    unsigned size() const { return element_count; }

  private:
    // TODO: 展開
    void init() {
      table = new node*[table_size];
      std::fill(table, table+table_size, &node::tail);
      rehash_border = table_size * rehash_threshold;
      index_mask = table_size-1;
    }

    // TODO: 整理
    void enlarge() {
      const unsigned old_table_size = table_size;

      table_size <<= 1;
      table = reinterpret_cast<node**>(std::realloc(table, sizeof(node*)*table_size));
      rehash_border = table_size * rehash_threshold;
      index_mask = table_size-1;

      for(unsigned i=0; i < old_table_size; i++)
        rehash_node(i, old_table_size);
    }
    
    void rehash_node(unsigned old_index, unsigned old_table_size) {
      const unsigned i0 = old_index;
      const unsigned i1 = old_index | old_table_size;
      
      node* cur = table[old_index];
      node* n0 = table[i0] = &node::tail;
      node* n1 = table[i1] = &node::tail;

      for(; cur != &node::tail; cur = cur->next)
        if(cur->hashcode & old_table_size)
          if(table[i1]==&node::tail)
            n1 = table[i1] = cur;
          else
            n1 = n1->next = cur;
        else
          if(table[i0]==&node::tail)
            n0 = table[i0] = cur;
          else
            n0 = n0->next = cur;

      n0->next = &node::tail;
      n1->next = &node::tail;
    }

    bool find_node(const Key& key, node**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, node**& place, unsigned& hashcode) const {
      hashcode = hash(key) & ORDINAL_NODE_HASHCODE_MASK;
      const unsigned index = hashcode & index_mask;
      
      for(place=&table[index]; (*place)->hashcode < hashcode; place=&(*place)->next);

      for(; (*place)->hashcode == hashcode; place=&(*place)->next)
        if(eql((*place)->key, key))
          return true;
      
      return false;
    }

    void call_all_node_destructor() {
      for(unsigned i=0; i < table_size; i++)
        for(node* cur=table[i]; cur != &node::tail; cur = cur->next)
          cur->~node();
    }

  private:
    fixed_size_allocator<sizeof(node)> node_alloca;
    
    node** table;
    unsigned table_size;
    unsigned index_mask;
    unsigned element_count;
    
    const float rehash_threshold;
    unsigned rehash_border;
    
    static const Hash hash;
    static const Eql eql;
  };

  template<class Key, class Value, class Hash, class Eql>
  typename map<Key,Value,Hash,Eql>::node map<Key,Value,Hash,Eql>::node::tail;

  template<class Key, class Value, class Hash, class Eql>
  const float map<Key,Value,Hash,Eql>::DEFAULT_REHASH_THRESHOLD = 0.75;
}

#endif
