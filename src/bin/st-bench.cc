#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "nano_timer.hh"
#include "map.hh"

typedef std::vector<std::string> KeyList;

enum MAPTYPE {
  MAPTYPE_HASH,
  MAPTYPE_TRIE
};

struct Param {
  Param(char ** argv)
    : map_type(strcmp(argv[1], "hash") == 0 ? MAPTYPE_HASH : MAPTYPE_TRIE),
      write_op_num(atoi(argv[2])),
      read_op_num(atoi(argv[3])),
      sum_op_num(atoi(argv[4]))
  {
  }
  
  const MAPTYPE map_type;
  const unsigned write_op_num;
  const unsigned read_op_num;
  const unsigned sum_op_num;
};

void gen_input_data(KeyList & write_keys, KeyList & read_keys, const Param & param) {
  char buf[1024];

  for(unsigned i=0; i < param.write_op_num; i++) {
    sprintf(buf, "%d", rand());
    write_keys.push_back(buf);
  }

  for(unsigned i=0; i < param.read_op_num; i++) {
    unsigned pos = rand() % param.write_op_num;
    read_keys.push_back(write_keys[pos]);
  }
}


int main(int argc, char ** argv) {
  if(argc != 5) {
    std::cerr << "Usage: st-bench MAPTYPE(hash|trie) WRITE_NUM READ_NUM SUM_NUM" << std::endl;
    return 1;
  }
  
  Param param(argv);
  std::cout << "[input parameter]" << std::endl
            << "  map_type    : " << argv[1] << "(" << param.map_type << ")" << std::endl
            << "  write_op_num: " << param.write_op_num << std::endl
            << "  read_op_num : " << param.read_op_num << std::endl
            << "  sum_op_num  : " << param.sum_op_num << std::endl
            << std::endl;

  KeyList write_keys;
  KeyList read_keys;
  {
    NanoTimer gen_input_time;
    gen_input_data(write_keys, read_keys, param);
    std::cout << "[generate input data]" << std::endl
              << "  write_keys: " << write_keys.size() << std::endl
              << "  read_keys : " << read_keys.size() << std::endl
              << "  elapsed   : " << gen_input_time.elapsed_sec() << " sec" << std::endl
              << std::endl;
  }

  Map * map = NULL;
  switch(param.map_type) {
  case MAPTYPE_HASH: map = new HashMap(); break;
  case MAPTYPE_TRIE: map = new TrieMap(); break;
  }

  double write_time;
  double read_time;
  double sum_time;
  {
    NanoTimer time;
    for(size_t i=0; i < write_keys.size(); i++) {
      map->store(write_keys[i], write_keys[i]);
    }
    write_time = time.elapsed_sec();
  }
  
  size_t found_count = 0;
  size_t sum_acc = 0;
  View * view = map->createView();
  {
    NanoTimer time;
    for(size_t i=0; i < read_keys.size(); i++) {
      if(view->member(read_keys[i])) {
        found_count++;
      }
    }
    read_time = time.elapsed_sec();
  }
  {
    NanoTimer time;
    for(size_t i=0; i < param.sum_op_num; i++) {
      sum_acc += view->totalValueLength();
    }
    sum_time = time.elapsed_sec();
  }
  delete view;

  std::cout << "[bench result]" << std::endl
            << "  founds       : " << found_count << std::endl
            << "  sum_acc      : " << sum_acc << std::endl
            << "  write_elapsed: " << write_time << " sec" << std::endl
            << "  read_elapsed : " << read_time << " sec" << std::endl
            << "  sum_elapsed  : " << sum_time << " sec" << std::endl
            << std::endl;

  delete map;

  return 0;
}
