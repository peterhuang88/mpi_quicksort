CXX = g++ -fPIC

quicksort: quicksort.cpp quicksort.h
	$(CXX) -g -pthread -o quicksort quicksort.cpp 

.PHONY: clean
clean:
	rm quicksort
