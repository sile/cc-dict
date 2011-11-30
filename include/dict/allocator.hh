#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

namespace dict {
  template <typename T>
  class GeneralAllocator {
  public:
    void* allocate() {
      return std::malloc(sizeof(T));
    }
    
    void* allocate(unsigned count) {
      return std::malloc(sizeof(T)*count);
    }

    // TODO: rename
    void free(void* ptr) {
      std::free(ptr);
    }

    static GeneralAllocator& instance() {
      static GeneralAllocator a;
      return a;
    }
  };

  class CacheAllocator {
    
  };

  // TODO: rename
  class NodeAllocator {
  };
}

#endif
