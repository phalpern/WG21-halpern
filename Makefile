# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

# This code was tested with g++ 4.8

CXXFLAGS = -std=c++11 -g
CXXFLAGS += -DUSE_DESTRUCTIVE_MOVE

FILEROOT = destructive_move

test : $(FILEROOT).test

$(FILEROOT).test : $(FILEROOT).t
	./$(FILEROOT).t

$(FILEROOT).t : $(FILEROOT).t.cpp $(FILEROOT).h simple_vec.h
	$(CXX) $(CXXFLAGS) -o $@ $<

html: $(FILEROOT).html
	:

pdf: $(FILEROOT).pdf
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
	rm -f $(FILEROOT).t $(FILEROOT).html $(FILEROOT).pdf

.FORCE:
