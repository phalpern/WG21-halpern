#!/bin/bash
pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H header.tex task_region_proposal-r2.md -o task_region_proposal-r2.pdf
