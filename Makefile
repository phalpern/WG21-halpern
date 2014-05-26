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

%.pdf : %.md header.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H header.tex $< -o $@

header.tex : header.tmplt.tex n4034_destructive_move.md
	sed -e s/DOCNUM/$$(sed -n  -e '/N[0-9][0-9][0-9][0-9]/s/.*\(N[0-9][0-9][0-9][0-9]\).*/\1/p' -e '/N[0-9][0-9][0-9][0-9]/q' n4034_destructive_move.md)/g $< > $@

clean:
	rm -f destructive_move.t n4034_destructive_move.html n4034_destructive_move.pdf header.tex

.FORCE:
