/**
 * @license cc-dict 0.0.2
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_HASH_HH
#define DICT_HASH_HH

#include <string>
#include <cstring>
#include <iostream>
namespace dict {
  const unsigned GOLDEN_RATIO_PRIME=(2^31) + (2^29) - (2^25) + (2^22) - (2^19) - (2^16) + 1;
  const unsigned ui_size_minus8 = sizeof(unsigned)*8 - 8;

  template<class Key>
  class hash_functor {
  public:
    unsigned operator()(const Key& key) const {
      return hash(key);
    }
  };

  unsigned hash(int key) {
    return key * GOLDEN_RATIO_PRIME;
  }

  unsigned hash(unsigned key) {
    return key * GOLDEN_RATIO_PRIME;
  }

  unsigned hash(long key) {
    return key * GOLDEN_RATIO_PRIME;
  }

  unsigned hash(const char* key) {
    unsigned h = GOLDEN_RATIO_PRIME;
    for(const char* c=key; *c != 0; c++)
      h = (h*33) + *c;
    return h;
  }

  unsigned hash(const std::string& key) {
    return hash(key.c_str());
  }

  // XXX:
  template<class Key>
  class eql_functor {
  public:
    bool operator()(const Key& k1, const Key& k2) const {
      return eql(k1,k2);
    }
  };

  template<class T>
  bool eql(T k1, T k2) {
    return k1 == k2;
  }
  
  template<>
  bool eql(const char* k1, const char* k2) {
    return std::strcmp(k1,k2)==0;
  }
}

#endif
