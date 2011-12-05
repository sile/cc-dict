#ifndef DICT_HASH_HH
#define DICT_HASH_HH

namespace dict {
  const unsigned GOLDEN_RATIO_PRIME=(2^31) + (2^29) - (2^25) + (2^22) - (2^19) - (2^16) + 1;

  template<typename Key>
  class hash {
  public:
    unsigned operator()(const Key& key) const {
      return key.hash();
    }
  };

  template<>
  class hash<int> {
  public:
    unsigned operator()(int  key) const {
      return key * GOLDEN_RATIO_PRIME;
    }
  };

  template<>
  class hash<unsigned> {
  public:
    unsigned operator()(unsigned  key) const {
      return key * GOLDEN_RATIO_PRIME;
    }
  };
}

#endif
