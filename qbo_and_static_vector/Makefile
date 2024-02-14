
inplace_vector_compile_test:
	rm -f inplace_vector.t
	time g++ -Wall -std=c++23 -O2 -I. -c inplace_vector.t.cpp
	time g++ -Wall -std=c++23 -O2 -I. -DINPLACE_VECTOR_WITH_ALLOC -c inplace_vector.t.cpp
	time clang++ -Wall -std=c++2b -O2 -I. -c inplace_vector.t.cpp
	time clang++ -Wall -std=c++2b -O2 -I. -DINPLACE_VECTOR_WITH_ALLOC -c inplace_vector.t.cpp
	clang++ -Wall -std=c++2b -O2 inplace_vector.t.o -o inplace_vector.t
	time ./inplace_vector.t
