#include "../Checks.hpp"

namespace flint {

/**
 * Check for static variables and functions in global/namespace scopes
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkNamespaceScopedStatics(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  if (!isHeader(path)) return;

  static constexpr array<TokenType, 3> regularNamespace{TK_NAMESPACE, TK_IDENTIFIER, TK_LCURL};

  static constexpr array<TokenType, 2> unnamedNamespace{TK_NAMESPACE, TK_LCURL};

  static constexpr array<TokenType, 2> usingNamespace{TK_USING, TK_NAMESPACE};

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (atSequence(tokens, pos, regularNamespace)) {
      pos += 2;
      continue;
    }

    if (atSequence(tokens, pos, unnamedNamespace)) {
      ++pos;
      continue;
    }

    const auto token = tokens[pos];
    if (isTok(token, TK_LCURL)) {
      pos = skipBlock(tokens, pos);
      continue;
    }

    if (isTok(tokens[pos], TK_STATIC))
      lintWarning(errors, tokens[pos], "Don't use static at global or namespace scopes in headers.");

    // Checking for 'using namespace' violations here as well
    if (atSequence(tokens, pos, usingNamespace))
      lintWarning(
          errors, tokens[pos], "Avoid the use of using namespace directives at global/namespace scope in headers");
  }
};
}  // namespace flint
