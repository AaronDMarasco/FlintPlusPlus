#include "Checks.hpp"

namespace flint {

/**
 * Warn about common errors with constructors, such as:
 *  - single-argument constructors that aren't marked as explicit, to avoid them
 *    being used for implicit type conversion (C++ only)
 *  - Non-const copy constructors, or useless const move constructors.
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkConstructors(ErrorFile&            errors,
                       const string&         path,
                       const vector<Token>&  tokens,
                       const vector<size_t>& structures) {
  if (getFileCategory(path) == FileCategory::SOURCE_C) return;

  static const string lintOverride{"/* implicit */"};

  static constexpr array<TokenType, 4> stdInitializerSequence{TK_IDENTIFIER, TK_DOUBLE_COLON, TK_IDENTIFIER, TK_LESS};
  static constexpr array<TokenType, 2> constructorSequence{TK_IDENTIFIER, TK_LPAREN};
  static constexpr array<TokenType, 4> voidConstructorSequence{TK_IDENTIFIER, TK_LPAREN, TK_VOID, TK_RPAREN};

  // Check for constructor specifications inside classes
  const size_t toksize = tokens.size();
  for (auto pos: structures) {
    if (!(isTok(tokens[pos], TK_STRUCT) || isTok(tokens[pos], TK_CLASS))) continue;

    ++pos;
    // Skip C-Style Structs with no name
    if (!isTok(tokens[pos], TK_IDENTIFIER)) continue;

    // Get the name of the object
    const auto& objName = tokens[pos].value_;

    // Skip to opening '{'
    for (; pos < toksize && !(isTok(tokens[pos], TK_LCURL) || isTok(tokens[pos], TK_SEMICOLON)); ++pos)
      ;
    if (isTok(tokens[pos], TK_SEMICOLON)) continue;
    ++pos;

    for (; pos < toksize && !isTok(tokens[pos], TK_EOF); ++pos) {
      const Token& tok = tokens[pos];

      // Any time we find an open curly skip straight to the closing one
      if (isTok(tok, TK_LCURL)) {
        pos = skipBlock(tokens, pos);
        continue;
      }

      // If we actually find a closing one we know it's the object's closing bracket
      if (isTok(tok, TK_RCURL)) break;

      if (isTok(tok, TK_EXPLICIT)) {
        pos = skipFunctionDeclaration(tokens, pos);
        continue;
      }

      // Are we on a potential constructor?
      if (atSequence(tokens, pos, constructorSequence) && cmpTok(tok, objName)) {
        // Ignore constructors like Foo(void) ...
        if (atSequence(tokens, pos, voidConstructorSequence)) {
          pos = skipFunctionDeclaration(tokens, pos);
          continue;
        }

        // Check for preceding /* implicit */
        if (contains(tok.precedingWhitespace_, lintOverride.cbegin(), lintOverride.cend())) {
          pos = skipFunctionDeclaration(tokens, pos);
          continue;
        }

        vector<Argument> args;
        Argument         func(pos, pos + 1);
        if (!getFunctionNameAndArguments(tokens, pos, func, args))
          // Parse fail can be due to limitations in skipTemplateSpec, such as with:
          // fn(std::vector<boost::shared_ptr<ProjectionOperator>> children);)
          break;

        // Allow zero-argument constructors
        if (args.empty()) {
          pos = skipFunctionDeclaration(tokens, pos);
          continue;
        }

        size_t argPos              = args[0].first;
        bool   foundConversionCtor = false;
        bool   isConstArgument     = false;
        if (isTok(tokens[argPos], TK_CONST)) {
          isConstArgument = true;
          ++argPos;
        }

        // Copy/move constructors may have const (but not type conversion) issues
        // Note: we skip some complicated cases (e.g. template arguments) here
        if (cmpTok(tokens[argPos], objName)) {
          TokenType nextType = (argPos + 1 != args[0].last) ? tokens[argPos + 1].type_ : TK_EOF;
          if (nextType != TK_STAR) {
            if (nextType == TK_AMPERSAND && !isConstArgument) {
              lintError(
                  errors, tok, "Copy constructors should take a const argument: " + formatFunction(tokens, func, args));
            } else if (nextType == TK_LOGICAL_AND && isConstArgument) {
              lintError(errors,
                        tok,
                        "Move constructors should not take a const argument: " + formatFunction(tokens, func, args));
            }

            pos = skipFunctionDeclaration(tokens, pos);
            continue;
          }
        }

        // Allow std::initializer_list constructors
        if (atSequence(tokens, argPos, stdInitializerSequence) && cmpTok(tokens[argPos], "std") &&
            cmpTok(tokens[argPos + 2], "initializer_list")) {
          pos = skipFunctionDeclaration(tokens, pos);
          continue;
        }

        if (args.size() == 1) {
          foundConversionCtor = true;
        } else if (args.size() >= 2) {
          // 2+ will only be an issue if the second argument is a default argument
          for (argPos = args[1].first; argPos != args[1].last; ++argPos)
            if (isTok(tokens[argPos], TK_ASSIGN)) {
              foundConversionCtor = true;
              break;
            }
        }

        if (foundConversionCtor)
          lintError(errors,
                    tok,
                    "Single - argument constructor '" + formatFunction(tokens, func, args) +
                        "' may inadvertently be used as a type conversion constructor.",
                    "Prefix the function with the 'explicit' keyword to avoid this, or add an "
                    "/* implicit */ comment to suppress this warning.");

        pos = skipFunctionDeclaration(tokens, pos++);
      }
    }
  }
};
}  // namespace flint
