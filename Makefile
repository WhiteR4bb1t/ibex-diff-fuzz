CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -g
# -MMD -MP: der Compiler schreibt pro .o eine .d mit den eingebundenen Headern,
# damit eine Header-Änderung die betroffenen .o neu baut.
CPPFLAGS := -MMD -MP

OBJDIR   := bin/obj
# Alles außer den beiden main()-Dateien gehört in beide Binaries.
LIB_SRCS := $(filter-out src/main.cpp src/tests.cpp,$(wildcard src/*.cpp))
LIB_OBJS := $(LIB_SRCS:src/%.cpp=$(OBJDIR)/%.o)

.DEFAULT_GOAL := fuzz
.PHONY: fuzz fuzztest all clean

fuzz: bin/fuzz
fuzztest: bin/fuzztest
all: fuzz fuzztest

bin/fuzz: $(OBJDIR)/main.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/fuzztest: $(OBJDIR)/tests.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf bin

-include $(wildcard $(OBJDIR)/*.d)
