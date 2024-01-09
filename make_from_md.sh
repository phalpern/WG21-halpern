#! /bin/bash

if [ $# -ne 1 ]; then
    echo >&2 Usage: "make_from_md [ file.pdf | file.html | file.pdf.show | file.html.show ]"
    exit 1
fi

pdfreader="/mnt/c/Program Files (x86)/Foxit Software/Foxit PDF Reader/FoxitPDFReader.exe"

set -x

base=$(basename ${1%%.*})
dest_ext=${1#*.}
src=$(ls $base*.md)

paper=$(sed -n -e '/^% [PD][0-9][0-9][0-9][0-9][Rr][0-9]\+\b/s/^% \([PD][0-9][0-9][0-9][0-9][Rr][0-9]\+\)\b.*$/\1/p' $src)
dest=$paper.$dest_ext

cat > tmp.mk <<EOF

pdfreader="$pdfreader"

$paper.pdf : $src
	pandoc -s -V geometry:margin=1in -V colorlinks=true --number-sections -f markdown \$< -o \$@

$paper.html : $src
	sed -n -e '/^%/p' < \$< > $paper.htmlsrc.md
	cat ~/WG21/html_header.html >> $paper.htmlsrc.md
	sed -e '/^%/d' < \$< >> $paper.htmlsrc.md
	pandoc -s -f markdown -t html $paper.htmlsrc.md -o \$@
	rm $paper.htmlsrc.md

$paper.pdf.show : $paper.pdf .FORCE
	"$pdfreader" "L:/$PWD/$<"

.FORCE:

EOF

cat tmp.mk

make -f tmp.mk $dest
rm tmp.mk
