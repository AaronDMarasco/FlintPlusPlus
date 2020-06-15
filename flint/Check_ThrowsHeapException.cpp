#include "Checks.hpp"

namespace flint {

/**
 * Don't allow heap allocated exception, i.e. throw new Class()
 *
 * A simple check for two consecutive tokens "throw new"
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkThrowsHeapException(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  static constexpr array<TokenType, 2> throwNew = {TK_THROW, TK_NEW};

  static constexpr array<TokenType, 3> throwConstructor = {TK_LPAREN, TK_IDENTIFIER, TK_RPAREN};

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (atSequence(tokens, pos, throwNew)) {
      string msg;
      size_t focal = pos + 2;
      if (isTok(tokens[focal], TK_IDENTIFIER)) {
        msg = "Heap-allocated exception: throw new " + to_string(tokens[focal].value_) + "();";
      } else if (atSequence(tokens, focal, throwConstructor)) {
        // Alternate syntax throw new (Class)()
        ++focal;
        msg = "Heap-allocated exception: throw new (" + to_string(tokens[focal].value_) + ")();";
      } else {
        // Some other usage of throw new Class().
        msg = "Heap-allocated exception: throw new was used.";
      }

      lintError(errors, tokens[focal], msg + " This is usually a mistake in c++.");
    }
  }
};
}  // namespace flint
