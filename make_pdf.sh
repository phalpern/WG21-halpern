#!/bin/bash
set -x
rev=${1:-r3}
pandoc --number-sections -f markdown+footnotes+definition_lists -s -S -H header.tex task_region_proposal-$rev.md -o task_region_proposal-$rev.pdf
