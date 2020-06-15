#include "../Checks.hpp"

namespace flint {

/**
 * Ensures that no files contain deprecated includes.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkDeprecatedIncludes(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  // Set storing the deprecated includes. Add new headers here if you'd like
  // to deprecate them
  static const unordered_set<string> deprecatedIncludes{
      "common/base/Base.h",
      "common/base/StringUtil.h",
  };

  for (size_t pos = 0, size = tokens.size(); pos < size - 1; ++pos) {
    if (!isTok(tokens[pos], TK_INCLUDE)) continue;

    ++pos;
    if (!isTok(tokens[pos], TK_STRING_LITERAL) || cmpTok(tokens[pos], "PRECOMPILED")) continue;

    const string includedFile{getIncludedPath(tokens[pos].value_)};
    if (deprecatedIncludes.find(includedFile) != deprecatedIncludes.end())
      lintWarning(errors, tokens[pos - 1], "Including deprecated header '" + includedFile + "'");
  }
};
}  // namespace flint
