#include <iht/hashtrie.hh>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

// ナノ秒単位の時間計測用のタイマー
// ※ 現在は可搬性のために gettimeofday(μ秒単位の時刻取得関数) を使用している
class NanoTimer {
public:
  NanoTimer() {
    gettimeofday(&t, NULL);
  }
  
  long elapsed() const {
    timeval now;
    gettimeofday(&now, NULL);
    return ns(now) - ns(t);
  }

  long elapsed_ms() const {
    return elapsed() / 1000 / 1000;
  }
  
private:
  long ns(const timeval& ts) const {
    return static_cast<long>(static_cast<long long>(ts.tv_sec)*1000*1000*1000 + ts.tv_usec*1000);
  }
  
private:
  timeval t;
};

std::string toStr(iht::String s) {
  if(s) {
    return std::string(s.data(), s.size());
  } else {
    return "";
  }
}

void gen_keys(std::vector<std::string> & keys, int num) {
  char buf[1024];
  for(int i=0; i < num; i++) {
    sprintf(buf, "%d", rand());
    keys.push_back(buf);
  }
}

int main(int argc, char** argv) {
  iht::HashTrie trie(1024*1024*100);
  if(! trie) {
    std::cerr << "trie initialization failed" << std::endl;
    return 1;
  }

  trie.store("key", "value");
  trie.store("1", "2");
  trie.store("a", "b");
  trie.store("key", "value2");

  for(int i=0; i < 20; i++) {
    char key[] = "a";
    key[0] = key[0] + i;
    trie.store(key, key);
  }

  std::vector<std::string> keys;
  gen_keys(keys, 50000);
  
  NanoTimer store_time;
  for(unsigned i=0; i < keys.size(); i++) {
    trie.store(keys[i], keys[i]);
  }
  std::cout << "store elapsed: " << store_time.elapsed_ms() << " ms" << std::endl;

  NanoTimer find_time;

  for(unsigned i=0; i < keys.size(); i++) {
    iht::View v(trie);
    v.find(keys[i]);
  }
  std::cout << "find elapsed: " << find_time.elapsed_ms() << " ms" << std::endl;  

  std::cout << "size: " << trie.size() << std::endl;

  iht::View view(trie);
  
  std::cout << "# find: " << std::endl
            << "  key: " << toStr(view.find("key")) << std::endl
            << "  key2: " << toStr(view.find("key2")) << std::endl
            << "  a: " << toStr(view.find("a")) << std::endl
            << "  1: " << toStr(view.find("1")) << std::endl
            << "  d: " << toStr(view.find("d")) << std::endl
            << std::endl;

  return 0;
}
