/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Token_Text.cc
*/

#include "flpr/Token_Text.hh"
#include "flpr/Syntax_Tags.hh"
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace FLPR {

Token_Text::Token_Text()
    : token(Syntax_Tags::BAD), start_line(-1), start_pos(-1) {}

std::string const &Token_Text::lower() const {
  // Lazy construction
  if (lower_.empty()) {
    lower_.resize(text_.size());
    std::transform(text_.begin(), text_.end(), lower_.begin(),
                   [](unsigned char c) { return std::tolower(c); });
  }
  return lower_;
}

std::ostream &operator<<(std::ostream &os, Token_Text const &tt) {
  return Syntax_Tags::print(os, tt.token)
         << ":\"" << tt.text() << "\" (" << tt.start_line << '.' << tt.start_pos
         << ')';
}

void render(std::ostream &os, TT_SEQ::const_iterator beg,
            TT_SEQ::const_iterator end) {
  if (beg == end)
    return;
  TT_SEQ::const_iterator next = std::next(beg);
  for (; next != end; ++beg, ++next) {
    os << beg->text();
    int spaces = std::max(beg->post_spaces(), next->pre_spaces());
    if (spaces > 0)
      os << std::setw(spaces) << ' ';
  }
  os << beg->text();
}

void unkeyword(TT_SEQ::iterator beg, const TT_SEQ::iterator end, int first_N) {
  while (beg != end && first_N--) {
    if (Syntax_Tags::is_keyword(beg->token))
      beg->token = Syntax_Tags::TK_NAME;
    beg++;
  }
}

} // namespace FLPR
