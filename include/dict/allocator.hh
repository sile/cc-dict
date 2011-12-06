#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>
#include <cstring>
#define offsetof(TYPE, FIELD, DUMMY_PTR) \
  ((char*)(&((TYPE *) DUMMY_PTR)->FIELD)-(char*)DUMMY_PTR)
#define obj_ptr(TYPE, FIELD, FIELD_PTR) \
  ((TYPE*)((char*)FIELD_PTR - offsetof(TYPE, FIELD, FIELD_PTR)))

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

  template<unsigned CHUNK_SIZE, class Alloca = CachedAllocator<char[CHUNK_SIZE]> >
  class fixed_size_allocator { 
  private:
    struct chunk {
      union {
        char buf[CHUNK_SIZE];
        chunk* next;
      };
    };
    
    struct chunk_block {
      chunk* chunks;
      unsigned size;
      chunk_block* prev;
      Alloca& a; // XXX:

      chunk_block(unsigned size) : chunks(NULL), size(size), prev(NULL), a(Alloca::instance()) {
        chunks = new (a.allocate(size)) chunk[size];
        init();
      }
      
      ~chunk_block() {
        a.free(chunks);
        delete prev;
      }

      void init() {
        for(unsigned i=0; i < size-1; i++)
          chunks[i].next = &chunks[i+1];
        chunks[size-1].next = NULL;        
      }
    };

  public:
    fixed_size_allocator(unsigned initial_size) : block(NULL), head(NULL) {
      block = new chunk_block(initial_size);
      head = block->chunks;
    }

    ~fixed_size_allocator() {
      delete block;
    }
    
    void* allocate() {
      if(head==NULL)
        enlarge();

      chunk* use = head;
      head = use->next;
      return use;
    }

    void release(void* ptr) {
      chunk* free = reinterpret_cast<chunk*>(ptr);
      free->next = head;
      head = free;
    }

    void clear() {
      delete block->prev;
      block->prev = NULL;
      block->init();
      head = block->chunks;
    }

  private:
    void enlarge () {
      chunk_block* new_block = new chunk_block(block->size*2);
      new_block->prev = block;

      block = new_block;
      head = block->chunks;
    }

  private:
    chunk_block* block;
    chunk* head;
  };
}

#endif
