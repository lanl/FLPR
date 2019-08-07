/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
   Testing for Syntax_Tag entry consistency.  This is more a sanity check
   on the contents of Syntax_Tag_Defs.hh than anything.
*/
#include "flpr/Syntax_Tags.hh"
#include "test_helpers.hh"
#include <iostream>

/* The non-control syntags are stored in alphabetical order, and each syntag
   type class (KW_, PG_, SG_, TK_) has an entity to represent the (open) lower
   bound of the class indices (e.g. SG_000_LB) and an (open) upper bound
   (SG_ZZZ_UB).  Check that the UB and LBs are consecutive. */
bool tag_bounds() {
  TEST_TRUE(FLPR::Syntax_Tags::KW_000_LB < FLPR::Syntax_Tags::KW_ZZZ_UB);
  TEST_EQ(FLPR::Syntax_Tags::PG_000_LB, FLPR::Syntax_Tags::KW_ZZZ_UB + 1);
  TEST_TRUE(FLPR::Syntax_Tags::PG_000_LB < FLPR::Syntax_Tags::PG_ZZZ_UB);
  TEST_EQ(FLPR::Syntax_Tags::SG_000_LB, FLPR::Syntax_Tags::PG_ZZZ_UB + 1);
  TEST_TRUE(FLPR::Syntax_Tags::SG_000_LB < FLPR::Syntax_Tags::SG_ZZZ_UB);
  TEST_EQ(FLPR::Syntax_Tags::TK_000_LB, FLPR::Syntax_Tags::SG_ZZZ_UB + 1);
  TEST_TRUE(FLPR::Syntax_Tags::TK_000_LB < FLPR::Syntax_Tags::TK_ZZZ_UB);
  TEST_EQ(FLPR::Syntax_Tags::CLIENT_EXTENSION,
          FLPR::Syntax_Tags::TK_ZZZ_UB + 1);
  return true;
}

bool kw_type() {
  for (int i = FLPR::Syntax_Tags::KW_000_LB + 1;
       i < FLPR::Syntax_Tags::KW_ZZZ_UB; ++i) {
    TEST_INT_LABEL(FLPR::Syntax_Tags::label(i), FLPR::Syntax_Tags::types[i], 4);
  }
  return true;
}

bool sg_stmt_type() {
  for (int i = FLPR::Syntax_Tags::SG_000_LB + 1;
       i < FLPR::Syntax_Tags::SG_ZZZ_UB; ++i) {
    std::string label = FLPR::Syntax_Tags::label(i);
    auto pos = label.find_last_of('-');
    std::string last_term = label.substr(pos + 1);
    if (last_term == "stmt") {
      TEST_INT_LABEL(label, FLPR::Syntax_Tags::types[i], 5);
    } else {
      TEST_OR_INT_LABEL(label, FLPR::Syntax_Tags::types[i], 1, 2);
    }
  }
  return true;
}

bool tk_type() {
  for (int i = FLPR::Syntax_Tags::TK_000_LB + 1;
       i < FLPR::Syntax_Tags::TK_ZZZ_UB; ++i) {
    TEST_INT_LABEL(FLPR::Syntax_Tags::label(i), FLPR::Syntax_Tags::types[i], 3);
  }
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(tag_bounds);
  TEST(sg_stmt_type);
  TEST_MAIN_REPORT;
}
