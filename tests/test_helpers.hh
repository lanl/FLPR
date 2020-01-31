/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#ifndef TEST_HELPERS_HH
#define TEST_HELPERS_HH 1

#include "flpr/Syntax_Tags.hh"
#include <iostream>
#include <string>

/* ---------------------- Some helper functions ------------------------ */

inline std::ostream &safe_print_char(std::ostream &os, const char c) {
  if ((int)c < 32 || (int)c > 126)
    os << "(char)" << (int)c;
  else
    os << '\'' << c << '\'';
  return os;
}

inline bool expect_str(char const *expected_val, std::string const &test_val,
                       char const *entity_name, char const *file,
                       int const line) {
  if (test_val != expected_val) {
    std::cerr << file << ':' << line << " Expecting " << entity_name << " = \""
              << expected_val << "\", got \"" << test_val << "\"\n";
    return false;
  }
  return true;
}

inline bool expect_char(char const expected_val, char const test_val,
                        char const *entity_name, char const *file,
                        int const line) {
  if (test_val != expected_val) {
    std::cerr << file << ':' << line << " Expecting " << entity_name << " = ";
    safe_print_char(std::cerr, expected_val) << ", got ";
    safe_print_char(std::cerr, test_val) << '\n';
    return false;
  }
  return true;
}

inline bool expect_token(int const expected_val, int const test_val,
                         char const *entity_name, char const *file,
                         int const line) {
  if (test_val != expected_val) {
    std::cerr << file << ':' << line << " Expecting ";
    FLPR::Syntax_Tags::print(std::cerr, expected_val) << ", got ";
    FLPR::Syntax_Tags::print(std::cerr, test_val) << '\n';
    return false;
  }
  return true;
}

// Use these generic macros in the individual tests
#define TEST_FALSE(A)                                                          \
  if ((A) != false) {                                                          \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting !" #A              \
              << std::endl;                                                    \
    return false;                                                              \
  }
#define TEST_TRUE(A)                                                           \
  if ((A) != true) {                                                           \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A << std::endl; \
    return false;                                                              \
  }
#define TEST_STR(A, B)                                                         \
  if (!expect_str((A), (B), #B, __FILE__, __LINE__))                           \
  return false
#define TEST_CHAR(A, B)                                                        \
  if (!expect_char((A), (B), #B, __FILE__, __LINE__))                          \
  return false
#define TEST_TOK(A, B)                                                         \
  if (!expect_token((FLPR::Syntax_Tags::A), (B), #B, __FILE__, __LINE__))      \
  return false

#define TEST_EQ(A, B)                                                          \
  if ((A) != (B)) {                                                            \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A "(=" << A     \
              << ") == " #B "(=" << B << ")" << std::endl;                     \
    return false;                                                              \
  }
#define TEST_EQ_NODISPLAY(A, B)                                                \
  if ((A) != (B)) {                                                            \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A " == " #B     \
              << std::endl;                                                    \
    return false;                                                              \
  }
#define TEST_INT(A, B)                                                         \
  if ((A) != (B)) {                                                            \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A "(=" << A     \
              << ") == " << B << ")" << std::endl;                             \
    return false;                                                              \
  }
#define TEST_INT_LABEL(LAB, A, B)                                              \
  if ((A) != (B)) {                                                            \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A "(=" << A     \
              << ", " << LAB << ") == " << B << ")" << std::endl;              \
    return false;                                                              \
  }
#define TEST_OR_INT_LABEL(LAB, A, B, C)                                        \
  if ((A) != (B) && (A) != (C)) {                                              \
    std::cerr << __FILE__ << ':' << __LINE__ << " Expecting " #A "(=" << A     \
              << ", " << LAB << ") == " << B << " or " << C << ")"             \
              << std::endl;                                                    \
    return false;                                                              \
  }

// Put this at the begining of main()
#define TEST_MAIN_DECL                                                         \
  bool res{true};                                                              \
  int count{0}, success { 0 }

// Run each test with this macro
#define TEST(A)                                                                \
  std::cerr << #A << ": ";                                                     \
  std::cerr.flush();                                                           \
  if (!A()) {                                                                  \
    std::cerr << " FAIL" << std::endl;                                         \
    res = false;                                                               \
  } else {                                                                     \
    std::cerr << " pass" << std::endl;                                         \
    success += 1;                                                              \
  }                                                                            \
  ++count

// Put this at the end of main()
#define TEST_MAIN_REPORT                                                       \
  std::cout << success << '/' << count << " tests ran succesfully\n";          \
  return (res == true) ? 0 : 1

#endif
