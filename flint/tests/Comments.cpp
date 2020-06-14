// Test a few comment things

#ifdef X // This is valid
#endif

#ifdef Y /* This too */
#endif

// A single quote means it's a contraction or a possessive.
// It's not an unterminated string.
// Neither is "this.

// Apparently, you can have empty preprocessor lines. With the same exact text:
// Seen in boost/container/detail/minimal_char_traits_header.hpp:
# // A single quote means it's a contraction or a possessive.
# // It's not an unterminated string.
# // Neither is "this.

// Empty comment:
//
/**/
// Particularly nasty empty comment with no newline, just EOF:
//
