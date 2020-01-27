/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "LL_Helper.hh"
#include "test_helpers.hh"

using namespace FLPR;

bool test_chop_at_open_paren() {
  /* This is less of a unit test, and more a test of a strange pattern used
     in the procedure-designator parser. */
  {
    LL_Helper l({"call foo(bar)"});
    TT_Stream ts = l.stream1();

    TEST_EQ(ts.peek(), Syntax_Tags::KW_CALL);
    ts.consume();
    TEST_EQ(ts.peek(), Syntax_Tags::TK_NAME);

    /* Hold this location */
    auto designator = ts.capture_begin();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
    bool move_okay = ts.move_to_open_paren();
    TEST_TRUE(move_okay);
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENL);

    /* Hold this location */
    ts.put_back();
    ts.capture_end(designator);

    TT_Stream ts2(ts.capture_to_range(designator));
    TEST_EQ(ts2.peek(), Syntax_Tags::TK_NAME);
    ts2.consume();
    TEST_TRUE(ts2.is_eol());

    TEST_EQ(ts.peek(1), Syntax_Tags::TK_PARENL);
    TEST_EQ(ts.peek(2), Syntax_Tags::TK_NAME);
    TEST_EQ(ts.peek(3), Syntax_Tags::TK_PARENR);
    ts.consume(3);
    TEST_TRUE(ts.is_eol());
  }

  {
    LL_Helper l({"call foo"});
    TT_Stream ts = l.stream1();

    TEST_EQ(ts.peek(), Syntax_Tags::KW_CALL);
    ts.consume();
    TEST_EQ(ts.peek(), Syntax_Tags::TK_NAME);

    /* Hold this location */
    auto designator = ts.capture_begin();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_NAME);
    bool move_okay = ts.move_to_open_paren();
    TEST_FALSE(move_okay);

    /* Hold this location */
    if (move_okay)
      ts.put_back();
    ts.capture_end(designator);

    TT_Stream ts2(ts.capture_to_range(designator));
    TEST_EQ(ts2.peek(), Syntax_Tags::TK_NAME);
    ts2.consume();
    TEST_STR("foo", ts2.curr_tt().text());
    TEST_TRUE(ts2.is_eol());

    TEST_TRUE(ts.is_eol());
  }

  return true;
}

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

bool test_move_to_open_paren() {
  {
    LL_Helper l({"a"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    bool move_okay = ts.move_to_open_paren();
    TEST_FALSE(move_okay);
  }
  {
    LL_Helper l({"a)"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
    bool move_okay = ts.move_to_open_paren();
    TEST_FALSE(move_okay);
  }
  {
    LL_Helper l({"a()"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
    bool move_okay = ts.move_to_open_paren();
    TEST_TRUE(move_okay);
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENL);
    /* because move_to_open_paren makes curr() == TK_PARENL, it is as if the
       TK_PARENL token was consumed, so we get a value of 2, rather than
       position == 1. */
    TEST_INT(ts.num_consumed(), 2);
  }
  {
    LL_Helper l({"a(())"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
    bool move_okay = ts.move_to_open_paren();
    TEST_TRUE(move_okay);
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENL);
    /* because move_to_open_paren makes curr() == TK_PARENL, it is as if the
       TK_PARENL token was consumed, so we get a value of 2, rather than
       position == 1. */
    TEST_INT(ts.num_consumed(), 2);
  }
  {
    LL_Helper l({"a(b(),c(d()))"});
    TT_Stream ts = l.stream1();
    ts.consume_until_eol();
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENR);
    bool move_okay = ts.move_to_open_paren();
    TEST_TRUE(move_okay);
    TEST_EQ(ts.curr(), Syntax_Tags::TK_PARENL);
    /* because move_to_open_paren makes curr() == TK_PARENL, it is as if the
       TK_PARENL token was consumed, so we get a value of 2, rather than
       position == 1. */
    TEST_INT(ts.num_consumed(), 2);
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
  TEST(test_move_to_open_paren);
  TEST(test_move_before_close_paren);

  TEST(test_chop_at_open_paren);
  TEST_MAIN_REPORT;
}
