#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at 
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

SCOPED_ALLOC_DIR = ../allocator_traits

CXX=g++ -m32 -std=c++0x
CXXFLAGS=-I. -I$(SCOPED_ALLOC_DIR) -Dnullptr=0 -Dnoexcept="throw()" -Wall

all : polymorphic_allocator.test

.SECONDARY :

%.t : %.t.cpp %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -g  $^

%.test : %.t
	./$< $(TESTARGS)
	rm ./$<

clean :
	rm -f *.t *.o
