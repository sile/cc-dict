CXX=g++
CXX_FLAGS=-O2 -Wall -ansi -pedantic-errors -I include
CXX_OPTS=
# CXX_OPTS=-DTLS_ENABLED -Dthread_local=__thread

all: bin bin/dict

bin:
	mkdir bin

bin/dict: src/bin/dict.cc include/dict/map.hh include/dict/allocator.hh include/dict/hash.hh
	${CXX} ${CXX_FLAGS} -o ${@} ${<} ${CXX_OPTS}

clean:
	rm -f bin/dict
