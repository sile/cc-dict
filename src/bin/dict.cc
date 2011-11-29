#include "dict/dict.hh"
#include <iostream>
#include <cstdlib>
using namespace std;

int main(int argc, char** argv) {
  dict::dict<int, int> d;
  int key = atoi(argv[1]);
  
  std::cout << key << ": " << d.contains(key) << std::endl;
  return 0;
}
