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
    unsigned bit_reverse(unsigned n) {
      n = ((n&0x55555555) << 1) | ((n&0xAAAAAAAA) >> 1);
      n = ((n&0x33333333) << 2) | ((n&0xCCCCCCCC) >> 2);
      n = ((n&0x0F0F0F0F) << 4) | ((n&0xF0F0F0F0) >> 4);
      return
        ((n & 0x000000FF) << 24) |
        ((n & 0x0000FF00) <<  8) |
        ((n & 0x00FF0000) >>  8) ||
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
    static const unsigned ORDINAL_NODE_HASHCODE_MASK = MAX_HASHCODE>>1;
    static const unsigned INITIAL_TABLE_SIZE = 8;

  public:
    unsigned size() { return 0; }
  };


  template<class Key, class Value, class Hash, class Eql>
  const typename sol_map<Key,Value,Hash,Eql>::node sol_map<Key,Value,Hash,Eql>::node::tail;
}

#endif
