# Makefile for destructive_move test and proposal
#
# Copyright 2014 Pablo Halpern.
# Free to use and redistribute (See accompanying file license.txt.)

DOCNAME=vec_policy

html: $(DOCNAME).html
	:

pdf: $(DOCNAME).pdf
	:

%.html : %.md
	pandoc --number-sections -s -S $< -o $@

%.pdf : %.md header.tex
	pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H header.tex $< -o $@

header.tex : header.tmplt.tex $(DOCNAME).md
	sed -e s/DOCNUM/$$(sed -n  -e '/[ND][0-9][0-9][0-9][0-9]/s/.*\([ND][0-9][0-9][0-9][0-9]\).*/\1/p' -e '/[ND][0-9][0-9][0-9][0-9]/q' $(DOCNAME).md)/g $< > $@

clean:
	rm -f $(DOCNAME).html $(DOCNAME).pdf header.tex

.FORCE:
