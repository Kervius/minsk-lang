
CXX=clang++
CXXFLAGS=-std=c++11 -Wall -O -g
#CPPFLAGS=-DSELF_TEST

all: min


t: min.cc
	$(CXX) $(CXXFLAGS) -DSELF_TEST $^ -o min
	./min


# clang++ -std=c++11 -Wall -O -g -DSELF_TEST   min.cc   -o min

mish: min.cc
	$(CXX) $(CXXFLAGS) -DMISH $^ -o mish

tags:
	ctags min.cc


