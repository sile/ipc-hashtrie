#include <iht/hashtrie.hh>
#include <iostream>

std::string toStr(iht::String s) {
  if(s) {
    return std::string(s.data(), s.size());
  } else {
    return "";
  }
}

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

  iht::View view(trie);
  
  std::cout << "# find: " << std::endl
            << "  key: " << toStr(view.find("key")) << std::endl
            << "  key2: " << toStr(view.find("key2")) << std::endl
            << "  a: " << toStr(view.find("a")) << std::endl
            << "  1: " << toStr(view.find("1")) << std::endl
            << std::endl;

  return 0;
}
