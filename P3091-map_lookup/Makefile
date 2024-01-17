
CXX = g++
CXX_FLAGS=-g -Wall -std=c++23 -I.

%.test : %.t
	./$<

%.t : %.t.cpp xmap.h
	$(CXX) $(CXX_FLAGS) -o $@ $<

.PRECIOUS: %.t
