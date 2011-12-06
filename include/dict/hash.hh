/**
 * @license cc-dict 0.0.1
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 * Date: 2011-12-07
 */
#ifndef DICT_HASH_HH
#define DICT_HASH_HH

namespace dict {
  const unsigned GOLDEN_RATIO_PRIME=(2^31) + (2^29) - (2^25) + (2^22) - (2^19) - (2^16) + 1;

  template<typename Key>
  class hash {
  public:
    hash() {}
    unsigned operator()(const Key& key) const {
      return key.hash();
    }
  };

  template<>
  class hash<int> {
  public:
    hash() {}
    unsigned operator()(int  key) const {
      return key * GOLDEN_RATIO_PRIME;
    }
  };

  template<>
  class hash<unsigned> {
  public:
    hash() {}
    unsigned operator()(unsigned  key) const {
      return key * GOLDEN_RATIO_PRIME;
    }
  };
}

#endif
