/**
 * @license cc-dict 0.0.6
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
    /*
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
    */

    const unsigned char REV_BYTE[]={
      0,128,64,192,32,160,96,224,16,144,
      80,208,48,176,112,240,8,136,72,200,
      40,168,104,232,24,152,88,216,56,184,
      120,248,4,132,68,196,36,164,100,228,
      20,148,84,212,52,180,116,244,12,140,
      76,204,44,172,108,236,28,156,92,220,
      60,188,124,252,2,130,66,194,34,162,
      98,226,18,146,82,210,50,178,114,242,
      10,138,74,202,42,170,106,234,26,154,
      90,218,58,186,122,250,6,134,70,198,
      38,166,102,230,22,150,86,214,54,182,
      118,246,14,142,78,206,46,174,110,238,
      30,158,94,222,62,190,126,254,1,129,
      65,193,33,161,97,225,17,145,81,209,
      49,177,113,241,9,137,73,201,41,169,
      105,233,25,153,89,217,57,185,121,249,
      5,133,69,197,37,165,101,229,21,149,
      85,213,53,181,117,245,13,141,77,205,
      45,173,109,237,29,157,93,221,61,189,
      125,253,3,131,67,195,35,163,99,227,
      19,147,83,211,51,179,115,243,11,139,
      75,203,43,171,107,235,27,155,91,219,
      59,187,123,251,7,135,71,199,39,167,
      103,231,23,151,87,215,55,183,119,247,
      15,143,79,207,47,175,111,239,31,159,
      95,223,63,191,127,255};

    inline unsigned bit_reverse(unsigned n) {
      return
        REV_BYTE[(n&0x000000FF)>> 0] << 24 | 
        REV_BYTE[(n&0x0000FF00)>> 8] << 16 |
        REV_BYTE[(n&0x00FF0000)>>16] <<  8 |
        REV_BYTE[(n&0xFF000000)>>24] <<  0;
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
      unsigned hashcode=0;
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

      if(head==&node::tail) {
        table[child_index] = head;
        return;
      }

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
      unsigned hashcode = 1;
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
