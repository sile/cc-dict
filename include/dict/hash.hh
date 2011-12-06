/**
 * @license cc-dict 0.0.1
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 * Date: 2011-12-07
 */
#ifndef DICT_HASH_HH
#define DICT_HASH_HH

#include <string>

namespace dict {
  const unsigned GOLDEN_RATIO_PRIME=(2^31) + (2^29) - (2^25) + (2^22) - (2^19) - (2^16) + 1;

  template<class Key>
  class hash_functor {
  public:
    hash_functor() {}
    unsigned operator()(const Key& key) const {
      return hash(key);
    }
  };
  
  unsigned hash(unsigned key) {
    return key * GOLDEN_RATIO_PRIME;
  }

  unsigned hash(int key) {
    return key * GOLDEN_RATIO_PRIME;
  }
  
  unsigned hash(const char* key) {
    return 0; // TODO:
  }

  unsigned hash(const std::string& key) {
    return 0; // TODO:
  }
}

#endif
