/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
   Demonstrating how to selectively insert a *use-stmt* into
   subprograms that contain a *call-stmt* to a particular name.  You
   may want functionality like this when moving old code into modules.
*/

#include "flpr/flpr.hh"
#include <cassert>
#include <iostream>
#include <set>

/*--------------------------------------------------------------------------*/

using File = FLPR::Parsed_File<>;
#define TAG(T) FLPR::Syntax_Tags::T
using Cursor = typename File::Parse_Tree::cursor_t;
using Stmt_Cursor = typename File::Stmt_Cursor;
using Stmt_Range = typename File::Parse_Tree::value::Stmt_Range;
using Stmt_Iter = typename Stmt_Range::iterator;
using Stmt_Const_Iter = typename Stmt_Range::const_iterator;
using Procedure = FLPR::Procedure<File>;

bool do_procedure(File &file, Cursor c, bool internal_procedure,
                  bool module_procedure);
bool do_file(std::string const &filename);
void write_file(std::ostream &os, File const &f);
bool has_call_named(FLPR::LL_Stmt const &stmt,
                    std::string const &lowercase_name);

/*--------------------------------------------------------------------------*/

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: module <filename>" << std::endl;
    return 1;
  }
  if (!do_file(std::string{argv[1]}))
    return 2;
  return 0;
}

/*--------------------------------------------------------------------------*/

bool do_file(std::string const &filename) {
  File file(filename);
  if (!file)
    return false;

  /* Contruct a procedure visitor */
  FLPR::Procedure_Visitor puv(file, do_procedure);

  /* Apply the do_procedure() function to each procedure in the file */
  bool const changed = puv.visit();
  if (changed) {
    write_file(std::cout, file);
  }
  return changed;
}

/*--------------------------------------------------------------------------*/

bool do_procedure(File &file, Cursor c, bool internal_procedure,
                  bool module_procedure) {
  if (internal_procedure) {
    return false;
  }

  /* A FLPR::Procedure is a simplified view of the structure of a Parse_Tree,
     where all of the subregions of a procedure have already been identified. */
  Procedure proc(file);

  /* The Procedure ingests the Parse_Tree starting at a cursor position, which
     is assumed to be at the start of a procedure.  */
  if (!proc.ingest(c)) {
    std::cerr << "\n******** Unable to ingest procedure *******\n" << std::endl;
    return false;
  }

  /* Skip procedures that don't have an *execution-part* (they aren't
     required) */
  if (!proc.has_region(Procedure::EXECUTION_PART)) {
    std::cerr << "skipping " << proc.name() << ": no execution part"
              << std::endl;
    return false;
  }

  /* Iterate through the statements, looking for a "CALL foo" statement.
     has_call_named() handles cases where the *call-stmt* is the action of an
     *if-stmt*. */
  auto execution_part{proc.crange(Procedure::EXECUTION_PART)};
  bool found{false};
  for (auto const &stmt : execution_part) {
    if (has_call_named(stmt, "foo")) {
      found = true;
      break;
    }
  }
  if (!found)
    return false;

  /* If we found a "call foo", we insert a new *use-stmt*.  Note that this does
     not check to see if that *use-stmt* already exists or not: that would be a
     useful feature. */
  auto use_it = proc.emplace_stmt(proc.end(Procedure::USES),
                                  FLPR::Logical_Line{"use :: foo_mod"},
                                  TAG(SG_USE_STMT), false);
  use_it->set_leading_spaces(std::next(use_it)->get_leading_spaces(), 2);

  return true;
}

/*--------------------------------------------------------------------------*/

bool has_call_named(FLPR::LL_Stmt const &stmt,
                    std::string const &lowercase_name) {
  int const stmt_tag = stmt.syntax_tag();
  if (TAG(SG_CALL_STMT) != stmt_tag && TAG(SG_IF_STMT) != stmt_tag)
    return false;

  auto c{stmt.stmt_tree().ccursor()};

  /* If this is an *if-stmt*, advance the cursor to the *action-stmt* */
  if (TAG(SG_IF_STMT) == stmt_tag) {
    assert(TAG(SG_ACTION_STMT) == c->syntag);
    c.down();
    assert(TAG(SG_IF_STMT) == c->syntag);
    c.down();
    assert(TAG(KW_IF) == c->syntag);
    c.next(4);
  }

  assert(TAG(SG_ACTION_STMT) == c->syntag);
  c.down(1);
  if (TAG(SG_CALL_STMT) != c->syntag)
    return false;

  /* Walk the cursor over to the *call-stmt* name */
  assert(TAG(SG_CALL_STMT) == c->syntag);
  c.down(1);
  assert(TAG(KW_CALL) == c->syntag);
  c.next(1);
  assert(TAG(SG_PROCEDURE_DESIGNATOR) == c->syntag);
  c.down();
  assert(TAG(SG_DATA_REF));
  c.down();
  assert(TAG(SG_PART_REF));
  c.down();
  assert(TAG(TK_NAME));

  /* Compare the name against the one we are looking for */
  std::string lname = c->token_range.front().lower();
  return (lowercase_name == lname);
}

/*--------------------------------------------------------------------------*/

void write_file(std::ostream &os, File const &f) {
  for (auto const &ll : f.logical_lines()) {
    os << ll;
  }
}
