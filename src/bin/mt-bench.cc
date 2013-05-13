#include <iostream>
#include <pthread.h>
#include <tr1/unordered_map>
#include <string>
#include <vector>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "nano_timer.hh"
#include "map.hh"

enum OP {
  OP_WRITE,
  OP_READ,
  OP_SUM
};

typedef std::vector<std::string> KeyList;
typedef std::vector<OP> OpList;

struct Param {
  Param(char ** argv)
    : thread_num(atoi(argv[1])),
      init_entry_num(atoi(argv[2])),
      write_op_num(atoi(argv[3])),
      read_op_num(atoi(argv[4])),
      sum_op_num(atoi(argv[5]))
  {
  }
  
  const unsigned thread_num;
  const unsigned init_entry_num;
  const unsigned write_op_num;
  const unsigned read_op_num;
  const unsigned sum_op_num;
};

struct ThreadData {
  ThreadData(unsigned thread_index, Map & map, const KeyList & keys, const OpList & ops)
    : thread_index(thread_index),
      map(map),
      keys(keys),
      ops(ops)
  {
  }

  const unsigned  thread_index;
  Map           & map;
  const KeyList & keys;
  const OpList  & ops;
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


int main(int argc, char ** argv) {
  if(argc != 6) {
    std::cerr << "Usage: mt-bench THREAD_NUM INIT_ENTRY_NUM WRITE_OP_NUM READ_OP_NUM SUM_OP_NUM" << std::endl;
    return 1;
  }
  
  const Param param(argv);

  std::cout << "[input parameter]" << std::endl
            << "  thread_num    : " << param.thread_num << std::endl
            << "  init_entry_num: " << param.init_entry_num << std::endl
            << "  write_op_num  : " << param.write_op_num << std::endl
            << "  read_op_num   : " << param.read_op_num << std::endl
            << "  sum_op_num    : " << param.sum_op_num << std::endl
            << std::endl;

  KeyList init_keys;
  KeyList keys;
  OpList ops;

  NanoTimer gen_input_time;
  gen_input_data(init_keys, keys, ops, param);
  std::cout << "[generate input data]" << std::endl
            << "  init_keys: " << init_keys.size() << std::endl
            << "  keys     : " << keys.size() << std::endl
            << "  ops      : " << ops.size() << std::endl
            << "  elapsed  : " << gen_input_time.elapsed_sec() << " sec" << std::endl
            << std::endl;

  return 0;
}
