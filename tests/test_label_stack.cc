/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
   Testing for the Label_Stack class
*/
#include "flpr/Label_Stack.hh"
#include "test_helpers.hh"

using FLPR::Label_Stack;

bool empty_stack() {
  Label_Stack ls;
  TEST_TRUE(ls.empty());
  TEST_INT(ls.size(), 0);
  TEST_INT(ls.level(999), -1);
  TEST_FALSE(ls.is_top(999));
  return true;
}

bool one_entry() {
  Label_Stack ls;
  ls.push(999);
  TEST_FALSE(ls.empty());
  TEST_INT(ls.size(), 1);
  TEST_INT(ls.level(555), -1);
  TEST_INT(ls.level(999), 0);
  TEST_TRUE(ls.is_top(999));
  ls.pop();
  TEST_TRUE(ls.empty());
  TEST_INT(ls.size(), 0);
  TEST_INT(ls.level(999), -1);
  TEST_FALSE(ls.is_top(999));
  return true;
}

bool two_diff_entry() {
  Label_Stack ls;
  ls.push(1);
  ls.push(2);
  TEST_FALSE(ls.empty());
  TEST_INT(ls.size(), 2);
  TEST_INT(ls.level(1), -1);
  TEST_INT(ls.level(2), 0);
  ls.pop();
  TEST_INT(ls.size(), 1);
  TEST_INT(ls.level(1), 0);
  return true;
}

bool two_same_entry() {
  Label_Stack ls;
  ls.push(1);
  ls.push(1);
  TEST_INT(ls.level(2), -1);
  TEST_INT(ls.level(1), 2);
  ls.pop();
  TEST_INT(ls.level(1), 1);
  return true;
}

bool seq1() {
  Label_Stack ls;
  ls.push(2);
  ls.push(2);
  ls.push(1);
  ls.push(1);
  ls.push(1);
  ls.push(2);
  TEST_INT(ls.level(2), 0);
  ls.pop();
  TEST_INT(ls.level(2), -1);
  TEST_INT(ls.level(1), 3);
  ls.pop();
  TEST_INT(ls.level(1), 2);
  ls.pop();
  TEST_INT(ls.level(1), 1);
  ls.pop();
  TEST_INT(ls.level(2), 2);
  ls.pop();
  TEST_INT(ls.level(2), 1);
  return true;
}

int main() {
  TEST_MAIN_DECL;

  TEST(empty_stack);
  TEST(one_entry);
  TEST(two_diff_entry);
  TEST(two_same_entry);
  TEST(seq1);

  TEST_MAIN_REPORT;
}
