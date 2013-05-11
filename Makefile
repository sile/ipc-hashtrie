.PHONY: all clean

SRCS=$(shell find src -name "*.cc")
OBJS=$(SRCS:%.cc=%.o)
DEPS=$(SRCS:%.cc=%.d)

SRCS_NO_MAIN=$(shell find src -name "*.cc" | grep -v 'src/bin')
OBJS_NO_MAIN=$(SRCS_NO_MAIN:%.cc=%.o)

CXX=g++
CFLAGS=-Wall -O2 -g
LINK=
INCLUDE=-Iinclude

all: bin bin/hashtrie

-include $(DEPS)

bin:
	mkdir -p bin

bin/hashtrie: src/bin/hashtrie.cc $(OBJS)
	$(CXX) $(CFLAGS) -MMD -MP -o $@ $< $(OBJS_NO_MAIN) $(LINK) $(INCLUDE)

%.o : %.cc
	$(CXX) $(CFLAGS) -c -MMD -MP -o $@ $< $(LINK) $(INCLUDE)

clean:
	rm -f bin/* $(OBJS) $(DEPS)
