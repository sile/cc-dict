/**
 * @license cc-dict 0.0.1
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 * Date: 2011-12-07
 */
#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

namespace dict {

#ifdef TLS_ENABLED

#define dict__offsetof(TYPE, FIELD, DUMMY_PTR) \
((char*)(&((TYPE *) DUMMY_PTR)->FIELD)-(char*)DUMMY_PTR)
#define dict__obj_ptr(TYPE, FIELD, FIELD_PTR) \
((TYPE*)((char*)FIELD_PTR - dict__offsetof(TYPE, FIELD, FIELD_PTR)))

  class cached_allocator {
  private:
    struct cache {
      cache* next;
      char chunk[1];

      cache() : next(this) {}

      bool empty() const { return this == next; }
      
      void append(cache* node) {
        node->next = next;
        next = node;
      }
      
      void unlink_next() {
        next = next->next;
      } 

      static cache* allocate(unsigned size) {
        void* p = new char[sizeof(cache)-1+size];
        return reinterpret_cast<cache*>(p);
      }
    };

    struct cache_head {
      cache* free;
      unsigned count;
      unsigned free_count;

      cache* pop() {
        free_count--;
        
        cache* p = free->next;
        free->unlink_next();
        return p;
      }

      void push(unsigned size) {
        count++;
        free_count++;
        free->append(cache::allocate(size));
      }

      void push(cache* p) {
        free_count++;
        free->append(p);
      }

      bool empty() const {
        return free_count==0;
      }

      bool oversupply() const {
        return free_count > ((count+4)*3/4);
      }

      void adjust() {
        for(unsigned i=0; i < free_count/2; i++) {
          cache* p = pop();
          delete p;
          count--;
        }
      }
    };

  public:
    void init() {
      for(unsigned i=0; i < CACHE_COUNT; i++) {
        caches[i].free = new cache;
        caches[i].count = 0;
        caches[i].free_count = 0;
      }
    }

    void* allocate(unsigned size) {
      if(size < CACHED_CHUNK_SIZE_LIMIT) {
        cache_head& head = caches[size/CHUNK_UNIT];
        if(head.empty()) {
          unsigned adj_size = (size + (CHUNK_UNIT-1)) & ~(CHUNK_UNIT-1);
          head.push(adj_size);
        }
        return head.pop()->chunk;
      } 
      
      return std::malloc(size);
    }
    
    void release(void* ptr, unsigned size) {
      if(size < CACHED_CHUNK_SIZE_LIMIT) {
        cache_head& head = caches[size/CHUNK_UNIT];
        cache* c = dict__obj_ptr(cache, chunk, ptr);
        head.push(c);
        if(head.oversupply())
          head.adjust();
        return;
      }
      std::free(ptr);
    }

  private:
    static const unsigned CHUNK_UNIT=32;
    static const unsigned CACHED_CHUNK_SIZE_LIMIT=1024;
    static const unsigned CACHE_COUNT=CACHED_CHUNK_SIZE_LIMIT/CHUNK_UNIT;
    
    cache_head caches[CACHE_COUNT];
  };

  // XXX:
  namespace {
    thread_local bool initialized = false;
    thread_local cached_allocator g_cached_alloca;
  }

  template <class T>
  class array_allocator {
  public:
    static void* allocate(unsigned count) {
      if(initialized==false) {
        g_cached_alloca.init();
        initialized = true;
      }
      return g_cached_alloca.allocate(sizeof(T)*count);
    }
    static void release(void* ptr, unsigned count) {
      g_cached_alloca.release(ptr, sizeof(T)*count);
    }    
  };

#else
  
  template <class T>
  class array_allocator {
  public:
    static void* allocate(unsigned count) {
      return std::malloc(sizeof(T)*count);
    }
    static void release(void* ptr, unsigned size) {
      std::free(ptr);
    }    
  };
  
#endif

  template<unsigned CHUNK_SIZE, class ChunkAllocator=array_allocator<char[CHUNK_SIZE]> >
  class fixed_size_allocator { 
  private:
    static const unsigned INITIAL_BLOCK_SIZE=8;

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
      
      chunk_block(unsigned size) : chunks(NULL), size(size), prev(NULL) {
        chunks = new (ChunkAllocator::allocate(size)) chunk[size];
      }
      
      ~chunk_block() {
        ChunkAllocator::release(chunks, size);
        delete prev;
      }
    };

  public:
    fixed_size_allocator()
      : block(NULL), position(0), recycle_count(0) {
      block = new chunk_block(INITIAL_BLOCK_SIZE);
    }
      
    ~fixed_size_allocator() {
      clear();
      delete block;
    }
    
    void* allocate() {
      chunk* p;
      
      if(recycle_count > 0) {
        chunk* head = peek_chunk();
        p = head->free_next;
        head->free_next = p->free_next;

        recycle_count--;
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
      recycle_count = 0;
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
      chunk_block* new_block = new chunk_block(block->size*2);
      new_block->prev = block;
      block = new_block;
      position = 0;
    }

  private:
    chunk_block* block;
    unsigned position;
    unsigned recycle_count;
  };
}

#endif
