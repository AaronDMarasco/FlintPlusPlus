#include "Checks.hpp"

namespace flint {

/**
 * Check for non-public std::exception inheritance
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkExceptionInheritance(ErrorFile&            errors,
                               const string&         path,
                               const vector<Token>&  tokens,
                               const vector<size_t>& structures) {
  static constexpr array<TokenType, 4> classMarkersWithColon{TK_EOF, TK_LCURL, TK_SEMICOLON, TK_COLON};

  static constexpr array<TokenType, 3> accessSpecifiers{TK_PUBLIC, TK_PRIVATE, TK_PROTECTED};

  for (unsigned long structure: structures) {
    // Start pos at the index of each identified structure
    const auto  pos = begin(tokens) + structure;
    const auto& tok = *pos;

    if (isTok(tok, TK_UNION)) continue;

    const auto colon = find_first_of(pos, end(tokens), begin(classMarkersWithColon), end(classMarkersWithColon), isTok);

    if (colon == end(tokens)) return;

    if (colon->type_ != TK_COLON) continue;

    const auto endOfClass   = getEndOfClass(colon + 1, end(tokens));
    const auto exceptionPos = find_if(colon + 1, endOfClass, [](const Token& candidate) {
      return isTok(candidate, TK_IDENTIFIER) && cmpTok(candidate, "exception");
    });

    if (exceptionPos == endOfClass) continue;

    const auto usingStdException = !isTok(*(exceptionPos - 1), TK_DOUBLE_COLON) ||
                                   (isTok(*(exceptionPos - 2), TK_IDENTIFIER) && cmpTok(*(exceptionPos - 2), "std"));
    if (!usingStdException) continue;

    // OK, we're going with the last access specifier before the exception token
    const auto lastAccess =
        accumulate(colon + 1, exceptionPos, TK_PROTECTED, [](const TokenType& curr, const Token& next) -> TokenType {
          if (isTok(next, TK_COMMA)) { return TK_PROTECTED; }

          const auto access = find(begin(accessSpecifiers), end(accessSpecifiers), next.type_);
          return access == end(accessSpecifiers) ? curr : *access;
        });

    if ((isTok(tok, TK_CLASS) && lastAccess != TK_PUBLIC) || (isTok(tok, TK_STRUCT) && lastAccess == TK_PRIVATE))
      lintWarning(errors, *exceptionPos, "std::exception should be inherited publically (C++ std: 11.2)");
  }
};
}  // namespace flint
