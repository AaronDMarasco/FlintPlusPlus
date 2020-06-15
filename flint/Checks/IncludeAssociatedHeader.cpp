#include "../Checks.hpp"
using namespace std;

namespace flint {

/**
 * Ensures .cpp files include their associated header first
 * (this catches #include-time dependency bugs where .h files don't
 * include things they depend on)
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkIncludeAssociatedHeader(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  if (!isSource(path)) return;

  string file(path);
  size_t fpos = file.find_last_of("/\\");
  if (fpos != string::npos) file = file.substr(fpos + 1);
  string fileBase = getFileNameBase(file);

  size_t includesFound = 0;

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (!isTok(tokens[pos], TK_INCLUDE)) continue;

    ++pos;

    if (cmpTok(tokens[pos], "PRECOMPILED")) continue;

    ++includesFound;

    if (!isTok(tokens[pos], TK_STRING_LITERAL)) continue;

    string       includedFile = getIncludedPath(tokens[pos].value_);
    const size_t ipos         = includedFile.find_last_of("/\\");
    if (ipos != string::npos) continue;

    if (cmpStr(getFileNameBase(includedFile), fileBase)) {
      if (includesFound > 1) {
        lintError(errors,
                  tokens[pos - 1],
                  "The associated header file of .cpp "
                  "files should be included before any other includes.",
                  "This helps catch missing header file dependencies in the .h");
        break;
      }
    }
  }
};
}  // namespace flint
