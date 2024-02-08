
CXX       ?= g++
CXX_OPT   ?= -g
CXX_STD   ?= c++23
CXX_FLAGS ?= -Wall $(CXX_OPT) -std=$(CXX_STD) -I.

GIT_ROOT := $(shell git rev-parse --show-toplevel)
FROM_MD = $(GIT_ROOT)/make-from-md.py

%.test : %.t
	./$< $(TEST_ARGS)

%.vg : %.t
	valgrind ./$< $(TEST_ARGS)

%.t : %.t.cpp *.h
	$(CXX) $(CXX_FLAGS) -o $@ $<

%.html : %.md
	$(FROM_MD) --html $<

%.pdf : %.md
	$(FROM_MD) --pdf $<

%.html.view : %.md .FORCE
	$(FROM_MD) --html --view $<

%.pdf.view : %.md .FORCE
	$(FROM_MD) --pdf --view $<

.FORCE:

.PRECIOUS: %.t %.html %.pdf
