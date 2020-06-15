#include "Checks.hpp"

namespace flint {

/**
 * Check for conflicting namespace usages
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkUsingNamespaceDirectives(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  vector<StringFragment> namespaces;
  stack<size_t>          scopes;

  static constexpr array<TokenType, 2> usingNamespace{TK_USING, TK_NAMESPACE};

  static const array<string, 6> exclusive{"std", "std::tr1", "boost", "::std", "::std::tr1", "::boost"};

  static const vector<StringFragment> exclusiveFragments = []() -> vector<StringFragment> {
    vector<StringFragment> out;
    for_each(begin(exclusive), end(exclusive), [&](const string& str) { out.emplace_back(begin(str), end(str)); });
    return out;
  }();

  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (isTok(tokens[pos], TK_LCURL)) {
      scopes.push(namespaces.size());
      continue;
    }

    if (isTok(tokens[pos], TK_RCURL)) {
      if (!scopes.empty()) {
        auto del = scopes.top();
        while (namespaces.size() > del) namespaces.pop_back();
        scopes.pop();
      }
      continue;
    }

    if (atSequence(tokens, pos, usingNamespace)) {
      pos += 2;

      const auto isExclusive =
          find_if(begin(exclusiveFragments), end(exclusiveFragments), [=](const StringFragment& frag) {
            return matchAcrossTokens(frag, begin(tokens) + pos, end(tokens));
          });
      if (isExclusive == end(exclusiveFragments)) continue;

      const auto conflict = find_if(
          begin(namespaces), end(namespaces), [&](const StringFragment& frag) { return !(frag == *isExclusive); });
      if (conflict != end(namespaces))
        lintWarning(
            errors, tokens[pos], "Conflicting namespaces: " + to_string(*isExclusive) + " and " + to_string(*conflict));

      namespaces.push_back(*isExclusive);
      continue;
    }
  }
};

}  // namespace flint
