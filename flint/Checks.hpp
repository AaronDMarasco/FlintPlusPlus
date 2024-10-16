#pragma once

// There are a lot included here because this header is common across ALL checks
#include <cassert>
#include <iterator>
#include <numeric>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Checks.hpp"
#include "ErrorReport.hpp"
#include "FileCategories.hpp"
#include "Tokenizer.hpp"

namespace flint {

const std::string emptyString;
struct Argument;  // Defined below
using TokenIter = std::vector<Token>::const_iterator;

// ******************************* These are the helper functions for the checkers
/*
 * Errors vs. Warnings vs. Advice:
 *
 *   Please select errors vs. warnings intelligently.  Too much spam
 *   on lines you don't touch reduces the value of lint output.
 *
 */

// C++17 these should be tagged [[maybe_unused]]; for now GCC and clang both agree on this...
void __attribute__((unused)) inline lintError(ErrorFile&         errors,
                                              const Token&       tok,
                                              const std::string& title,
                                              const std::string& desc = emptyString) {
  errors.addError(ErrorObject(Lint::ERROR, tok.line_, title, desc));
};
void __attribute__((unused)) inline lintWarning(ErrorFile&         errors,
                                                const Token&       tok,
                                                const std::string& title,
                                                const std::string& desc = emptyString) {
  errors.addError(ErrorObject(Lint::WARNING, tok.line_, title, desc));
};
void __attribute__((unused)) inline lintAdvice(ErrorFile&         errors,
                                               const Token&       tok,
                                               const std::string& title,
                                               const std::string& desc = emptyString) {
  errors.addError(ErrorObject(Lint::ADVICE, tok.line_, title, desc));
};

void inline lint(ErrorFile&         errors,
                 const Token&       tok,
                 const Lint         level,
                 const std::string& title,
                 const std::string& desc = emptyString) {
  errors.addError(ErrorObject(level, tok.line_, title, desc));
};

// Shorthand for comparing two strings (or fragments)
template<class S, class T>
inline auto cmpStr(const S& a, const T& b) -> bool {
  auto const len_a = distance(a.begin(), a.end());
  auto const len_b = distance(b.begin(), b.end());
  if (len_a < len_b)
    return equal(a.begin(), a.end(), b.begin());
  return equal(b.begin(), b.end(), a.begin());
}
inline auto cmpStr(const StringFragment& a, const StringFragment& b) -> bool { return (a == b); }
// This version fails on OSX because it's SPECIAL:
/* inline bool cmpStr(const StringFragment& a, const char* b) { return (a == StringFragment{b}); }
 */
inline auto cmpStr(const StringFragment& a, const char* b) -> bool { return (to_string(a) == std::string{b}); }
inline auto cmpStr(const std::string& a, const std::string& b) -> bool { return a == b; }
inline auto cmpToks(const Token& a, const Token& b) -> bool { return cmpStr(a.value_, b.value_); };

template<class S, class T>
inline auto cmpTok(const S& a, const T& b) -> bool {
  return cmpStr(a.value_, b);
};

// Shorthand for comparing a Token and TokenType
inline auto isTok(const Token& token, TokenType type) -> bool { return token.type_ == type; }

/**
 * Returns whether the current token is a built-in type
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @return
 *        Returns true is the token at pos is a built-in type
 */
inline auto atBuiltinType(const std::vector<Token>& tokens, size_t pos) -> bool {
  static constexpr std::array<TokenType, 11> builtIns{
      TK_DOUBLE, TK_FLOAT, TK_INT, TK_SHORT, TK_UNSIGNED, TK_LONG, TK_SIGNED, TK_VOID, TK_BOOL, TK_WCHAR_T, TK_CHAR};

  return std::find(std::begin(builtIns), std::end(builtIns), tokens[pos].type_) != std::end(builtIns);
};

/**
 * Returns whether the current token is at the start of a given sequence
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @param list
 *        The token list for the desired sequence
 * @return
 *        Returns true if we were at the start of a given sequence
 */
template<class Container>
inline auto atSequence(const std::vector<Token>& tokens, size_t pos, const Container& list) -> bool {
  return equal(std::begin(list), std::end(list), std::begin(tokens) + pos, [](TokenType type, const Token& token) {
    return type == token.type_;
  });
};

/**
 * Take the bounds of an argument list and pretty print it to a string
 *
 * @param tokens
 *        The token list for the file
 * @param arg
 *        A struct representing the bounds of the argument list tokens
 * @return
 *        Returns a string representation of the argument token list
 */
auto formatArg(const std::vector<Token>& tokens, const Argument& arg) -> std::string;

/**
 * Pretty print a function declaration/prototype to a string
 *
 * @param tokens
 *        The token list for the file
 * @param func
 *        A reference to the name of the function
 * @param args
 *        A list of arguments for the function
 * @return
 *        Returns a string representation of the argument token list
 */
auto formatFunction(const std::vector<Token>& tokens, const Argument& func, const std::vector<Argument>& args)
    -> std::string;

/**
 * No description available at this time!
 */
inline auto getEndOfClass(TokenIter start, TokenIter maxPos) -> TokenIter {
  static constexpr std::array<TokenType, 3> classMarkers{TK_EOF, TK_LCURL, TK_SEMICOLON};

  return std::find_first_of(start, maxPos, std::begin(classMarkers), std::end(classMarkers), isTok);
};

/**
 * Get the argument list of a function, with the first argument being the
 * function name plus the template spec.
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @param func
 *        A reference to the name of the function
 * @param args
 *        A reference to the list to fill with arguments
 * @return
 *        Returns true if we believe (sorta) that everything went okay,
 *        false if something bad happened (maybe)
 */
auto getFunctionNameAndArguments(const std::vector<Token>& tokens,
                                 size_t&                   pos,
                                 Argument&                 func,
                                 std::vector<Argument>&    args) -> bool;

/**
 * Strips the ""'s or <>'s from an #include path
 *
 * @param path
 *        The string fragment to trim
 * @return
 *        Returns the include path without it's wrapping quotes/brackets
 */
auto getIncludedPath(const StringFragment& path) -> std::string;

/**
 * Get the list of arguments of a function, assuming that the current
 * iterator is at the open parenthesis of the function call. After the this
 * method is call, the iterator will be moved to after the end of the function
 * call.
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @param args
 *        A reference to the list to fill with arguments
 * @return
 *        Returns true if we believe (sorta) that everything went okay,
 *        false if something bad happened (maybe)
 */
auto getRealArguments(const std::vector<Token>& tokens, size_t& pos, std::vector<Argument>& args) -> bool;

/**
 * No description available at this time!
 */
auto matchAcrossTokens(const StringFragment& frag, TokenIter start, TokenIter end_iter) -> bool;

/**
 * Heuristically read a potentially namespace-qualified identifier,
 * advancing 'pos' in the process.
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @return
 *        Returns a vector of all the identifier values involved, or an
 *        empty vector if no identifier was detected.
 */
auto readQualifiedIdentifier(const std::vector<Token>& tokens, size_t& pos) -> std::vector<StringFragment>;

/**
 * Traverses the token list until the whole code block has been passed
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @return
 *        Returns the position of the closing curly bracket
 */
auto skipBlock(const std::vector<Token>& tokens, size_t pos) -> size_t;

/**
 * Starting from a function name or one of its arguments, skips the entire
 * function prototype or function declaration (including function body).
 *
 * Implementation is simple: stop at the first semicolon, unless an opening
 * curly brace is found, in which case we stop at the matching closing brace.
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @return
 *        Returns the position of the closing curly bracket or semicolon
 */
auto skipFunctionDeclaration(const std::vector<Token>& tokens, size_t pos) -> size_t;

/**
 * Moves pos to the next position of the target token
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @param target
 *        The token to match
 * @return
 *        Returns true if we are at the given token
 */
auto skipToToken(const std::vector<Token>& tokens, size_t& pos, TokenType target) -> bool;

/**
 * Traverses the token list until the whole template sequence has been passed
 *
 * @param tokens
 *        The token list for the file
 * @param pos
 *        The current index position inside the token list
 * @param containsArray
 *        Optional parameter to return a bool of whether an array was found inside
 *        the template list
 * @return
 *        Returns the position of the closing angle bracket
 */
auto skipTemplateSpec(const std::vector<Token>& tokens, size_t pos, bool* containsArray = nullptr) -> size_t;

// ******************************* End of helper functions for the checkers

// Bring in all the checks from the two directories...

// Most checks get just the tokenized file
#define X(func) void check##func(ErrorFile& errors, const std::string& path, const std::vector<Token>& tokens)

// More advanced checks get access to a list of identified structs/classes/unions
#define X_struct(func)                                \
  void check##func(ErrorFile&                 errors, \
                   const std::string&         path,   \
                   const std::vector<Token>&  tokens, \
                   const std::vector<size_t>& structures)

// Makefile automatically regenerates this when you do "make clean"
#include "Checks.inc"

#undef X_struct
#undef X

/**
 * Represent an argument or the name of a function.
 * first is an iterator that points to the start of the argument.
 * last is an iterator that points to the token right after the end of the
 * argument.
 */
struct Argument {
  size_t first;
  size_t last;

  inline Argument(size_t a, size_t b): first(a), last(b) {
    // Just to check the port hasn't broken Token traversal somehow
    assert(first <= last);
  };
};
}  // namespace flint
