/**
 * @license cc-dict 0.0.2
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_EQL_HH
#define DICT_EQL_HH

#include <cstring>

namespace dict {
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
