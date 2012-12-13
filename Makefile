#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at 
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

SCOPED_ALLOC_DIR = ../allocator_traits

CXX=g++ -m32 -std=c++0x
CXXFLAGS=-I. -I$(SCOPED_ALLOC_DIR) -Wall

all : polymorphic_allocator.test

.SECONDARY :

%.t : %.t.cpp %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ -g $*.cpp $*.t.cpp

%.test : %.t
	./$< $(TESTARGS)

clean :
	rm -f *.t *.o
