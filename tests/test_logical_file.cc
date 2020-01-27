/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "LL_Helper.hh"
#include "flpr/Logical_File.hh"
#include "test_helpers.hh"
#include <iostream>
#include <sstream>

using FLPR::LL_SEQ;
using FLPR::LL_STMT_SEQ;
using FLPR::Logical_File;

bool replace_stmt_text_1() {
  // clang-format off
  LL_Helper helper({"   return"});
  // clang-format on
  Logical_File &file = helper.logical_file();
  TEST_INT(file.ll_stmts.size(), 1);
  LL_STMT_SEQ::iterator stmt_it = file.ll_stmts.begin();
  TEST_INT(stmt_it->size(), 1);
  LL_SEQ::iterator orig_ll_it = stmt_it->it();
  file.replace_stmt_text(stmt_it, {"RETURN"},
                         FLPR::Syntax_Tags::SG_RETURN_STMT);
  TEST_INT(file.ll_stmts.size(), 1);
  TEST_TRUE(file.ll_stmts.begin() == stmt_it);
  TEST_TRUE(stmt_it->it() == orig_ll_it);

  TEST_INT(file.ll_stmts.front().size(), 1);
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(replace_stmt_text_1);
  TEST_MAIN_REPORT;
}
