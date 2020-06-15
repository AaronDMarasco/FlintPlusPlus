#include "../Checks.hpp"

namespace flint {

/**
 * Warn about implicit casts
 *
 * Implicit casts not marked as explicit can be dangerous if not used carefully
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkImplicitCast(ErrorFile&            errors,
                       const string&         path,
                       const vector<Token>&  tokens,
                       const vector<size_t>& structures) {
  if (getFileCategory(path) == FileCategory::SOURCE_C) return;

  static const string lintOverride{"/* implicit */"};

  static constexpr array<TokenType, 3> explicitConstOperator{TK_EXPLICIT, TK_CONSTEXPR, TK_OPERATOR};
  static constexpr array<TokenType, 2> explicitOperator{TK_EXPLICIT, TK_OPERATOR};
  static constexpr array<TokenType, 2> doubleColonOperator{TK_DOUBLE_COLON, TK_OPERATOR};

  static constexpr array<TokenType, 4> boolOperator{TK_OPERATOR, TK_BOOL, TK_LPAREN, TK_RPAREN};
  static constexpr array<TokenType, 2> operatorDelete{TK_ASSIGN, TK_DELETE};
  static constexpr array<TokenType, 3> operatorConstDelete{TK_CONST, TK_ASSIGN, TK_DELETE};

  // Check for constructor specifications inside classes
  const size_t toksize = tokens.size();
  for (unsigned long pos: structures) {
    if (!(isTok(tokens[pos], TK_STRUCT) || isTok(tokens[pos], TK_CLASS))) continue;

    // Skip to opening '{'
    for (; pos < toksize && !isTok(tokens[pos], TK_LCURL); ++pos)
      if (!(pos < toksize) || isTok(tokens[pos], TK_SEMICOLON)) return;
    ++pos;

    for (; pos < toksize && !isTok(tokens[pos], TK_EOF); ++pos) {
      const auto tok = tokens[pos];

      // Any time we find an open curly skip straight to the closing one
      if (isTok(tok, TK_LCURL)) {
        pos = skipBlock(tokens, pos);
        continue;
      }

      // If we actually find a closing one we know it's the object's closing bracket
      if (isTok(tok, TK_RCURL)) break;

      // Skip explicit functions
      if (atSequence(tokens, pos, explicitConstOperator)) {
        ++(++pos);
        continue;
      }
      if (atSequence(tokens, pos, explicitOperator) || atSequence(tokens, pos, doubleColonOperator)) {
        ++pos;
        continue;
      }

      // bool Operator case
      if (atSequence(tokens, pos, boolOperator)) {
        if (atSequence(tokens, pos + 4, operatorDelete) || atSequence(tokens, pos + 4, operatorConstDelete)) {
          // Deleted implicit operators are ok.
          continue;
        }

        lintError(errors,
                  tok,
                  "operator bool() is dangerous.",
                  "In C++11 use explicit conversion (explicit operator bool()), "
                  "otherwise use something like the safe-bool idiom if the syntactic "
                  "convenience is justified in this case, or consider defining a "
                  "function (see http://www.artima.com/cppsource/safebool.html for more "
                  "details).");
        continue;
      }

      // Only want to process operators which do not have the overide
      if (!isTok(tok, TK_OPERATOR) || contains(tok.precedingWhitespace_, lintOverride.cbegin(), lintOverride.cend()))
        continue;

      // Assume it is an implicit conversion unless proven otherwise
      bool   isImplicitConversion = false;
      string typeString           = "";
      for (size_t typePos = pos + 1; typePos < toksize; ++typePos) {
        if (isTok(tokens[typePos], TK_LPAREN)) break;

        if (atBuiltinType(tokens, typePos) || isTok(tokens[typePos], TK_IDENTIFIER)) { isImplicitConversion = true; }

        if (!typeString.empty()) typeString += ' ';
        const auto& val = tokens[typePos].value_;
        typeString.append(val.begin(), val.end());
      }

      // The operator my not have been an implicit conversion
      if (!isImplicitConversion) continue;

      lintWarning(errors,
                  tok,
                  "Implicit conversion to '" + typeString + "' may inadvertently be used.",
                  "Prefix the function with the 'explicit' keyword to avoid this,"
                  " or add an /* implicit *"
                  "/ comment to suppress this warning.");
    }
  }
};
}  // namespace flint
