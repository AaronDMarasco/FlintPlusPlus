#include "Checks.hpp"

namespace flint {

/**
 * Identifies usage of shared_ptr() and suggests replacing with
 * make_shared(). When shared_ptr takes 3 arguments, a custom allocator is used
 * and allocate_shared() is suggested.
 * The suggested replacements perform less memory allocations.
 *
 * Overall, matches usages of <namespace>::shared_ptr<T> id(new Ctor(),...);
 * where <namespace> is one of "std" or "boost". It also matches unqualified usages.
 * Requires the first argument of the call to be a "new expression" starting
 * with the "new" keyword.
 * That is not inclusive of all usages of that construct but it allows
 * to easily distinguish function calls vs. function declarations.
 * Essentially this function matches the following
 * <namespace>::shared_ptr TemplateSpc identifier Arguments
 * where the first argument starts with "new" and <namespace> is optional
 * and, when present, one of the values described above.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkSmartPtrUsage(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  static constexpr array<TokenType, 2> funcSignature{TK_IDENTIFIER, TK_LPAREN};

  for (size_t pos = 0, size = tokens.size(); pos < size - 1; ++pos) {
    const auto ident = readQualifiedIdentifier(tokens, pos);

    if (!((ident.size() == 1 && cmpStr(ident[0], "shared_ptr")) ||
          (ident.size() == 2 && (cmpStr(ident[0], "std") || cmpStr(ident[0], "boost")) &&
           cmpStr(ident[1], "shared_ptr"))))
      continue;

    // Stash indices for later
    size_t       i           = pos;
    const size_t sharedPtrIt = pos;

    // Determine if the template parameter is an array type.
    if (!isTok(tokens[i], TK_LESS)) continue;
    i = skipTemplateSpec(tokens, i);
    if (isTok(tokens[i], TK_EOF)) return;
    assert(isTok(tokens[i], TK_GREATER));
    ++i;

    // look for a possible function call
    if (!atSequence(tokens, i, funcSignature)) continue;

    ++i;
    vector<Argument> args;
    // ensure the function call first argument is a new expression
    if (!getRealArguments(tokens, i, args)) continue;

    if (isTok(tokens[i], TK_RPAREN) && isTok(tokens[i + 1], TK_SEMICOLON) && (args.size() > 0) &&
        (isTok(tokens[(args[0].first)], TK_NEW))) {
      // identifies what to suggest:
      // shared_ptr should be  make_shared unless there are 3 args in which
      // case an allocator is used and thus suggests allocate_shared.
      const string newFn{(args.size() == 3) ? "allocate_shared" : "make_shared"};

      lintWarning(
          errors, tokens[sharedPtrIt], "Consider using '" + newFn + "' which performs better with fewer allocations.");
    }
  }
};
}  // namespace flint
