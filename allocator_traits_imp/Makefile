# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

# This code was tested with g++ 4.8

%.html : %.md
	pandoc --number-sections -s -f markdown $< -o $@

%.pdf : %.md header.tex
	$(eval DOCNUM=$(shell sed -n  -e '/^% [NDP][0-9x][0-9x][0-9x][0-9x]/s/^% \([NDP][0-9x][0-9x][0-9x][0-9x]\([Rr][0-9]\+\)\?\).*/\1/p' -e '/^[NDP][0-9x][0-9x][0-9x][0-9x]\([Rr][0-9]\+\)\?$$/q' $<))
	sed -e s/DOCNUM/$(DOCNUM)/g header.tex > $*.hdr.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists --variable colorlinks=true -s -H $*.hdr.tex $< -o $@
	rm -f $*.hdr.tex

%.docx : %.md
	pandoc --number-sections -s -f markdown $< -o $@

%.clean: %.md
	mkdir -p old
	mv $*.[hpd][tdo][mfc]* old  # Move .html, .pdf, and .docx files to old

.FORCE:
