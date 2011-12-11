/**
 * @license cc-dict 0.0.3
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

namespace dict {
  namespace {
    // XXX: assume as sizeof(unsigned) is 4
    inline unsigned bit_reverse(unsigned n) {
      n = ((n&0x55555555) << 1) | ((n&0xAAAAAAAA) >> 1);
      n = ((n&0x33333333) << 2) | ((n&0xCCCCCCCC) >> 2);
      n = ((n&0x0F0F0F0F) << 4) | ((n&0xF0F0F0F0) >> 4);
      return
        ((n & 0x000000FF) << 24) |
        ((n & 0x0000FF00) <<  8) |
        ((n & 0x00FF0000) >>  8) |
        ((n & 0xFF000000) >> 24);   // n >> 24
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
      
      node() 
        : next(this), hashcode(MAX_HASHCODE) {}

      node(const Key& key, node* next, unsigned hashcode) 
        : next(next), hashcode(hashcode), key(key) {}
    
      static const node tail;
    };
    
  private:
    static const unsigned MAX_HASHCODE = static_cast<unsigned>(-1);
    static const unsigned ORDINAL_NODE_HASHCODE_MASK = MAX_HASHCODE<<1;
    static const unsigned INITIAL_TABLE_SIZE = 8;

  public:
    sol_map() :
      table(NULL),
      table_size(INITIAL_TABLE_SIZE),
      rehash_border(0),
      rehash_threshold(1.00), //0.75),
      element_count(0)
    {
      init();
    }

    ~sol_map() {
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
    unsigned size() const { return element_count; }

  private:
    void init() {
      table = new node*[table_size]; // TODO: recalloc(?) を使ってみる？
      std::fill(table, table+table_size, const_cast<node*>(&node::tail));
      rehash_border = table_size * rehash_threshold;
    }

    void enlarge() {
      std::cout << "IN: " << table_size << " #" << element_count << std::endl;

      const unsigned old_table_size = table_size;
      node** old_table = table;

      
      //init();
      // XXX: new_table_size
      table = new node*[table_size<<1]; // TODO: recalloc(?) を使ってみる？
      rehash_border = (table_size<<1) * rehash_threshold;

      /*
      for(unsigned i=0; i < old_table_size; i++) {
        table[i] = old_table[i];
      }
      */
      
      node* buf = (node*)node_alloca.alloc_block(element_count);
      for(unsigned i=0; i < old_table_size; i++) {
        //        std::cout << " - " << i << std::endl;
        table[i] = buf;
        if(old_table[i]==&node::tail)
          table[i] = const_cast<node*>(&node::tail);
        
        for(node* c=old_table[i]; c != &node::tail; c = c->next) {
          *buf = *c;
          if(buf->next != &node::tail)
            buf->next = buf+1;
          buf++;
        }
      }
      
      for(unsigned i=0; i < old_table_size; i++) {
        rehash_node(table[i], i, old_table_size);
      }
      
      table_size <<= 1;

      delete [] old_table;
    }
    
    void rehash_node(node* head, unsigned old_index, unsigned old_table_size) {
      // TODO: 整理
      unsigned child = old_index | old_table_size; // TODO: comment (新しい最上位ビットを1にする)
      unsigned hashcode = bit_reverse(child);
      node** place;
      //std::cout << " - " << child << std::endl;

      find_candidate(hashcode, place);

      table[child] = *place;
      *place = const_cast<node*>(&node::tail);
    }

    bool find_node(const Key& key, node**& place) const {
      unsigned hashcode;
      return find_node(key, place, hashcode);
    }
    
    bool find_node(const Key& key, node**& place, unsigned& hashcode) const {
      hashcode = hash(key) << 4; // XXX: & ORDINAL_NODE_HASHCODE_MASK;
      find_candidate(hashcode, place);

      for(node* node=*place; hashcode==node->hashcode; node=*(place=&node->next))
        if(eql(key,node->key))
          return true;
      return false;
    }

    void find_candidate(const unsigned hashcode, node**& place) const {
      const unsigned index = bit_reverse(hashcode) & (table_size-1);
      //      std::cout << " i: " << index <<", "<< bit_reverse(hashcode) << ", " << (table_size-1) << std::endl;
      for(node* node=*(place=&table[index]); node->hashcode < hashcode; node=*(place=&node->next));
    }

  private:
    fixed_size_allocator2<sizeof(node)> node_alloca;
    node** table;
    unsigned table_size;
    unsigned rehash_border;
    const float rehash_threshold;
    unsigned element_count;
  };


  template<class Key, class Value, class Hash, class Eql>
  const typename sol_map<Key,Value,Hash,Eql>::node sol_map<Key,Value,Hash,Eql>::node::tail;
}

#endif
