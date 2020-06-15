#include "../Checks.hpp"
using namespace std;

namespace flint {

/**
 * Disallow the declaration of mutex holders
 * with no name, since that causes the destructor to be called
 * on the same line, releasing the lock immediately.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkMutexHolderHasName(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  if (getFileCategory(path) == FileCategory::SOURCE_C) return;

  static const string mutexHolder{"lock_guard"};

  static constexpr array<TokenType, 2> mutexSequence{TK_IDENTIFIER, TK_LESS};

  static constexpr array<TokenType, 2> mutexConstructor{TK_GREATER, TK_LPAREN};

  for (size_t pos = 0, size = tokens.size(); pos < size - 1; ++pos) {
    if (atSequence(tokens, pos, mutexSequence) && cmpTok(tokens[pos], mutexHolder)) {
      pos = skipTemplateSpec(tokens, ++pos);
      if (atSequence(tokens, pos, mutexConstructor))
        lintError(errors,
                  tokens[pos],
                  "Mutex holder variable declared without a name, "
                  "causing the lock to be released immediately.");
    }
  }
};
}  // namespace flint
