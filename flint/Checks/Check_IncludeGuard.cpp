#include "../Checks.hpp"

namespace flint {

/**
 * If header file contains include guard.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkIncludeGuard(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  if (getFileCategory(path) != FileCategory::HEADER) return;

  static constexpr array<TokenType, 2> pragmaOnce = {TK_PRAGMA, TK_IDENTIFIER};

  // Allow #pragma once as an include guard
  if (atSequence(tokens, 0, pragmaOnce) && cmpTok(tokens[1], "once")) return;

  static constexpr array<TokenType, 4> includeGuard = {TK_IFNDEF, TK_IDENTIFIER, TK_DEFINE, TK_IDENTIFIER};

  if (!atSequence(tokens, 0, includeGuard)) {
    lintError(errors, tokens[0], "Missing include guard.");
    return;
  }

  if (!cmpToks(tokens[1], tokens[3]))
    lintError(errors,
              tokens[1],
              "Include guard name mismatch; expected " + to_string(tokens[1].value_) + ", saw " +
                  to_string(tokens[3].value_));

  int openIf = 1;

  size_t       pos;
  const size_t size = tokens.size();
  for (pos = 1; pos < size; ++pos) {
    if (isTok(tokens[pos], TK_IFNDEF) || isTok(tokens[pos], TK_IFDEF) || isTok(tokens[pos], TK_POUNDIF)) {
      ++openIf;
      continue;
    }
    if (isTok(tokens[pos], TK_ENDIF)) {
      --openIf;
      if (openIf == 0) break;
      continue;
    }
  }

  if (openIf != 0 || pos < size - 2) {
    lintError(errors, tokens.back(), "Include guard doesn't cover the entire file.");
    return;
  }
};
}  // namespace flint
