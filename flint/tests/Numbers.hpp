#ifndef NUMBERS_HPP
#define NUMBERS_HPP

namespace NumbersCpp14 {
  // Older version of Tokenizer barfed on some of these and combined them; want to ensure EVERY line gives a warning...
  static constexpr int a = 10'000'000;
  static constexpr int b = 0b0000'1111'0000'0000;
  static constexpr int b2 = 0b0'1'0'1;
  static constexpr int c = 0xFFFF'FFFF;
  static constexpr int d = 0xFFFF'FFFFul;
  static constexpr int e = 0xFFFF'FFFFull;
  static constexpr int c2 = 0xFFFF'0FFF;
  static constexpr int d2 = 0xFFFF'0FFFul;
  static constexpr int e2 = 0xFFFF'0FFFull;
}

#endif
