#include "../Checks.hpp"

namespace flint {

/**
 * Classes should not have protected inheritance.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkProtectedInheritance(ErrorFile&            errors,
                               const string&         path,
                               const vector<Token>&  tokens,
                               const vector<size_t>& structures) {
  static constexpr array<TokenType, 3> protectedSequence{TK_COLON, TK_PROTECTED, TK_IDENTIFIER};

  const size_t toksize = tokens.size();
  for (auto pos: structures) {
    for (; pos < toksize - 2; ++pos) {
      if (isTok(tokens[pos], TK_LCURL) || isTok(tokens[pos], TK_SEMICOLON)) break;

      if (atSequence(tokens, pos, protectedSequence))
        lintWarning(errors,
                    tokens[pos],
                    "Protected inheritance is sometimes not a good idea.",
                    "Read "
                    "http://stackoverflow.com/questions/6484306/"
                    "effective-c-discouraging-protected-inheritance "
                    "for more information.");
    }
  }
};
}  // namespace flint
