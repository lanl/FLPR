/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "flpr/File_Line.hh"
#include "test_helpers.hh"
#include <iostream>
#include <string>

using FLPR::File_Line;

/* -------------------------- The unit tests ---------------------------- */
bool fixed_blank1() {
  //              "123456"
  std::string str("");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_blank2() {
  //              "123456"
  std::string str("  ");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_blank3() {
  //              "123456"
  std::string str("             \t");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_comment1() {
  //              "123456"
  std::string str("C     This is an aligned comment");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_comment());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_comment2() {
  //              "123456"
  std::string str("c     This is an aligned comment");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_comment());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_comment3() {
  //              "123456"
  std::string str("!     This is an aligned comment");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_comment());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_comment4() {
  //              "123456"
  std::string str("  !     This is an aligned comment");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_comment());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_comment5() {
  //              "123456"
  std::string str("       !     This is an aligned comment");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_comment());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_flprpp() {
  //              "123456"
  std::string str("!#flpr ");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_FALSE(fl.is_comment());
  TEST_TRUE(fl.is_flpr_pp());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool fixed_labelled() {
  //              "123456"
  std::string str(" 100  continue");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_fortran());
  TEST_TRUE(fl.has_label());
  TEST_STR(" 100", fl.left_txt);
  TEST_STR("continue", fl.main_txt);
  return true;
}

bool fixed_indent() {
  //              "123456"
  std::string str("        call foo()");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_STR("", fl.left_txt);
  TEST_STR("  ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  return true;
}

bool fixed_continuation() {
  //              "123456"
  std::string str("     a   call foo()");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_TRUE(fl.is_continuation());
  TEST_FALSE(fl.is_continued());
  TEST_STR("     a", fl.left_txt);
  TEST_STR("   ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  TEST_CHAR('\0', fl.open_delim);
  return true;
}

bool fixed_not_a_continuation() {
  /* section 6.3.3.3 points out that a '0' in col 6 is not a continuation */
  //              "123456"
  std::string str("     0   call foo()");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_FALSE(fl.is_continued());
  TEST_STR("     0", fl.left_txt);
  TEST_STR("   ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  TEST_CHAR('\0', fl.open_delim);
  return true;
}

bool fixed_trailing_comment() {
  //              "123456"
  std::string str("        call foo() ! trailing ");
  File_Line fl = File_Line::analyze_fixed(1, str, '\0');
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.is_blank());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_FALSE(fl.is_continued());
  TEST_STR("", fl.left_txt);
  TEST_STR("  ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  TEST_STR(" ", fl.right_space);
  TEST_STR("! trailing ", fl.right_txt);
  return true;
}

bool free_blank1() {
  const std::string str("");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool free_blank2() {
  const std::string str("  ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool free_blank3() {
  const std::string str("             \t ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_blank());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  return true;
}

bool free_comment1() {
  const std::string str("!     Boring comment");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_comment());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  TEST_STR("!     Boring comment", fl.left_txt);
  return true;
}

bool free_comment2() {
  const std::string str("    !     Boring comment ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_comment());
  TEST_TRUE(fl.is_trivial());
  TEST_FALSE(fl.is_fortran());
  // It is supposed to trim trailing whitespace
  TEST_STR("    !     Boring comment", fl.left_txt);
  return true;
}

bool free_flprpp() {
  const std::string str("!#flpr foo");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_FALSE(fl.is_comment());
  TEST_TRUE(fl.is_flpr_pp());
  TEST_FALSE(fl.is_fortran());
  TEST_FALSE(fl.is_continued());
  TEST_FALSE(fl.is_continuation());
  TEST_STR("!#flpr foo", fl.left_txt);
  return true;
}

bool free_labelled() {
  const std::string str(" 100 continue");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_TRUE(fl.has_label());
  TEST_STR(" 100", fl.left_txt);
  TEST_STR("continue", fl.main_txt);
  return true;
}

bool free_cont_notlabel() {
  const std::string str("  100_8)");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', true, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_STR("100_8)", fl.main_txt);
  return true;
}

bool free_indent() {
  const std::string str("        call foo()");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_STR("", fl.left_txt);
  TEST_STR("        ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  return true;
}

bool free_continuation() {
  const std::string str("        call foo(& ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("", fl.left_txt);
  TEST_STR("        ", fl.left_space);
  TEST_STR("call foo(", fl.main_txt);
  TEST_STR("", fl.right_space);
  TEST_STR("&", fl.right_txt);
  TEST_CHAR('\0', fl.open_delim);
  return true;
}

bool free_char_context_continue1() {
  const std::string str("        call foo(' & ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("", fl.left_txt);
  TEST_STR("        ", fl.left_space);
  TEST_STR("call foo(' ", fl.main_txt);
  TEST_STR("", fl.right_space);
  TEST_STR("&", fl.right_txt);
  TEST_CHAR('\'', fl.open_delim);
  return true;
}

bool free_char_context_continue2() {
  const std::string str("        call foo(\"& ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("", fl.left_txt);
  TEST_STR("        ", fl.left_space);
  TEST_STR("call foo(\"", fl.main_txt);
  TEST_STR("&", fl.right_txt);
  TEST_CHAR('"', fl.open_delim);
  return true;
}

bool free_lead_cont() {
  const std::string str("   & foo)");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', true, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_TRUE(fl.is_continuation());
  TEST_FALSE(fl.is_continued());
  TEST_STR("   &", fl.left_txt);
  TEST_TRUE(fl.left_space.empty());
  TEST_STR(" foo)", fl.main_txt);
  TEST_CHAR('\0', fl.open_delim);
  return true;
}

bool free_lead_follow_cont1() {
  const std::string str("   & foo', foo, &");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\'', true, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_TRUE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("   &", fl.left_txt);
  TEST_STR("", fl.left_space);
  TEST_STR(" foo', foo,", fl.main_txt);
  TEST_STR(" ", fl.right_space);
  TEST_CHAR('\0', fl.open_delim);
  return true;
}

bool free_lead_follow_cont2() {
  const std::string str("   & foo', foo, \" &  ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\'', true, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_TRUE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("   &", fl.left_txt);
  TEST_STR("", fl.left_space);
  TEST_STR(" foo', foo, \" ", fl.main_txt);
  TEST_STR("&", fl.right_txt);
  TEST_CHAR('\"', fl.open_delim);
  return true;
}

bool free_contcomment() {
  const std::string str("        call foo(  & ! this");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_TRUE(fl.is_continued());
  TEST_STR("", fl.left_txt);
  TEST_STR("        ", fl.left_space);
  TEST_STR("call foo(", fl.main_txt);
  TEST_STR("  ", fl.right_space);
  TEST_STR("& ! this", fl.right_txt);
  return true;
}

bool free_trailing_comment() {
  const std::string str("100    call foo() ! okay ");
  bool in_literal{false};
  File_Line fl = File_Line::analyze_free(1, str, '\0', false, in_literal);
  TEST_TRUE(fl.is_fortran());
  TEST_FALSE(fl.is_blank());
  TEST_TRUE(fl.has_label());
  TEST_FALSE(fl.is_continuation());
  TEST_FALSE(fl.is_continued());
  TEST_STR("100", fl.left_txt);
  TEST_STR("    ", fl.left_space);
  TEST_STR("call foo()", fl.main_txt);
  TEST_STR(" ", fl.right_space);
  TEST_STR("! okay", fl.right_txt);
  return true;
}

int main() {
  TEST_MAIN_DECL;

  TEST(fixed_blank1);
  TEST(fixed_blank2);
  TEST(fixed_blank3);
  TEST(fixed_comment1);
  TEST(fixed_comment2);
  TEST(fixed_comment3);
  TEST(fixed_comment4);
  TEST(fixed_comment5);
  TEST(fixed_flprpp);
  TEST(fixed_labelled);
  TEST(fixed_indent);
  TEST(fixed_continuation);
  TEST(fixed_not_a_continuation);
  TEST(fixed_trailing_comment);
  TEST(free_blank1);
  TEST(free_blank2);
  TEST(free_blank3);
  TEST(free_comment1);
  TEST(free_comment2);
  TEST(free_flprpp);
  TEST(free_labelled);
  TEST(free_cont_notlabel);
  TEST(free_indent);
  TEST(free_continuation);
  TEST(free_char_context_continue1);
  TEST(free_char_context_continue2);
  TEST(free_lead_cont);
  TEST(free_lead_follow_cont1);
  TEST(free_lead_follow_cont2);
  TEST(free_contcomment);
  TEST(free_trailing_comment);

  TEST_MAIN_REPORT;
}
