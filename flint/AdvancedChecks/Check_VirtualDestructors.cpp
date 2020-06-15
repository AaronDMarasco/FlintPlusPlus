#include "../Checks.hpp"

namespace flint {

/**
 * Check for public non-virtual destructors in classes with virtual functions
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkVirtualDestructors(ErrorFile&            errors,
                             const string&         path,
                             const vector<Token>&  tokens,
                             const vector<size_t>& structures) {
  static constexpr array<TokenType, 3> accessSpecifiers{TK_PUBLIC, TK_PRIVATE, TK_PROTECTED};

  static const string msg{"Classes with virtual functions should not have a public non-virtual destructor."};

  auto size        = structures.size();
  auto penultimate = size - 1;
  for (size_t i = 0; i < size; ++i) {
    auto startIter = begin(tokens) + structures[i];
    auto endIter   = (i == penultimate) ? end(tokens) : begin(tokens) + structures[i + 1];

    const auto& tok = *startIter;

    if (isTok(tok, TK_UNION)) continue;

    // Start at end of class definition to avoid virtual bases
    const auto endOfClass = getEndOfClass(startIter + 1, endIter);
    if (endOfClass == endIter || !isTok(*endOfClass, TK_LCURL)) continue;

    // Find something virtual
    const auto virtualLocation =
        find_if(endOfClass + 1, endIter, [](const Token& token) { return isTok(token, TK_VIRTUAL); });
    if (virtualLocation == endIter) continue;  // No virtual functions or destructor

    // Now that we have something virtual, we need a destructor
    const auto userDestructor = adjacent_find(startIter, endIter, [](const Token& first, const Token& second) {
      return isTok(first, TK_TILDE) && isTok(second, TK_IDENTIFIER);
    });

    // compiler defined is not virtual
    if (userDestructor == endIter) {
      lintWarning(errors, *startIter, msg);
      continue;
    }

    // We're good, we've got a virtual destructor
    if (isTok(*(userDestructor - 1), TK_VIRTUAL)) continue;

    // Now what kind of access do we have for our virtual destructor
    using rev_iter        = reverse_iterator<TokenIter>;
    const auto lastAccess = find_first_of(
        rev_iter(userDestructor), rev_iter(startIter), begin(accessSpecifiers), end(accessSpecifiers), isTok);
    const auto access =
        (lastAccess != rev_iter(startIter)) ? lastAccess->type_ : isTok(tok, TK_STRUCT) ? TK_PUBLIC : TK_PRIVATE;

    if (access == TK_PUBLIC) lintWarning(errors, *startIter, msg);
  }
};
}  // namespace flint
