
MPARK_MAKEFILE = ~/md-to-wg21/Makefile

CXX = g++
CXX_FLAGS = -g -Wall -std=c++23 -I.

GIT_ROOT := $(shell git rev-parse --show-toplevel)
PAPERNUM_FROM_MD := $(GIT_ROOT)/papernum_from_md.py

%.test : %.t
	./$<

%.t : %.t.cpp xmap.h
	$(CXX) $(CXX_FLAGS) -o $@ $<

%.html : %.md
	make -f $(MPARK_MAKEFILE) $@
	cp -p generated/$@ $$($(PAPERNUM_FROM_MD) $<).html

%.pdf : %.md
	make -f $(MPARK_MAKEFILE) $@
	cp -p generated/$@ $$($(PAPERNUM_FROM_MD) $<).pdf

.PRECIOUS: %.t %.html %.pdf
