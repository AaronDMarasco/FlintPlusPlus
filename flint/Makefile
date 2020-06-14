define HELP
Flint++ building Makefile options:

all     - (default) builds flint++ (respecting CXXFLAGS)
check   - run built-in tests
clean   - clean up
install - install into /usr/bin (respecting DESTDIR)
format  - run clang-format on source code

Packaging options available at top-level

endef

# We always set the C++11 and optimization flags in addition
# to anything the caller has set. This makes it possible
# for users or build systems to specify their own CXXFLAGS.
EXTRA_CXXFLAGS := -O3 -std=c++11 -Wall -Werror=format-security
# Extra strict when building RPMs
ifdef RPM_BUILD_ROOT
EXTRA_CXXFLAGS += -Werror
endif
# Undocumented feature if you are trying to ensure things are clean export USEASAN=1
ifdef USEASAN
EXTRA_CXXFLAGS += -fsanitize=address
ASANLINK := -lasan
endif

HEADERS := $(sort $(wildcard *.hpp))
# Inputs (grab all .cpp files, and assume change to .o)
SOURCES := $(sort $(wildcard *.cpp))
OBJS := $(SOURCES:.cpp=.o)

# Output binary
BINF := flint++

# Test files
ACTUAL = tests/actual.txt
EXPECTED = tests/expected.txt
JACTUAL = tests/actual.json
JEXPECTED = tests/expected.json

.PHONY: all check tests
.SILENT: check tests

all: $(BINF)

$(BINF): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BINF) $(OBJS) $(ASANLINK)

# More standard alias
check: tests

tests: $(BINF)
	-./$(BINF) ./tests > $(ACTUAL)
	diff -u $(EXPECTED) $(ACTUAL) || (echo "Plaintext output DOES NOT MATCH EXPECTED!" && false)
	echo "Plaintext output matches expected"
	-./$(BINF) -j ./tests > $(JACTUAL)
	diff -u $(JEXPECTED) $(JACTUAL) || (echo "JSON output DOES NOT MATCH EXPECTED!" && false)
	echo "JSON output matches expected"

# NOTE: Lazy handling of headers - don't do this in a "real" project ;)
$(OBJS): $(HEADERS)

.cpp.o: $<
	$(CXX) $(EXTRA_CXXFLAGS) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm -rf *.o $(BINF) $(ACTUAL) $(JACTUAL)

.PHONY: install
INSTALL ?= install
install: $(BINF)
	mkdir -p $(DESTDIR)/usr/bin/
	$(INSTALL) -m 755 flint++ $(DESTDIR)/usr/bin/

.PHONY: format
.SILENT: format
format:
	clang-format -fallback-style=none -i --verbose $(HEADERS) $(SOURCES)

.PHONY: help
help: $(and $(filter help,$(MAKECMDGOALS)),$(info $(HELP)))
	@false

# Undocumented syntax checker to ensure GCC and clang both like what's been done
.PHONY: scheck
scheck: clean
	$(MAKE) -j
	$(MAKE) clean
	CXX=clang++ $(MAKE) -j
	$(MAKE) check