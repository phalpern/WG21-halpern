# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

# This code was tested with g++ 4.8

CXXFLAGS = -std=c++11 -g
CXXFLAGS += -DUSE_DESTRUCTIVE_MOVE

DOCNAME=simd-execution-model

html: $(DOCNAME).html
	:

pdf: $(DOCNAME).pdf
	:

%.html : %.md
	pandoc --number-sections -s -S $< -o $@

%.pdf : %.md header.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H header.tex $< -o $@

header.tex : header.tmplt.tex $(DOCNAME).md
	sed -e s/DOCNUM/$$(sed -n  -e '/N[0-9][0-9][0-9][0-9]/s/.*\(N[0-9][0-9][0-9][0-9]\).*/\1/p' -e '/N[0-9][0-9][0-9][0-9]/q' $(DOCNAME).md)/g $< > $@

clean:
	rm -f destructive_move.t $(DOCNAME).html $(DOCNAME).pdf header.tex

.FORCE:
