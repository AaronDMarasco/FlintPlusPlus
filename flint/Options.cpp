#include "Options.hpp"

#include <unordered_map>

using namespace std;

namespace flint {

OptionsInfo Options;

/**
 * Prints the usage information for the program, then exits with error.
 */
void __attribute__((noreturn)) printHelp() {
  printf(
      "Usage: flint++ [options] [files]\n\n"
      "\t-r, --recursive\t\t: Search subfolders for files.\n"
      "\t-c, --cmode\t\t: Only perform C based lint checks.\n"
      "\t-j, --json\t\t: Output report in JSON format.\n"
      "\t-v, --verbose\t\t: Print full file paths.\n"
      "\t-l, --level [def=3] : Set the lint level.\n"
      "\t\t\t          1 : Errors only\n"
      "\t\t\t          2 : Errors & Warnings\n"
      "\t\t\t          3 : All feedback\n"
      "\t-h, --help\t\t: Print usage.\n\n");
#ifdef _DEBUG
  // Stop visual studio from closing the window...
  system("PAUSE");
#endif
  exit(1);
};

/**
 * Given an argument count and list, parses the arguments
 * and sets the global options as desired
 *
 * @param argc
 *        The number of arguments
 * @param argv
 *        The list of cmdline arguments
 * @param paths
 *        A vector of strings to be filled with lint paths
 */
void parseArgs(int argc, char* argv[], vector<string>& paths) {
  bool HELP = false;
  bool l1   = false;
  bool l2   = false;
  bool l3   = false;

  // TODO: C++17 std::variant
  enum ArgType { BOOL, INT };
  struct Arg {
    ArgType type;
    void*   ptr;
  };

  // Map values to their cmdline flags
  Arg argHelp      = {ArgType::BOOL, &HELP};
  Arg argRecursive = {ArgType::BOOL, &Options.RECURSIVE};
  Arg argCMode     = {ArgType::BOOL, &Options.CMODE};
  Arg argJSON      = {ArgType::BOOL, &Options.JSON};
  Arg argVerbose   = {ArgType::BOOL, &Options.VERBOSE};
  Arg argLevel     = {ArgType::INT, &Options.LEVEL};
  Arg argL1        = {ArgType::BOOL, &l1};
  Arg argL2        = {ArgType::BOOL, &l2};
  Arg argL3        = {ArgType::BOOL, &l3};
  // clang-format off
  static const unordered_map<string, Arg &> params {
    { "-h", argHelp },
    { "--help", argHelp },

    { "-r", argRecursive },
    { "--recursive", argRecursive },

    { "-c", argCMode },
    { "--cmode", argCMode },

    { "-j", argJSON },
    { "--json", argJSON },

    { "-l", argLevel },
    { "--level", argLevel },
    { "-l1", argL1 },
    { "-l2", argL2 },
    { "-l3", argL3 },

    { "-v", argVerbose },
    { "--verbose", argVerbose }
  };
  // clang-format on
  // Loop over the given argument list
  for (int i = 1; i < argc; ++i) {
    // If the current argument is in the map
    // then set its value to true
    const auto it = params.find(string(argv[i]));
    if (it != params.end()) {
      if (it->second.type == ArgType::BOOL) {
        auto arg = static_cast<bool*>(it->second.ptr);
        *arg     = true;
      } else if (it->second.type == ArgType::INT) {
        auto arg = static_cast<int*>(it->second.ptr);

        if (++i >= argc) {
          printf("Missing (int) value for parameter: %s\n\n", it->first.c_str());
          printHelp();
        }

        int val = atoi(argv[i]) - 1;
        *arg    = val;
        continue;
      }
    } else {
      // Push another path onto the lint list
      string p = argv[i];
      if (p.back() == '/' || p.back() == '\\') p.erase(p.end() - 1, p.end());
      paths.push_back(move(p));
    }
  }

  if (HELP) printHelp();

  if (l1)
    Options.LEVEL = Lint::ERROR;
  else if (l2)
    Options.LEVEL = Lint::WARNING;
  else if (l3)
    Options.LEVEL = Lint::ADVICE;

  // Make sure level was given a valid value
  Options.LEVEL = std::min(Options.LEVEL, static_cast<int>(Lint::ADVICE));
  Options.LEVEL = std::max(Options.LEVEL, static_cast<int>(Lint::ERROR));

  if (paths.empty()) paths.emplace_back(".");
};
};  // namespace flint
