#
# PROGRAM: C++ Multithreading
# AUTHOR:  Daniel Bendik
#

CXXFLAGS = -g -ansi -pedantic -Wall -Werror -Wextra -std=c++14 -pthread

reduce: reduce.o
	g++ $(CXXFLAGS) -o reduce reduce.o

reduce.o: reduce.cpp
	g++ $(CXXFLAGS) -c reduce.cpp

clean:
	rm -f *.o reduce