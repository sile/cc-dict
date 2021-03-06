/**
 * @license cc-dict 0.0.7
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_HASH_HH
#define DICT_HASH_HH

#include <string>

namespace dict {
  const unsigned GOLDEN_RATIO_PRIME=0x9e370001; // (2^31) + (2^29) - (2^25) + (2^22) - (2^19) - (2^16) + 1;

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
    // XXX: if sizeof(long) > sizeof(unsigned), the calculation will lose high bits information of key.
    return key * GOLDEN_RATIO_PRIME;
  }

  unsigned hash(const char* key) {
    return hash(reinterpret_cast<long>(key));
  }

  unsigned hash(const std::string& key) {
    unsigned h = GOLDEN_RATIO_PRIME;
    for(const char* c=key.c_str(); *c != 0; c++)
      h = (h*33) + *c;
    return h;
  }
}

#endif
