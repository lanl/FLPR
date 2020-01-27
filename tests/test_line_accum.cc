/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "flpr/Line_Accum.hh"
#include "test_helpers.hh"
#include <iostream>

using FLPR::Line_Accum;

// One line, no offset
bool simple() {
  Line_Accum la;
  int ln, cn;
  la.add_line(0, 0, 1, "foo", 0);
  TEST_STR("foo", la.accum());
  la.linecolno(0, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 1);
  la.linecolno(1, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 2);
  la.linecolno(2, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 3);
  return true;
}

// One line, 2 char offset
bool simple1() {
  Line_Accum la;
  int ln, cn;
  la.add_line(0, 0, 2, "foo", 0);
  TEST_STR("foo", la.accum());
  la.linecolno(0, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 2);
  la.linecolno(1, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 3);
  la.linecolno(2, ln, cn);
  TEST_EQ(ln, 0);
  TEST_EQ(cn, 4);
  return true;
}

// One line (#3), 2 char offset
bool simple2() {
  Line_Accum la;
  int ln, cn;
  la.add_line(3, 0, 2, "foo", 0);
  TEST_STR("foo", la.accum());
  la.linecolno(0, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 2);
  la.linecolno(1, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 3);
  la.linecolno(2, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 4);
  return true;
}

bool subname() {
  Line_Accum la;
  int ln, cn;
  la.add_line(11, 0, 1, "subroutine b(kdd)", 0);
  la.linecolno(11, ln, cn);
  TEST_EQ(ln, 11);
  TEST_EQ(cn, 12);
  return true;
}

// Two line, 2 char offset
bool twoline1() {
  Line_Accum la;
  int ln, cn;
  la.add_line(3, 0, 2, "foo", 1);
  la.add_line(4, 0, 5, "bar", 0);
  TEST_STR("foo bar", la.accum());
  la.linecolno(0, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 2);
  la.linecolno(1, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 3);
  la.linecolno(2, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 4);
  la.linecolno(3, ln, cn);
  TEST_EQ(ln, 3);
  TEST_EQ(cn, 5);
  la.linecolno(4, ln, cn);
  TEST_EQ(ln, 4);
  TEST_EQ(cn, 5);
  la.linecolno(5, ln, cn);
  TEST_EQ(ln, 4);
  TEST_EQ(cn, 6);
  la.linecolno(6, ln, cn);
  TEST_EQ(ln, 4);
  TEST_EQ(cn, 7);
  return true;
}

bool continued_string() {
  Line_Accum la;
  la.add_line(1, 0, 6,
              "print *,                           "
              "                           'abc",
              0);
  la.add_line(2, 0, 6, "def'", 0);
  TEST_STR("print *,                           "
           "                           'abcdef'",
           la.accum());
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(simple);
  TEST(simple1);
  TEST(simple2);
  TEST(subname);
  TEST(twoline1);
  TEST(continued_string);
  TEST_MAIN_REPORT;
}
