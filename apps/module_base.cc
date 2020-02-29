/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file module_base.cc

  Demonstrating how to selectively insert a *use-stmt* into
  subprograms that contain a *call-stmt* to a particular name.  You
  may want functionality like this when moving old code into modules.
*/

#include "module_base.hh"
#include <cassert>
#include <fstream>

#define TAG(X) FLPR::Syntax_Tags::X

namespace FLPR_Module {

/*--------------------------------------------------------------------------*/

bool do_file(std::string const &filename, int const last_fixed_col,
             FLPR::File_Type file_type, Module_Action const &visit_action) {

  File file(filename, last_fixed_col, file_type);
  if (!file)
    exit(1);

  FLPR::Procedure_Visitor puv(file, visit_action);

  bool const changed = puv.visit();

  if (changed) {
    std::string bak{filename + ".bak"};
    if (!rename(filename.c_str(), bak.c_str())) {
      std::ofstream os(filename);
      write_file(os, file);
      os.close();
    } else {
      std::cerr << "Unable to rename \"" << filename << "\" to \"" << bak
                << std::endl;
      exit(2);
    }
  }
  return changed;
}

/*--------------------------------------------------------------------------*/

bool Module_Action::operator()(File &file, Cursor c,
                               bool const internal_procedure,
                               bool const module_procedure) const {
  if (internal_procedure) {
    return false;
  }

  Procedure proc(file);
  if (!proc.ingest(c)) {
    std::cerr << "\n******** Unable to ingest procedure *******\n" << std::endl;
    return false;
  }

  if (!proc.has_region(Procedure::EXECUTION_PART)) {
    std::cerr << "skipping " << proc.name() << ": no execution part"
              << std::endl;
    return false;
  }

  {
    auto execution_part{proc.crange(Procedure::EXECUTION_PART)};
    bool found{false};
    for (auto const &stmt : execution_part) {
      if (has_call_named(stmt, subroutine_names_)) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  auto use_stmts{proc.range(Procedure::USES)};
  bool found{false};
  for (auto &stmt : use_stmts) {
    Stmt_Cursor use_c = find_use_module_name(stmt);
    assert(use_c->token_range.size() == 1);
    if (use_c->token_range.front().lower() == module_lc_)
      found = true;
  }

  if (!found) {
    auto use_it = proc.emplace_stmt(proc.end(Procedure::USES),
                                    FLPR::Logical_Line{"use " + module_name_},
                                    TAG(SG_USE_STMT), false);
    use_it->set_leading_spaces(std::next(use_it)->get_leading_spaces(), 2);
  }

  return true;
}

/*--------------------------------------------------------------------------*/

bool has_call_named(FLPR::LL_Stmt const &stmt,
                    std::unordered_set<std::string> const &lowercase_names) {
  int const stmt_tag = stmt.syntax_tag();
  if (TAG(SG_CALL_STMT) != stmt_tag && TAG(SG_IF_STMT) != stmt_tag)
    return false;

  auto c{stmt.stmt_tree().ccursor()};

  /* If we are on an if-stmt, we need to move over to the action-stmt */
  if (TAG(SG_IF_STMT) == stmt_tag) {
    assert(TAG(SG_ACTION_STMT) == c->syntag);
    c.down();
    assert(TAG(SG_IF_STMT) == c->syntag);
    c.down();
    assert(TAG(KW_IF) == c->syntag);
    c.next(4);
  }

  /* Enter the action-stmt */
  assert(TAG(SG_ACTION_STMT) == c->syntag);
  c.down(1);

  /* If it is not a call-stmt, return */
  if (TAG(SG_CALL_STMT) != c->syntag)
    return false;

  /* Enter the call-stmt */
  c.down(1);

  /* First, we should have a CALL keyword */
  assert(TAG(KW_CALL) == c->syntag);
  c.next(1);

  /* Then, a procedure-designator, which we will enter */
  assert(TAG(SG_PROCEDURE_DESIGNATOR) == c->syntag);
  c.down();

  /* See if we have a "fancy" procedure-designator */
  if (TAG(SG_PART_REF) == c->syntag) {
    /* This is some elaborated name (proc-component-ref or data-ref %
       binding-name), so we move to the last item in the list, which should be a
       name) */
    while (c.has_next())
      c.next(1);
  }

  /* Now, we are on the procedure-name, the procedure-component-name, or the
     binding-name (depending on the type of procedure-designator).  See if it is
     listed in our set of matchers */
  assert(TAG(TK_NAME) == c->syntag);
  std::string lname = c->token_range.front().lower();
  return (lowercase_names.count(lname) == 1);
}

Stmt_Cursor find_use_module_name(FLPR::LL_Stmt &stmt) {
  Stmt_Cursor c{stmt.stmt_tree().cursor()};
  assert(TAG(SG_USE_STMT) == c->syntag);
  c.down(1);
  assert(TAG(KW_USE) == c->syntag);
  c.next(1);
  if (TAG(TK_COMMA) == c->syntag)
    c.next(2);
  if (TAG(TK_DBL_COLON) == c->syntag)
    c.next(1);
  assert(TAG(TK_NAME) == c->syntag);
  return c;
}

/*--------------------------------------------------------------------------*/

void write_file(std::ostream &os, File const &f) {
  for (auto const &ll : f.logical_lines()) {
    os << ll;
  }
}

} // namespace FLPR_Module
