
MPARK_MAKEFILE = ~/md-to-wg21/Makefile

CXX = g++
CXX_FLAGS = -g -Wall -std=c++23 -I. -fconcepts-diagnostics-depth=2

GIT_ROOT := $(shell git rev-parse --show-toplevel)
PAPERNUM_FROM_MD := $(GIT_ROOT)/papernum_from_md.py

%.test : %.t
	./$< $(TEST_ARGS)

%.vg : %.t
	valgrind ./$< $(TEST_ARGS)

%.t : %.t.cpp move_with_alloc.h
	$(CXX) $(CXX_FLAGS) -o $@ $<

%.html : %.md
	make -f $(MPARK_MAKEFILE) $@
	cp -p generated/$@ $$($(PAPERNUM_FROM_MD) $<).html

%.pdf : %.md
	make -f $(MPARK_MAKEFILE) $@
	cp -p generated/$@ $$($(PAPERNUM_FROM_MD) $<).pdf

%.html.show : %.md .FORCE
	make $*.html
	browser $$(wslpath -w $$($(PAPERNUM_FROM_MD) $<)).html

%.pdf.show : %.md .FORCE
	make $*.pdf
	foxit $$(wslpath -w $$($(PAPERNUM_FROM_MD) $<)).pdf

.FORCE:

.PRECIOUS: %.t %.html %.pdf
