#include <iostream>
#include "dict/dict.hh"
#include <cstdlib>

#include <tr1/unordered_map>

using namespace std;
using namespace tr1;

#define DSIZE 1000000
#define N 10

/**
 * clock function
 */
#ifdef __unix__

#include <sys/time.h>
inline double gettime(){
  timeval tv;
  gettimeofday(&tv,NULL);
  return static_cast<double>(tv.tv_sec)+static_cast<double>(tv.tv_usec)/1000000.0;
}

#else

#include <ctime>
inline double gettime() {
  return static_cast<double>(clock())/CLOCKS_PER_SEC;
}

#endif

struct chunk {
  struct {
    const char* type;
    int size;
  } tag;
  void* buf;
};

inline void* mem_allocate(const char* type, int size) {
  chunk* c = (chunk*)new char[sizeof(chunk)-sizeof(chunk::buf)+size];
  c->tag.type = type;
  c->tag.size = size;

  std::cout << "[" << c->tag.type << "] allocate: " << c->tag.size << " bytes" << std::endl;
  return &c->buf;
}

void mem_free(void* p) {
  chunk* c = obj_ptr(chunk, buf, p);
  std::cout << "[" << c->tag.type << "] free: " << c->tag.size << " bytes" << std::endl;
  delete c;
}

struct abc {
  int a;
  char b;
  long c;
};

void dict_bench(int data1[DSIZE], int data2[DSIZE]) {
  void* ptrs[3];

  std::cout << "# allocate:" << std::endl;
  ptrs[0] = new (mem_allocate("int", sizeof(int))) int;
  ptrs[1] = new (mem_allocate("pointer", sizeof(void*))) void*;
  ptrs[2] = new (mem_allocate("struct", sizeof(abc))) abc;
  
  std::cout << std::endl;
  std::cout << "# free:" << std::endl;
  for(int i=2; i >=0; i--)
    mem_free(ptrs[i]);

  dict::dict<int,int> dic;
  double beg_t = gettime();
  int count = 0;

  for(int i=0; i < DSIZE; i++) {
    dic.put(data1[i], data1[i]);
  }
  std::cout << " put: " << gettime()-beg_t << " #" << dic.size() << std::endl;

  beg_t = gettime();
  count = 0;
  for(int i=0; i < DSIZE; i++) {
    if(dic.contains(data1[i]))
      count++;
  }
  std::cout << " get1: " << gettime()-beg_t << " #" << count << std::endl;
  
  count = 0;
  for(int i=0; i < DSIZE; i++) {
    if(dic.contains(data2[i]))
      count++;
  }
  std::cout << " get2: " << gettime()-beg_t << " #" << count << std::endl;
}

void dict_bench_small(int data1[DSIZE], int data2[DSIZE]) {
  double beg_t = gettime();
  unsigned count = 0;

  for(int i=0; i < DSIZE; i += N) {
    dict::dict<int,int> dic;  
    
    for(int j=0; j < N; j++)
      dic.put(data1[i+j], data1[i+j]);
    for(int j=0; j < N; j++)
      if(dic.contains(data1[i+j]))
        count++;
    for(int j=0; j < N; j++)
      if(dic.contains(data2[i+j]))
        count++;
  }
  std::cout << " put, get1, get2: " << gettime()-beg_t << " #" << count << std::endl;
}

void unorderedmap_bench_small(int data1[DSIZE], int data2[DSIZE]) {
  double beg_t = gettime();
  unsigned count = 0;

  for(int i=0; i < DSIZE; i += N) {
    unordered_map<int,int> dic;

    for(int j=0; j < N; j++)
      dic[data1[i+j]] = data1[i+j];
    for(int j=0; j < N; j++)
      if(dic.find(data1[i+j]) != dic.end())
        count++;
    for(int j=0; j < N; j++)
      if(dic.find(data2[i+j]) != dic.end())
        count++;
  }
  std::cout << " put, get1, get2: " << gettime()-beg_t << " #" << count << std::endl;
}

void unorderedmap_bench(int data1[DSIZE], int data2[DSIZE]) {
  unordered_map<int,int> dic;
  double beg_t = gettime();
  int count = 0;

  for(int i=0; i < DSIZE; i++) {
    dic[data1[i]] = data1[i];
  }
  std::cout << " put: " << gettime()-beg_t << " #" << dic.size() << std::endl;

  beg_t = gettime();
  count = 0;
  for(int i=0; i < DSIZE; i++) {
    if(dic.find(data1[i]) != dic.end())
      count++;
  }
  std::cout << " get1: " << gettime()-beg_t << " #" << count << std::endl;
  
  count = 0;
  for(int i=0; i < DSIZE; i++) {
    if(dic.find(data2[i]) != dic.end())
      count++;
  }
  std::cout << " get2: " << gettime()-beg_t << " #" << count << std::endl;  
}

int main(int argc, char** argv) {
  int data1[DSIZE];
  int data2[DSIZE];

  for(int i=0; i < DSIZE; i++) {
    data1[i] = rand() % (DSIZE*5);
    data2[i] = rand() % (DSIZE*5);
  }
  
  double beg_t = gettime();
  std::cout << "dict: large" << std::endl;
  dict_bench(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;
  
  beg_t = gettime();
  std::cout << "unorderedmap: large" << std::endl;
  unorderedmap_bench(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;  
  
  beg_t = gettime();
  std::cout << "dict: small" << std::endl;
  dict_bench_small(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;

  beg_t = gettime();
  std::cout << "unorderedmap: small" << std::endl;
  unorderedmap_bench_small(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;

  return 0;
}
