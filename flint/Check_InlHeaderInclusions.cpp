#include "Checks.hpp"

namespace flint {

/**
 * Makes sure inl headers are included correctly
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkInlHeaderInclusions(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  static constexpr array<TokenType, 2> includeSequence{TK_INCLUDE, TK_STRING_LITERAL};

  string file(path);
  size_t fpos = file.find_last_of("/\\");
  if (fpos != string::npos) file = file.substr(fpos + 1);
  const string fileBase{getFileNameBase(file)};

  for (size_t pos = 0, size = tokens.size(); pos < size - 1; ++pos) {
    if (!atSequence(tokens, pos, includeSequence)) continue;
    ++pos;

    string includedFile = getIncludedPath(tokens[pos].value_);

    if (getFileCategory(includedFile) != FileCategory::INL_HEADER) continue;

    file = includedFile;
    fpos = includedFile.find_last_of("/\\");
    if (fpos != string::npos) file = includedFile.substr(fpos + 1);
    string includedBase = getFileNameBase(file);

    if (cmpStr(fileBase, includedBase)) continue;

    lintError(errors,
              tokens[pos],
              "An -inl file (" + includedFile + ") was included even though this is not its associated header.",
              "Usually files like Foo-inl.h are implementation details and should "
              "not be included outside of Foo.h.");
  }
};
}  // namespace flint
