
CXX       ?= g++
CXX_OPT   ?= -g
CXX_STD   ?= c++23
CXX_FLAGS ?= -Wall $(CXX_OPT) -std=$(CXX_STD) -I.
OBJDIR    ?= obj

GIT_ROOT := $(shell git rev-parse --show-toplevel)
FROM_MD = $(GIT_ROOT)/make-from-md.py

vpath %.cpp .
vpath %.h .
vpath %.t $(OBJDIR)
vpath %.o $(OBJDIR)

%.test : %.t
	$(OBJDIR)/$*.t $(TEST_ARGS)

%.vg : %.t
	valgrind $(OBJDIR)/$*.t $(TEST_ARGS)

%.t : %.t.cpp *.h
	mkdir -p $(OBJDIR)
	$(CXX) $(CXX_FLAGS) -o $(OBJDIR)/$@ $<

%.html : %.md
	$(FROM_MD) --html $<

%.pdf : %.md
	$(FROM_MD) --pdf $<

%.html.view : %.md .FORCE
	$(FROM_MD) --html --view $<

%.pdf.view : %.md .FORCE
	$(FROM_MD) --pdf --view $<

clean : .FORCE
	rm -rf $(OBJDIR)/*.t $(OBJDIR)/*.o
	rm -rf generated/*.html generated/*.pdf

.FORCE:

.PRECIOUS: %.t %.html %.pdf
