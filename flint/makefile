# We always set the c++11 and optimization flags in addition
# to anything the make invokation has set. this makes it possible
# for users to specify their own flags with "make CXXFLAGS=.... "
EXTRA_CXXFLAGS := -O3 -std=c++11

# Inputs (grab all .cpp files, and assume change to .o)
SOURCES := $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)

# Output to binary
BINF:=flint++

# Test files
EXPECTED=tests/expected.txt
ACTUAL=tests/actual.txt

.PHONY: all tests
.SILENT: tests

all: $(BINF)

$(BINF): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINF) $(OBJS)

tests: $(BINF)
	-./$(BINF) ./tests > $(ACTUAL)
	diff -u $(EXPECTED) $(ACTUAL) || (echo "Output DOES NOT MATCH EXPECTED!" && false)
	echo "Output matches expected"

.cpp.o: $<
	$(CXX) $(EXTRA_CXXFLAGS) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm -rf *.o $(BINF) $(ACTUAL)
