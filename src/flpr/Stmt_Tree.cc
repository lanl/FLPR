/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Stmt_Tree.cc
*/

#include "flpr/Stmt_Tree.hh"
#include <cassert>
#include <ostream>

namespace FLPR {
namespace Stmt {
void cover_branches(Stmt_Tree::reference st) {
  // Scan to first non-empty branch
  auto b1 = st.branches().begin();
  while (b1 != st.branches().end() && (*b1)->token_range.empty())
    ++b1;
  if (b1 == st.branches().end())
    return; // No valid ranges to cover
  st->token_range.clear();
  st->token_range.set_it((*b1)->token_range.it());
  st->token_range.push_back((*b1)->token_range);
  for (b1 = std::next(b1); b1 != st.branches().end(); ++b1) {
    st->token_range.push_back((*b1)->token_range);
  }
}

void hoist_back(Stmt_Tree &t, Stmt_Tree &&donor) {
  if (!donor)
    return;
  if ((*donor)->syntag == Syntax_Tags::HOIST) {
    for (auto &&b : donor->branches()) {
      auto new_loc = t->branches().emplace(t->branches().end(), std::move(b));
      new_loc->link(new_loc, t.root());
    }
  } else {
    t.graft_back(std::move(donor));
  }
}

std::ostream &operator<<(std::ostream &os, ST_Node_Data const &nd) {
  return Syntax_Tags::print(os, nd.syntag);
}

int get_label_do_label(Stmt_Tree const &t) {
  auto c = t.ccursor();
  if (FLPR::Syntax_Tags::SG_DO_STMT == c->syntag)
    c.down();
  if (FLPR::Syntax_Tags::SG_LABEL_DO_STMT != c->syntag) {
    return 0;
  }
  c.down().next().down();
  assert(FLPR::Syntax_Tags::SG_INT_LITERAL_CONSTANT == c->syntag);
  assert(c->token_range.size() == 1);
  return std::stoi(c->token_range.front().text());
}

} // namespace Stmt
} // namespace FLPR
