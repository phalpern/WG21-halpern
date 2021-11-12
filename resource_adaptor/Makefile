#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

CXX = g++
CXXFLAGS = -std=c++20 -I. -Wall $(VARIANT)
WD := $(shell basename $(PWD))

all : resource_adaptor.test

.SECONDARY :

.FORCE :

.PHONY : .FORCE clean

%.t : %.t.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ -g $*.t.cpp

%.test : %.t
	./$< $(TESTARGS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -g $<

%.s : %.cpp
	$(CXX) $(CXXFLAGS) -DQUICK_TEST -S -o $@.mangled -O2 $<
	c++filt < $@.mangled > $@

%.pdf : %.md
	cd .. && make $(WD)/$@

%.html : %.md
	cd .. && make $(WD)/$@

clean :
	rm -f *.t *.o *.s
