#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

# VARIANT should be one of BINARY_SEARCH, LINEAR, or SWITCH
VARIANT ?= BINARY_SEARCH

CXX = g++
CXXFLAGS = -std=c++20 -I. -Wall -DRA_$(VARIANT)
WD := $(shell basename $(PWD))
OUTDIR = obj.$(VARIANT)

all : asm test

asm : $(OUTDIR)/resource_adaptor.t.s

test : resource_adaptor.test

.SECONDARY :

.FORCE :

.PHONY : .FORCE clean

$(OUTDIR)/%.t : %.t.cpp %.h
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) -o $@ -g $*.t.cpp

%.test : $(OUTDIR)/%.t
	$< $(TESTARGS)

$(OUTDIR)/%.o : %.cpp
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ -g $<

$(OUTDIR)/%.s : %.cpp
	mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) -DQUICK_TEST -S -o $@.mangled -O2 $<
	c++filt < $@.mangled > $@
	rm $@.mangled

%.pdf : %.md
	cd .. && make $(WD)/$@

%.html : %.md
	cd .. && make $(WD)/$@

clean :
	rm -rf $(OUTDIR)
