/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Prgm_Tree.hh
*/
#ifndef FLPR_PRGM_TREE_HH
#define FLPR_PRGM_TREE_HH 1

#include "flpr/LL_Stmt.hh"
#include "flpr/Stmt_Tree.hh"
#include "flpr/Syntax_Tags.hh"
#include "flpr/Tree.hh"

#include <optional>
#include <ostream>

namespace FLPR {
namespace Prgm {

//! The contents of each \c Prgm_Tree node
class Prgm_Node_Data {
public:
  using Stmt_Tree = FLPR::LL_Stmt::Stmt_Tree;
  using Stmt_Range = FLPR::SL_Range<FLPR::LL_Stmt>;

  //! default constructor
  constexpr Prgm_Node_Data() noexcept
      : syntag_{Syntax_Tags::UNKNOWN}, stmt_range_{}, stmt_data_(std::nullopt) {
  }
  //! interior node constructor
  constexpr explicit Prgm_Node_Data(int const syntag) noexcept
      : syntag_{syntag}, stmt_range_{}, stmt_data_(std::nullopt) {}
  //! leaf node constructor associated with a statement
  Prgm_Node_Data(int const syntag,
                 FLPR::LL_STMT_SEQ::iterator const &ll_stmt_it)
      : syntag_{syntag}, stmt_range_{ll_stmt_it},
        stmt_data_(std::in_place, ll_stmt_it) {}

  constexpr int syntag() const { return syntag_; }
  Stmt_Range &stmt_range() noexcept { return stmt_range_; }

  constexpr bool is_stmt() const noexcept { return stmt_data_.has_value(); }
  Stmt_Tree &stmt_tree() { return stmt_data_->stmt_tree(); }
  Stmt_Tree const &stmt_tree() const { return stmt_data_->stmt_tree(); }
  LL_Stmt &ll_stmt() { return stmt_data_->ll_stmt(); }
  LL_Stmt const &ll_stmt() const { return stmt_data_->ll_stmt(); }
  FLPR::LL_STMT_SEQ::iterator ll_stmt_iter() const noexcept {
    return stmt_data_->ll_stmt_iter();
  }

private:
  //! The Syntax_Tags::Tags associated with this (sub)tree
  int syntag_;
  //! Track the range of LL_Stmt's covered by this node
  Stmt_Range stmt_range_;

  class Stmt_Data {
  public:
    Stmt_Data() = delete;
    Stmt_Data(Stmt_Data const &) = delete;
    constexpr Stmt_Data(FLPR::LL_STMT_SEQ::iterator const &ll_stmt_iter)
        : ll_stmt_iter_{ll_stmt_iter} {}
    FLPR::LL_Stmt const &ll_stmt() const noexcept { return *ll_stmt_iter_; }
    FLPR::LL_Stmt &ll_stmt() noexcept { return *ll_stmt_iter_; }
    FLPR::LL_STMT_SEQ::iterator ll_stmt_iter() const noexcept {
      return ll_stmt_iter_;
    }
    Stmt_Tree const &stmt_tree() const noexcept {
      return ll_stmt_iter_->stmt_tree();
    }
    Stmt_Tree &stmt_tree() noexcept { return ll_stmt_iter_->stmt_tree(); }

  private:
    FLPR::LL_STMT_SEQ::iterator ll_stmt_iter_;
  };

  //! Data for leaf-nodes associated with statements
  std::optional<Stmt_Data> stmt_data_;
};

std::ostream &operator<<(std::ostream &os, Prgm_Node_Data const &pn);
} // namespace Prgm
} // namespace FLPR

#endif
