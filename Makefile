.PHONY: all clean

SRCS=$(shell find src -name "*.cc")
OBJS=$(SRCS:%.cc=%.o)
DEPS=$(SRCS:%.cc=%.d)

SRCS_NO_MAIN=$(shell find src -name "*.cc" | grep -v 'src/bin')
OBJS_NO_MAIN=$(SRCS_NO_MAIN:%.cc=%.o)

CXX=g++
CFLAGS=-Wall -O2 -g
#CFLAGS=-Wall -g
LINK=-lpthread
INCLUDE=-Iinclude

all: bin bin/hashtrie bin/hashmap bin/mt-bench

-include $(DEPS)

bin:
	mkdir -p bin

bin/hashtrie: src/bin/hashtrie.cc $(OBJS)
	$(CXX) $(CFLAGS) -MMD -MP -o $@ $(<:%.cc=%.o) $(OBJS_NO_MAIN) $(LINK) $(INCLUDE)

bin/hashmap: src/bin/hashmap.cc $(OBJS)
	$(CXX) $(CFLAGS) -MMD -MP -o $@ $(<:%.cc=%.o) $(OBJS_NO_MAIN) $(LINK) $(INCLUDE)

bin/mt-bench: src/bin/mt-bench.cc $(OBJS)
	$(CXX) $(CFLAGS) -MMD -MP -o $@ $(<:%.cc=%.o) $(OBJS_NO_MAIN) $(LINK) $(INCLUDE)

%.o : %.cc
	$(CXX) $(CFLAGS) -c -MMD -MP -o $@ $< $(LINK) $(INCLUDE)

clean:
	rm -f bin/* $(OBJS) $(DEPS)
