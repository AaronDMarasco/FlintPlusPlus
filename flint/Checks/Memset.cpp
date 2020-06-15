#include "../Checks.hpp"

namespace flint {

/**
 * If encounter memset(foo, sizeof(foo), 0), we warn that the order
 * of the arguments is wrong.
 * Known unsupported case: calling memset inside another memset. The inner
 * call will not be checked.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkMemset(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  static constexpr array<TokenType, 2> funcSequence = {TK_IDENTIFIER, TK_LPAREN};

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    const auto tok = tokens[pos];

    if (!atSequence(tokens, pos, funcSequence) || !cmpTok(tok, "memset")) continue;

    vector<Argument> args;
    Argument         func(pos, pos);
    if (!getFunctionNameAndArguments(tokens, pos, func, args)) return;

    // If there are more than 3 arguments, then there might be something wrong
    // with skipTemplateSpec but the iterator didn't reach the EOF (because of
    // a '>' somewhere later in the code). So we only deal with the case where
    // the number of arguments is correct.
    if (args.size() == 3) {
      // wrong calls include memset(..., ..., 0) and memset(..., sizeof..., 1)
      const bool error = ((args[2].last - args[2].first) == 1) &&
                         (cmpTok(tokens[args[2].first], "0") ||
                          (cmpTok(tokens[args[2].first], "1") && cmpTok(tokens[args[1].first], "sizeof")));

      if (!error) continue;

      swap(args[1], args[2]);
      lintError(errors, tok, "Did you mean " + formatFunction(tokens, func, args) + " ?");
    }
  }
};
}  // namespace flint
