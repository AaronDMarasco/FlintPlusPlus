#include "Checks.hpp"

namespace flint {

/**
 * Check all member intializations to make sure they do not initialize on themselves
 *
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 * @return
 *        Returns the number of errors this check found in the token stream
 */
void checkInitializeFromItself(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  // Token Sequences for parameter initializers
  static constexpr array<TokenType, 5> firstInitializer{TK_COLON, TK_IDENTIFIER, TK_LPAREN, TK_IDENTIFIER, TK_RPAREN};
  static constexpr array<TokenType, 5> nthInitializer{TK_COMMA, TK_IDENTIFIER, TK_LPAREN, TK_IDENTIFIER, TK_RPAREN};

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (atSequence(tokens, pos, firstInitializer) || atSequence(tokens, pos, nthInitializer)) {
      const size_t outerPos = ++pos;      // +1 for identifier
      const size_t innerPos = ++(++pos);  // +2 again for the inner identifier

      const bool isMember = tokens[outerPos].value_.back() == '_' || startsWith(tokens[outerPos].value_.begin(), "m_");

      if (isMember && cmpToks(tokens[outerPos], tokens[innerPos]))
        lintError(errors,
                  tokens[outerPos],
                  "Initializing class member '" + to_string(tokens[outerPos].value_) + "' with itself.");
    }
  }
};
}  // namespace flint
