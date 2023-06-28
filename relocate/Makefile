
CXX = clang++
CXXFLAGS = -std=c++20 -I. -g

%.t : %.t.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

%.test : %.t .FORCE
	./$<

.FORCE :
