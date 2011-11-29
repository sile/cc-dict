#ifndef DICT_DICT_HH
#define DICT_DICT_HH

#include <cstddef>
#include <algorithm>

/*
 * TODO: 
 * - アロケータ自作 (ハッシュテーブル単位でも。template引数でmalloc版と独自版を選べるようにする)
 * - operator[]
 * - remove
 * - each
 * - clear
 * - Node::TAIL
 */
namespace dict {
  // TODO: 別ファイルに分ける
  // TODO: lisp/dictのcommit and push (inline展開指定ミス)
  template<typename Key>
  unsigned hash(const Key& key) {
    return key.hash();
  }

  template <>
  unsigned hash(const int& key) {
    return key;
  }

  template <>
  unsigned hash(const unsigned& key) {
    return key;
  }

  template<typename Key, typename Value>
  struct Node {
    unsigned hashcode;
    Key key;
    Value value;
    
    Node* next;
  };

  template <typename T>
  class Cache {
  public:
    Cache() : 
      used(NULL),
      free(NULL),
      used_size(0),
      free_size(0)
    {
      used = new Node;
      free = new Node;
    }

    ~Cache() {
      while(!used->empty()) used->delete_next();
      while(!free->empty()) free->delete_next();
        
      delete used;
      delete free;
    }

    T* get() {
      Node* x;
      if(free->empty()) {
        x = new Node;
      } else {
        x = free->next;
        x->unlink();
        free_size--;
      }
      used->append(x);
      used_size++;
      //std::cerr << "- " << (long)&x->buf << std::endl;
      return &x->buf;
    }

    void release(T* addr) { 
      Node* x = reinterpret_cast<Node*>(reinterpret_cast<char *>(addr)-(sizeof(Node*)*2));
      //std::cerr << "# " << (long)addr << std::endl;
      //std::cerr << "@ " << (long)&x->buf << std::endl;
      x->unlink();
      used_size--;

      // XXX: 8
      if(used_size+8 < free_size) {
        delete x;
      } else {
        free->append(x);
        free_size++;
      }
    }

  private:
    struct Node {
      Node* pred; // TODO: ちゃんとメンテする
      Node* next;
      T buf;
      
      Node() : pred(this), next(this) {}

      bool empty() const { return this == next; }
      
      void append(Node* node) {
        node->next = next;
        node->pred = this;
        next = node;
      }
      
      void unlink() {
        pred->next = next;
        next->pred = pred;
      }

      void delete_next() {
        Node* tmp = next;
        tmp->unlink();
        delete tmp;
      }
    };
    
  private:
    Node* used;
    Node* free;
    unsigned used_size;
    unsigned free_size;
  };

  template<typename T>
  class Allocator { // NodeAllocator
  private:
    struct Chunk {
      T* buf;
      unsigned size;
      Chunk* prev;

      Chunk(unsigned size) : buf(NULL), size(size), prev(NULL) {
        buf = new T[size];
      }

      ~Chunk() {
        delete [] buf;
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
      if(position == chunk->size)
        enlarge();
      
      return chunk->buf + (position++);
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

  class BucketsCache {
  public:
    void* allocate(unsigned size) {
      switch(size) {
      case sizeof(void*[8]):
        return c8.get();
      case sizeof(void*[16]):
        return c16.get();
      default:
        return new char[size];
      }
    }

    void release(void* addr, unsigned size) {
      switch(size) {
      case sizeof(void*[8]):
        c8.release(static_cast<void* (*)[8]>(addr));
        break;
      case sizeof(void*[16]):
        c16.release(static_cast<void* (*)[16]>(addr));
        break;
      default:
        delete [] static_cast<char*>(addr);
      }
    }
    
  private:
    Cache<void*[8]> c8;
    Cache<void*[16]> c16;    
  };
  
  // XXX:
  BucketsCache g_bc;

  template<typename Key, typename Value>
  class dict {
  public:
    dict() :
      buckets(NULL),
      bitlen(3),
      count(0),
      rehash_threshold(1.0),
      alc(8) // XXX:
    {
      bucket_size = 1 << bitlen;
      //buckets = new BucketNode* [bucket_size];
      buckets = new (g_bc.allocate(sizeof(BucketNode*)*bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, &TAIL);

      bitmask = (1 << bitlen)-1;
      recalc_rehash_border();

      TAIL.next = &TAIL;
      TAIL.hashcode = -1;
    }

    ~dict() {
      //delete [] buckets;
      g_bc.release(buckets, sizeof(BucketNode*)*bucket_size);
    }

    bool contains(const Key& key) const {
      Place p;
      return find_node(key, p);
    }

    unsigned size() const { return count; }
    
    bool put(const Key& key, const Value& value) {
      Place p;
      bool exists = find_node(key, p);
      if(exists) {
        p.node->value = value;
      } else {
        BucketNode* new_node = new (alc.allocate()) BucketNode;
        new_node->key = key;
        new_node->value = value;
        new_node->hashcode = p.hashcode;
        new_node->next = p.node;
        *p.place = new_node;

        count++;
        if(count >= rehash_border)
          enlarge();
      }

      return exists;
    }
    
    Value& get(const Key& key) const {
      Place p;
      if(find_node(key, p)) 
        return p.node->value;
      throw "TODO: error";
    }
    
  private:
    typedef Node<Key,Value> BucketNode;

    struct Candidate {
      BucketNode** place;
      BucketNode* pred;
      BucketNode* node;
    };

    struct Place {
      BucketNode** place;
      BucketNode* pred;  // XXX: 不要？
      BucketNode* node;
      unsigned hashcode;
    };

  private:
    void enlarge() {
      BucketNode** old_buckets = buckets;
      unsigned old_bucket_size = bucket_size;

      bitlen <<= 1;
      bucket_size = 1 << bitlen;
      bitmask = (1 << bitlen)-1;

      // buckets = new BucketNode*[bucket_size];
      buckets = new (g_bc.allocate(sizeof(BucketNode*)*bucket_size)) BucketNode* [bucket_size];
      std::fill(buckets, buckets+bucket_size, &TAIL);
      recalc_rehash_border();

      for(unsigned i=0; i < old_bucket_size; i++)
        for(BucketNode* node=old_buckets[i]; node != &TAIL;)
          node = rehash_node(node);
      //delete [] old_buckets;
      //std::cerr << "+" << (long)old_buckets << std::endl;
      g_bc.release(old_buckets, sizeof(BucketNode*)*old_bucket_size);
    }
    
    BucketNode* rehash_node(BucketNode* node) {
      BucketNode* next = node->next;

      Candidate ca;
      find_candidate(node->hashcode, ca);
      node->next = ca.node;
      *ca.place = node;

      return next;
    }

    void recalc_rehash_border() {
      rehash_border = bucket_size * rehash_threshold;
    }

    bool find_node(const Key& key, Place& p) const {
      const unsigned hashcode = hash(key) & 0x7FFFFFFF; // XXX: sizeof(unsigned)==32 以外の環境に対応

      Candidate ca;
      find_candidate(hashcode, ca);

      for(;;) {
        if(hashcode != ca.node->hashcode) {
          p.node = ca.node;
          p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? ca.place : &ca.pred->next;
          p.hashcode = hashcode;            
          return false;
        } 
        if(key == ca.node->key) {
          p.node = ca.node;
          p.pred = ca.pred;
          p.place = ca.pred==&TAIL ? ca.place : &ca.pred->next;
          p.hashcode = hashcode;
          return true;
        }
        ca.pred = ca.node;
        ca.node = ca.node->next;
      }
    }

    void find_candidate(const unsigned hashcode, Candidate& ca) const {
      const unsigned index = hashcode & bitmask;
      
      BucketNode* pred = const_cast<BucketNode*>(&TAIL);
      BucketNode* node = buckets[index];
      for(; node->hashcode < hashcode; pred=node, node=node->next);

      ca.place = pred==&TAIL ? &buckets[index] : &pred->next;
      ca.pred = pred;
      ca.node = node;
    }

  private:
    BucketNode** buckets;
    unsigned bucket_size; // => buckets_size
    unsigned bitlen;
    unsigned bitmask;
    unsigned count;
    
    unsigned rehash_border;
    float rehash_threshold;
    
    BucketNode TAIL;

    Allocator<BucketNode> alc;
  };
}

#endif
