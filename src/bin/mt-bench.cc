#include <iostream>
#include <pthread.h>
#include <tr1/unordered_map>
#include <string>
#include <vector>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nano_timer.hh"
#include "map.hh"

enum OP {
  OP_WRITE,
  OP_READ,
  OP_SUM
};

enum MAP_TYPE {
  MAP_TYPE_MUTEX,
  MAP_TYPE_RWLOCK,
  MAP_TYPE_PERSISTENT
};

typedef std::vector<std::string> KeyList;
typedef std::vector<OP> OpList;

struct Param {
  Param(char ** argv)
    : map_type(strcmp(argv[1], "mutex") == 0 ? MAP_TYPE_MUTEX : 
               strcmp(argv[1], "rwlock") == 0 ? MAP_TYPE_RWLOCK : MAP_TYPE_PERSISTENT),
      thread_num(atoi(argv[2])),
      init_entry_num(atoi(argv[3])),
      write_op_num(atoi(argv[4])),
      read_op_num(atoi(argv[5])),
      sum_op_num(atoi(argv[6]))
  {
  }
  
  const MAP_TYPE map_type;
  const unsigned thread_num;
  const unsigned init_entry_num;
  const unsigned write_op_num;
  const unsigned read_op_num;
  const unsigned sum_op_num;
};

struct Result {
  Result() : found_count(0), sum_acc(0) {}

  unsigned found_count;
  unsigned sum_acc;
};

struct ThreadData {
  ThreadData() : thread_index(0), param(NULL), map(NULL), keys(NULL), ops(NULL) {}
  ThreadData(unsigned thread_index, Param * param, Map * map, KeyList * keys, OpList * ops)
    : thread_index(thread_index),
      param(param),
      map(map),
      keys(keys),
      ops(ops)
  {
  }

  unsigned  thread_index;
  Param   * param;
  Map     * map;
  KeyList * keys;
  OpList  * ops;
  Result    rlt;
};

void gen_input_data(KeyList & init_keys, KeyList & keys, OpList & ops, const Param & param) {
  char buf[1024];

  for(unsigned i=0; i < param.init_entry_num; i++) {
    sprintf(buf, "%d", rand());
    init_keys.push_back(buf);
  }

  for(unsigned i=0; i < param.write_op_num; i++) {
    sprintf(buf, "%d", rand());
    keys.push_back(buf);
    ops.push_back(OP_WRITE);
  }

  for(unsigned i=0; i < param.read_op_num; i++) {
    unsigned pos = rand() % (param.write_op_num + init_keys.size());
    if(pos < param.write_op_num) {
      keys.push_back(keys[pos]);
    } else {
      keys.push_back(init_keys[pos - param.write_op_num]);
    }
    ops.push_back(OP_READ);
  }

  for(unsigned i=0; i < param.sum_op_num; i++) {
    keys.push_back("");
    ops.push_back(OP_SUM);
  }

  for(unsigned i=0; i < keys.size(); i++) {
    unsigned idx = rand() / keys.size();
    std::swap(keys[i], keys[idx]);
    std::swap(ops[i], ops[idx]);
  }
}

void * do_bench(void * data) {
  ThreadData & td = *static_cast<ThreadData*>(data);
  Map     & map  = *td.map;
  KeyList & keys = *td.keys;
  OpList  & ops  = *td.ops;
  Result & rlt = td.rlt;
  View * view = map.createView();

  const int beg_i = keys.size() * static_cast<double>(td.thread_index)   / td.param->thread_num;
  const int end_i = keys.size() * static_cast<double>(td.thread_index+1) / td.param->thread_num;

  for(int i=beg_i; i < end_i; i++) {
    const std::string & key = keys[i];
    const OP op = ops[i];
    
    switch(op) {
    case OP_READ:
      if(view->member(key)) {
        rlt.found_count++;
      }
      break;
      
    case OP_WRITE:
      map.store(key, key);
      break;

    case OP_SUM:
      rlt.sum_acc += view->totalValueLength();
      break;
    }
  }
  
  delete view;
  return &rlt;
}

int main(int argc, char ** argv) {
  if(argc != 7) {
    std::cerr << "Usage: mt-bench MAP_TYPE(mutex|rwlock|persistent) THREAD_NUM INIT_ENTRY_NUM WRITE_OP_NUM READ_OP_NUM SUM_OP_NUM" << std::endl;
    return 1;
  }
  
  Param param(argv);

  std::cout << "[input parameter]" << std::endl
            << "  map_type      : " << argv[1] << "(" << param.map_type << ")" << std::endl
            << "  thread_num    : " << param.thread_num << std::endl
            << "  init_entry_num: " << param.init_entry_num << std::endl
            << "  write_op_num  : " << param.write_op_num << std::endl
            << "  read_op_num   : " << param.read_op_num << std::endl
            << "  sum_op_num    : " << param.sum_op_num << std::endl
            << std::endl;

  KeyList init_keys;
  KeyList keys;
  OpList ops;

  Map * map = NULL;
  switch (param.map_type) {
  case MAP_TYPE_MUTEX:      map = new MutexMap(); break;
  case MAP_TYPE_RWLOCK:     map = new RWLockMap(); break;
  case MAP_TYPE_PERSISTENT: map = new PersistentMap(); break;
  }

  {
    NanoTimer gen_input_time;
    gen_input_data(init_keys, keys, ops, param);
    std::cout << "[generate input data]" << std::endl
              << "  init_keys: " << init_keys.size() << std::endl
              << "  keys     : " << keys.size() << std::endl
              << "  ops      : " << ops.size() << std::endl
              << "  elapsed  : " << gen_input_time.elapsed_sec() << " sec" << std::endl
              << std::endl;
  }

  for(size_t i=0; i < init_keys.size(); i++) {
    map->store(init_keys[i], init_keys[i]);
  }

  {
    NanoTimer bench_time;

    std::vector<pthread_t> threads(param.thread_num);
    std::vector<ThreadData> tdatas(param.thread_num);

    for(unsigned i=0; i < param.thread_num; i++) {
      tdatas[i] = ThreadData(i, &param, map, &keys, &ops);
      int ret = pthread_create(&threads[i], NULL, do_bench, &tdatas[i]);
      assert(ret == 0);
    }
    
    Result rlt;
    for(unsigned i=0; i < param.thread_num; i++) {
      Result * tmp;
      pthread_join(threads[i], reinterpret_cast<void**>(&tmp));
      rlt.found_count += tmp->found_count;
      rlt.sum_acc += tmp->sum_acc;
    }

    std::cout << "[bench result]" << std::endl
              << "  founds : " << rlt.found_count << std::endl
              << "  sum_acc: " << rlt.sum_acc << std::endl
              << "  keys   : " << map->size() << std::endl
              << "  elapsed: " << bench_time.elapsed_sec() << " sec" << std::endl
              << std::endl;

    delete map;
  }
  return 0;
}
