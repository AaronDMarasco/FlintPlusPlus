#include "../Checks.hpp"

namespace flint {

/**
 * Only the following forms of catch are allowed:
 *
 * catch (Type &)
 * catch (const Type &)
 * catch (Type const &)
 * catch (Type & e)
 * catch (const Type & e)
 * catch (Type const & e)
 *
 * Type cannot be built-in; this function enforces that it's
 * user-defined.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkCatchByReference(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  for (size_t pos = 0, size = tokens.size(); pos < size; ++pos) {
    if (!isTok(tokens[pos], TK_CATCH)) continue;

    size_t focal = pos + 1;
    if (!isTok(tokens[focal], TK_LPAREN)) {  // a "(" comes always after catch
      throw runtime_error(path + ':' + std::to_string(tokens[focal].line_) +
                          ": Invalid C++ source code, please compile before lint.");
    }
    ++focal;

    if (isTok(tokens[focal], TK_ELLIPSIS))
      // catch (...
      continue;
    if (isTok(tokens[focal], TK_CONST))
      // catch (const
      ++focal;
    if (isTok(tokens[focal], TK_TYPENAME))
      // catch ([const] typename
      ++focal;
    if (isTok(tokens[focal], TK_DOUBLE_COLON))
      // catch ([const] [typename] ::
      ++focal;

    // At this position we must have an identifier - the type caught,
    // e.g. FBException, or the first identifier in an elaborate type
    // specifier, such as facebook::FancyException<int, string>.
    if (!isTok(tokens[focal], TK_IDENTIFIER)) {
      const Token& tok = tokens[focal];
      lintWarning(errors,
                  tok,
                  "Symbol '" + to_string(tok.value_) +
                      "' invalid in catch clause. You may only catch user-defined types.");
      continue;
    }
    ++focal;

    // We move the focus to the closing paren to detect the "&". We're
    // balancing parens because there are weird corner cases like
    // catch (Ex<(1 + 1)> & e).
    for (size_t parens = 1;; ++focal) {
      if (focal >= size - 1)
        throw runtime_error(path + ':' + std::to_string(tokens[focal].line_) +
                            ": Invalid C++ source code, please compile before lint.");
      if (isTok(tokens[focal], TK_RPAREN)) {
        --parens;
        if (parens == 0) { break; }
        continue;
      }
      if (isTok(tokens[focal], TK_LPAREN)) {
        ++parens;
        continue;
      }
    }

    // At this point we're straight on the closing ")". Backing off
    // from there we should find either "& identifier" or "&" meaning
    // anonymous identifier.
    if (isTok(tokens[focal - 1], TK_AMPERSAND))
      // check! catch (whatever &)
      continue;
    if (isTok(tokens[focal - 1], TK_IDENTIFIER) && isTok(tokens[focal - 2], TK_AMPERSAND))
      // check! catch (whatever & ident)
      continue;

    // Oopsies times
    const Token& tok = tokens[focal - 1];
    // Get the type string
    string theType = "";
    for (size_t j = pos + 2; j <= focal - 1; ++j) {
      if (j > 2) theType += ' ';
      const auto& val = tokens[j].value_;
      theType.append(val.begin(), val.end());
    }
    lintError(errors,
              tok,
              "Symbol '" + to_string(tok.value_) + "' of type '" + theType +
                  "' caught by value. Use catch by (preferably const) reference throughout.");
  }
};
}  // namespace flint
