# CXX = g++ -fPIC
CXX = mpic++
quicksort: quicksort.cpp quicksort.h
	$(CXX) -g -o quicksort quicksort.cpp 

.PHONY: clean
clean:
	rm quicksort
