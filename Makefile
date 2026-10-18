CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -g

default: bin/fuzz bin/fuzztest

bin/fuzz: src/main.cpp src/fuzzer.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp src/fuzzer.cpp

bin/fuzztest: src/tests.cpp src/fuzzer.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ src/tests.cpp src/fuzzer.cpp
