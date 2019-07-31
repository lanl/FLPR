/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Manage indent spacing for Prgm Grammar (PG_) syntax tags.
*/

#ifndef INDENT_TABLE_HH
#define INDENT_TABLE_HH 1

#include <array>
#include <cassert>
#include <flpr/Syntax_Tags.hh>

class Indent_Table {
public:
  Indent_Table() : continued_{2} { offset_.fill(0); }
  inline void apply_constant_indent(int const spaces);
  inline void apply_constant_fixed_indent(int const spaces);
  inline void apply_emacs_indent();
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
  constexpr static int PG_BEGIN = FLPR::Syntax_Tags::pg_begin_tag();
  constexpr static int PG_END = FLPR::Syntax_Tags::pg_end_tag();
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

inline void Indent_Table::apply_constant_indent(int const spaces) {
  set_indent(FLPR::Syntax_Tags::PG_BLOCK, spaces);
  set_indent(FLPR::Syntax_Tags::PG_EXECUTION_PART, spaces);
  set_indent(FLPR::Syntax_Tags::PG_INTERFACE_SPECIFICATION, spaces);
  set_indent(FLPR::Syntax_Tags::PG_INTERNAL_SUBPROGRAM, spaces);
  set_indent(FLPR::Syntax_Tags::PG_MODULE_SUBPROGRAM, spaces);
  set_indent(FLPR::Syntax_Tags::PG_SPECIFICATION_PART, spaces);
  set_indent(FLPR::Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, spaces);

  for (int i = FLPR::Syntax_Tags::PG_000_LB + 1;
       i < FLPR::Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, spaces);
  set_continued_offset(5);
}

inline void Indent_Table::apply_emacs_indent() {
  set_indent(FLPR::Syntax_Tags::PG_BLOCK, 3);
  set_indent(FLPR::Syntax_Tags::PG_EXECUTION_PART, 2);
  set_indent(FLPR::Syntax_Tags::PG_INTERFACE_SPECIFICATION, 3);
  set_indent(FLPR::Syntax_Tags::PG_INTERNAL_SUBPROGRAM, 2);
  set_indent(FLPR::Syntax_Tags::PG_MODULE_SUBPROGRAM, 2);
  set_indent(FLPR::Syntax_Tags::PG_SPECIFICATION_PART, 2);
  set_indent(FLPR::Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, 3);

  for (int i = FLPR::Syntax_Tags::PG_000_LB + 1;
       i < FLPR::Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, 3);
  set_continued_offset(5);
}


inline void Indent_Table::apply_constant_fixed_indent(int const spaces) {
  set_indent(FLPR::Syntax_Tags::PG_BLOCK, spaces);
  set_indent(FLPR::Syntax_Tags::PG_EXECUTION_PART, 0);
  set_indent(FLPR::Syntax_Tags::PG_INTERFACE_SPECIFICATION, spaces);
  set_indent(FLPR::Syntax_Tags::PG_INTERNAL_SUBPROGRAM, spaces);
  set_indent(FLPR::Syntax_Tags::PG_MODULE_SUBPROGRAM, spaces);
  set_indent(FLPR::Syntax_Tags::PG_SPECIFICATION_PART, 0);
  set_indent(FLPR::Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, spaces);

  for (int i = FLPR::Syntax_Tags::PG_000_LB + 1;
       i < FLPR::Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, spaces);
  set_continued_offset(5);
}

// You don't need to add ones where begin/end just surround a block
constexpr bool Indent_Table::begin_end_construct(int const syntag) {
  switch (syntag) {
  case FLPR::Syntax_Tags::PG_DERIVED_TYPE_DEF:
  case FLPR::Syntax_Tags::PG_ENUM_DEF:
  case FLPR::Syntax_Tags::PG_CASE_CONSTRUCT:
    return true;
  }
  return false;
}

#endif
