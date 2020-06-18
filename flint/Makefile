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
EXTRA_CXXFLAGS := -O3 -std=c++11 -Wall -Wshadow -Werror=format-security
# Extra strict when building RPMs
ifdef RPM_BUILD_ROOT
EXTRA_CXXFLAGS += -Werror
endif
# Undocumented feature if you are trying to ensure things are clean export USEASAN=1
ifdef USEASAN
EXTRA_CXXFLAGS += -fsanitize=address
ASANLINK := -lasan
endif
# Undocumented feature if you are building with clang use libc++ even on gcc-based systems
# This allows quicker diagnosing of issues on OSX even when building on Linux
# On Fedora, this required the libcxx-devel RPMs.
ifeq ($(CXX),clang++)
EXTRA_CXXFLAGS += -stdlib=libc++
EXTRA_LDFLAGS := -lc++
# -nostdinc++ -I/usr/include/c++/v1/ -L/usr/lib64/
endif

HEADERS := $(sort $(wildcard *.hpp))
# Inputs (grab all .cpp files, and assume change to .o)
ADV_CHECKS := $(sort $(wildcard AdvancedChecks/*.cpp))
REG_CHECKS := $(sort $(wildcard Checks/*.cpp))
SOURCES := $(sort $(wildcard *.cpp)) $(ADV_CHECKS) $(REG_CHECKS)
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
	$(CXX) $(CXXFLAGS) -o $(BINF) $(OBJS) $(ASANLINK) $(LD_FLAGS) $(EXTRA_LDFLAGS)

# More standard alias
check: tests

tests: $(BINF)
	-./$(BINF) -r ./tests > $(ACTUAL)
	diff -u $(EXPECTED) $(ACTUAL) || (echo "Plaintext output DOES NOT MATCH EXPECTED!" && false)
	echo "Plaintext output matches expected"
	-./$(BINF) -j -r ./tests > $(JACTUAL)
	diff -u $(JEXPECTED) $(JACTUAL) || (echo "JSON output DOES NOT MATCH EXPECTED!" && false)
	echo "JSON output matches expected"

# NOTE: Lazy handling of headers - don't do this in a "real" project ;)
$(OBJS): $(HEADERS) Checks.inc

.SILENT: Checks.inc
# This needs to transform AdvancedChecks/*.cpp to "X_struct(CheckName);"
# and Checks/*.cpp to "X(DefinedNames);"
Checks.inc: ADV_CHECK_NAMES := $(patsubst AdvancedChecks/%.cpp,%,$(ADV_CHECKS))
Checks.inc: REG_CHECK_NAMES := $(patsubst Checks/%.cpp,%,$(REG_CHECKS))
Checks.inc:
	rm -rf $@
	for test in $(ADV_CHECK_NAMES); do echo "X_struct($${test});" >> $@; done
	for test in $(REG_CHECK_NAMES); do echo "X($${test});" >> $@; done

.cpp.o: $<
	$(CXX) $(EXTRA_CXXFLAGS) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm -rf $(OBJS) $(BINF) $(ACTUAL) $(JACTUAL) Checks.inc

.PHONY: install
INSTALL ?= install
install: $(BINF)
	mkdir -p $(DESTDIR)/usr/bin/
	$(INSTALL) -m 755 flint++ $(DESTDIR)/usr/bin/

.PHONY: format
.SILENT: format
format:
	clang-format -fallback-style=none -i --verbose $(HEADERS) $(SOURCES)
	# clang-tidy --checks='modernize-*' *.hpp *.cpp -- -std=c++11
	-git grep -P "\t" *.hpp *.cpp

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
