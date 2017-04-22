
CXX=clang++
CXXFLAGS=-std=c++11 -Wall -O -g

all: min

t: min
	./min
