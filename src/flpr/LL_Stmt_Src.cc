/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_Stmt_Src.cc
*/

#include "flpr/LL_Stmt_Src.hh"

namespace FLPR {
void LL_Stmt_Src::refill_() {
  const bool make_macro_stmts{false};
  LL_Stmt::LL_IT_SEQ prefix_lines;
  buf_.clear();

  // Advance through non-statement prefix lines
  while (it_ &&
         (!make_macro_stmts || it_->cat != LineCat::MACRO || it_->suppress) &&
         it_->stmts().empty()) {
    assert(it_->stmts().empty());
    if (!it_->suppress)
      prefix_lines.push_back(it_);
    it_.advance();
  }

  // Now at a non-trivial LL, a Macro, OR the end of the input
  if (it_) {
    assert(!it_->suppress);
    if (it_->cat == LineCat::MACRO) {
      // This will create an "empty()" LL_Stmt with prefix_lines
      // with preceding trivial LL and the MACRO in prefix_lines.back().
      prefix_lines.push_back(it_);
      buf_.emplace_back();
      buf_.back().prefix_lines.swap(prefix_lines);
    } else {
      int label = it_->label;
      int compound = (it_->stmts().size() > 1) ? 1 : 0;

      for (auto &srange : it_->stmts()) {
        buf_.emplace_back(it_, srange, label, compound);
        compound += 1;
        // Assign any trivial prefix to the first statement of
        // a compound
        if (!prefix_lines.empty())
          buf_.back().prefix_lines.swap(prefix_lines);
        // Clear the label, as it only applies to the first statement
        label = 0;
      }
    }
    it_.advance();
  } else // at end of input
  {
    if (!prefix_lines.empty()) {
      // This used to create  an "empty()" LL_Stmt with prefix_lines
      // containing the end-of-file comments, but that really isn't a statement.
      // buf_.emplace_back();
      // buf_.back().prefix_lines.swap(prefix_lines);
    }
  }

  curr_ = buf_.begin();
}

#if 0
void LL_Stmt_Src::push_front(value_type const &s) { buf_.insert(curr_, s); }
#endif

} // namespace FLPR
