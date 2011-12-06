#ifndef DICT_ALLOCATOR_HH
#define DICT_ALLOCATOR_HH

#include <cstdlib>

namespace dict {
  template<unsigned CHUNK_SIZE>
  class fixed_size_allocator { 
  private:
    static const unsigned INITIAL_SIZE=4;

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
        delete chunks;
      }
    };

  public:
    fixed_size_allocator()
      : position(0), recycle_count(0), init_block(INITIAL_SIZE), block(&init_block) {}

    ~fixed_size_allocator() {
      if(block != &init_block)
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
      for(chunk_block* b=block->prev; b != &init_block; b = b->prev)
        delete b;
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
    unsigned position;
    unsigned recycle_count;

    chunk_block init_block;
    chunk_block* block;
  };
}

#endif
