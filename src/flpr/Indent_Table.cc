/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Indent_Table.cc

  Manage indent spacing for Prgm Grammar (PG_) syntax tags.
*/

#include "flpr/Indent_Table.hh"

namespace FLPR {

void Indent_Table::apply_constant_indent(int const spaces) {
  set_indent(Syntax_Tags::PG_BLOCK, spaces);
  set_indent(Syntax_Tags::PG_EXECUTION_PART, spaces);
  set_indent(Syntax_Tags::PG_INTERFACE_SPECIFICATION, spaces);
  set_indent(Syntax_Tags::PG_INTERNAL_SUBPROGRAM, spaces);
  set_indent(Syntax_Tags::PG_MODULE_SUBPROGRAM, spaces);
  set_indent(Syntax_Tags::PG_SPECIFICATION_PART, spaces);
  set_indent(Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, spaces);

  for (int i = Syntax_Tags::PG_000_LB + 1; i < Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, spaces);
  set_continued_offset(5);
}

void Indent_Table::apply_emacs_indent() {
  set_indent(Syntax_Tags::PG_BLOCK, 3);
  set_indent(Syntax_Tags::PG_EXECUTION_PART, 2);
  set_indent(Syntax_Tags::PG_INTERFACE_SPECIFICATION, 3);
  set_indent(Syntax_Tags::PG_INTERNAL_SUBPROGRAM, 2);
  set_indent(Syntax_Tags::PG_MODULE_SUBPROGRAM, 2);
  set_indent(Syntax_Tags::PG_SPECIFICATION_PART, 2);
  set_indent(Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, 3);

  for (int i = Syntax_Tags::PG_000_LB + 1; i < Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, 3);
  set_continued_offset(5);
}

void Indent_Table::apply_constant_fixed_indent(int const spaces) {
  set_indent(Syntax_Tags::PG_BLOCK, spaces);
  set_indent(Syntax_Tags::PG_EXECUTION_PART, 0);
  set_indent(Syntax_Tags::PG_INTERFACE_SPECIFICATION, spaces);
  set_indent(Syntax_Tags::PG_INTERNAL_SUBPROGRAM, spaces);
  set_indent(Syntax_Tags::PG_MODULE_SUBPROGRAM, spaces);
  set_indent(Syntax_Tags::PG_SPECIFICATION_PART, 0);
  set_indent(Syntax_Tags::PG_WHERE_BODY_CONSTRUCT, spaces);

  for (int i = Syntax_Tags::PG_000_LB + 1; i < Syntax_Tags::PG_ZZZ_UB; ++i)
    if (begin_end_construct(i))
      set_indent(i, spaces);
  set_continued_offset(5);
}

} // namespace FLPR
