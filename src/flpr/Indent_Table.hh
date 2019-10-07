/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Indent_Table.hh

  Manage indent spacing for Prgm Grammar (PG_) syntax tags.
*/

#ifndef INDENT_TABLE_HH
#define INDENT_TABLE_HH 1

#include "flpr/Syntax_Tags.hh"
#include <array>
#include <cassert>

namespace FLPR {
class Indent_Table {
public:
  Indent_Table() : continued_{2} { offset_.fill(0); }
  void apply_constant_indent(int const spaces);
  void apply_constant_fixed_indent(int const spaces);
  void apply_emacs_indent();
  constexpr static bool begin_end_construct(int const syntag);
  constexpr void set_indent(int const syntag, int const spaces) noexcept {
    spaces_(syntag) = spaces;
  }
  constexpr int get_indent(int const syntag) const noexcept {
    return spaces_(syntag);
  }
  constexpr int operator[](int const syntag) const noexcept {
    return spaces_(syntag);
  }
  constexpr void set_continued_offset(int const spaces) noexcept {
    continued_ = spaces;
  }
  constexpr int continued_offset() const noexcept { return continued_; }

private:
  /* Find the extent of the Program Grammar syntax tags */
  constexpr static int PG_BEGIN = Syntax_Tags::pg_begin_tag();
  constexpr static int PG_END = Syntax_Tags::pg_end_tag();
  constexpr static int PG_COUNT = PG_END - PG_BEGIN;

  /* Storage for indent spaces offsets by PG syntag */
  std::array<int, PG_COUNT> offset_;
  /* Additional spacing for continued lines */
  int continued_;

private:
  /* Accessors that perform map from syntax tag to offset_ index */
  constexpr int &spaces_(int const syntag) noexcept {
    assert(PG_BEGIN <= syntag && PG_END > syntag);
    return offset_[syntag - PG_BEGIN];
  }
  constexpr int spaces_(int const syntag) const noexcept {
    assert(PG_BEGIN <= syntag && PG_END > syntag);
    return offset_[syntag - PG_BEGIN];
  }
};

// You don't need to add ones where begin/end just surround a block
constexpr bool Indent_Table::begin_end_construct(int const syntag) {
  switch (syntag) {
  case Syntax_Tags::PG_DERIVED_TYPE_DEF:
  case Syntax_Tags::PG_ENUM_DEF:
  case Syntax_Tags::PG_CASE_CONSTRUCT:
    return true;
  }
  return false;
}
} // namespace FLPR
#endif
