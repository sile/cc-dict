#include <iostream>
#include "dict/dict.hh"
#include <cstdlib>

#include <tr1/unordered_map>

using namespace std;
using namespace tr1;

#define DSIZE 1000
//000

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

void dict_bench(int data1[DSIZE], int data2[DSIZE]) {
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
  std::cout << "dict: " << std::endl;
  dict_bench(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;
  
  beg_t = gettime();
  std::cout << "unorderedmap: " << std::endl;
  unorderedmap_bench(data1, data2);
  std::cout << " => total: " << gettime()-beg_t << std::endl << std::endl;;  
  /*
  dict::dict<int, int> d;
  int key = atoi(argv[1]);

  for(int i=0; i < 100; i++)
    d.put(i, i*10);

  std::cout << key << ": " << (d.contains(key) ? d.get(key) : -1) << std::endl;
  */
  return 0;
}
