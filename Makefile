# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

CXXFLAGS = -std=c++11 -g
CXXFLAGS += -DUSE_DESTRUCTIVE_MOVE

DOCNAME=vector-execution-models

pdf: $(DOCNAME).pdf
	:

html: $(DOCNAME).html
	:

docx: $(DOCNAME).docx
	:

%.html : %.md
	pandoc --number-sections -s -S $< -o $@

%.pdf : %.md header.tex
	$(eval DOCNUM=$(shell sed -n  -e '/^% .*[ND][0-9x][0-9x][0-9x][0-9x]/s/^.*\([ND][0-9x][0-9x][0-9x][0-9x]\).*/\1/p' -e '/^[ND][0-9x][0-9x][0-9x][0-9x]$$/q' $<))
	sed -e s/DOCNUM/$(DOCNUM)/g header.tex > $*.hdr.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H $*.hdr.tex $< -o $@
	rm -f $*.hdr.tex

%.docx : %.md
	pandoc --number-sections -s -S $< -o $@

clean:
	rm -f $(DOCNAME).t $(DOCNAME).html $(DOCNAME).pdf $(DOCNAME).docx

.FORCE:
