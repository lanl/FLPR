/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "LL_Helper.hh"
#include "test_helpers.hh"

using namespace FLPR;

bool test_consume_until_eol() {
  {
    LL_Helper l({"(this is a test)"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
  }
  return true;
}

bool test_move_to_close_paren() {
  {
    LL_Helper l({"(test(()))="});
    TT_Stream ts = l.stream1();
    ts.move_to_close_paren();
    TEST_EQ(ts.peek(), Syntax_Tags::TK_EQUAL);
  }
  {
    LL_Helper l({"(test(())="});
    TT_Stream ts = l.stream1();
    ts.move_to_close_paren();
    TEST_EQ(ts.peek(), Syntax_Tags::BAD);
  }
  return true;
}

bool test_move_before_close_paren() {
  {
    LL_Helper l({"(test(()))="});
    TT_Stream ts = l.stream1();
    ts.move_before_close_paren();
    TEST_EQ(ts.peek(1), Syntax_Tags::TK_PARENR);
    TEST_EQ(ts.peek(2), Syntax_Tags::TK_EQUAL);
  }
  {
    LL_Helper l({"(test(())="});
    TT_Stream ts = l.stream1();
    ts.move_before_close_paren();
    TEST_EQ(ts.peek(), Syntax_Tags::BAD);
  }
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(test_consume_until_eol);
  TEST(test_move_to_close_paren);
  TEST(test_move_before_close_paren);
  TEST_MAIN_REPORT;
}
