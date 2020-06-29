# `Flint++`
![](https://img.shields.io/travis/com/aarondmarasco/FlintPlusPlus)

## A Cross Platform Port of Facebook's C++ Linter

`Flint++` is cross-platform, zero-dependency port of `flint`, a lint program for C++ developed and used at Facebook.

This project was motivated by a desire for a modern and extendable C++ Linter that just worked. Facebook had already done a fantastic job with their `flint` project; but through an unnecessarily high number of dependencies, poor documentation, and OS-dependent coding, the project was almost unusable. `Flint++` aims to solve these problems by only using the C++11 std::library along with a minimal number of functions developed to bridge the gaps in the needed functionality.

The original `flint` is published on [Github](https://github.com/facebook/flint); and for discussions, there is a [Google group](https://groups.google.com/d/forum/facebook-flint).

[@AaronDMarasco]( https://github.com/AaronDMarasco/FlintPlusPlus ) forked from the [@kanielc]( https://github.com/kanielc/FlintPlusPlus ) fork of the [@JossWhittle]( https://github.com/JossWhittle/FlintPlusPlus ) fork in 2020 (with no activity since 2014). Cleaned up a little, manually merged in the following forks (closing all JossWhittle PRs that were open at the time), added RPM building and CI abilities, and now it's "as-is":

* [@vix597]( https://github.com/vix597/FlintPlusPlus ) - more sane makefile, debian packaging, return values (#63)
* [@warmsocks]( https://github.com/warmsocks/FlintPlusPlus ) - allow spaces (#60)

# Why Lint?
Linting is a form of *static-code analysis* by which common errors and bad practices are flagged for review. This can help to both optimize poorly written code and to set a unified code style for all the code in a project. For large organizations this can be tremendously powerful as it helps to keep the whole codebase consistent.

# Future Ideas
* More lint tests
* Visual Studio Integration
* Command line options for level of error in return values
* JSON Config files to allow project dependent Lint settings
	* Set custom blacklisted identifiers/token sequences/includes
	* Enable/Disable certain tests
	* Track the config file with Git to give everyone on your team the same Lint checks

# Current Lint Checks
* Errors
	* Blacklisted Identifiers
	* Initialization from Self
	* `#if ... #endif` Balance
	* `memset` Usage
	* Include Associated Header First
	* Include Guards
	* Inl-Header Inclusions
	* Check for unamed `mutex` holders
	* `explicit` single argument constructors
	* `try-catch` by reference
	* Check for `throw new ...`
	* Check `unique_ptr` arrays
* Warnings
	* Blacklisted Sequences
	* `#define` name rules
	* Deprecated `#includes`
	* Check for `static` scope
	* Warn about `smart_ptr` usage
	* `implicit` casts
	* `protected` inheritance
	* Check `exception` inheritance
	* Check for `virtual` destructors
	* function level `throws`
	* UTF-8 BOM
* Advice
	* `nullptr` over `NULL`

# Usage

	$ flint++ --help
	Usage: flint++ [options:] [files:]

	-r, --recursive		  : Search subfolders for files.
	-c, --cmode			  : Only perform C based lint checks.
	-j, --json			  : Output report in JSON format.
	-v, --verbose		  : Print full file paths.
	-l, --level [def = 3] : Set the lint level.
			            1 : Errors only
			            2 : Errors & Warnings
			            3 : All feedback

	-h, --help		      : Print usage.

# Compiling `Flint++` from source
From the `flint` subdirectory, use `make` with the included `Makefile` to build. To run the simple output test cases, run `make check` after compilation. This will run `Flint++` on the test directory and compare its output to the text stored in `tests/expected.txt`.

Additional information can be found by examining `.travis.yml`.

## Flinting Flint
All good.

	$ flint++ ./ Checks/ AdvancedChecks/ # NOT -r or test cases included

	Lint Summary: 37 files
	Errors: 0 Warnings: 0 Advice: 0

	Estimated Lines of Code: 3953

## Choosing Compiler
`make` will use your system-default compiler and C++ library. On `GCC`-based systems, you can explicitly force `clang` (and `libc++`) by calling `CXX=clang++ make -j`. This may fail and may require additional packages, _e.g._ `libcxx-devel` on Fedora or `libc++-dev` on Ubuntu.

## Packaging
### TAR
To create a tarball of the source, `make dist` at the top-level.
 * *Note*: This requires GNU `tar` so OSX and other BSD systems will need to ensure their `$PATH` is configured properly!

### RPM
An RPM package can be built by calling `make rpm` at the top-level.

### DEB
A DEB package can be built by calling `make deb` at the top-level.

### Docker
An "official" image is [available on Docker Hub](https://hub.docker.com/r/admarasco/flint).

If you would like to locally build an image, it can be built by calling `make docker` at the top-level. It will automatically use the `docker` or `podman` CLI.

#### Using Docker Image:
_Note_: This document uses `podman`, but every command shown _should_ work using the `docker` executable with the same options if you are using an older Fedora-based system or a non-Fedora-based Linux distribution.

By default, the image will recursively scan `/src` within the container; this must be mounted in the proper manner (_e.g._ SELinux context with `:z`).

Some examples:
 * Using the "official" image: `podman run --rm -v $(pwd):/src:z admarasco/flint`
 * Using a locally built image: `podman run --rm -v $(pwd):/src:z flint`
 * With some pretty JSON output: `podman run --rm -v $(pwd):/src:z flint -j | jq .`
 * Getting help: `podman run --rm flint -h.`

# Tested On
Current maintainer develops with G++ 10.1.1 (Fedora 32) and Travis CI currently tests [10 different configurations](https://travis-ci.com/github/AaronDMarasco/FlintPlusPlus/) using GCC and clang on Linux, MacOS X, and Windows.

## Pull Requests Welcomed!
