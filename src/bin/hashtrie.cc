#include <iht/hashtrie.hh>
#include <iostream>

int main(int argc, char** argv) {
  iht::HashTrie trie(1024*10);
  if(! trie) {
    std::cerr << "trie initialization failed" << std::endl;
    return 1;
  }
  
  return 0;
}
