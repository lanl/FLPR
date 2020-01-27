/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "flpr/utils.hh"
#include "test_helpers.hh"

bool no_trim() {
  const std::string str("no leading/trailing ws");
  const std::string res = FLPR::trim(str);
  TEST_STR("no leading/trailing ws", res);
  return true;
}

bool trim_tail() {
  const std::string str("trailing ws   ");
  const std::string res = FLPR::trim(str);
  TEST_STR("trailing ws", res);
  return true;
}

bool trim_head() {
  const std::string str("  leading ws");
  const std::string res = FLPR::trim(str);
  TEST_STR("leading ws", res);
  return true;
}

bool trim_both() {
  const std::string str("  both           ");
  const std::string res = FLPR::trim(str);
  TEST_STR("both", res);
  return true;
}

bool test_lower() {
  const std::string str(" ThIS 1s a TEST  ");
  std::string res = str;
  FLPR::tolower(res);
  TEST_STR(" this 1s a test  ", res);
  return true;
}

bool test_upper() {
  const std::string str(" ThIS 1s a TEST  ");
  std::string res = str;
  FLPR::toupper(res);
  TEST_STR(" THIS 1S A TEST  ", res);
  return true;
}

bool test_simple_tokenize1() {
  const std::string str(" this is a   test");
  std::vector<std::string> res;
  FLPR::simple_tokenize(str, res);
  TEST_EQ(4, res.size());
  TEST_STR("this", res[0]);
  TEST_STR("is", res[1]);
  TEST_STR("a", res[2]);
  TEST_STR("test", res[3]);
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(no_trim);
  TEST(trim_tail);
  TEST(trim_head);
  TEST(trim_both);
  TEST(test_lower);
  TEST(test_upper);
  TEST(test_simple_tokenize1);
  TEST_MAIN_REPORT;
}
