/**
 * @license cc-dict 0.0.7
 * Copyright (c) 2011, Takeru Ohta, phjgt308@gmail.com
 * MIT license
 * https://github.com/sile/cc-dict/blob/master/COPYING
 */
#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>
#include <vector>
#include <iostream>

namespace dict {
  template<class T>
  class fixed_size_allocator {
  public:
    /*
    template<class U>
    struct ptr_t {
      ptr_t(fixed_size_allocator& a) {
        ptr = (U)a.allocate();
        ptr->hashcode = 0xFFFFFFFF; // XXX
      }

      ptr_t() : ptr(NULL) {}
      
      const U ref(const fixed_size_allocator& a) const {
        return ptr;
      }
      
      bool operator!=(const ptr_t& x) const {
        return ptr != x.ptr;
      }

      bool operator==(const ptr_t& x) const {
        return ptr == x.ptr;
      }

      U ptr;
    };
    */

    template<class U>
    struct ptr_t {
      ptr_t(fixed_size_allocator& a) {
        b.base = a.vec.size()-1;
        b.offset = a.position;
        //std::cout << "# " << a.vec.size() << ":" << b.base << ", " << b.offset << std::endl;
        
        U ptr = (U)a.allocate();
        ptr->hashcode = 0xFFFFFFFF; // XXX
      }

      ptr_t() : n(0) {}
      
      const U ref(const fixed_size_allocator& a) const {
        //std::cout << "IN: " << b.base << ", " << b.offset << std::endl;
        return (U)(&a.vec[b.base]->chunks[b.offset]);
      }
      
      bool operator!=(const ptr_t& x) const {
        return n != x.n;
      }

      bool operator==(const ptr_t& x) const {
        return n == x.n;
      }

      union {
        struct {
          unsigned base:8;
          unsigned offset:24;
        } b;
        unsigned n;
      };
    };
    
    typedef ptr_t<T*> index_t;
    
  private:
    static const unsigned CHUNK_SIZE = sizeof(T);
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
        chunks = new chunk[size];
      }
      
      ~chunk_block() {
        delete [] chunks;
        delete prev;
      }
    };

  public:
    fixed_size_allocator()
      : block(NULL), position(0), recycle_count(0) {
      block = new chunk_block(INITIAL_BLOCK_SIZE);
      vec.push_back(block);
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
      if(block->size > 10000000)
        std::cout << "enlarge: " << block->size << std::endl;
      chunk_block* new_block = new chunk_block(block->size*1.25);
      new_block->prev = block;
      block = new_block;
      position = 0;
      vec.push_back(block);
    }

    //  protected:
  public:
    chunk_block* block;
    unsigned position;
    unsigned recycle_count;

    std::vector<chunk_block*> vec;
  };
}

#endif
