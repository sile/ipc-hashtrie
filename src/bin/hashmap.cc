#include <iostream>
#include <tr1/unordered_map>
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

void gen_keys(std::vector<std::string> & keys, int num) {
  char buf[1024];
  for(int i=0; i < num; i++) {
    sprintf(buf, "%d", rand());
    keys.push_back(buf);
  }
}

int main(int argc, char** argv) {
  std::tr1::unordered_map<std::string, std::string> map;

  std::vector<std::string> keys;
  gen_keys(keys, 50000);
  
  NanoTimer store_time;
  for(unsigned i=0; i < keys.size(); i++) {
    map[keys[i]] = keys[i];
  }
  std::cout << "store elapsed: " << store_time.elapsed_ms() << " ms" << std::endl;

  NanoTimer find_time;

  for(unsigned i=0; i < keys.size(); i++) {
    map.find(keys[i]);
  }
  std::cout << "find elapsed: " << find_time.elapsed_ms() << " ms" << std::endl;  

  std::cout << "size: " << map.size() << std::endl;

  return 0;
}
