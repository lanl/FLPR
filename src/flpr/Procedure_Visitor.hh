/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Procedure_Visitor.hh
*/
#ifndef FLPR_PROCEDURE_VISITOR_HH
#define FLPR_PROCEDURE_VISITOR_HH 1

#include "flpr/Syntax_Tags.hh"

namespace FLPR {

//! Calls Action() on every procedure in a Parsed_File
/*! Action needs to look like:
      bool(PFile_T &file, Cursor c, bool internal, bool module)

       file: reference to file passed in ctor
       c: a cursor to the specific procedure (e.g. c->syntag() will be
          SUBROUTINE_SUBPROGRAM, FUNCTION_SUBPROGRAM, MAIN_PROGRAM,
          or SEPARATE_MODULE_SUBPROGRAM)
       internal: true if c is internal to another procedure
       module: true if c is in a module

     the return value will be ||'d together and returned from visit()
*/
template <typename PFile_T, typename Action> class Procedure_Visitor {
public:
  using Cursor = typename PFile_T::Parse_Tree::cursor_t;

public:
  Procedure_Visitor(PFile_T &file, Action &a) : file_{file}, action_{a} {}
  bool visit();

private:
  bool visit_module_(Cursor c, bool const submodule);
  bool visit_procedure_(Cursor c, bool const internal, bool const module);
  inline Cursor down_copy_(Cursor c) { return c.down(); }
  PFile_T &file_;
  Action &action_;
};

#define TAG(T) FLPR::Syntax_Tags::T

template <typename PFile_T, typename Action>
bool Procedure_Visitor<PFile_T, Action>::visit() {
  bool retval = false;
  if (file_.parse_tree().empty())
    return false;
  Cursor top_level_cursor = file_.parse_tree().cursor();
  /* This should be a program */
  assert(TAG(PG_PROGRAM) == top_level_cursor->syntag());
  top_level_cursor.down();
  do {
    /* These are the program-unit entities.  First, we move down to a specific
       program-unit. */
    assert(TAG(PG_PROGRAM_UNIT) == top_level_cursor->syntag());
    top_level_cursor.down();
    switch (top_level_cursor->syntag()) {
    case TAG(PG_EXTERNAL_SUBPROGRAM):
      retval |= visit_procedure_(down_copy_(top_level_cursor), false, false);
      break;
    case TAG(PG_MODULE):
      retval |= visit_module_(top_level_cursor, false);
      break;
    case TAG(PG_MAIN_PROGRAM):
      retval |= visit_procedure_(top_level_cursor, false, false);
      break;
    case TAG(PG_SUBMODULE):
      retval |= visit_module_(top_level_cursor, true);
      break;
    }
    top_level_cursor.up();
  } while (top_level_cursor.try_next());
  return retval;
}

template <typename PFile_T, typename Action>
bool Procedure_Visitor<PFile_T, Action>::visit_procedure_(Cursor c,
                                                          bool const internal,
                                                          bool const module) {
  /* Call the visit function on this one */
  bool retval = action_(file_, c, internal, module);

  if (internal) // can't have internal subprograms
    return retval;

  bool const is_main_program = (TAG(PG_MAIN_PROGRAM) == c->syntag());

  /* Go see it there are internal subprograms */
  /* c should be on some sort of procedure root */
  c.down();
  if (is_main_program) {
    /* the program-stmt is optional */
    if (TAG(SG_PROGRAM_STMT) == c->syntag()) {
      c.next();
    }
  } else {
    // Skip the procedure statement
    c.next();
  }
  if (TAG(PG_SPECIFICATION_PART) == c->syntag()) {
    if (!c.try_next())
      return retval;
  }
  if (TAG(PG_EXECUTION_PART) == c->syntag()) {
    if (!c.try_next())
      return retval;
  }
  if (TAG(PG_INTERNAL_SUBPROGRAM_PART) == c->syntag()) {
    if (!c.try_down())
      return retval;
    assert(TAG(SG_CONTAINS_STMT) == c->syntag());
    while (c.try_next()) { // zero or more entries
      retval |= visit_procedure_(down_copy_(c), true, module);
    };
  }
  return retval;
}

template <typename PFile_T, typename Action>
bool Procedure_Visitor<PFile_T, Action>::visit_module_(Cursor c,
                                                       bool const submodule) {
  bool retval = false;
  /* c should be on a module */
  c.down();
  assert(TAG(SG_MODULE_STMT) == c->syntag());
  c.next();
  if (TAG(PG_SPECIFICATION_PART) == c->syntag()) {
    if (!c.try_next())
      return retval;
  }
  if (TAG(PG_MODULE_SUBPROGRAM_PART) == c->syntag()) {
    c.down();
    assert(TAG(SG_CONTAINS_STMT) == c->syntag());
    while (c.try_next()) { // handles case were contains section is empty
      c.down();
      if (TAG(PG_FUNCTION_SUBPROGRAM) == c->syntag() ||
          TAG(PG_SUBROUTINE_SUBPROGRAM) == c->syntag() ||
          TAG(PG_SEPARATE_MODULE_SUBPROGRAM) == c->syntag()) {
        retval |= visit_procedure_(c, false, true);
      }
      c.up();
    }
  }
  return retval;
}

#undef TAG
} // namespace FLPR
#endif
