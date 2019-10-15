/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#ifndef PARSE_HELPERS_HH
#define PARSE_HELPERS_HH 1

#include "LL_Helper.hh"
#include "flpr/Prgm_Parsers.hh"
#include "flpr/Prgm_Tree.hh"
#include "flpr/Syntax_Tags.hh"
#include "test_helpers.hh"

#include <iostream>

using FLPR::Syntax_Tags;

inline bool expect_tag(Syntax_Tags::Tags const expected_val, int const test_val,
                       char const *entity_name, char const *filename,
                       int const linenum) {
  if (test_val != expected_val) {
    std::cerr << filename << ':' << linenum << " Expecting " << entity_name
              << " == \"" << Syntax_Tags::label(expected_val)
              << "\", but got \"" << Syntax_Tags::label(test_val) << "\"\n";
    return false;
  }
  return true;
}

//! Error out if a tree wasn't returned
#define TEST_TREE(T, P, LLH)                                                   \
  if (!T) {                                                                    \
    std::cerr << "Expecting " #P " to parse: ";                                \
    LLH.print(std::cerr);                                                      \
    return false;                                                              \
  }

//! Error out if a tree WAS returned
#define FAIL_TREE(T, P, LLH)                                                   \
  if (T.tree_initialized()) {                                                  \
    std::cerr << "Expecting " #P " NOT to parse: ";                            \
    LLH.print(std::cerr);                                                      \
    std::cerr << "but got result:\n" << T << '\n';                             \
    return false;                                                              \
  }

#define TEST_TAG(A, B)                                                         \
  if (!expect_tag(Syntax_Tags::B, A, #A, __FILE__, __LINE__))                  \
  return false

#define TEST_TOK_EQ(A, B, LLH)                                                 \
  if ((A) != (B)) {                                                            \
    std::cerr << "Expecting " #A " '" << A << "' == " #B " '";                 \
    Syntax_Tags::print(std::cerr, B) << "' after parsing\n";                   \
    LLH.print(std::cerr);                                                      \
    return false;                                                              \
  }

//! Test Single Statement (expects EOL when complete)
#define TSS(SP, S)                                                             \
  {                                                                            \
    LL_Helper l({S});                                                          \
    TT_Stream ts = l.stream1();                                                \
    Stmt_Tree st = FLPR::Stmt::SP(ts);                                         \
    TEST_TREE(st, SP, l);                                                      \
    TEST_TOK_EQ(Syntax_Tags::BAD, ts.peek(), l);                               \
  }

//! Fail Single Statement
#define FSS(SP, S)                                                             \
  {                                                                            \
    LL_Helper l({S});                                                          \
    TT_Stream ts = l.stream1();                                                \
    Stmt_Tree st = FLPR::Stmt::SP(ts);                                         \
    FAIL_TREE(st, SP, l);                                                      \
  }

//! Test Partial Statement (expects token Syntax_Tags::ET when complete)
#define TPS(SP, S, ET)                                                         \
  {                                                                            \
    LL_Helper l({S});                                                          \
    TT_Stream ts = l.stream1();                                                \
    Stmt_Tree st = FLPR::Stmt::SP(ts);                                         \
    TEST_TREE(st, SP, l);                                                      \
    TEST_TOK_EQ(Syntax_Tags::ET, ts.peek(), l);                                \
  }

//! Fail Partial Statement (expects token Syntax_Tags::ET when complete)
#define FPS(SP, S, ET)                                                         \
  {                                                                            \
    LL_Helper l({S});                                                          \
    TT_Stream ts = l.stream1();                                                \
    Stmt_Tree st = FLPR::Stmt::SP(ts);                                         \
    FAIL_TREE(st, SP, l);                                                      \
    TEST_TOK_EQ(Syntax_Tags::ET, ts.peek(), l);                                \
  }

#endif
