#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

#define offsetof(TYPE, FIELD, DUMMY_PTR) \
  ((char*)(&((TYPE *) DUMMY_PTR)->FIELD)-(char*)DUMMY_PTR)
#define obj_ptr(TYPE, FIELD, FIELD_PTR) \
  ((TYPE*)((char*)FIELD_PTR - offsetof(TYPE, FIELD, FIELD_PTR)))

namespace dict {
  template <class T>
  class general_allocator {
  public:
    static general_allocator* create() { return NULL; }

    void* allocate() { return std::malloc(sizeof(T)); }
    void* allocate(unsigned count) { return std::malloc(sizeof(T)*count); }
    void release(void* ptr) { std::free(ptr); }

  private:
    general_allocator() {}
  };

  /**
   * GeneralAllocator
   */
  template <typename T>
  class GeneralAllocator {
  public:
    GeneralAllocator() {}

    void* allocate() {
      return std::malloc(sizeof(T));
    }
    
    void* allocate(unsigned count) {
      return std::malloc(sizeof(T)*count);
    }

    void release(void* ptr) {
      std::free(ptr);
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

  template<unsigned CHUNK_SIZE, class Alloca = CachedAllocator<char[CHUNK_SIZE]> >
  class fixed_size_allocator { 
  private:
    struct chunk {
      union {
        char buf[CHUNK_SIZE];
        chunk* free_next;
      };
    };
    
    struct chunk_block {
      chunk* chunks;
      unsigned size;
      chunk_block* prev;
      Alloca& a; // XXX:

      chunk_block(unsigned size) : chunks(NULL), size(size), prev(NULL), a(Alloca::instance()) {
        chunks = new (a.allocate(size)) chunk[size];
      }
      
      ~chunk_block() {
        a.free(chunks);
        delete prev;
      }
    };

  public:
    fixed_size_allocator(unsigned initial_size) : block(NULL), position(0), recycle_count(0) { 
      block = new chunk_block(initial_size);
    }

    ~fixed_size_allocator() {
      delete block;
    }
    
    void* allocate() {
      chunk* p;
      
      if(recycle_count > 0) {
        recycle_count--;
        
        chunk* head = peek_chunk();
        p = head->free_next;
        head->free_next = p->free_next;
      } else {
        p = read_chunk();
      }
      
      return p;
    }

    void release(void* ptr) {
      chunk* free = reinterpret_cast<chunk*>(ptr);
      chunk* head = peek_chunk();
      free->free_next = head->free_next;
      head->free_next = free;
      recycle_count++;
    }

    void clear() {
      delete block->prev;
      block->prev = NULL;
      position = 0;
    }

  private:
    chunk* read_chunk() {
      chunk* p = peek_chunk();
      if(++position == block->size)
        enlarge();      
      return p;
    }
    
    chunk* peek_chunk() const {
      return block->chunks + position;
    }
    
    void enlarge () {
      position = 0;
      chunk_block* new_block = new chunk_block(block->size*2);
      new_block->prev = block;
      block = new_block;
    }

  private:
    chunk_block* block;
    unsigned position;
    unsigned recycle_count;
  };
}

#endif
