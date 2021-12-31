#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

CXX = g++
CXXFLAGS = -std=c++20 -I. -Wall
TEST_OPT = -g
ASM_OPT  = -O2
WD := $(shell basename $(PWD))
OUTDIR = obj
MK_OUTDIR := $(shell mkdir -p $(OUTDIR))

ALGORITHMS = binsearch linear switch

all : asm test

asm :  $(ALGORITHMS:%=$(OUTDIR)/resource_adaptor_%.t.s)

test : aligned_type.test $(ALGORITHMS:%=resource_adaptor_%.test)

pdf : $(OUTDIR)/P1083.pdf
	open $<

.SECONDARY :

.FORCE :

.PHONY : .FORCE clean

%.test : $(OUTDIR)/%.t .FORCE
	$< $(TESTARGS)

$(OUTDIR)/aligned_type.t : aligned_type.t.cpp aligned_type.h
	$(CXX) $(CXXFLAGS) $(TEST_OPT) -o $@ $<

$(OUTDIR)/%.t : %.t.cpp %.h resource_adaptor.t.h aligned_type.h
	$(CXX) $(CXXFLAGS) $(TEST_OPT) -o $@ $<

$(OUTDIR)/%.o : %.cpp %.h resource_adaptor.t.h aligned_type.h
	$(CXX) $(CXXFLAGS) $(TEST_OPT) -c -o $@ $<

$(OUTDIR)/%.t.s : %.t.cpp %.h resource_adaptor.t.h aligned_type.h
	$(CXX) $(CXXFLAGS) $(ASM_OPT) -DQUICK_TEST -S -o $@.mangled $<
	c++filt < $@.mangled > $@
	rm $@.mangled

$(OUTDIR)/%.pdf : %.md
	pandoc -s --variable colorlinks=true -f markdown $< -o $@

$(OUTDIR)/%.html : %.md
	pandoc -s -f markdown $< -o $@

clean :
	rm -rf $(OUTDIR)
	mkdir -p $(OUTDIR)
