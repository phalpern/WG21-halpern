# This code was tested with g++ 4.8

CXXFLAGS = -std=c++11 -g
CXXFLAGS += -DUSE_DESTRUCTIVE_MOVE

destructive_move.test : destructive_move.t
	./destructive_move.t

destructive_move.t : destructive_move.t.cpp destructive_move.h simple_vec.h

clean:
	rm -f destructive_move.t
