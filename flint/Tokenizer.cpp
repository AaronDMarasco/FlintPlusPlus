#include "Tokenizer.hpp"

#include <numeric>
#include <unordered_map>

using namespace std;

// Tell map and unordered_map how to store StringFragments
namespace std {
template<>
struct hash<flint::StringFragment> {
  using argument_type = flint::StringFragment;
  using value_type    = uint64_t;

  inline auto operator()(const argument_type& fragment) const -> value_type {
    return accumulate(fragment.begin(), fragment.end(), uint64_t(5381), [](uint64_t curr, char next) {
      return ((curr << 5) + curr) + next;
    });
  };
};
};  // namespace std

namespace flint {

namespace {  // Anonymous Namespace for Tokenizing and munching functions
using str_iter = string::const_iterator;

static auto initializeKeywords() -> unordered_map<StringFragment, TokenType> {
  static unordered_map<string, TokenType>  root;  // Will own all the strings; everybody else has StringFragments
  unordered_map<StringFragment, TokenType> result;

#define CPPLINT_ASSIGN(s, tk) root[string{s}] = tk;
  CPPLINT_FORALL_KEYWORDS(CPPLINT_ASSIGN)
#undef CPPLINT_ASSIGN

  for (const auto& item: root) {
    auto& key                                      = item.first;
    result[StringFragment{key.begin(), key.end()}] = item.second;
  }

  return result;
};

// Oh good lord... Keep this for now,
// then code review and find a cleaner method
#define FBEXCEPTION(e) \
  do { throw runtime_error(string((e))); } while (0)

#define ENFORCE(e, m) \
  if (e) {            \
  } else {            \
    FBEXCEPTION(m);   \
  }

/**
 * Map containing mappings of the kind "virtual" -> TK_VIRTUAL.
 */
static unordered_map<StringFragment, TokenType> keywords = initializeKeywords();

/**
 * Eats howMany characters out of pc, advances pc appropriately, and
 * returns the eaten portion.
 */
static auto munchChars(str_iter& pc, size_t howMany) -> StringFragment {
  // assert(pc.size() >= howMany);
  assert(howMany > 0);
  auto result = StringFragment{pc, pc + howMany};
  advance(pc, howMany);
  return result;
};

/**
 * Assuming pc is positioned at the start of an identifier, munches it
 * from pc and returns it.
 */
static auto munchIdentifier(str_iter& pc, str_iter inputEnd) -> StringFragment {
  const size_t size = distance(pc, inputEnd);
  for (size_t i = 0; i < size; ++i) {
    assert(i < size);
    const char c = pc[i];
    // g++ allows '$' in identifiers. Also, some crazy inline
    // assembler uses '@' in identifiers, see e.g.
    // fbcode/external/cryptopp/rijndael.cpp, line 527
    if (!isalnum(c) && c != '_' && c != '$' && c != '@') {
      // done
      ENFORCE(i > 0, "Invalid identifier: " + string(&*pc));
      return munchChars(pc, i);
    }
  }
  return munchChars(pc, size);
};

/**
 * Assuming pc is positioned at the start of a C-style comment,
 * munches it from pc and returns it.
 */
static auto munchComment(str_iter& pc, size_t& line) -> StringFragment {
  assert(pc[0] == '/' && pc[1] == '*');
  for (size_t i = 2;; ++i) {
    // assert(i < pc.size());
    auto c = pc[i];
    if (c == '\n') {
      ++line;
    } else if (c == '*') {
      if (pc[i + 1] == '/') {
        // end of comment
        return munchChars(pc, i + 2);
      }
    } else if (!c) {
      // end of input
      FBEXCEPTION("Unterminated comment: " + string(&*pc));
    }
  }
  assert(false);
};

/**
 * Assuming pc is positioned at the start of a single-line comment,
 * munches it from pc and returns it.
 */
static auto munchSingleLineComment(str_iter& pc, str_iter inputEnd, size_t& line) -> StringFragment {
  assert(pc[0] == '/' && pc[1] == '/');

  size_t size = distance(pc, inputEnd);
  for (size_t i = 2; i < size; ++i) {
    // assert(i < pc.size());
    auto c = pc[i];
    if (c == '\n') {
      ++line;
      if (i > 0 && pc[i - 1] == '\\') {
        // multiline single-line comment (sic)
        // TODO: This should probably be a warning
        continue;
      }
      // end of comment
      return munchChars(pc, i + 1);
    }
  }

  return munchChars(pc, size);
};

/**
 * Assuming pc is positioned at the start of a number (be it decimal
 * or floating-point), munches it off pc and returns it. Note that the
 * number is assumed to be correct so a number of checks are not
 * necessary.
 */
static auto munchNumber(str_iter& pc) -> StringFragment {
  bool sawDot = false, sawExp = false, sawX = false, sawSuffix = false;
  for (size_t i = 0;; ++i) {
    // assert(i < pc.size());
    const auto c = pc[i];
    if (c == '.' && !sawDot && !sawExp && !sawSuffix) {
      sawDot = true;
    } else if (isdigit(c)) {
      // Nothing to do
    } else if (c == '\'' && !sawX && isdigit(pc[i + 1])) {
      // Nothing to do (numeric separator)
    } else if (c == '\'' && sawX && pc[i + 1] && strchr("1234567890AaBbCcDdEeFf", pc[i + 1])) {
      // Nothing to do (hex numeric separator)
    } else if (sawX && !sawExp && c && strchr("AaBbCcDdEeFf", c)) {
      // Hex digit; nothing to do. The condition includes !sawExp
      // because the exponent is decimal even in a hex floating-point
      // number!
    } else if (c == '+' || c == '-') {
      // Sign may appear at the start or right after E or P
      if (i > 0 && !strchr("EePp", pc[i - 1]))
        // Done, the sign is the next token
        return munchChars(pc, i);
    } else if (!sawX && !sawExp && !sawSuffix && (c == 'e' || c == 'E')) {
      sawExp = true;
    } else if (sawX && !sawExp && !sawSuffix && (c == 'p' || c == 'P')) {
      sawExp = true;
    } else if ((c == 'x' || c == 'X') && i == 1 && pc[0] == '0') {
      sawX = true;
    } else if (c && strchr("FfLlUu", c)) {
      // It's a suffix. There could be several of them (including
      // repeats a la LL), so let's not return just yet
      sawSuffix = true;
    } else {
      // done
      ENFORCE(i > 0, "Invalid number: " + string(&*pc));
      return munchChars(pc, i);
    }
  }
  assert(false);
};

/**
 * Assuming pc is positioned at the start of a character literal,
 * munches it from pc and returns it. A reference to line is passed in
 * order to track multiline character literals (yeah, that can
 * actually happen) correctly.
 */
static auto munchCharLiteral(str_iter& pc, size_t& line) -> StringFragment {
  assert(pc[0] == '\'');
  for (size_t i = 1;; ++i) {
    const auto c = pc[i];
    if (c == '\'') {
      // That's about it
      return munchChars(pc, i + 1);
    }
    if (c == '\\') {
      ++i;
      if (pc[i] == '\n') { ++line; }
      continue;
    }
    ENFORCE(c, "Unterminated character constant: " + string(&*pc));
  }
};

/**
 * Assuming pc is positioned at the start of a string literal, munches
 * it from pc and returns it. A reference to line is passed in order
 * to track multiline strings correctly.
 */
static auto munchString(str_iter& pc, size_t& line, bool isIncludeLiteral = false) -> StringFragment {
  const char stringEnd = isIncludeLiteral ? '>' : '"';
  assert(pc[0] == (isIncludeLiteral ? '<' : '"'));

  for (size_t i = 1;; ++i) {
    const auto c = pc[i];
    if (c == stringEnd)
      // That's about it
      return munchChars(pc, i + 1);
    if (c == '\\') {
      ++i;
      if (pc[i] == '\n') ++line;
      continue;
    }
    ENFORCE(c, "Unterminated string constant: " + string(&*pc));
  }
};

/**
 * Assuming pc is positioned at the start of a raw string literal, munches
 * it from pc and returns it. A reference to line is passed in order
 * to track multiline strings correctly.
 */
static auto munchRawString(str_iter& pc, size_t& line) -> StringFragment {
  assert(pc[0] == 'R');
  assert(pc[1] == '"');
  std::string    delim{')'};
  const str_iter start{pc};
  pc++;  // Jump to "
  // Capture the optional delimeter up to the first open '('
  while (*(++pc) != '(') delim += *pc;
  delim += '"';  // The string will always end with this
  // Now we are at the beginning of the "real" string.
  // We will ignore EVERYTHING that is not the delimeter (or a newline)
  size_t match_index = 0;  // Very lame character-by-character state machine
  for (size_t i = 0;; ++i) {
    const auto c = pc[i];
    if (c == delim[match_index]) {
      if (++match_index == delim.length()) {            // Done!
        pc = start;                                     // Restore original
        return munchChars(pc, i + 2 + delim.length());  // +2 for the leading R"
      }
      continue;  // continue attempting to match delim
    } else {
      i -= match_index;  // Rollback to where our attempted match started (then for loop will skip the first)
      match_index = 0;
    }
    // Check if newline to fix line count
    if (c == '\n') ++line;
    ENFORCE(c, "Unterminated raw literal: " + string(&*pc));
  }

  FBEXCEPTION("Raw String unknown error with delimiter=" + delim);
};

/**
 * Munches horizontal spaces from pc. If we want to disallow tabs in
 * sources, here is the place. No need for end-of-input checks as the
 * input always has a '\0' at the end.
 */
static auto munchSpaces(str_iter& pc) -> StringFragment {
  size_t i;
  for (i = 0; pc[i] == ' ' || pc[i] == '\t'; ++i) {}

  const auto result = StringFragment{pc, pc + i};
  advance(pc, i);
  return result;
};

};  // Anonymous Namespace

/**
 * Given the contents of a C++ file and a filename, tokenizes the
 * contents and places it in output.
 */
auto tokenize(const string&   input,
              const string&   file,
              vector<Token>&  output,
              vector<size_t>& structures,
              ErrorFile&      errors) -> size_t {
  output.clear();
  structures.clear();

  static const string         eof{"\0"};
  static const string         empty{};
  static const StringFragment nothing{begin(empty), end(empty)};

  auto   pc   = input.begin();
  size_t line = 1;
  if (startsWith(pc, "\xEF\xBB\xBF")) {  // UTF-8 BOM
    pc += 3;
    errors.addError(ErrorObject(Lint::WARNING,
                                line,
                                "UTF-8 BOM found",
                                "The Unicode Standard permits this, but does not require nor recommend its use"));
  }

  size_t         tokenLen{0};
  StringFragment whitespace = nothing;

  while (pc != input.end()) {
    const char c = pc[0];
    TokenType  t{TK_UNEXPECTED};

    if (output.size() > 0) {
      const auto tok = output.back().type_;
      if ((tok == TK_CLASS || tok == TK_STRUCT || tok == TK_UNION) &&
          (structures.empty() || structures.back() != output.size() - 1)) {
        // If the last token added was the start of a structure, push it onto
        // the list of structures
        structures.push_back(output.size() - 1);
      } else if (c == '<' && tok == TK_INCLUDE) {
        // Special case for parsing #include <...>
        // Previously the include path would not be captured as a string literal
        const auto str = munchString(pc, line, true);
        output.emplace_back(TK_STRING_LITERAL, move(str), line, whitespace);
        whitespace = nothing;
        continue;
      }
    }

    switch (c) {
      // *** One-character tokens that don't require lookahead (comma,
      // *** semicolon etc.)
#define CPPLINT_INTRODUCE_CASE(c0, t0) \
  case (c0):                           \
    t        = (t0);                   \
    tokenLen = 1;                      \
    goto INSERT_TOKEN;
      CPPLINT_FORALL_ONE_CHAR_TOKENS(CPPLINT_INTRODUCE_CASE);
#undef CPPLINT_INTRODUCE_CASE
      // *** One- or two-character tokens
#define CPPLINT_INTRODUCE_CASE(c1, t1, c2, t2) \
  case c1:                                     \
    if (pc[1] == (c2)) {                       \
      t        = (t2);                         \
      tokenLen = 2;                            \
    } else {                                   \
      t        = (t1);                         \
      tokenLen = 1;                            \
    }                                          \
    goto INSERT_TOKEN;
      CPPLINT_FORALL_ONE_OR_TWO_CHAR_TOKENS(CPPLINT_INTRODUCE_CASE);
#undef CPPLINT_INTRODUCE_CASE
#define CPPLINT_INTRODUCE_CASE(c1, t1, c2, t2, c3, t3) \
  case c1:                                             \
    if (pc[1] == (c2)) {                               \
      t        = (t2);                                 \
      tokenLen = 2;                                    \
    } else if (pc[1] == (c3)) {                        \
      t        = (t3);                                 \
      tokenLen = 2;                                    \
    } else {                                           \
      t        = (t1);                                 \
      tokenLen = 1;                                    \
    }                                                  \
    goto INSERT_TOKEN;
      CPPLINT_FORALL_ONE_OR_TWO_CHAR_TOKENS2(CPPLINT_INTRODUCE_CASE);
#undef CPPLINT_INTRODUCE_CASE
#define CPPLINT_INTRODUCE_CASE(c1, t1, c2, t2, c3, t3, c4, t4) \
  case c1:                                                     \
    if (pc[1] == (c2)) {                                       \
      t        = (t2);                                         \
      tokenLen = 2;                                            \
    } else if (pc[1] == (c3)) {                                \
      if (pc[2] == (c4)) {                                     \
        t        = (t4);                                       \
        tokenLen = 3;                                          \
      } else {                                                 \
        t        = (t3);                                       \
        tokenLen = 2;                                          \
      }                                                        \
    } else {                                                   \
      t        = (t1);                                         \
      tokenLen = 1;                                            \
    }                                                          \
    goto INSERT_TOKEN;
      CPPLINT_FORALL_ONE_TO_THREE_CHAR_TOKENS(CPPLINT_INTRODUCE_CASE);
#undef CPPLINT_INTRODUCE_CASE
        // *** Everything starting with a slash: /, /=, single- and
        // *** multi-line comments
      case '/':
        if (pc[1] == '*') {
          const auto& comment = munchComment(pc, line);
          whitespace.append(comment.begin(), comment.end());
          break;
        }
        if (pc[1] == '/') {
          const auto& single = munchSingleLineComment(pc, input.end(), line);
          whitespace.append(single.begin(), single.end());
          break;
        }
        if (pc[1] == '=') {
          t        = TK_DIVIDE_ASSIGN;
          tokenLen = 2;
        } else {
          t        = TK_DIVIDE;
          tokenLen = 1;
        }
        goto INSERT_TOKEN;
        // *** Backslash
      case '\\':
        // Consume trailing whitespace after a valid backslash
        {
          const auto& spaces = munchSpaces(pc);
          whitespace.append(spaces.begin(), spaces.end());
        }
        // Take the case into account where a comment comes after a macro backslash
        ENFORCE(pc[1] == '\n' || pc[1] == '\r' || (pc[1] == '/' && (pc[2] == '/' || pc[2] == '*')),
                "Misplaced backslash in " + file + ":" + std::to_string(line));
        ++line;
        whitespace.append(pc, pc + 2);
        advance(pc, 2);
        break;
        // *** Newline
      case '\n':
        whitespace.append(pc, pc + 1);
        pc++;
        ++line;
        break;
        // *** Part of a DOS newline; ignored
      case '\r':
        whitespace.append(pc, pc + 1);
        pc++;
        break;
        // *** ->, --, -=, ->*, and -
      case '-':
        if (pc[1] == '-') {
          t        = TK_DECREMENT;
          tokenLen = 2;
          goto INSERT_TOKEN;
        }
        if (pc[1] == '=') {
          t        = TK_MINUS_ASSIGN;
          tokenLen = 2;
          goto INSERT_TOKEN;
        }
        if (pc[1] == '>') {
          if (pc[2] == '*') {
            t        = TK_ARROW_STAR;
            tokenLen = 3;
          } else {
            t        = TK_ARROW;
            tokenLen = 2;
          }
          goto INSERT_TOKEN;
        }
        t        = TK_MINUS;
        tokenLen = 1;
        goto INSERT_TOKEN;
        // *** Whitespace
      case ' ':
      case '\t': {
        const auto& spaces = munchSpaces(pc);
        whitespace.append(spaces.begin(), spaces.end());
      } break;
        // *** Done parsing!
      case '\0':
        // assert(pc.size() == 0);
        // Push last token, the EOF
        output.emplace_back(TK_EOF, StringFragment{eof.begin(), eof.end()}, line, whitespace);
        return line;
        // *** Verboten characters (do allow '@' and '$' as extensions)
      case '`':
        errors.addError(ErrorObject(Lint::ERROR, line, "Invalid character found: Back-tick `", ""));
        output.emplace_back(TK_UNEXPECTED, StringFragment{pc, pc + 1}, line, whitespace);
        ++pc;
        // cerr << ("Invalid character: " + string(1, c) + " in " + string(file + ":" +
        // to_string(line))) << endl;
        break;
        // *** Number, member selector, ellipsis, or .*
      case '.':
        if (isdigit(pc[1])) { goto ITS_A_NUMBER; }
        if (pc[1] == '*') {
          t        = TK_DOT_STAR;
          tokenLen = 2;
        } else if (pc[1] == '.' && pc[2] == '.') {
          t        = TK_ELLIPSIS;
          tokenLen = 3;
        } else {
          t        = TK_DOT;
          tokenLen = 1;
        }
        goto INSERT_TOKEN;
        // *** Numbers
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      ITS_A_NUMBER: {
        auto symbol = munchNumber(pc);
        assert(symbol.size() > 0);
        output.emplace_back(TK_NUMBER, move(symbol), line, whitespace);
        whitespace = nothing;
      } break;
        // *** C++11 Raw String Literal
      case 'R':
        if (pc[1] == '"') {
          auto str{munchRawString(pc, line)};
          output.emplace_back(TK_RAW_STRING_LITERAL, move(str), line, whitespace);
          whitespace = nothing;
          break;
        }
        goto INSERT_TOKEN;
        // *** Character literal
      case '\'': {
        auto charLit = munchCharLiteral(pc, line);
        output.emplace_back(TK_CHAR_LITERAL, move(charLit), line, whitespace);
        whitespace = nothing;
      } break;
        // *** String literal
      case '"': {
        auto str = munchString(pc, line);
        output.emplace_back(TK_STRING_LITERAL, move(str), line, whitespace);
        whitespace = nothing;
      } break;
      case '#': {
        // Skip leading ws
        auto pc1 = pc + 1;
        tokenLen = 1 + munchSpaces(pc1).size();
        // The entire #line line is the token value
        if (startsWith(pc1, "line")) {
          t = TK_HASHLINE;
          tokenLen += distance(pc1, find(pc1, input.end(), '\n'));
        } else if (startsWith(pc1, "error")) {
          // The entire #error line is the token value
          t = TK_ERROR;
          tokenLen += distance(pc1, find(pc1, input.end(), '\n'));
          ENFORCE(tokenLen > 0, "Unterminated #error message");
        } else if (startsWith(pc1, "include")) {
          t = TK_INCLUDE;
          tokenLen += 7;  // strlen("include");
        } else if (startsWith(pc1, "ifdef")) {
          t = TK_IFDEF;
          tokenLen += 5;  // strlen("ifdef");
        } else if (startsWith(pc1, "ifndef")) {
          t = TK_IFNDEF;
          tokenLen += 6;  // strlen("ifndef");
        } else if (startsWith(pc1, "if")) {
          t = TK_POUNDIF;
          tokenLen += 2;  // strlen("if");
        } else if (startsWith(pc1, "undef")) {
          t = TK_UNDEF;
          tokenLen += 5;  // strlen("undef");
        } else if (startsWith(pc1, "elif")) {
          t = TK_POUNDELIF;
          tokenLen += 4;  // strlen("elif");
        } else if (startsWith(pc1, "else")) {
          t = TK_POUNDELSE;
          tokenLen += 4;  // strlen("else");
        } else if (startsWith(pc1, "endif")) {
          t = TK_ENDIF;
          tokenLen += 5;  // strlen("endif");
        } else if (startsWith(pc1, "define")) {
          t = TK_DEFINE;
          tokenLen += 6;  // strlen("define");
        } else if (startsWith(pc1, "pragma")) {
          t = TK_PRAGMA;
          tokenLen += 6;  // strlen("pragma");
        } else if (startsWith(pc1, "#")) {
          t = TK_DOUBLEPOUND;
          tokenLen += 2;                     // strlen("##");
        } else if (startsWith(pc1, "/*")) {  // Empty preprocessor directive but multi-line comment
          t = TK_POUND;
          tokenLen += munchComment(pc1, line).size();
        } else if (startsWith(pc1, "//")) {  // Empty preprocessor directive but single line comment
          t = TK_POUND;
          tokenLen += munchSingleLineComment(pc1, input.end(), line).size();
        } else {
          // We can only assume this is inside a macro definition
          t = TK_POUND;
          tokenLen += 1;
        }
      }
        goto INSERT_TOKEN;
        // *** Everything else
      default:
        if (iscntrl(c)) {
          whitespace.append(pc, pc + 1);
          pc++;
        } else if (isalpha(c) || c == '_' || c == '$' || c == '@') {
          // it's a word
          auto symbol = munchIdentifier(pc, input.cend());
          auto iter   = keywords.find(symbol);
          if (iter != keywords.end()) {
            // keyword
            output.emplace_back(iter->second, move(symbol), line, whitespace);
            whitespace = nothing;
          } else {
            // Some identifier
            assert(symbol.size() > 0);
            output.emplace_back(TK_IDENTIFIER, move(symbol), line, whitespace);
            whitespace = nothing;
          }
        } else {
          // what could this be? (BOM?)
          FBEXCEPTION("Unrecognized character in " + file + ":" + std::to_string(line));
        }
        break;
        // *** All
      INSERT_TOKEN:
        output.emplace_back(t, munchChars(pc, tokenLen), line, whitespace);
        t = TK_UNEXPECTED;
        whitespace = nothing;
        break;
    }
  }

  output.emplace_back(TK_EOF, StringFragment{eof.begin(), eof.end()}, line, nothing);

  return line;
};

/**
 * Converts e.g. TK_VIRTUAL to "TK_VIRTUAL".
 */
auto toString(const TokenType t) -> string {
// Build up macros that take 2, 4, 6, or 8 inputs
#define CPPLINT_X1(c1, t1) \
  if ((t1) == t) return (#t1);
#define CPPLINT_X2(c1, t1, c2, t2)         CPPLINT_X1(c1, t1) CPPLINT_X1(c2, t2)
#define CPPLINT_X3(c1, t1, c2, t2, c3, t3) CPPLINT_X1(c1, t1) CPPLINT_X2(c2, t2, c3, t3)

#define CPPLINT_X4(c1, t1, c2, t2, c3, t3, c4, t4) CPPLINT_X2(c1, t1, c2, t2) CPPLINT_X2(c3, t3, c4, t4)

  // Expansion
  CPPLINT_FOR_ALL_TOKENS(CPPLINT_X1, CPPLINT_X2, CPPLINT_X3, CPPLINT_X4)
#undef CPPLINT_X1
#undef CPPLINT_X2
#undef CPPLINT_X3
#undef CPPLINT_X4
  FBEXCEPTION("Unknown token type given to toString()");
};

};  // namespace flint
