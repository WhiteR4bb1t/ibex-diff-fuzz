CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -g

bin/fuzz: src/main.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $<
