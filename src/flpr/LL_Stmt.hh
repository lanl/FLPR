/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_Stmt.hh
*/

#ifndef FLPR_LL_STMT_HH
#define FLPR_LL_STMT_HH 1

#include "flpr/LL_TT_Range.hh"
#include "flpr/Safe_List.hh"
#include "flpr/Stmt_Tree.hh"
#include <ostream>

namespace FLPR {
//! Identify a LL_TT_Range that describes a Fortran statement
class LL_Stmt : public LL_TT_Range {
public:
  using Stmt_Tree = FLPR::Stmt::Stmt_Tree;

public:
  LL_Stmt()
      : LL_TT_Range(), label_{0}, compound_{-1}, hook_{nullptr},
        stmt_syntag_{Syntax_Tags::UNKNOWN} {};
  LL_Stmt(LL_IT line_ref, TT_Range r, int label, int compound)
      : LL_TT_Range(line_ref, r), label_{label}, compound_{compound},
        hook_{nullptr}, stmt_syntag_{Syntax_Tags::UNKNOWN} {}

  void update_range(LL_Stmt &&src) {
    LL_TT_Range::operator=(src);
    compound_ = src.compound_;
    label_ = src.label_;
    stmt_tree_.clear(); // It is bad at this point
  }

  constexpr bool has_label() const { return label_ > 0; }
  constexpr int label() const { return label_; }
  //! Note that this does NOT update the label on ll()!!!!
  constexpr void cache_new_label_value(int newval) { label_ = newval; }

  //! See commentary on compound_ member
  constexpr void set_compound(int newval) { compound_ = newval; }
  constexpr int is_compound() const { return compound_; }

  std::ostream &print_me(std::ostream &, bool const print_prefix = true) const;
  virtual std::ostream &print(std::ostream &os) const override {
    return print_me(os, true);
  }
  constexpr bool equal(LL_Stmt const &s) const {
    return LL_TT_Range::equal(s) && label_ == s.label_ &&
           compound_ == s.compound_;
  }
  constexpr void set_hook(void *ptr) noexcept { hook_ = ptr; }
  constexpr void unhook() noexcept { hook_ = nullptr; }
  constexpr void *get_hook() const { return hook_; }
  constexpr bool has_hook() const noexcept { return hook_ != nullptr; }

  //! Re-indent the prefix lines and statement
  /*!
    Returns true if any spacing was modified, false otherwise.
    \param[in] spaces the starting column (index-0) of the statement
    \param[in] continued_offset the additional offset for continued lines
  */
  bool set_leading_spaces(int const spaces, int const continued_offset);
  int get_leading_spaces() const noexcept { return ll().get_leading_spaces(); }

  Stmt_Tree const &stmt_tree() const noexcept {
    if (stmt_tree_.empty()) {
      bool res = rebuild_tree_();
      assert(res);
    }
    return stmt_tree_;
  }
  Stmt_Tree &stmt_tree() noexcept {
    if (stmt_tree_.empty()) {
      bool res = rebuild_tree_();
      assert(res);
    }
    return stmt_tree_;
  }
  void set_stmt_tree(Stmt_Tree &&stmt_tree) {
    stmt_tree_ = std::move(stmt_tree);
    extract_tree_tag_();
  }
  void drop_stmt_tree() { stmt_tree_.clear(); }
  void reset_stmt_tree() {
    stmt_tree_.clear();
    extract_tree_tag_();
  }
  void set_stmt_syntag(int syntag) {
    /* This overrules anything in the tree */
    if (syntag != stmt_syntag_) {
      stmt_tree_.clear();
    }
    stmt_syntag_ = syntag;
  }

  //! Produce a meaningful tag for statements, BAD otherwise
  int stmt_tag(bool look_inside_if_stmt) const;

  size_t prefix_size() const { return prefix_lines.size(); }
  LL_IT prefix_ll_begin() const {
    if (prefix_lines.empty()) {
      return it();
    }
    return prefix_lines.front();
  }
  LL_IT prefix_ll_end() const {
    if (prefix_lines.empty()) {
      return it();
    }
    /* This should be the same as stmt_ll(), unless we're moving the prefix from
       one LL_Stmt to another */
    return std::next(prefix_lines.back());
  }
  LL_IT stmt_ll() const { return it(); }
  constexpr int syntax_tag() const { return stmt_syntag_; }

public:
  //! A sequence of non-Fortran Logical_Lines before this statement.
  /*!
    These are attached to a statement so that, as a list of statements
    gets reordered, the comments and macros move with the statements.
  */
  using LL_IT_SEQ = std::vector<LL_IT>;
  LL_IT_SEQ prefix_lines;

private:
  int label_; // = 0 -> no label.  Needs to sync with Logical_Line
  //! Statement sequence number in a compound logical line
  /*! 0 -> single statement,
      1, 2, ... -> order of this statement in a compound line
  */
  int compound_;

  /* This is used to allow Prgm_Tree to install a generic uplink to a Tree node.
     We don't know what the template parameter on Prgm_Tree is going to be, so
     we use void* instead. */
  void *hook_;
  mutable Stmt_Tree stmt_tree_;
  mutable int stmt_syntag_;

private:
  void extract_tree_tag_() const {
    if (stmt_tree_.empty())
      stmt_syntag_ = Syntax_Tags::UNKNOWN;
    else {
      auto c = stmt_tree_.ccursor();
      if (c->syntag == Syntax_Tags::SG_ACTION_STMT)
        c.down();
      stmt_syntag_ = c->syntag;
    }
  }
  bool rebuild_tree_() const;
};

//! Container for a sequence of LL_Stmts
using LL_STMT_SEQ = Safe_List<LL_Stmt>;

inline std::ostream &operator<<(std::ostream &os, LL_Stmt const &s) {
  return s.print_me(os, true);
}

} // namespace FLPR

#endif
