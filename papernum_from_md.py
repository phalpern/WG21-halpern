#! /usr/bin/python3

import sys
import re

mpark_docnum = re.compile(r"^ *document: *(\S*).*$")
pandoc_docnum = re.compile(r"^% ([PD][0-9][0-9][0-9][0-9]([Rr][0-9]+))\b.*$")

with open(sys.argv[1]) as mdfile:
    for line in mdfile:
        m = mpark_docnum.match(line)
        if m:
            print(m[1])
            break;
        m = pandoc_docnum.match(line)
        if m:
            print(m[1])
            break;
