# comment this out for gcc-5:
# STDLIB = -stdlib=libc++

CXX = g++
OPT = -O3
# OPT = -g
OBJ = obj
CXXFLAGS = -std=c++17 $(OPT) $(STDLIB)

RESULTS = results
BINARIES = $(OBJ)/benchmark-CP $(OBJ)/benchmark-MV

MKDIRS := $(shell mkdir -p $(OBJ) $(RESULTS))

all : $(BINARIES)

smoke : all
	./runtest 18 4 . 3 1 1 8 -v
#	$(OBJ)/benchmark-CP 4 2 3 1 1 5 -v
#	$(OBJ)/benchmark-MV 4 2 3 1 1 5 -v

$(OBJ)/benchmark-CP : benchmark.cpp
	mkdir -p $(RESULTS)
	$(CXX) $(CXXFLAGS) -DUSE_COPY -o $@ $<

$(OBJ)/benchmark-MV : benchmark.cpp
	mkdir -p $(RESULTS)
	$(CXX) $(CXXFLAGS) -DUSE_MOVE -o $@ $<

test8: $(BINARIES)
	./runtest 8

test9: $(BINARIES)
	./runtest 9

clean:
	rm -f $(BINARIES)

cleanall:
	rm -rf $(OBJ) $(RESULTS)
	mkdir -p $(OBJ) $(RESULTS)
