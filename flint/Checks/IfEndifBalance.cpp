#include "../Checks.hpp"
using namespace std;

namespace flint {

/**
 * Balance of #if(#ifdef, #ifndef)/#endif.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkIfEndifBalance(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  int openIf = 0;

  // Return after the first found error, because otherwise
  // even one missed #if can be cause of a lot of errors.
  for (const auto& tok: tokens) {
    if (isTok(tok, TK_IFNDEF) || isTok(tok, TK_IFDEF) || isTok(tok, TK_POUNDIF)) {
      ++openIf;
    } else if (isTok(tok, TK_ENDIF)) {
      --openIf;
      if (openIf < 0) lintError(errors, tok, "Unmatched #endif.");
    } else if (isTok(tok, TK_POUNDELSE)) {
      if (openIf == 0) lintError(errors, tok, "Unmatched #else.");
    }
  }

  if (openIf != 0) lintError(errors, tokens.back(), "Unmatched #if/#endif.");
};
}  // namespace flint
