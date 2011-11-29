#include <iostream>
#include "dict/dict.hh"
#include <cstdlib>
using namespace std;

int main(int argc, char** argv) {
  dict::dict<int, int> d;
  int key = atoi(argv[1]);

  for(int i=0; i < 10; i++)
    d.put(i, i*10);

  std::cout << key << ": " << d.contains(key) << std::endl;
  return 0;
}
