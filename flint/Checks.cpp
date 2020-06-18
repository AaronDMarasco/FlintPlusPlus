#include "Checks.hpp"

using namespace std;

namespace flint {

auto formatArg(const vector<Token>& tokens, const Argument& arg) -> string {
  string result;

  for (size_t pos = arg.first; pos < arg.last; ++pos) {
    if (pos != arg.first && !(tokens[pos].precedingWhitespace_.empty())) result += ' ';

    const auto& val = tokens[pos].value_;
    result.append(val.begin(), val.end());
  }
  return result;
};

auto formatFunction(const vector<Token>& tokens, const Argument& func, const vector<Argument>& args) -> string {
  static const string sep{", "};

  string result = formatArg(tokens, func) + '(';

  if (!args.empty()) result += formatArg(tokens, args[0]);

  for (size_t i = 1, size = args.size(); i < size; ++i) result += sep + formatArg(tokens, args[i]);

  result += ')';
  return result;
};

auto getFunctionNameAndArguments(const vector<Token>& tokens, size_t& pos, Argument& func, vector<Argument>& args)
    -> bool {
  func.first = pos;
  ++pos;

  const size_t size = tokens.size();
  if (pos < size && isTok(tokens[pos], TK_LESS)) {
    pos = skipTemplateSpec(tokens, pos);

    if (pos >= size || isTok(tokens[pos], TK_EOF)) return false;
    ++pos;
  }
  func.last = pos;
  return getRealArguments(tokens, pos, args);
};

auto getRealArguments(const vector<Token>& tokens, size_t& pos, vector<Argument>& args) -> bool {
  assert(isTok(tokens[pos], TK_LPAREN));

  ++pos;
  size_t argStart   = pos;  // First arg starts after parenthesis
  int    parenCount = 1;

  const size_t size = tokens.size();
  for (; pos < size && !isTok(tokens[pos], TK_EOF); ++pos) {
    const auto tok = tokens[pos].type_;

    if (tok == TK_LPAREN) {
      ++parenCount;
      continue;
    }
    if (tok == TK_RPAREN) {
      if (--parenCount == 0) break;
      continue;
    }
    /*
    if (tok == TK_LESS) {
            // This is a heuristic which would fail when < is used with
            // the traditional meaning in an argument, e.g.
            //  memset(&foo, a < b ? c : d, sizeof(foo));
            // but currently we have no way to distinguish that use of
            // '<' and
            //  memset(&foo, something<A,B>(a), sizeof(foo));
            // We include this heuristic in the hope that the second
            // use of '<' is more common than the first.
            pos = skipTemplateSpec(tokens, pos);
            continue;
    }
    */
    if (tok == TK_COMMA) {
      if (parenCount == 1) {
        // end an argument of the function we are looking at
        args.emplace_back(argStart, pos);
        argStart = pos + 1;
      }
      continue;
    }
  }

  if (pos >= size || isTok(tokens[pos], TK_EOF)) return false;

  if (argStart != pos) args.emplace_back(argStart, pos);
  return true;
};

auto getIncludedPath(const StringFragment& path) -> string {
  assert(*path.begin() == '<' or *path.begin() == '"');
  assert(path.back() == '>' or path.back() == '"');
  return string(path.begin() + 1, path.end() - 1);
};

auto matchAcrossTokens(const StringFragment& frag, TokenIter start, TokenIter end_iter) -> bool {
  auto f_pos       = frag.begin();
  auto f_end       = frag.end();
  auto f_token_pos = start->value_.begin();
  auto f_curr_end  = start->value_.end();

  while (f_pos != f_end && start != end_iter && *f_pos == *f_token_pos) {
    f_pos++;
    f_token_pos++;

    if (f_token_pos == f_curr_end) start++;
  }

  return (f_pos == f_end);
};

auto readQualifiedIdentifier(const vector<Token>& tokens, size_t& pos) -> vector<StringFragment> {
  vector<StringFragment> ret;
  for (; isTok(tokens[pos], TK_IDENTIFIER) || isTok(tokens[pos], TK_DOUBLE_COLON); ++pos)
    if (isTok(tokens[pos], TK_IDENTIFIER)) ret.push_back(tokens[pos].value_);

  return ret;
};

auto skipBlock(const vector<Token>& tokens, size_t pos) -> size_t {
  assert(isTok(tokens[pos], TK_LCURL));

  size_t openBraces = 1;  // Because we began on the leading '{'

  ++pos;
  for (const size_t size = tokens.size(); pos < size && !isTok(tokens[pos], TK_EOF); ++pos) {
    const Token& tok = tokens[pos];

    if (isTok(tok, TK_LCURL)) {
      ++openBraces;
      continue;
    }
    if (isTok(tok, TK_RCURL)) {
      if (--openBraces == 0) break;
      continue;
    }
  }

  return pos;
};

auto skipFunctionDeclaration(const vector<Token>& tokens, size_t pos) -> size_t {
  for (const size_t size = tokens.size(); pos < size && !isTok(tokens[pos], TK_EOF); ++pos) {
    TokenType tok = tokens[pos].type_;

    if (tok == TK_SEMICOLON) {  // Function Prototype
      break;
    } else if (tok == TK_LCURL) {  // Full Declaration
      pos = skipBlock(tokens, pos);
      break;
    }
  }

  return pos;
};

auto skipToToken(const vector<Token>& tokens, size_t& pos, TokenType target) -> bool {
  const size_t size = tokens.size();
  for (; pos < size && !isTok(tokens[pos], target); ++pos) {}
  return (pos < size && isTok(tokens[pos], target));
};

auto skipTemplateSpec(const vector<Token>& tokens, size_t pos, bool* containsArray /* = nullptr */) -> size_t {
  assert(isTok(tokens[pos], TK_LESS));

  size_t angleNest = 1;  // Because we began on the leading '<'
  // Previous versions counted parens but commented out all usage

  if (containsArray != nullptr) *containsArray = false;

  ++pos;
  for (const size_t size = tokens.size(); pos < size && !isTok(tokens[pos], TK_EOF); ++pos) {
    const auto tok = tokens[pos].type_;

    if (tok == TK_LPAREN) continue;
    if (tok == TK_RPAREN) continue;

    if (tok == TK_LSQUARE) {
      if (angleNest == 1 && containsArray != nullptr) *containsArray = true;
      continue;
    }

    if (tok == TK_LESS) {
      ++angleNest;
      continue;
    }

    if (tok == TK_GREATER) {
      if (--angleNest == 0) return pos;
      continue;
    }
  }

  return pos;
};

#if 0
// ******************************************
// Deprecated due to too many false positives
// ******************************************
/**
 * Check for postfix incrementers
 *
 * @param errors
 *        Struct to track how many errors/warnings/advice occured
 * @param path
 *        The path to the file currently being linted
 * @param tokens
 *        The token list for the file
 */
void checkIncrementers(ErrorFile& errors, const string& path, const vector<Token>& tokens) {
  const vector<TokenType> iteratorPlus  = {TK_IDENTIFIER, TK_INCREMENT};
  const vector<TokenType> iteratorMinus = {TK_IDENTIFIER, TK_DECREMENT};

  for (size_t pos = 0; pos < tokens.size(); ++pos)
    if (atSequence(tokens, pos, iteratorPlus) || atSequence(tokens, pos, iteratorMinus))
        lintAdvice(errors,
                   tokens[pos],
                   "Use prefix notation '" + tokens[pos + 1].value_ + tokens[pos].value_ + "'.",
                   "Postfix incrementers inject a copy operation, almost doubling the workload.");
};
#endif

};  // namespace flint
