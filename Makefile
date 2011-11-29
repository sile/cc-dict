CXX=g++
CXX_FLAGS=-O2 -Wall -ansi -pedantic-errors
CXX_OPTS=-I include

all: bin bin/dict

bin:
	mkdir bin

bin/dict: src/bin/dict.cc include/dict/dict.hh
	${CXX} ${CXX_FLAGS} -o ${@} ${<} ${CXX_OPTS}

clean:
	rm -f bin/dict
