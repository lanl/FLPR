/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
   Testing for the Tree class
*/
#include "flpr/Syntax_Tags.hh"
#include "flpr/Tree.hh"
#include "test_helpers.hh"
#include <memory>

using FLPR::Tree;

/* -------------------------- The unit tests ---------------------------- */

struct A {
  A() : a{-1}, b{-1} {}
  A(int a, int b) : a{a}, b{b} {}
  int a, b;
};

bool ctor_test_helper(Tree<A> const &t) {
  TEST_TRUE(t);
  TEST_TRUE(t->is_root());
  TEST_TRUE(t->is_leaf());
  TEST_FALSE(t->is_fork());
  TEST_TRUE(t->self() == t.root());
  TEST_TRUE(t->trunk() == t->self());
  TEST_INT((*t)->a, 1);
  TEST_INT((*t)->b, 2);
  return true;
}

bool ctor_default() {
  Tree<A> t;
  TEST_TRUE(!t);
  TEST_TRUE(t.empty());
  return true;
}

bool ctor_ref_val() {
  A a(1, 2);
  Tree<A> t(a);
  TEST_FALSE(t.empty());
  TEST_INT(t.size(), 1);
  return ctor_test_helper(t);
}

bool ctor_move_val() {
  A a(1, 2);
  Tree<A> t(std::move(a));
  return ctor_test_helper(t);
}

bool ctor_fwd_val() {
  Tree<A> t(1, 2);
  return ctor_test_helper(t);
}

bool ctor_move_tree() {
  Tree<A> t1(1, 2);
  Tree<A> t2(std::move(t1));
  if (!ctor_test_helper(t2))
    return false;
  TEST_TRUE(!t1);
  return true;
}

bool assign_move_tree() {
  Tree<A> t1(1, 2);
  Tree<A> t2;
  if (!ctor_test_helper(t1))
    return false;
  TEST_TRUE(!t2);
  t2 = std::move(t1);
  if (!ctor_test_helper(t2))
    return false;
  TEST_TRUE(!t1);
  return true;
}

bool clear_one_node() {
  auto p = std::make_shared<int>(1);
  TEST_INT(p.use_count(), 1);
  Tree<std::shared_ptr<int>> t(p);
  TEST_INT(p.use_count(), 2);
  TEST_INT(***t, 1); // Tree->Tree_Node->unique_ptr->1
  t.clear();
  TEST_INT(p.use_count(), 1);
  TEST_TRUE(!t);
  return true;
}

bool graft_back() {
  auto p1 = std::make_shared<int>(1);
  TEST_INT(p1.use_count(), 1);
  Tree<std::shared_ptr<int>> t(p1);
  TEST_INT(p1.use_count(), 2);

  Tree<std::shared_ptr<int>> sub_tree1(p1);
  TEST_INT(p1.use_count(), 3);

  t.graft_back(sub_tree1);
  TEST_TRUE(!sub_tree1);
  TEST_TRUE(t->is_root());
  TEST_TRUE(t->is_fork());
  TEST_FALSE(t->is_leaf());
  TEST_INT(t->num_branches(), 1);
  auto root_iter = t.root();
  auto child_iter = t->branches().begin();
  TEST_TRUE(root_iter == child_iter->trunk());

  auto p2 = std::make_shared<int>(2);
  Tree<std::shared_ptr<int>> sub_tree2(p2);
  TEST_INT(p2.use_count(), 2);
  t.graft_back(sub_tree2);
  TEST_INT(p2.use_count(), 2);
  TEST_TRUE(!sub_tree2);
  TEST_TRUE(t->is_root());
  TEST_TRUE(t->is_fork());
  TEST_FALSE(t->is_leaf());
  TEST_INT(t->num_branches(), 2);
  TEST_INT(t.size(), 3);

  TEST_TRUE(root_iter == t.root()); // shouldn't change
  TEST_TRUE(child_iter ==
            t->branches().begin()); // shouldn't change on push_back
  TEST_INT(**(t->branches().front()), 1);
  TEST_INT(**(t->branches().back()), 2);
  t.clear();
  TEST_INT(p1.use_count(), 1);
  TEST_INT(p2.use_count(), 1);

  return true;
}


bool cursor() {
  Tree<int> t{0};
  t.graft_back(Tree<int>{1});
  t.graft_back(Tree<int>{2});
  TEST_INT(t.size(), 3);
  TEST_INT(t->num_branches(), 2);
  auto c = t.cursor();
  TEST_INT(*c, 0);
  TEST_TRUE(c.is_root());
  TEST_FALSE(c.is_leaf());
  TEST_TRUE(c.is_fork());
  TEST_INT(c.num_branches(), 2);
  TEST_FALSE(c.has_up());
  TEST_TRUE(c.has_down());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.try_next());


  TEST_TRUE(c.try_down());
  TEST_INT(*c, 1);
  TEST_TRUE(c.has_up());
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());
  TEST_TRUE(c.is_leaf());
  TEST_FALSE(c.is_root());
  
  TEST_TRUE(c.try_next());
  TEST_INT(*c, 2);
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_down());
  TEST_TRUE(c.is_leaf());
  TEST_FALSE(c.is_root());

  TEST_FALSE(c.try_next());
  c.up();
  TEST_INT(*c, 0);
  
  return true;
}

bool const_cursor() {
  Tree<int> t{0};
  t.graft_back(Tree<int>{1});
  t.graft_back(Tree<int>{2});
  TEST_INT(t.size(), 3);
  TEST_INT(t->num_branches(), 2);
  auto c = t.ccursor();
  TEST_INT(*c, 0);
  TEST_TRUE(c.is_root());
  TEST_FALSE(c.is_leaf());
  TEST_TRUE(c.is_fork());
  TEST_INT(c.num_branches(), 2);
  TEST_FALSE(c.has_up());
  TEST_TRUE(c.has_down());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_prev());
  TEST_FALSE(c.try_next());


  TEST_TRUE(c.try_down());
  TEST_INT(*c, 1);
  TEST_TRUE(c.has_up());
  TEST_FALSE(c.has_prev());
  TEST_TRUE(c.has_next());
  TEST_FALSE(c.has_down());
  TEST_TRUE(c.is_leaf());
  TEST_FALSE(c.is_root());
  
  TEST_TRUE(c.try_next());
  TEST_INT(*c, 2);
  TEST_TRUE(c.has_up());
  TEST_TRUE(c.has_prev());
  TEST_FALSE(c.has_next());
  TEST_FALSE(c.has_down());
  TEST_TRUE(c.is_leaf());
  TEST_FALSE(c.is_root());

  TEST_FALSE(c.try_next());
  c.up();
  TEST_INT(*c, 0);
  
  return true;
}


bool graft_front() {
  auto p1 = std::make_shared<int>(1);
  TEST_INT(p1.use_count(), 1);
  Tree<std::shared_ptr<int>> t(p1);
  TEST_INT(p1.use_count(), 2);

  Tree<std::shared_ptr<int>> sub_tree1(p1);
  TEST_INT(p1.use_count(), 3);

  t.graft_front(sub_tree1);
  TEST_TRUE(!sub_tree1);
  TEST_TRUE(t->is_root());
  TEST_TRUE(t->is_fork());
  TEST_FALSE(t->is_leaf());
  TEST_INT(t->num_branches(), 1);
  TEST_INT(t.size(), 2);

  auto root_iter = t.root();
  auto child_iter = t->branches().begin();
  TEST_TRUE(root_iter == child_iter->trunk());

  auto p2 = std::make_shared<int>(2);
  Tree<std::shared_ptr<int>> sub_tree2(p2);
  TEST_INT(p2.use_count(), 2);
  t.graft_front(sub_tree2);
  TEST_INT(p2.use_count(), 2);
  TEST_TRUE(!sub_tree2);
  TEST_TRUE(t->is_root());
  TEST_TRUE(t->is_fork());
  TEST_FALSE(t->is_leaf());
  TEST_INT(t->branches().size(), 2);

  TEST_TRUE(root_iter == t.root()); // shouldn't change
  TEST_FALSE(child_iter ==
             t->branches().begin()); // should change on push_front
  TEST_INT(**(t->branches().front()), 2);
  TEST_INT(**(t->branches().back()), 1);
  t.clear();
  TEST_INT(p1.use_count(), 1);
  TEST_INT(p2.use_count(), 1);

  return true;
}

int main() {
  TEST_MAIN_DECL;

  TEST(ctor_default);
  TEST(ctor_move_val);
  TEST(ctor_ref_val);
  TEST(ctor_fwd_val);
  TEST(ctor_move_tree);
  TEST(assign_move_tree);
  TEST(clear_one_node);
  TEST(graft_back);
  TEST(graft_front);
  TEST(cursor);
  TEST(const_cursor);
  
  TEST_MAIN_REPORT;
}
