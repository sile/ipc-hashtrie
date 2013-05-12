#include <pthread.h>
#include <iostream>
#include <tr1/unordered_map>
#include <string>
#include <vector>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <utility>
#include "nano_timer.hh"

class SyncHashMap {
  typedef std::tr1::unordered_map<std::string, std::string> hashmap;

public:
  SyncHashMap() {
    int ret = pthread_mutex_init(&mtx_, NULL);
    assert(ret == 0);
  }

  ~SyncHashMap() {
    int ret = pthread_mutex_destroy(&mtx_);
    assert(ret == 0);
  }

  void store(const std::string & key, const std::string & value) {
    pthread_mutex_lock(&mtx_);
    impl_[key] = value;
    pthread_mutex_unlock(&mtx_);
  }

  bool find(const std::string & key, std::string & value) {
    pthread_mutex_lock(&mtx_);
    hashmap::const_iterator it = impl_.find(key);
    
    bool exists;
    if(it == impl_.end()) {
      exists = false;
    } else {
      value = it->second;
      exists =  true;
    }
    pthread_mutex_unlock(&mtx_);

    return exists;
  }
  
  bool isMember(const std::string & key) {
    pthread_mutex_lock(&mtx_);
    bool exists = impl_.find(key) != impl_.end();
    pthread_mutex_unlock(&mtx_);

    return exists;
  }

  size_t size() { 
    pthread_mutex_lock(&mtx_);
    size_t size = impl_.size(); 
    pthread_mutex_unlock(&mtx_);
    return size;
  }

  template <class Callback>
  void foreach(Callback & callback) {
    pthread_mutex_lock(&mtx_);
    hashmap::const_iterator it = impl_.begin();
    for(; it != impl_.end(); ++it) {
      callback(it->first, it->second);
    }
    pthread_mutex_unlock(&mtx_);
  }
  
private:
  hashmap impl_;
  pthread_mutex_t mtx_;
};

struct Param {
  int thread_num;
  int key_num;
  int read_num;
  int sum_num;

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
    int idx = rand() / p.key_num;
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

  Result * r = new Result;

  SumCallback fn;

  for(int i=beg_i; i < end_i; i++) {
    const OP op = keys[i].first;
    const std::string & key = keys[i].second;
    
    switch(op) {
    case OP_READ:
      if(map.isMember(key)) {
        r->op_read++;
      }
      break;
      
    case OP_WRITE:
      map.store(key, key);
      r->op_write++;
      break;

    case OP_SUM:
      fn.reset();
      map.foreach(fn);
      r->op_sum += fn.getSum();
      break;
    }
  }

  return r;
}

int main(int argc, char** argv) {
  if(argc != 5) {
    std::cerr << "Usage: mt-mutex TREHAD_NUM KEY_NUM READ_NUM SUM_NUM" << std::endl;
    return 1;
  }

  Param param;
  param.thread_num = atoi(argv[1]);
  param.key_num = atoi(argv[2]);
  param.read_num = atoi(argv[3]);
  param.sum_num = atoi(argv[4]);

  std::cout << "[param]" << std::endl
            << "  threads : " << param.thread_num << std::endl
            << "  keys    : " << param.key_num << std::endl
            << "  read_num: " << param.read_num << std::endl
            << "  sum_num : " << param.sum_num << std::endl
            << std::endl;
  
  std::vector<std::pair<OP, std::string> > keys;
  gen_keys(keys, param);

  std::vector<pthread_t> threads(param.thread_num);
  std::vector<ThreadParam> tparams(param.thread_num);

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
