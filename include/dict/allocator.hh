#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

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

  
  // TODO: 整理
  template<typename T, class Alloca = CachedAllocator<T> >
  class Allocator { // NodeAllocator
  private:
    struct Chunk {
      unsigned recycles;
      T* buf;
      unsigned size;
      Chunk* prev;
      Alloca& a;

      Chunk(unsigned size) : recycles(0), buf(NULL), size(size), prev(NULL), a(Alloca::instance()) {
        buf = new (a.allocate(size)) T[size];
      }

      ~Chunk() {
        a.free(buf);
        delete prev;
      }
    };

  public:
    Allocator(unsigned initial_size) :
      chunk(NULL),
      position(0)
    {
      chunk = new Chunk(initial_size);
    }

    ~Allocator() {
      delete chunk;
    }
    
    T* allocate() {
      if(chunk->recycles > 0) {
        T* head = chunk->buf + position;
        T* p = head->next;
        
        if(--chunk->recycles > 0)
          head->next = p->next;
        
        return p;
      }
      
      if(position == chunk->size-1) // XXX: releaseとうまいことタイミングを合わせて-1はなくす
        enlarge();
      
      return chunk->buf + (position++);
    }

    void release(T* ptr) {
      // TODO: T = Node<,> として明示的に表明する (recyclesも不要になる)
      T* head = chunk->buf + position;
      if(chunk->recycles > 0)  // recyclesはchunkではなく、Allocaterにつくべき
        ptr->next = head->next;
      
      chunk->recycles++;
      head->next = ptr;
    }
    
  private:
    void enlarge () {
      position = 0;
      Chunk* new_chunk = new Chunk(chunk->size*2);
      new_chunk->prev = chunk;
      chunk = new_chunk;
    }

  private:
    Chunk* chunk;
    unsigned position;
  };


}

#endif
