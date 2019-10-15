/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Test the mechanics of Stmt_Tree
*/

#include "flpr/parse_stmt.hh"
#include "parse_helpers.hh"

using namespace FLPR;
using FLPR::Stmt::Stmt_Tree;

#define PARSE(SP, S)                                                           \
  LL_Helper l({S});                                                            \
  FLPR::TT_Stream ts{l.stream1()};                                             \
  Stmt_Tree st = FLPR::Stmt::SP(ts)

#define FIRST_LL l.ll_stmts().front().ll()

bool one_tok() {
  PARSE(contains_stmt, "contains");
  auto c{st.cursor()};
  TEST_INT(c.node().num_branches(), 1);
  TEST_TAG(c->syntag, SG_CONTAINS_STMT);
  c.down();
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_CONTAINS);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_CONTAINS);
  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_CONTAINS);
  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_CONTAINS);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());
  return true;
}

bool two_tok() {
  PARSE(end_if_stmt, "end if");
  auto c{st.cursor()};
  TEST_INT(c.node().num_branches(), 2);
  TEST_TAG(c->syntag, SG_END_IF_STMT);
  c.down();
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_TRUE(c.has_up());
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_END);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_END);
  c.next();
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_TRUE(c.has_up());
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_IF);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_IF);
  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 2);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_END);
  TEST_TAG(c->token_range.back().token, KW_IF);

  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_END);
  TEST_TAG(ti++->token, KW_IF);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());
  return true;
}

// Use a higher-level parser
bool buried_one_tok() {
  PARSE(action_stmt, "return");
  auto c{st.cursor()};
  TEST_INT(c.node().num_branches(), 1);
  TEST_TAG(c->syntag, SG_ACTION_STMT);
  c.down();
  TEST_INT(c.node().num_branches(), 1);
  TEST_TAG(c->syntag, SG_RETURN_STMT);
  c.down();
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_RETURN);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_RETURN);
  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_RETURN);
  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_RETURN);

  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_RETURN);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());

  return true;
}

bool opt_tok() {
  PARSE(end_if_stmt, "end if a_name");
  auto c{st.cursor()};
  TEST_INT(c.node().num_branches(), 3);
  TEST_TAG(c->syntag, SG_END_IF_STMT);
  c.down();
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_TRUE(c.has_up());
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_END);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_END);
  c.next();
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_TRUE(c.has_up());
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_IF);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_IF);
  c.next();
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_TRUE(c.has_up());
  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, TK_NAME);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, TK_NAME);
  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 3);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));

  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_END);
  TEST_TAG(ti++->token, KW_IF);
  TEST_TAG(ti++->token, TK_NAME);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());

  TEST_TAG(c->token_range.front().token, KW_END);
  TEST_TAG(std::next(c->token_range.begin())->token, KW_IF);
  TEST_TAG(c->token_range.back().token, TK_NAME);
  return true;
}

bool function_stmt() {
  PARSE(function_stmt, "function f(a)");
  auto c{st.cursor()};
  TEST_TAG(c->syntag, SG_FUNCTION_STMT);
  std::cout << "STMT: " << st << std::endl;
  TEST_INT(c.node().num_branches(), 6);

  TEST_TRUE(c.is_root());
  TEST_TRUE(c.is_fork());
  TEST_FALSE(c.is_leaf());
  TEST_FALSE(c.has_up());
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_TRUE(c.has_down());

  // prefix
  c.down();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, SG_PREFIX);
  TEST_TRUE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 0);

  // FUNCTION
  c.next();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, KW_FUNCTION);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, KW_FUNCTION);

  // NAME
  c.next();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, TK_NAME);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, TK_NAME);

  // (
  c.next();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, TK_PARENL);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, TK_PARENL);

  // dummy-arg-name-list
  c.next();
  TEST_FALSE(c.is_root());
  TEST_TRUE(c.is_fork());
  TEST_FALSE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_TRUE(c.has_down());

  TEST_INT(c.node().num_branches(), 1);
  TEST_TAG(c->syntag, SG_DUMMY_ARG_NAME_LIST);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));

  // name
  c.down();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, TK_NAME);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, TK_NAME);
  c.up();

  // )
  c.next();
  TEST_FALSE(c.is_root());
  TEST_FALSE(c.is_fork());
  TEST_TRUE(c.is_leaf());
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_down());

  TEST_INT(c.node().num_branches(), 0);
  TEST_TAG(c->syntag, TK_PARENR);
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 1);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));
  TEST_TAG(c->token_range.front().token, TK_PARENR);

  c.up();
  TEST_FALSE(c->token_range.empty());
  TEST_INT(c->token_range.size(), 5);
  TEST_EQ(&(FIRST_LL), &(l.lines().front()));
  TEST_EQ(&(c->token_range.ll()), &(l.lines().front()));

  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_FUNCTION);
  TEST_TAG(ti++->token, TK_NAME);
  TEST_TAG(ti++->token, TK_PARENL);
  TEST_TAG(ti++->token, TK_NAME);
  TEST_TAG(ti++->token, TK_PARENR);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());

  return true;
}

bool type_declaration_stmt() {
  PARSE(type_declaration_stmt, "integer::f,a");
  auto c{st.cursor()};
  TEST_TAG(c->syntag, SG_TYPE_DECLARATION_STMT);
  TEST_INT(c.node().num_branches(), 2); // type-decl-attr-seq entity-decl-list
  c.down();
  TEST_TAG(c->syntag, SG_TYPE_DECL_ATTR_SEQ);
  TEST_INT(c.node().num_branches(), 2);  // "INTEGER" "::"
  
  c.down();
  // INTEGER
  TEST_TAG(c->syntag, SG_DECLARATION_TYPE_SPEC);
  TEST_TRUE(c.has_next());
  TEST_INT(c.node().num_branches(), 1);
  c.down();
  TEST_TAG(c->syntag, SG_INTRINSIC_TYPE_SPEC);
  TEST_INT(c.node().num_branches(), 1);
  c.down();
  TEST_TAG(c->syntag, SG_INTEGER_TYPE_SPEC);
  TEST_INT(c.node().num_branches(), 1);
  c.down();
  TEST_TAG(c->syntag, KW_INTEGER);
  TEST_TRUE(c.is_leaf());
  c.up(3);

  // ::
  c.next();
  TEST_TAG(c->syntag, TK_DBL_COLON);
  TEST_TRUE(c.is_leaf());
  TEST_FALSE(c.has_next());

  c.up();
  TEST_TRUE(c.has_next());
  c.next();

  // f,a
  TEST_TAG(c->syntag, SG_ENTITY_DECL_LIST);
  TEST_TRUE(c.is_fork());
  TEST_FALSE(c.has_next());
  TEST_INT(c.node().num_branches(), 3);
  c.down();
  // f
  TEST_TAG(c->syntag, SG_ENTITY_DECL);
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_INT(c.node().num_branches(), 1);
  c.down();
  TEST_TAG(c->syntag, TK_NAME);
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_TRUE(c.is_leaf());
  c.up();
  // ,
  c.next();
  TEST_TAG(c->syntag, TK_COMMA);
  TEST_TRUE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_TRUE(c.is_leaf());
  // a
  c.next();
  TEST_TAG(c->syntag, SG_ENTITY_DECL);
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_INT(c.node().num_branches(), 1);
  c.down();
  TEST_TAG(c->syntag, TK_NAME);
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_TRUE(c.is_leaf());

  c.up(3);

  TEST_TAG(c->syntag, SG_TYPE_DECLARATION_STMT);
  auto ti = c->token_range.begin();
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().begin());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().begin());
  TEST_TAG(ti++->token, KW_INTEGER);
  TEST_TAG(ti++->token, TK_DBL_COLON);
  TEST_TAG(ti++->token, TK_NAME);
  TEST_TAG(ti++->token, TK_COMMA);
  TEST_TAG(ti++->token, TK_NAME);
  TEST_EQ_NODISPLAY(ti, c->token_range.end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().stmts().front().end());
  TEST_EQ_NODISPLAY(ti, l.lines().front().fragments().end());

  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(one_tok);
  TEST(two_tok);
  TEST(buried_one_tok);
  TEST(opt_tok);
  TEST(function_stmt);
  TEST(type_declaration_stmt);
  TEST_MAIN_REPORT;
}
