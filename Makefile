# comment this out for gcc-5:
# STDLIB = -stdlib=libc++

CXX = g++
OPT = -O3
# OPT = -g
OBJ = obj
CXXFLAGS = -std=c++17 $(OPT) $(STDLIB)

XTRA_ARGS ?=

RESULTS = results
BINARIES = $(OBJ)/benchmark

MKDIRS := $(shell mkdir -p $(OBJ) $(RESULTS))

all : $(BINARIES)

smoke : all
	$(OBJ)/benchmark 4 2 3 1 1 5 -v -p

$(OBJ)/% : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test.%: $(BINARIES)
	./runtest $(XTRA_ARGS) $*

clean:
	rm -f $(BINARIES)

cleanall:
	rm -rf $(OBJ) $(RESULTS)
	mkdir -p $(OBJ) $(RESULTS)
