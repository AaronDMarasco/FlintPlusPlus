# We always set the c++11 and optimization flags in addition
# to anything the make invokation has set. this makes it possible
# for users to specify their own flags with "make CXXFLAGS=.... "
EXTRA_CXXFLAGS := -O3 -std=c++11

HEADERS := $(sort $(wildcard *.hpp))
# Inputs (grab all .cpp files, and assume change to .o)
SOURCES := $(sort $(wildcard *.cpp))
OBJS := $(SOURCES:.cpp=.o)

# Output to binary
BINF := flint++

# Test files
ACTUAL = tests/actual.txt
EXPECTED = tests/expected.txt

.PHONY: all check tests
.SILENT: check tests

all: $(BINF)

$(BINF): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINF) $(OBJS)

check: tests

tests: $(BINF)
	-./$(BINF) ./tests > $(ACTUAL)
	diff -u $(EXPECTED) $(ACTUAL) || (echo "Output DOES NOT MATCH EXPECTED!" && false)
	echo "Output matches expected"

# NOTE: Lazy handling of headers - don't do this in a "real" project
$(OBJS): $(HEADERS)

.cpp.o: $<
	$(CXX) $(EXTRA_CXXFLAGS) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm -rf *.o $(BINF) $(ACTUAL)

.PHONY: format
.SILENT: format

format:
	clang-format -fallback-style=none -i --verbose $(HEADERS) $(SOURCES)
