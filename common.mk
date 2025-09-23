
CXX      ?= g++
CXXOPT   ?= -g
CXXSTD   ?= c++23
CXXFLAGS ?= -Wall $(CXXOPT) -std=$(CXXSTD) -I.
OBJDIR   ?= obj

GIT_ROOT := $(shell git rev-parse --show-toplevel)
FROM_MD = $(GIT_ROOT)/make-from-md.py

vpath %.cpp .
vpath %.h .
vpath %.t $(OBJDIR)
vpath %.o $(OBJDIR)

# Remember compiler and flags from previous run. If they change, regenerate
# the cxx_config file, thus forcing a rebuild.
CXX_CONFIG_FILE = $(OBJDIR)/cxx_config.txt
OLD_CXX_CONFIG = $(shell cat $(CXX_CONFIG_FILE) 2> /dev/null)
NEW_CXX_CONFIG = $(CXX) $(CXXFLAGS)
ifneq ($(OLD_CXX_CONFIG),$(NEW_CXX_CONFIG))
	REBUILD_CONFIG = .FORCE   # Build flags have changed; force rebuild.
else
	REBUILD_CONFIG =          # Build flags have not changed.
endif

$(CXX_CONFIG_FILE) : $(REBUILD_CONFIG)
	mkdir -p $(OBJDIR)
	echo "$(NEW_CXX_CONFIG)" > $@

%.test : %.t
	$(OBJDIR)/$*.t $(TEST_ARGS)

%.vg : %.t
	valgrind $(OBJDIR)/$*.t $(TEST_ARGS)

%.t : %.t.cpp *.h $(CXX_CONFIG_FILE)
	$(CXX) $(CXXFLAGS) -o $(OBJDIR)/$@ $<

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
