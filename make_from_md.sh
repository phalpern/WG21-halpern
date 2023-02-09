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
	pandoc -s --variable colorlinks=true -f markdown \$< -o \$@

$paper.html : $src
	pandoc -s -f markdown -t html \$< -o \$@

EOF

cat tmp.mk

make -f tmp.mk $dest
rm tmp.mk
