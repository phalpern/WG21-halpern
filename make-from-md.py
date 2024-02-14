#! /usr/bin/python3

# Convert a WG21 paper in markdown format into html or pdf
#
# Usage: make-from-md.py ( --html | --pdf ) [ --view ] paper.md
#
# The output file name is generated by reading the paper number out of the .md
# file and appending the "html" or "pdf" extension.  If the paper number is not
# found, the output file is the same as the input file name, except for the
# extension.  The output file is always created in the same directory as the
# input file.

# Dependencies
# 1. pandoc
#   `sudo apt install pandoc`
# 2. texlive-latex-recommended
#    `sudo apt install texlive-latex-recommended`)
# 3. Michael Park's pandoc plugin for WG21 papers
#    git clone git@github.com:t3nsor/mpark-wg21-internal.git ~/md-to-wg21
# OR git clone git@github.com:mpark/wg21.git ~/md-to-wg21

import sys
import os
import re
import subprocess

mparkDocnumRe  = re.compile(r"^ *document: *(\S*).*$")
pandocDocnumRe = re.compile(r"^% ([PD][0-9][0-9][0-9][0-9]([Rr][0-9]+))\b.*$")

def userError(errorStr):
    print(errorStr, file=sys.stderr)
    sys.exit(2)

def usage(errorStr = None):
    usageStr = "Usage: make-from-md.py ( --html | --pdf ) [ --view ] paper.md"
    if errorStr is None:
        userError(usageStr)
    else:
        userError("Usage error: " + errorStr + '\n' + usageStr)

def readDocnumAndType(filename):
    """Returns a tuple (type, docnum) for the specified filename, where *type*
    is either 'mpark' for Michael Park's WG21 md format or 'pandoc' for vanilla
    pandoc md format"""
    with open(filename) as mdfile:
        for line in mdfile:
            m = mparkDocnumRe.match(line)
            if m:
                return ("mpark", m[1])
            m = pandocDocnumRe.match(line)
            if m:
                return ("pandoc", m[1])
    print("Warning: Cannot read document number from " + filename, sys.stderr)
    return ("pandic", filename.replace(".md", ""))

def createMakefile(mdfile, mdType, docnum):

    if mdType == "mpark":
        mdroot, md = os.path.splitext(mdfile)
        mkfileContent = f"""
MPARK_REPO ?= ~/md-to-wg21
MPARK_MAKEFILE = $(MPARK_REPO)/Makefile

{docnum}.html : {mdfile}
	make -f $(MPARK_MAKEFILE) {mdroot}.html
	cp -p generated/{mdroot}.html $@

{docnum}.pdf : {mdfile}
	make -f $(MPARK_MAKEFILE) {mdroot}.pdf
	cp -p generated/{mdroot}.pdf $@
"""

    else:
        mkfileContent = f"""
{docnum}.pdf : {mdfile}
	pandoc -s -V geometry:margin=1in -V colorlinks=true --number-sections -f markdown $< -o $@

{docnum}.html : {mdfile}
	cat ~/WG21/html_header.html {mdfile} >> {docnum}.htmlsrc.md
	pandoc -s -f markdown -t html {docnum}.htmlsrc.md -o $@
	rm {docnum}.htmlsrc.md
"""

    mkfileName = docnum + ".mk"
    with open(mkfileName, 'w') as mkfile:
        mkfile.write(mkfileContent)

    return mkfileName

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == "--help":
        usage()
    elif len(sys.argv) < 3:
        usage("Requires at least 3 arguments")

    mdfile = None
    outType = None
    view = False
    for arg in sys.argv[1:]:
        if mdfile is not None:
            usage(f"Cannot specify {arg} after file name")
        elif arg == "--html":
            outType = "html"
        elif arg == "--pdf":
            outType = "pdf"
        elif arg == "--view":
            view = True
        elif arg[1] == "-":
            usage("Unknown argument: " + arg)
        else:
            mdfile = arg

    mddir, mdfile = os.path.split(mdfile)
    if mddir: os.chdir(mddir)
    mdType, docnum = readDocnumAndType(mdfile)
    target = docnum + '.' + outType

    makefile = createMakefile(mdfile, mdType, docnum)
    if subprocess.run(["make", "-f", makefile, target]).returncode != 0:
        userError("Error running temporary makefile: " + makefile)
    os.remove(makefile)

    if view:
        viewer = "browser" if outType == "html" else "pdfreader"
        subprocess.run([viewer, target])
