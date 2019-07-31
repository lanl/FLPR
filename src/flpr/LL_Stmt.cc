/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_Stmt.cc
*/

#include "flpr/LL_Stmt.hh"
#include "flpr/parse_stmt.hh"
#include <ostream>

#define DEBUG_PRINT 0

#if DEBUG_PRINT
#include <iostream>
#endif

namespace FLPR {

bool LL_Stmt::set_leading_spaces(int const spaces, int const continued_offset) {
  assert(spaces >= 0);
  bool changed = false;

  if (is_compound() < 2) {
    for (FLPR::LL_SEQ::iterator ll_it : prefix_lines) {
      changed |= ll_it->set_leading_spaces(spaces, continued_offset);
    }
    changed |= ll().set_leading_spaces(spaces, continued_offset);
  }
  return changed;
}

int LL_Stmt::stmt_tag(bool const look_inside_if_stmt) const {
  int retval;
  if (look_inside_if_stmt && Syntax_Tags::SG_IF_STMT == stmt_syntag_ &&
      !stmt_tree().empty()) {
    auto cursor = stmt_tree_.ccursor();
    assert(Syntax_Tags::SG_ACTION_STMT == cursor->syntag);
    cursor.down();
    assert(Syntax_Tags::SG_IF_STMT == cursor->syntag);
    cursor.down();
    assert(Syntax_Tags::KW_IF == cursor->syntag);
    cursor.next(4);
    assert(Syntax_Tags::SG_ACTION_STMT == cursor->syntag);
    cursor.down();
    retval = -cursor->syntag;
  } else {
    retval = stmt_syntag_;
  }
  return retval;
}

bool LL_Stmt::rebuild_tree_() const {
  if (Syntax_Tags::UNKNOWN == stmt_syntag_) {
#if DEBUG_PRINT
    std::cerr << "UNKNOWN stmt_syntag on ";
    print_me(std::cerr, false) << '\n';
#endif
    return false;
  }
  TT_Stream tts{*const_cast<LL_Stmt *>(this)};
  if (Stmt::is_action_stmt(stmt_syntag_)) {
    stmt_tree_ = Stmt::parse_stmt_dispatch(Syntax_Tags::SG_ACTION_STMT, tts);
  } else {
    stmt_tree_ = Stmt::parse_stmt_dispatch(stmt_syntag_, tts);
  }
  extract_tree_tag_();
#if DEBUG_PRINT
  if (stmt_tree_.empty()) {
    Syntax_Tags::print(std::cerr << "Parsing ", stmt_syntag_) << " failed on\n";
    print_me(std::cerr, false) << '\n';
  }
#endif
  return !stmt_tree_.empty();
}

std::ostream &LL_Stmt::print_me(std::ostream &os,
                                bool const print_prefix) const {
  if (empty())
    os << "? : <empty stmt>";
  else {
    std::string line_label;
    if (ll().file_info) {
      line_label = ll().file_info->filename;
    } else {
      line_label = "(unknown file)";
    }
    line_label.push_back(':');

    if (print_prefix) {
      for (auto const &it : prefix_lines) {
        os << line_label << it->start_line() << ":\n" << *it;
      }
    }

    os << line_label << linenum() << ": ";
    render(os, cbegin(), cend());
  }
  return os;
}
} // namespace FLPR
