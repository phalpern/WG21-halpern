
CXXFLAGS = -std=c++11 -g

destructive_move.test : destructive_move.t
	./destructive_move.t

destructive_move.t : destructive_move.t.cpp destructive_move.h simple_vec.h
