#include "Checks.hpp"

namespace flint {

/**
 * Any usage of throw specifications is a lint error.
 *
 * We track whether we are at either namespace or class scope by
 * looking for class/namespace tokens and tracking nesting level.  Any
 * time we go into a { } block that's not a class or namespace, we
 * disable the lint checks (this is to avoid false positives for throw
 * expressions).
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkThrowSpecification(ErrorFile&            errors,
                             const string&         path,
                             const vector<Token>&  tokens,
                             const vector<size_t>& structures) {
  auto numTokens = tokens.size();
  auto posLimit  = numTokens - 1;

  static constexpr array<TokenType, 7> destructorSequence{
      TK_TILDE, TK_IDENTIFIER, TK_LPAREN, TK_RPAREN, TK_THROW, TK_LPAREN, TK_RPAREN};
  static constexpr array<TokenType, 6> whatSequence{TK_LPAREN, TK_RPAREN, TK_CONST, TK_THROW, TK_LPAREN, TK_RPAREN};

  // Check for throw specifications inside classes
  for (auto pos: structures) {
    // Skip to opening '{'
    if (!skipToToken(tokens, pos, TK_LCURL)) { continue; }
    ++pos;

    for (; pos < numTokens && !isTok(tokens[pos], TK_EOF); ++pos) {
      const auto tok = tokens[pos];

      // Skip warnings for empty throw specifications on destructors,
      // because sometimes it is necessary to put a throw() clause on
      // classes deriving from std::exception.
      if (atSequence(tokens, pos, destructorSequence)) {
        pos += destructorSequence.size();
        continue;
      }

      // This avoids warning if the function is named "what", to allow
      // inheriting from std::exception without upsetting lint.
      if (isTok(tok, TK_IDENTIFIER) && cmpTok(tok, "what")) {
        ++pos;
        if (atSequence(tokens, pos, whatSequence)) pos += whatSequence.size();
        continue;
      }

      // Any time we find an open curly skip straight to the closing one
      if (isTok(tok, TK_LCURL)) {
        pos = skipBlock(tokens, pos);
        continue;
      }

      // If we actually find a closing one we know it's the object's closing bracket
      if (isTok(tok, TK_RCURL)) break;

      // Because we skip the bodies of functions the only throws we should find are function throws
      if (pos < posLimit && isTok(tok, TK_THROW) && isTok(tokens[pos + 1], TK_LPAREN)) {
        lintWarning(errors, tok, "Throw specifications on functions are deprecated.");
        continue;
      }
    }
  }

  // Check for throw specifications in functional style code
  for (size_t pos = 0; pos < numTokens; ++pos) {
    const auto tok = tokens[pos];

    // Don't accidentally identify a using statement as a namespace
    if (isTok(tok, TK_USING)) {
      if (isTok(tokens[pos + 1], TK_NAMESPACE)) { ++pos; }
      continue;
    }

    // Skip namespaces, classes, and blocks
    if (isTok(tok, TK_NAMESPACE) || isTok(tok, TK_CLASS) || isTok(tok, TK_STRUCT) || isTok(tok, TK_UNION) ||
        isTok(tok, TK_LCURL)) {
      // Move to opening object '{'
      for (; !isTok(tokens[pos], TK_LCURL) && !isTok(tokens[pos], TK_EOF); ++pos) {}

      // Return if we didn't find a '{'
      if (!isTok(tokens[pos], TK_LCURL)) return;

      // Skip to closing '}'
      pos = skipBlock(tokens, pos);
    }

    // Because we skip the bodies of functions the only throws we should find are function throws
    if (isTok(tok, TK_THROW) && isTok(tokens[pos + 1], TK_LPAREN)) {
      lintWarning(errors, tok, "Throw specifications on functions are deprecated.");
      continue;
    }
  }
};
}  // namespace flint
