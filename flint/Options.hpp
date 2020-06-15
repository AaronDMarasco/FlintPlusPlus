#pragma once

#include <string>
#include <vector>

namespace flint {

enum Lint { ERROR, WARNING, ADVICE };

struct OptionsInfo {
  bool RECURSIVE{false};
  bool CMODE{false};
  bool JSON{false};
  bool VERBOSE{false};
  int  LEVEL{Lint::ADVICE};
};
extern OptionsInfo Options;

void printHelp();
void parseArgs(int argc, char* argv[], std::vector<std::string>& paths);
};  // namespace flint
