#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

#define offsetof(TYPE, FIELD) ((std::size_t) &((TYPE *) 0)->FIELD)
#define obj_ptr(TYPE, FIELD, FIELD_PTR) ((TYPE*)((char*)FIELD_PTR - offsetof(TYPE, FIELD)))

namespace dict {
  /**
   * GeneralAllocator
   */
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

  template <typename T>
  class Cache {
  public:
    ~Cache() {
      while(! used.empty()) delete used.pop();
      while(! free.empty()) delete free.pop();
    }

    T* get() {
      Chunk* x = free.empty() ? new Chunk : reinterpret_cast<Chunk*>(free.pop());
      used.push(x);
      return &x->buf;
    }

    void release(void* addr) { 
      Chunk* x = obj_ptr(Chunk, buf, addr);
      used.remove(x);
      
      if(used.count+8 < free.count)
        delete x;
      else
        free.push(x);
    }

  private:
    struct Link {
      Link* pred;
      Link* next;

      Link() : pred(this), next(this) {}

      bool empty() const { return this == next; }
      
      void append(Link* node) {
        node->next = next;
        node->pred = this;
        next->pred = node;
        next = node;
      }
      
      void unlink() {
        pred->next = next;
        next->pred = pred;
      }      
    };
    
    struct Chunk : public Link {
      T buf;
    };

    struct Head : public Link {
      Head() : count(0) {}
      
      void push(Link* node) {
        count++;
        append(node);
      }

      Link* pop() {
        count--;
        Link* tmp = Link::next; // XXX: 
        tmp->unlink();
        return tmp;
      }

      void remove(Link* x) {
        x->unlink();
        count--;
      }
      unsigned count;
    };
    
  private:
    Head used;
    Head free;
  };
  
  template <typename T>
  class CachedAllocator {
  public:
   void* allocate() {
     return allocate(1);
    }
    
    void* allocate(unsigned count) {
      void *ptr;
      if(count > 32)
        ptr = std::malloc(sizeof(unsigned) + sizeof(T)*count);
      else if(count > 16)
        ptr = x32.get();
      else if(count > 8)
        ptr = x16.get();
      else if(count > 4)
        ptr = x8.get();
      else
        ptr = x4.get();

      Chunk<int>* c = reinterpret_cast<Chunk<int>*>(ptr);
      c->count = count;
      //std::cerr << "@ " << (long)&c->chunk << ": " << c->count << std::endl;

      return &c->chunk;
    }

    // TODO: rename
    void free(void* ptr) {
      Chunk<int>* c = obj_ptr(Chunk<int>, chunk, ptr);
      //std::cerr << "# " << (long)ptr << ": " << c->count << std::endl;
      if(c->count > 32)
        std::free(c);
      else if(c->count > 16)
        x32.release(c);
      else if(c->count > 8)
        x16.release(c);
      else if(c->count > 4)
        x8.release(c);
      else
        x4.release(c);
    }

    static CachedAllocator& instance() {
      static CachedAllocator a;
      return a;
    }
    
  private:
    template<typename U>
    struct Chunk {
      unsigned count;
      U chunk;
    };

  private:
    Cache<Chunk<T[4]> > x4;
    Cache<Chunk<T[8]> > x8;
    Cache<Chunk<T[16]> > x16;
    Cache<Chunk<T[32]> > x32;
  };

  // TODO: rename
  class NodeAllocator {
  };
}

#endif
