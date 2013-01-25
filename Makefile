#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at 
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

CXX=g++ -m32 -std=c++0x
CXXFLAGS=-I. -Wall

all : polymorphic_allocator.test uses_allocator_wrapper.test xfunctional.test

.SECONDARY :

.FORCE :

.PHONY : .FORCE clean

%.t : %.t.cpp %.h polymorphic_allocator.o .FORCE
	$(CXX) $(CXXFLAGS) -o $@ -g $*.t.cpp polymorphic_allocator.o

%.test : %.t
	./$< $(TESTARGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -g $<

clean :
	rm -f *.t *.o
