#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <utility>
#include "nano_timer.hh"

#include <iht/hashtrie.hh>
#include <pthread.h>

class SyncHashMap {
public:
  SyncHashMap() : impl_(1024*1024*250) {
    int ret = pthread_mutex_init(&mtx_, NULL);
    assert(ret == 0);
  }

  ~SyncHashMap() {
    int ret = pthread_mutex_destroy(&mtx_);
    assert(ret == 0);
  }

  void store(const std::string & key, const std::string & value) {
    pthread_mutex_lock(&mtx_);
    impl_.store(key, value);
    pthread_mutex_unlock(&mtx_);
  }

  bool find(const std::string & key, std::string & value, iht::View & v) {
    v.updateIfNeed();
    iht::String s = v.find(key);
    //iht::View v(impl_);
    //iht::String s = v.find(key);
    if(s) {
      value.assign(s.data(), s.size());
      return true;
    } else {
      return false;
    }
  }
  
  bool isMember(const std::string & key, iht::View & v) {
    v.updateIfNeed();
    return v.find(key);
    //iht::View v(impl_);
    //return v.find(key);
  }

  size_t size() {
    return impl_.size();
  }

  template <class Callback>
  void foreach(Callback & callback) {
    // TODO:
  }
  
  iht::HashTrie & getTrie() { return impl_; }
  
private:
  iht::HashTrie impl_;
  pthread_mutex_t mtx_;
};

struct Param {
  int thread_num;
  int key_num;
  int read_num;
  int sum_num;
  bool write_first;

  SyncHashMap map;
};

enum OP {
  OP_WRITE,
  OP_READ,
  OP_SUM
};

struct ThreadParam {
  Param * param;
  std::vector<std::pair<OP, std::string> > * keys;
  int id;
};

typedef std::vector<std::pair<OP, std::string> > Keys;

void gen_keys(std::vector<std::pair<OP, std::string> > & keys, const Param & p) {
  char buf[1024];
  for(int i=0; i < p.key_num; i++) {
    sprintf(buf, "%d", rand());

    OP op = OP_WRITE;
    keys.push_back(std::make_pair(op, buf));
  }

  for(int i=0; i < p.read_num; i++) {
    OP op = OP_READ;
    int idx = rand() % p.key_num;
    keys.push_back(std::make_pair(op, keys[idx].second));
  }

  for(int i=0; i < p.sum_num; i++) {
    OP op = OP_SUM;
    keys.push_back(std::make_pair(op, ""));
  }

  for(int i=0; i < static_cast<int>(keys.size()); i++) {
    int idx = rand() / static_cast<int>(keys.size());
    std::swap(keys[i], keys[idx]);
  }
}

struct Result {
  int op_read;
  int op_write;
  unsigned op_sum;
};

class SumCallback {
public:
  SumCallback() : sum(0) {}
  
  void operator()(const std::string & key, const std::string & value) {
    sum += value.size();
  }

  unsigned getSum() const { return sum; }

  void reset() { sum = 0; }
  
private:
  unsigned sum;
};

void * work(void * arg) {
  ThreadParam & p = *reinterpret_cast<ThreadParam*>(arg);
  SyncHashMap & map = p.param->map;
  Keys & keys = *p.keys;
  
  const int beg_i = keys.size() * static_cast<double>(p.id) / p.param->thread_num;
  const int end_i = keys.size() * static_cast<double>(p.id+1) / p.param->thread_num;

  iht::View v(map.getTrie());
  
  Result * r = new Result;

  SumCallback fn;

  for(int i=beg_i; i < end_i; i++) {
    const OP op = keys[i].first;
    const std::string & key = keys[i].second;
    
    switch(op) {
    case OP_READ:
      if(map.isMember(key, v)) {
        r->op_read++;
      }
      break;
      
    case OP_WRITE:
      if(! p.param->write_first) {
        map.store(key, key);
        r->op_write++;
      }
      break;

    case OP_SUM:
      fn.reset();
      map.foreach(fn);
      r->op_sum += fn.getSum();
      break;
    }
  }

  std::cerr << "# " << map.size() << ", " << v.size() << ", " << r->op_read << std::endl;
  return r;
}

int main(int argc, char** argv) {
  if(argc != 6) {
    std::cerr << "Usage: mt-mutex TREHAD_NUM KEY_NUM READ_NUM SUM_NUM WRITE_FIRST" << std::endl;
    return 1;
  }

  Param param;
  param.thread_num = atoi(argv[1]);
  param.key_num = atoi(argv[2]);
  param.read_num = atoi(argv[3]);
  param.sum_num = atoi(argv[4]);
  param.write_first = atoi(argv[5]) == 1;

  std::cout << "[param]" << std::endl
            << "  threads : " << param.thread_num << std::endl
            << "  keys    : " << param.key_num << std::endl
            << "  read_num: " << param.read_num << std::endl
            << "  sum_num : " << param.sum_num << std::endl
            << "  Wfirst  : " << param.write_first << std::endl
            << std::endl;
  
  std::vector<std::pair<OP, std::string> > keys;
  gen_keys(keys, param);

  std::vector<pthread_t> threads(param.thread_num);
  std::vector<ThreadParam> tparams(param.thread_num);

  if(param.write_first) {
    for(size_t i=0; i < keys.size(); i++) {
      if(keys[i].first == OP_WRITE) {
        param.map.store(keys[i].second, keys[i].second);
      }
    }
  }

  NanoTimer time;
  for(int i=0; i < param.thread_num; i++) {
    tparams[i].param = &param;
    tparams[i].keys = &keys;
    tparams[i].id = i;

    int ret = pthread_create(&threads[i], NULL, work, &tparams[i]);
    assert(ret == 0);
  }

  for(int i=0; i < param.thread_num; i++) {
    pthread_join(threads[i], NULL);
  }

  std::cout << "[result]" << std::endl
            << "  elapsed: " << time.elapsed_sec() << std::endl
            << "  size   : " << param.map.size() << std::endl
            << std::endl;

  return 0;
}
