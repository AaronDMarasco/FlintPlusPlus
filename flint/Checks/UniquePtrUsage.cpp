#include "../Checks.hpp"
using namespace std;

namespace flint {

/**
 * Identifies incorrect usage of unique_ptr() with arrays. In other
 * words the unique_ptr is used with an array allocation, but not declared as
 * an array. The canonical example is: unique_ptr<Foo> Bar(new Foo[8]), which
 * compiles fine but should be unique_ptr<Foo[]> Bar(new Foo[8]).
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkUniquePtrUsage(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  for (size_t pos = 0, size = tokens.size(); pos < size - 1; ++pos) {
    const auto ident = readQualifiedIdentifier(tokens, pos);

    if (!((ident.size() == 1 && cmpStr(ident[0], "unique_ptr")) ||
          (ident.size() == 2 && cmpStr(ident[0], "std") && cmpStr(ident[1], "unique_ptr"))))
      continue;

    // Stash indices for later
    size_t       i           = pos;
    const size_t uniquePtrIt = pos;

    // Determine if the template parameter is an array type.
    if (!isTok(tokens[i], TK_LESS)) continue;
    bool uniquePtrHasArray = false;
    i                      = skipTemplateSpec(tokens, i, &uniquePtrHasArray);
    if (isTok(tokens[i], TK_EOF)) return;
    assert(isTok(tokens[i], TK_GREATER));
    ++i;

    /*
     * We should see an optional identifier, then an open paren, or
     * something is weird so bail instead of giving false positives.
     *
     * Note that we could be looking at a function declaration and its
     * return type right now---we're assuming we won't see a
     * new-expression in the argument declarations.
     */
    if (isTok(tokens[i], TK_IDENTIFIER)) ++i;
    if (!isTok(tokens[i], TK_LPAREN)) continue;  // Bail
    ++i;

    size_t parenNest = 1;
    for (; i < size - 1; ++i) {
      if (isTok(tokens[i], TK_LPAREN)) {
        ++parenNest;
        continue;
      }
      if (isTok(tokens[i], TK_RPAREN)) {
        --parenNest;
        if (parenNest == 0) { break; }
        continue;
      }

      if (!isTok(tokens[i], TK_NEW) || parenNest != 1) continue;
      ++i;

      // We're looking at the new expression we care about.  Try to
      // ensure it has array brackets only if the unique_ptr type did.
      while (isTok(tokens[i], TK_IDENTIFIER) || isTok(tokens[i], TK_DOUBLE_COLON)) ++i;
      if (isTok(tokens[i], TK_LESS)) {
        i = skipTemplateSpec(tokens, i);
        if (i == (size - 1)) return;
        ++i;
      } else {
        while (atBuiltinType(tokens, i)) ++i;
      }
      while (isTok(tokens[i], TK_STAR) || isTok(tokens[i], TK_CONST) || isTok(tokens[i], TK_VOLATILE)) ++i;

      if (isTok(tokens[i], TK_LSQUARE) != uniquePtrHasArray)
        lintError(errors,
                  tokens[uniquePtrIt],
                  (uniquePtrHasArray ? "unique_ptr<T[]> should be used with an array type."
                                     : "unique_ptr<T> should be unique_ptr<T[]> when used with an array."));
      break;
    }
  }
};
}  // namespace flint
