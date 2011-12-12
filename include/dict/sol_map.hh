/**
 * @license cc-dict 0.0.5
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_SOL_MAP_HH
#define DICT_SOL_MAP_HH

#include "hash.hh"
#include "eql.hh"
#include "allocator.hh"
#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace dict {
  namespace {
    inline unsigned bit_reverse(unsigned n) {
      n = ((n&0x55555555) << 1) | ((n&0xAAAAAAAA) >> 1);
      n = ((n&0x33333333) << 2) | ((n&0xCCCCCCCC) >> 2);
      n = ((n&0x0F0F0F0F) << 4) | ((n&0xF0F0F0F0) >> 4);
      return
        ((n & 0x000000FF) << 24) |
        ((n & 0x0000FF00) <<  8) |
        ((n & 0x00FF0000) >>  8) |
        ((n & 0xFF000000) >> 24); 
    }
  }

  template<class Key, class Value, class Hash=dict::hash_functor<Key>, class Eql=dict::eql_functor<Key> >
  class sol_map {
  private:
   struct node {
     node* next;
     unsigned hashcode;
     Key key;
     Value value;
     
     node() : next(this), hashcode(MAX_HASHCODE) {}
     node(const Key& key, node* next, unsigned hashcode) : next(next), hashcode(hashcode), key(key) {}
     ~node() {}

     static const node tail;
   };
    
  private:
    static const float DEFAULT_REHASH_THRESHOLD;
    static const unsigned MAX_HASHCODE = 0xFFFFFFFF;
    static const unsigned ORDINAL_NODE_HASHCODE_MASK = (MAX_HASHCODE<<1) & MAX_HASHCODE;
    static const unsigned INITIAL_TABLE_SIZE = 8;

  public:
    sol_map(float rehash_threshold=DEFAULT_REHASH_THRESHOLD) :
      table(NULL),
      table_size(INITIAL_TABLE_SIZE),
      index_mask(table_size-1),
      rehash_border(0),
      rehash_threshold(rehash_threshold),
      element_count(0)
    {
      table = reinterpret_cast<node**>(std::malloc(sizeof(node*)*table_size));
      std::fill(table, table+table_size, const_cast<node*>(&node::tail));
      rehash_border = table_size * rehash_threshold;
    }

    ~sol_map() {
      std::free(table);
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
      if(++element_count >= rehash_border) {
        enlarge();
        return operator[](key);
      }

      return (*place)->value;
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
      element_count = 0;
      for(unsigned i=0; i < table_size; i++)
        for(node* cur=table[i]; cur != &node::tail; cur = cur->next)
          cur->~node();
      std::fill(table, table+table_size, const_cast<node*>(&node::tail));
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
    void enlarge() {
      const unsigned new_table_size = table_size<<1;
      
      table = reinterpret_cast<node**>(std::realloc(table, sizeof(node*)*new_table_size));

      for(unsigned i=0; i < table_size; i++)
        rehash_node(table[i], i);

      table_size = new_table_size;
      index_mask = table_size-1;
      rehash_border = table_size * rehash_threshold;
    }
    
    void rehash_node(node* head, unsigned parent_index) {
      // 有効な最上位ビットを1にする => 子のインデックス
      const unsigned child_index = parent_index | table_size; 

      const unsigned hashcode = bit_reverse(child_index);
      node** place;

      find_candidate(parent_index, hashcode, place);
      
      // table[parent] -> node1 -> node2 -> node3 -> tail
      //  ↓
      // table[parent] -> node1 -> tail
      // table[child]  -> node2 -> node3 -> tail
      table[child_index] = *place;
      *place = const_cast<node*>(&node::tail);      
    }

    bool find_node(const Key& key, node**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, node**& place, unsigned& hashcode) const {
      const unsigned row_hash = hash(key);
      const unsigned index = row_hash & index_mask;
      hashcode = bit_reverse(row_hash) & ORDINAL_NODE_HASHCODE_MASK;
      
      find_candidate(index, hashcode, place);

      for(node* node=*place; hashcode==node->hashcode; node=*(place=&node->next))
        if(eql(key,node->key))
          return true;
      return false;
    }

    void find_candidate(const unsigned index, const unsigned hashcode, node**& place) const {
      for(node* node=*(place=&table[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  private:
    fixed_size_allocator<sizeof(node)> node_alloca;
    node** table;
    unsigned table_size;
    unsigned index_mask;
    unsigned rehash_border;
    const float rehash_threshold;
    unsigned element_count;
  };

  template<class Key, class Value, class Hash, class Eql>
  const typename sol_map<Key,Value,Hash,Eql>::node sol_map<Key,Value,Hash,Eql>::node::tail;

  template<class Key, class Value, class Hash, class Eql>
  const float sol_map<Key,Value,Hash,Eql>::DEFAULT_REHASH_THRESHOLD = 0.75;
}

#endif
