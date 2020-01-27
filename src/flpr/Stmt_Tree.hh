/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Stmt_Tree.hh
*/
#ifndef FLPR_STMT_TREE_HH
#define FLPR_STMT_TREE_HH 1

#include "flpr/LL_TT_Range.hh"
#include "flpr/Syntax_Tags.hh"
#include "flpr/Tree.hh"
#include <ostream>

namespace FLPR {
namespace Stmt {

//! The contents of each \c Stmt_Tree node
struct ST_Node_Data {
  ST_Node_Data() : syntag{Syntax_Tags::UNKNOWN}, token_range{} {}
  explicit ST_Node_Data(int const syntag) : syntag{syntag}, token_range{} {}
  ST_Node_Data(int const syntag, LL_TT_Range const &token_range)
      : syntag{syntag}, token_range{token_range} {}
  ST_Node_Data(int const syntag, LL_TT_Range &&token_range)
      : syntag{syntag}, token_range{std::move(token_range)} {}

  //! The Syntax_Tags::Tags associated with this (sub)tree
  int syntag;
  //! The Logical_Line and range of tokens covered by this (sub)tree
  LL_TT_Range token_range;
};

std::ostream &operator<<(std::ostream &os, ST_Node_Data const &nd);

/*! A parse tree/concrete syntax tree for the components of individual
    statments.  Compare to Pgrm_Tree, which organizes statements into blocks of
    various sorts */
using Stmt_Tree = Tree<ST_Node_Data>;

//! Update the (*st)->token_range to cover the token_ranges of the branches
void cover_branches(Stmt_Tree::reference st);

//! Treat donor as a list, rather than a tree, in certain circumstances
/*! If the root of the donor tree is marked with Syntax_Tags::HOIST, append the
    branches of donor to t.branches(), otherwise t.graft_back(donor) */
void hoist_back(Stmt_Tree &t, Stmt_Tree &&donor);

//! Return the label from a label-do-stmt
int get_label_do_label(Stmt_Tree const &t);

} // namespace Stmt

} // namespace FLPR

#endif
