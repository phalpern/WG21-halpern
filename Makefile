# comment this out for gcc-5:
# STDLIB = -stdlib=libc++

CXX = g++
OPT = -O3
OBJ = obj
CXXFLAGS = -std=c++17 -Wall $(STDLIB)

XTRA_ARGS ?=

RESULTS = results
BINARIES     = $(OBJ)/benchmark
DBG_BINARIES = $(OBJ)/dbg-benchmark

MKDIRS := $(shell mkdir -p $(OBJ) $(RESULTS))

all : $(BINARIES)

debug : $(DBG_BINARIES)

smoke : $(BINARIES)
	$(OBJ)/benchmark . 2^4 16 8 1 2^0 1k -vp

$(OBJ)/% : %.cpp
	$(CXX) $(CXXFLAGS) $(OPT) -o $@ $<

$(OBJ)/dbg-% : %.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $<

test.%: $(BINARIES)
	./runtest $(XTRA_ARGS) $*

clean:
	rm -f $(BINARIES) $(DBG_BINARIES)

cleanall:
	rm -rf $(OBJ) $(RESULTS)
	mkdir -p $(OBJ) $(RESULTS)
