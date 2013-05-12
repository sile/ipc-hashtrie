#include <iht/hashtrie.hh>
#include <iostream>

int main(int argc, char** argv) {
  iht::HashTrie trie(1024*1024*10);
  if(! trie) {
    std::cerr << "trie initialization failed" << std::endl;
    return 1;
  }

  trie.store("key", "value");
  trie.store("1", "2");
  trie.store("a", "b");
  trie.store("key", "value2");

  std::cout << "size: " << trie.size() << std::endl;

  return 0;
}
