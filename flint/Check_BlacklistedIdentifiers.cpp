#include "Checks.hpp"

namespace flint {

/**
 * Check for blacklisted identifiers
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkBlacklistedIdentifiers(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  static const unordered_map<string, pair<Lint, string>> blacklist{
      {"strtok", {Lint::ERROR, "'strtok' is not thread safe. Consider 'strtok_r'."}},

      {"NULL", {Lint::ADVICE, "Prefer `nullptr' to `NULL' in new C++ code."}}};

  for (const auto& token: tokens)
    if (isTok(token, TK_IDENTIFIER))
      for (const auto& entry: blacklist)
        if (cmpTok(token, entry.first.c_str())) {
          const auto& desc = entry.second;
          lint(errors, token, desc.first, desc.second);
          continue;
        }
};
}  // namespace flint
