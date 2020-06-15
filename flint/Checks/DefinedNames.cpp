#include "../Checks.hpp"
using namespace std;

namespace flint {

/**
 * No #defined names use an identifier reserved to the
 * implementation.
 *
 * These are enforcing rules that actually apply to all identifiers,
 * but we're only raising warnings for #define'd ones right now.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkDefinedNames(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  // Exceptions to the check
  static const unordered_set<string> okNames{
      "__STDC_LIMIT_MACROS", "__STDC_FORMAT_MACROS", "_GNU_SOURCE", "_XOPEN_SOURCE"};

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (!isTok(tokens[pos], TK_DEFINE)) continue;

    const auto   tok = tokens[pos + 1];
    const string sym{to_string(tok.value_)};

    if (!isTok(tok, TK_IDENTIFIER)) {
      // This actually happens because people #define private public
      //   for unittest reasons
      lintWarning(errors, tok, "You're not supposed to #define " + sym);
      continue;
    }

    if (sym.size() >= 2 && sym[0] == '_' && isupper(sym[1])) {
      if (okNames.find(sym) != okNames.end()) continue;
      lintWarning(errors,
                  tok,
                  "Symbol " + sym + " invalid.",
                  "A symbol may not start with an underscore followed by a capital letter.");
    } else if (sym.size() >= 2 && sym[0] == '_' && sym[1] == '_') {
      if (okNames.find(sym) != okNames.end()) continue;
      lintWarning(errors, tok, "Symbol " + sym + " invalid.", "A symbol may not begin with two adjacent underscores.");
    } else if (!Options.CMODE &&
               sym.find("__") != string::npos) {  // !FLAGS_c_mode /* C is less restrictive about this */ &&
      if (okNames.find(sym) != okNames.end()) continue;
      lintWarning(errors, tok, "Symbol " + sym + " invalid. ", "A symbol may not contain two adjacent underscores.");
    }
  }
};

}  // namespace flint
