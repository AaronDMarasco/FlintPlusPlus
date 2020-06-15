#include "Checks.hpp"

namespace flint {

/**
 * Check for blacklisted sequences of tokens
 *
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 * @return
 *        Returns the number of errors this check found in the token stream
 */
void checkBlacklistedSequences(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  struct BlacklistEntry {
    vector<TokenType> tokens;
    string            title, descr;
    bool              cpponly;
    BlacklistEntry(vector<TokenType> t, string h, string d, bool cpponly_)
        : tokens(move(t)), title(move(h)), descr(move(d)), cpponly(cpponly_){};
  };

  static const array<BlacklistEntry, 1> blacklist{{{
      {TK_VOLATILE},
      "'volatile' is not thread-safe.",
      "If multiple threads are sharing data, use std::atomic or locks. In addition, 'volatile' may "
      "force the compiler to generate worse code than it could otherwise. "
      "For more about why 'volatile' doesn't do what you think it does, see "
      "http://www.kernel.org/doc/Documentation/volatile-considered-harmful.txt.",
      true,  // C++ only.
  }}};

  static const array<vector<TokenType>, 1> exceptions{{{TK_ASM, TK_VOLATILE}}};

  bool isException = false;

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    // Make sure we aren't at an exception to the blacklist
    for (const auto& e: exceptions) {
      if (atSequence(tokens, pos, e)) {
        isException = true;
        break;
      }
    }

    for (const BlacklistEntry& entry: blacklist) {
      if (!atSequence(tokens, pos, entry.tokens)) continue;
      if (isException) {
        isException = false;
        continue;
      }
      if (Options.CMODE && entry.cpponly) continue;

      lintWarning(errors, tokens[pos], entry.title, entry.descr);
    }
  }
};
}  // namespace flint
