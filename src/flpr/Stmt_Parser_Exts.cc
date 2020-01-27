/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Stmt_Parser_Exts.cc
*/

#include "flpr/Stmt_Parser_Exts.hh"

namespace FLPR {
namespace Stmt {
Parser_Exts &get_parser_exts() noexcept {
  static Parser_Exts the_parser_exts;
  return the_parser_exts;
}

void Parser_Exts::register_action_stmt(stmt_parser ext) noexcept {
  action_exts_.push_back(ext);
}

void Parser_Exts::register_other_specification_stmt(stmt_parser ext) noexcept {
  other_specification_exts_.push_back(ext);
}

SP_Result Parser_Exts::parse_action_stmt(TT_Stream &ts) {
  auto ts_rewind_point = ts.mark();
  for (auto parser : action_exts_) {
    Stmt_Tree st{parser(ts)};
    if (st) {
      Stmt_Tree new_root{Syntax_Tags::SG_ACTION_STMT, (*st)->token_range};
      hoist_back(new_root, std::move(st));
      cover_branches(*new_root);
      return SP_Result{std::move(new_root), true};
    }
    ts.rewind(ts_rewind_point);
  }
  return SP_Result{Stmt_Tree{}, false};
}

SP_Result Parser_Exts::parse_other_specification_stmt(TT_Stream &ts) {
  auto ts_rewind_point = ts.mark();
  for (auto parser : other_specification_exts_) {
    Stmt_Tree st{parser(ts)};
    if (st) {
      Stmt_Tree new_root{Syntax_Tags::SG_OTHER_SPECIFICATION_STMT,
                         (*st)->token_range};
      hoist_back(new_root, std::move(st));
      cover_branches(*new_root);
      return SP_Result{std::move(new_root), true};
    }
    ts.rewind(ts_rewind_point);
  }
  return SP_Result{Stmt_Tree{}, false};
}

void Parser_Exts::clear() noexcept {
  action_exts_.clear();
  other_specification_exts_.clear();
}

} // namespace Stmt
} // namespace FLPR
