/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
   Testing for the Safe_List class
*/

#include "flpr/Safe_List.hh"
#include "test_helpers.hh"
#include <vector>

using FLPR::Safe_List;

/* -------------------------- The unit tests ---------------------------- */

struct A {
  A() : a{-1}, b{-1} {}
  A(int a, int b) : a{a}, b{b} {}
  int a, b;
};

bool ctor_default() {
  Safe_List<A> sl;
  TEST_TRUE(sl.empty());
  TEST_INT(sl.size(), 0);
  return true;
}

bool ctor_count_val() {
  Safe_List<int> sl(6, 6);
  TEST_INT(sl.size(), 6);
  for (auto e : sl)
    TEST_INT(e, 6);
  return true;
}

bool ctor_count() {
  Safe_List<A> sl(10);
  TEST_INT(sl.size(), 10);
  for (auto &e : sl) {
    TEST_INT(e.a, -1);
    TEST_INT(e.b, -1);
  }

  return true;
}

bool ctor_list() {
  Safe_List<int> sl{1, 2, 3};
  TEST_INT(sl.size(), 3);
  int i = 1;
  for (auto e : sl)
    TEST_INT(e, i++);
  return true;
}

const int bigval = 0xa5a5a5a5;

bool move_ctor_helper1(std::vector<Safe_List<int>> &l);
bool move_ctor_helper2(std::vector<Safe_List<int>> &l);
bool move_ctor() {
  /* Note that this doesn't do a good job of identifying the case were
     list_ is initialized with {src.list_} rather than {std::move(src.list_)} */
  {
    std::vector<Safe_List<int>> l;
    TEST_TRUE(move_ctor_helper1(l));
    TEST_INT(l.size(), 1);
    TEST_INT(l[0].size(), 4096);
    for (auto const &e : l[0])
      TEST_INT(e, bigval);
    for (auto &e : l[0])
      e = ~bigval;
    for (auto const &e : l[0])
      TEST_INT(e, ~bigval);
    TEST_TRUE(move_ctor_helper1(l));
    TEST_INT(l.size(), 2);
    TEST_INT(l[0].size(), 4096);
    for (auto const &e : l[0])
      TEST_INT(e, ~bigval);
    for (auto &e : l[0])
      e = bigval;
    for (auto const &e : l[0])
      TEST_INT(e, bigval);
  }
  {
    std::vector<Safe_List<int>> l;
    TEST_TRUE(move_ctor_helper2(l));
    TEST_INT(l.size(), 1);
    TEST_INT(l[0].size(), 4096);
    for (auto const &e : l[0])
      TEST_INT(e, bigval);
    for (auto &e : l[0])
      e = ~bigval;
    for (auto const &e : l[0])
      TEST_INT(e, ~bigval);
  }
  return true;
}

bool push_back() {
  Safe_List<std::string> sl;

  std::string s("a");

  sl.push_back(s);
  TEST_FALSE(sl.empty());
  TEST_INT(sl.size(), 1);
  TEST_STR("a", s);

  sl.push_back(std::move(s));
  TEST_FALSE(sl.empty());
  TEST_INT(sl.size(), 2);

  sl.push_back(std::string("a"));
  TEST_FALSE(sl.empty());
  TEST_INT(sl.size(), 3);

  for (std::string const &str : sl)
    TEST_STR("a", str);

  for (auto const &str : sl)
    TEST_STR("a", str);

  return true;
}

bool emplace_back() {
  Safe_List<A> sl;
  sl.emplace_back(1, 2);
  sl.emplace_back(2, 3);
  sl.emplace_back(3, 4);
  TEST_INT(sl.size(), 3);
  int i = 1;
  for (auto &e : sl) {
    TEST_INT(e.a, i);
    TEST_INT(e.b, i + 1);
    i += 1;
  }
  return true;
}

bool erase() {
  Safe_List<int> sl{1, 2, 3, 4};
  TEST_INT(sl.size(), 4);
  sl.erase(std::prev(sl.end()));
  TEST_INT(sl.size(), 3);
  int i = 1;
  for (auto e : sl)
    TEST_INT(e, i++);
  sl.erase(sl.begin(), sl.end());
  TEST_INT(sl.size(), 0);
  TEST_TRUE(sl.empty());
  return true;
}

bool clear() {
  Safe_List<int> sl{1, 2, 3};
  TEST_FALSE(sl.empty());
  TEST_INT(sl.size(), 3);
  sl.clear();
  TEST_TRUE(sl.empty());
  TEST_INT(sl.size(), 0);
  return true;
}

bool pop_back() {
  Safe_List<int> sl{1, 2, 3};
  TEST_INT(sl.size(), 3);
  sl.pop_back();
  TEST_INT(sl.size(), 2);
  {
    int i = 1;
    for (auto e : sl)
      TEST_INT(e, i++);
  }
  sl.pop_back();
  TEST_INT(sl.size(), 1);
  {
    int i = 1;
    for (auto e : sl)
      TEST_INT(e, i++);
  }
  TEST_TRUE(sl.begin() == std::prev(sl.end()));
  return true;
}

int main() {
  TEST_MAIN_DECL;

  TEST(ctor_default);
  TEST(ctor_count_val);
  TEST(ctor_count);
  TEST(ctor_list);
  TEST(move_ctor);
  TEST(push_back);
  TEST(emplace_back);
  TEST(erase);
  TEST(clear);
  TEST(pop_back);

  TEST_MAIN_REPORT;
}

bool move_ctor_helper1(std::vector<Safe_List<int>> &l) {
  Safe_List<int> sl(4096, bigval);
  TEST_INT(sl.size(), 4096);
  l.emplace_back(std::move(sl));
  TEST_INT(l.back().size(), 4096);
  sl = Safe_List<int>();
  return true;
}

bool move_ctor_helper2(std::vector<Safe_List<int>> &l) {
  l.emplace_back(Safe_List<int>(4096, bigval));
  TEST_INT(l.back().size(), 4096);
  return true;
}
