/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "LL_Helper.hh"
#include "flpr/Logical_Line.hh"
#include "test_helpers.hh"
#include <iostream>
#include <sstream>

using FLPR::Logical_Line;

bool test_default_ctor() {
  Logical_Line ll;
  TEST_FALSE(ll.has_stmts());
  TEST_FALSE(ll.needs_reformat);
  TEST_FALSE(ll.suppress);
  TEST_INT(ll.label, 0);
  return true;
}

bool test_simple_1() {
  Logical_Line ll("subROUTINE foo()");
  TEST_INT(ll.layout().size(), 1);
  TEST_INT(ll.fragments().size(), 4);

  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_TOK(KW_SUBROUTINE, curr->token);
  TEST_STR("subROUTINE", curr->text());

  std::advance(curr, 1);
  TEST_TOK(TK_NAME, curr->token);
  TEST_STR("foo", curr->text());

  std::advance(curr, 1);
  TEST_TOK(TK_PARENL, curr->token);
  TEST_STR("(", curr->text());

  std::advance(curr, 1);
  TEST_TOK(TK_PARENR, curr->token);
  TEST_STR(")", curr->text());

  return true;
}

bool offsets_1() {
  Logical_Line ll("call bar()", true);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 0);
  TEST_INT(curr->start_pos, 1);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 5);
  TEST_INT(curr->start_pos, 6);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 8);
  TEST_INT(curr->start_pos, 9);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 9);
  TEST_INT(curr->start_pos, 10);
  return true;
}

bool offsets_2() {
  Logical_Line ll("      call bar()", true);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->main_txt_col(), 0);
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_col(), 5);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_col(), 8);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->main_txt_col(), 9);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool offsets_3() {
  Logical_Line ll("10    call bar()", true);
  TEST_INT(ll.label, 10);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool offsets_4() {
  Logical_Line ll("   10 call bar()", true);
  TEST_INT(ll.label, 10);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool offsets_5() {
  Logical_Line ll("       10 call bar()", true);
  TEST_INT(ll.label, 10);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 11);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 19);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 20);
  return true;
}

bool offsets_6() {
  Logical_Line ll("      call bar()", false);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool offsets_7() {
  Logical_Line ll("10    call bar()", false);
  TEST_INT(ll.label, 10);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool offsets_8() {
  Logical_Line ll("   10 call bar()", false);
  TEST_INT(ll.label, 10);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->start_pos, 7);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 12);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 15);
  std::advance(curr, 1);
  TEST_INT(curr->start_pos, 16);
  return true;
}

bool replace_fragment_1() {
  Logical_Line ll("   call bar()", true);
  FLPR::TT_SEQ::iterator name = std::next(ll.fragments().begin());
  TEST_STR("bar", name->text());
  ll.replace_fragment(name, FLPR::Syntax_Tags::TK_NAME,
                      std::string{"longer_bar"});
  TEST_STR("longer_bar", name->text());

  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 0);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 5);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 15);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 16);
  return true;
}

bool replace_fragment_2() {
  Logical_Line ll("   call bar()", true);
  FLPR::TT_SEQ::iterator name = std::next(ll.fragments().begin());
  TEST_STR("bar", name->text());
  ll.replace_fragment(name, FLPR::Syntax_Tags::TK_NAME, std::string{"b"});
  TEST_STR("b", name->text());

  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 0);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 5);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 6);

  std::advance(curr, 1);
  TEST_INT(curr->main_txt_line(), 0);
  TEST_INT(curr->main_txt_col(), 7);
  return true;
}

bool split_after_0() {
  Logical_Line ll("   call bar  ! comment", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = std::next(ll.fragments().begin());
  /* Shouldn't generate empty lines... */
  TEST_FALSE(ll.split_after(tt, remain));

  /* ...and shouldn't damage ll */
  TEST_INT(ll.fragments().size(), 2);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_TOK(TK_NAME, ll.fragments().back().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_STR("   ", ll.layout()[0].left_space);
  TEST_STR("call bar", ll.layout()[0].main_txt);
  TEST_STR("  ", ll.layout()[0].right_space);
  TEST_STR("! comment", ll.layout()[0].right_txt);

  return true;
}

bool split_after_1() {
  Logical_Line ll("call bar", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 1);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_TRUE(ll.layout()[0].left_space.empty());
  TEST_STR("call", ll.layout()[0].main_txt);
  TEST_TRUE(ll.layout()[0].right_space.empty());
  TEST_TRUE(ll.layout()[0].right_txt.empty());

  TEST_INT(remain.fragments().size(), 1);
  TEST_TOK(TK_NAME, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_TRUE(remain.layout()[0].left_space.empty());
  TEST_STR("bar", remain.layout()[0].main_txt);
  TEST_TRUE(remain.layout()[0].right_space.empty());
  TEST_TRUE(remain.layout()[0].right_txt.empty());
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }

  return true;
}

bool split_after_2() {
  Logical_Line ll("call  bar    ! foo", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 1);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_TRUE(ll.layout()[0].left_space.empty());
  TEST_STR("call", ll.layout()[0].main_txt);
  TEST_STR("         ", ll.layout()[0].right_space);
  TEST_STR("! foo", ll.layout()[0].right_txt);

  TEST_INT(remain.fragments().size(), 1);
  TEST_TOK(TK_NAME, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_TRUE(remain.layout()[0].left_space.empty());
  TEST_STR("bar", remain.layout()[0].main_txt);
  TEST_TRUE(remain.layout()[0].right_space.empty());
  TEST_TRUE(remain.layout()[0].right_txt.empty());
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }

  return true;
}

bool split_after_3() {
  Logical_Line ll("   call  bar    ! foo", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 1);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_STR("   ", ll.layout()[0].left_space);
  TEST_STR("call", ll.layout()[0].main_txt);
  TEST_STR("         ", ll.layout()[0].right_space);
  TEST_STR("! foo", ll.layout()[0].right_txt);

  TEST_INT(remain.fragments().size(), 1);
  TEST_TOK(TK_NAME, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_STR("   ", remain.layout()[0].left_space);
  TEST_STR("bar", remain.layout()[0].main_txt);
  TEST_TRUE(remain.layout()[0].right_space.empty());
  TEST_TRUE(remain.layout()[0].right_txt.empty());
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }
  return true;
}

bool split_after_4() {
  Logical_Line ll(" 2 call  bar    ! foo", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 1);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_STR(" 2", ll.layout()[0].left_txt);
  TEST_STR(" ", ll.layout()[0].left_space);
  TEST_STR("call", ll.layout()[0].main_txt);
  TEST_STR("         ", ll.layout()[0].right_space);
  TEST_STR("! foo", ll.layout()[0].right_txt);
  TEST_INT(ll.label, 2);

  TEST_INT(remain.fragments().size(), 1);
  TEST_TOK(TK_NAME, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_STR("   ", remain.layout()[0].left_space);
  TEST_STR("bar", remain.layout()[0].main_txt);
  TEST_TRUE(remain.layout()[0].right_space.empty());
  TEST_TRUE(remain.layout()[0].right_txt.empty());
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }
  return true;
}

bool split_after_5() {
  Logical_Line ll("   call bar;a=a+1!hey", true);
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = std::next(ll.fragments().begin());
  TEST_TRUE(ll.split_after(tt, remain));
  TEST_INT(ll.fragments().size(), 2);
  TEST_TOK(KW_CALL, ll.fragments().front().token);
  TEST_TOK(TK_NAME, ll.fragments().back().token);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_STR("   ", ll.layout()[0].left_space);
  TEST_STR("call bar", ll.layout()[0].main_txt);
  TEST_STR("      ", ll.layout()[0].right_space);
  TEST_STR("!hey", ll.layout()[0].right_txt);

  TEST_INT(remain.fragments().size(), 5);
  TEST_TOK(TK_NAME, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_STR("   ", remain.layout()[0].left_space);
  TEST_STR("a=a+1", remain.layout()[0].main_txt);
  TEST_TRUE(remain.layout()[0].right_space.empty());
  TEST_TRUE(remain.layout()[0].right_txt.empty());
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }
  return true;
}

bool split_after_6() {
  // clang-format off
  LL_Helper ls({"  if(a>2) & !comment 1",
                "     return !comment 2"});
  // clang-format on
  TEST_INT(ls.lines().size(), 1);

  Logical_Line &ll{ls.lines().front()};
  Logical_Line remain;

  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  std::advance(tt, 5);
  TEST_TOK(TK_PARENR, tt->token);
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 6);
  TEST_INT(ll.layout().size(), 1);
  TEST_TRUE(ll.layout()[0].left_txt.empty());
  TEST_STR("  ", ll.layout()[0].left_space);
  TEST_STR("if(a>2)", ll.layout()[0].main_txt);
  TEST_STR("   ", ll.layout()[0].right_space);
  TEST_STR("!comment 1", ll.layout()[0].right_txt);
  for (auto const &tt : ll.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = ll.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }

  TEST_INT(remain.fragments().size(), 1);
  TEST_TOK(KW_RETURN, remain.fragments().front().token);
  TEST_INT(remain.layout().size(), 1);
  TEST_TRUE(remain.layout()[0].left_txt.empty());
  TEST_STR("  ", remain.layout()[0].left_space);
  TEST_STR("return", remain.layout()[0].main_txt);
  TEST_STR("   ", ll.layout()[0].right_space);
  TEST_STR("!comment 2", remain.layout()[0].right_txt);
  for (auto const &tt : remain.fragments()) {
    TEST_INT(tt.main_txt_line(), 0);
    size_t tlen = tt.text().size();
    size_t tpos = tt.main_txt_col();
    std::string main_txt = remain.layout()[0].main_txt.substr(tpos, tlen);
    TEST_EQ(main_txt, tt.text());
  }

  return true;
}

bool split_after_7() {
  // clang-format off
  Logical_Line ll({"3 a=b; c= &     !comment 1",
                   "      d; e=f ;& !comment 2",
                   "          g=h   !comment 3"});
  // clang-format on
  TEST_INT(ll.layout().size(), 3);

  Logical_Line remain;
  FLPR::TT_SEQ::iterator tt = ll.fragments().begin();
  std::advance(tt, 6);
  TEST_TOK(TK_NAME, tt->token);
  TEST_TRUE(ll.split_after(tt, remain));

  TEST_INT(ll.fragments().size(), 7);
  TEST_INT(ll.layout().size(), 2);
  TEST_STR("3", ll.layout()[0].left_txt);
  TEST_STR(" ", ll.layout()[0].left_space);
  TEST_STR("a=b; c=", ll.layout()[0].main_txt);
  TEST_STR(" ", ll.layout()[0].right_space);
  TEST_STR("&     !comment 1", ll.layout()[0].right_txt);

  // FIX: needs more checks
  for (auto const &fl : remain.layout())
    fl.dump(std::cout) << '\n';
  return true;
}

bool replace_main_generic(std::vector<std::string> const &orig_lines,
                          std::vector<std::string> const &new_text) {
  Logical_Line ll(orig_lines);
  Logical_Line copy_ll(ll);

  std::vector<std::string> new_text_continued;
  for (size_t i = 0; i + 1 < new_text.size(); ++i) {
    new_text_continued.push_back(new_text[i] + " &");
  }
  new_text_continued.push_back(new_text.back());

  ll.replace_main_text(new_text);
  TEST_INT(ll.layout().size(), std::max(new_text.size(), orig_lines.size()));
  for (size_t i = 0; i < new_text.size(); ++i)
    TEST_EQ(ll.layout()[i].main_txt, new_text[i]);

  for (size_t i = 0; i < orig_lines.size(); ++i) {
    if (!copy_ll.layout()[i].is_continued() && ll.layout()[i].is_continued()) {
      TEST_EQ(ll.layout()[i].right_txt.substr(2),
              copy_ll.layout()[i].right_txt);
    } else {
      TEST_EQ(ll.layout()[i].right_txt, copy_ll.layout()[i].right_txt);
    }
  }

  Logical_Line fresh_ll(new_text_continued);

  for (size_t i = 0; i < fresh_ll.layout().size(); ++i) {
    TEST_EQ(ll.layout()[i].is_continued(), fresh_ll.layout()[i].is_continued());
  }

  if (fresh_ll.fragments().size() != ll.fragments().size()) {
    std::cout << "\nDEBUG replace_main_txt fragments size\n";
    for (auto const &fl : fresh_ll.layout())
      fl.dump(std::cout) << '\n';
    std::cout << "-----------------------------------------\n";
    for (auto const &fl : ll.layout())
      fl.dump(std::cout) << '\n';
  }

  TEST_EQ(fresh_ll.fragments().size(), ll.fragments().size());
  size_t const num_frag = ll.fragments().size();
  auto frag_0 = ll.fragments().begin();
  auto frag_1 = fresh_ll.fragments().begin();
  for (size_t i = 0; i < num_frag; ++i) {
    TEST_EQ(frag_0->token, frag_1->token);
    TEST_EQ(frag_0->text(), frag_1->text());
    std::advance(frag_0, 1);
    std::advance(frag_1, 1);
  }

  return true;
}

bool replace_main_1() {
  // clang-format off
  std::vector<std::string> const orig_lines(
      {"  return"});
  std::vector<std::string> const new_text(
      {"goto 10"});
  // clang-format on
  return replace_main_generic(orig_lines, new_text);
}

bool replace_main_2() {
  // clang-format off
  std::vector<std::string> const orig_lines(
      {"  return  ! mycomment"});
  std::vector<std::string> const new_text(
      {"goto", "10"});
  // clang-format on
  return replace_main_generic(orig_lines, new_text);
}

bool continued_if() {
  Logical_Line ll(
      std::vector<std::string>{"                if ( a(b)%c(d) < e ) exit",
                               "     &            f"},
      false);

  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  std::advance(curr, 14);
  TEST_STR("exit", curr->text());
  TEST_INT(ll.fragments().size(), 16);
  return true;
}

//          1         2         3         4         5         6         7
// 1234567890123456789012345678901234567890123456789012345678901234567890123456

bool continued_if_fixed_string() {
  Logical_Line ll(
      std::vector<std::string>{"      print *,                                 "
                               "                     'abcXXX",
                               "     1def'"},
      false);
  /* Note that we accept text beyond column 72 */
  TEST_INT(ll.fragments().size(), 4);
  FLPR::TT_SEQ::const_iterator curr = ll.cfragments().begin();
  std::advance(curr, 3);
  TEST_STR("'abcXXXdef'", curr->text());
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(test_default_ctor);
  TEST(test_simple_1);
  TEST(offsets_1);
  TEST(offsets_2);
  TEST(offsets_3);
  TEST(offsets_4);
  TEST(offsets_5);
  TEST(offsets_6);
  TEST(offsets_7);
  TEST(offsets_8);
  TEST(replace_fragment_1);
  TEST(replace_fragment_2);
  TEST(split_after_0);
  TEST(split_after_1);
  TEST(split_after_2);
  TEST(split_after_3);
  TEST(split_after_4);
  TEST(split_after_5);
  TEST(split_after_6);
  TEST(split_after_7);
  TEST(replace_main_1);
  TEST(replace_main_2);
  TEST(continued_if);
  TEST(continued_if_fixed_string);
  TEST_MAIN_REPORT;
}
