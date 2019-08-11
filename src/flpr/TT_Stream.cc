/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file TT_Stream.cc
*/

#include "flpr/TT_Stream.hh"
#include <iostream>
#include <stdexcept>

#define ll_ ll_tt_range_.ll()

namespace FLPR {
void TT_Stream::e_expect_tok(int tok_found, int tok_expect) const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  std::cerr << "rewrite: expecting token ";
  Syntax_Tags::print(std::cerr, tok_expect);
  std::cerr << ", but got ";
  Syntax_Tags::print(std::cerr, tok_found);
  std::cerr << " at " << filename << ':' << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    std::cerr << ':' << curr_tt().start_pos << std::endl;
  else
    std::cerr << ":?" << std::endl;
  std::cerr << ll_;
  std::exit(5);
}

void TT_Stream::e_expect_eol() const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  std::cerr << "rewrite: expecting end-of-line at" << filename << ':'
            << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    std::cerr << ':' << curr_tt().start_pos << std::endl;
  else
    std::cerr << ":?" << std::endl;
  std::cerr << ll_;
  std::exit(5);
}

void TT_Stream::e_expect_id(int tok_found) const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  std::cerr << "rewrite: expecting an identifier, but got ";
  Syntax_Tags::print(std::cerr, tok_found);
  std::cerr << " at " << filename << ':' << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    std::cerr << ':' << curr_tt().start_pos << std::endl;
  else
    std::cerr << ":?" << std::endl;
  std::cerr << ll_;
  std::exit(5);
}

void TT_Stream::e_general(char const *const errmsg) const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  std::cerr << "rewrite: at " << filename << ':' << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    std::cerr << ':' << curr_tt().start_pos;
  else
    std::cerr << ":?";

  std::cerr << ", general error:\n" << errmsg << '\n';
  std::cerr << ll_;
  std::exit(5);
}

void TT_Stream::w_general(char const *const warnmsg) const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  std::cerr << "rewrite: at " << filename << ':' << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    std::cerr << ':' << curr_tt().start_pos;
  else
    std::cerr << ":?";

  std::cerr << ", warning:\n" << warnmsg << '\n';
  std::cerr << ll_;
}

// This is a utility function used to burn through a substring
// delimited by parens, but perhaps containing nested paren
// substrings.
bool TT_Stream::move_to_close_paren() {
  expect_tok(Syntax_Tags::TK_PARENL);
  consume();
  int nesting_depth = 1;
  while (nesting_depth > 0) {
    if (Syntax_Tags::TK_PARENR == peek())
      nesting_depth -= 1;
    else if (Syntax_Tags::TK_PARENL == peek())
      nesting_depth += 1;
    else if (Syntax_Tags::BAD == peek())
      break;
    consume();
  }
  return nesting_depth == 0;
}

// This is a utility function used to burn through a substring
// delimited by parens, but perhaps containing nested paren
// substrings.
bool TT_Stream::move_before_close_paren() {
  expect_tok(Syntax_Tags::TK_PARENL);
  consume();
  int nesting_depth = 1;
  while (nesting_depth > 0) {
    if (Syntax_Tags::TK_PARENR == peek())
      nesting_depth -= 1;
    else if (Syntax_Tags::TK_PARENL == peek())
      nesting_depth += 1;
    else if (Syntax_Tags::BAD == peek())
      break;
    if (nesting_depth > 0)
      consume();
  }
  return nesting_depth == 0;
}

bool TT_Stream::move_to_open_paren() {
  if (curr() != Syntax_Tags::TK_PARENR) {
    std::cout << "not on parenr" << std::endl;
    return false;
  }
  int nesting_depth = 1;
  while (nesting_depth > 0) {
    put_back();
    const int tok = curr();
    if (Syntax_Tags::TK_PARENL == tok)
      nesting_depth -= 1;
    else if (Syntax_Tags::TK_PARENR == tok)
      nesting_depth += 1;
    else if (Syntax_Tags::BAD == tok)
      break;
  }
  return nesting_depth == 0;
}

// This is a utility function used to burn through a substring
// delimited by parens, but perhaps containing nested paren
// substrings.
bool TT_Stream::ignore_bracket_expr() {
  expect_tok(Syntax_Tags::TK_BRACKETL);
  int nesting_depth = 1;
  while (nesting_depth > 0) {
    if (Syntax_Tags::TK_BRACKETR == peek())
      nesting_depth -= 1;
    else if (Syntax_Tags::TK_BRACKETL == peek())
      nesting_depth += 1;
    else if (Syntax_Tags::BAD == peek())
      break;
    consume();
  }
  return nesting_depth == -1;
}

void TT_Stream::debug_print(std::ostream &os) const {
  std::string filename;
  if (!ll_.file_info)
    filename = std::string("(unknown file)");
  else
    filename = ll_.file_info->filename;
  os << "rewrite: at " << filename << ':' << ll_.start_line();
  if (curr() != Syntax_Tags::BAD)
    os << ':' << curr_tt().start_pos;
  else
    os << ":?";

  os << ", debug:\n";
  os << ll_;
}

} // namespace FLPR
