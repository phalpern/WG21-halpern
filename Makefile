# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

# This code was tested with g++ 4.8

CXXFLAGS = -std=c++11 -g
CXXFLAGS += -DUSE_DESTRUCTIVE_MOVE

test : destructive_move.test

destructive_move.test : destructive_move.t
	./destructive_move.t

destructive_move.t : destructive_move.t.cpp destructive_move.h simple_vec.h
	$(CXX) $(CXXFLAGS) -o $@ $<

html: n4034_destructive_move.html
	:

pdf: n4034_destructive_move.pdf
	:

%.html : %.md
	pandoc --number-sections -s -S $< -o $@

%.pdf : %.md
	pandoc --number-sections -s -S $< -o $@

clean:
	rm -f destructive_move.t n4034_destructive_move.html n4034_destructive_move.pdf

.FORCE:
