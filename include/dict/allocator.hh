#ifndef DICT_DICT_HH
#define DICT_DICT_HH

namespace dict {
  class GeneralAllocator {
  public:
    template <typename T>
    void* allocate() {
      return new T;
    }
    
    template <typename T>
    void* allocate(unsigned count) {
      return new T[count];
    }

    
  };

  class CachedChunkAllocator {
    
  };

  class NodeAllocator {
  };
}

#endif
