#!/bin/bash
set -x
rev=${1:-r5}
pandoc --number-sections -f markdown+footnotes+definition_lists+multiline_tables -s -S -H header.tex task_block_proposal-$rev.md -o task_block_proposal-$rev.pdf
