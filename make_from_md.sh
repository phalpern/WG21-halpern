#! /bin/bash

if [ $# -ne 1 ]; then
    echo >&2 Usage: "make_from_md [ file.pdf | file.html ]"
    exit 1
fi

base=$(basename ${1%%.*})
dest_ext=${1##*.}
src=$(ls $base*.md)

paper=$(sed -n -e '/^% [PD][0-9][0-9][0-9][0-9][Rr][0-9]\+\b/s/^% \([PD][0-9][0-9][0-9][0-9][Rr][0-9]\+\)\b.*$/\1/p' $src)
dest=$paper.$dest_ext

cat > tmp.mk <<EOF

$paper.pdf : $src
	pandoc -s -V geometry:margin=1in -V colorlinks=true -f markdown \$< -o \$@

$paper.html : $src
	sed -n -e '/^%/p' < \$< > $paper.htmlsrc.md
	cat ~/WG21/html_header.html >> $paper.htmlsrc.md
	sed -e '/^%/d' < \$< >> $paper.htmlsrc.md
	pandoc -s -f markdown -t html $paper.htmlsrc.md -o \$@
	rm $paper.htmlsrc.md

EOF

cat tmp.mk

make -f tmp.mk $dest
rm tmp.mk
