/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/**
  /file mark_executable.cc

  /brief Add `continue` statements between the declaration and executable
  section of each each external and module subprogram in a file.

  Demonstration program to insert a `continue` line at the beginning of the
  executable section of each subprogram as an aid to visualizing the boundary
  between declaration and executable portions of a routine.
*/

#include "mark_executable.hh"
#include "flpr/flpr.hh"
#include <cassert>
#include <iostream>

/*--------------------------------------------------------------------------*/

using File = FLPR::Parsed_File<>;
#define TAG(T) FLPR::Syntax_Tags::T
using Cursor = typename File::Parse_Tree::cursor_t;
using Procedure = FLPR::Procedure<File>;

/*--------------------------------------------------------------------------*/

/**
 *  \brief Main markexe code driver
 *  \param[in] argc Argument count
 *  \param[in] argv List of argument values
 *  \returns  Status code
 */
int main(int argc,
         char const *argv[]) {
  int err {0};
  if (argc != 2) {
    std::cerr << "Usage: markexe <filename>" << std::endl;
    err = 1;
  } else {
    if (!markexe_file(std::string{argv[1]})) {
      err = 2;
    }
  }
  return err;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Apply markexe transform to source file
 *  \param[in] filename Name of target file
 *  \returns Changed status; true if target file has been changed
 */
bool markexe_file(std::string const &filename) {
  File file(filename);
  if (!file)
    return false;

  FLPR::Procedure_Visitor puv(file, markexe_procedure);
  bool const changed = puv.visit();
  if (changed) {
    write_file(std::cout, file);
  }
  return changed;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Apply markexe transform to a given procedure.
 *
 *  Mark_Executable transform will be applied to procedures which
 *  can be `ingest()`ed and have an executable body and are not otherwise
 *  explicitly excluded from processing.
 *
 *  \param[inout] file Target source file object
 *  \param[in] c Cursor pointing to current procedure within `file`
 *  \param[in] internal_procedure Flag indicating the current procedure is an
 *             internal procedure, `contain`ed within another function or
 *             subroutine
 *  \param[in] module_procedure Flag indicating the current procedure is a
 *             module procedure, `contain`ed within a module
 *  \returns Changed status; true if target file has been changed
 */
bool markexe_procedure(File &file,
                       Cursor c,
                       bool internal_procedure,
                       bool module_procedure) {
  bool line_inserted {false};
  Procedure proc(file);

  if (!proc.ingest(c)) {
    std::cerr << "\n******** Unable to ingest procedure *******\n"
              << std::endl;
    return false;
  }

//   if (internal_procedure) {
//     std::cerr << "skipping " << proc.name() << ": internal procedure"
//               << std::endl;
//     return false;
//   }

  if (!proc.has_region(Procedure::EXECUTION_PART)) {
    std::cerr << "skipping " << proc.name() << ": no execution part"
              << std::endl;
    return false;
  }

  if (exclude_procedure(proc)) {
    std::cerr << "skipping " << proc.name() << ": excluded" << std::endl;
    return false;
  }

  std::cerr << "adjusting " << proc.name() << std::endl;

  std::string const continue_stmt = std::string{"continue"};

  /* Insert a continue statement at the begining of the execution-part
   * if one doesn't already exist */

  // Set iterator to the beginning of Procedure::EXECUTION_PART
  // Q: What kind of iterator? Logical-line?
  // A: Define range instead - see below
  // A2: No, I actually want an iterator so I can just check the first executable statement of proc
  // A3: On second thought, I don't want an iterator, I really just want the first statement

  // Set range of proc corresponding to Procedure::EXECUTION_PART
  auto execution_part{proc.crange(Procedure::EXECUTION_PART)};
  
  // Check if first statement is a `continue`
  // Set to true to replace loop with a direct check
  bool found_continue {false};
  if (false) {
    for (auto const &stmt : execution_part) {
      int const stmt_tag = stmt.syntax_tag();
      found_continue = (   TAG(SG_CONTINUE_STMT) == stmt_tag
                        || TAG(KW_CONTINUE) == stmt_tag);
      // Unconditional break because we only want the first statement
      break;
    }
  } else {
    FLPR::LL_STMT_SEQ::iterator proc_it;
    proc_it = proc.begin(Procedure::EXECUTION_PART);
    // Q: How do I get a stmt from proc_it?
//    auto stmt_2 = *proc_it; //?
//    int const stmt_tag_2 = stmt_2.syntax_tag();
    // A: Really, I only want the syntax_tag() of the first statement
    int const stmt_tag_2 = proc_it->syntax_tag();
    found_continue = (   TAG(SG_CONTINUE_STMT) == stmt_tag_2
                      || TAG(KW_CONTINUE) == stmt_tag_2);
  }

  // If not, insert `continue` statement at top of Procedure::EXECUTION_PART
  if (!found_continue) {
    FLPR::LL_STMT_SEQ::iterator beg_it;
    beg_it =
        proc.emplace_stmt(proc.begin(Procedure::EXECUTION_PART),
                          FLPR::Logical_Line{continue_stmt},
                          TAG(SG_CONTINUE_STMT),
                          false);
    beg_it->set_leading_spaces(std::next(beg_it)->get_leading_spaces(), 2);
    line_inserted = true;
  }

  return line_inserted;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Test if a given procedure is excluded from processing
 *  \param[in] proc Procedure to check
 *  \returns Exclusion status; true if procedure is excluded from processing
 */
bool exclude_procedure(Procedure const &proc) {
  if (proc.headless_main_program()) {
    return false;
  }
  using Stmt_Const_Cursor = typename Procedure::Stmt_Const_Cursor;
  Stmt_Const_Cursor s =
      proc.range_cursor(Procedure::PROC_BEGIN)->stmt_tree().ccursor();
  s.down();
  if (TAG(SG_PREFIX) == s->syntag && s.has_down()) {
    s.down();
    do {
      assert(TAG(SG_PREFIX_SPEC) == s->syntag);
      s.down();
      if (TAG(KW_PURE) == s->syntag || TAG(KW_ELEMENTAL) == s->syntag)
        return true;
      s.up();
    } while (s.try_next());
    s.up();
  }
  return false;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Write modified file contents (logical lines)
 *  \param[inout] os Output stream
 *  \param[in] f Source file object
 */
void write_file(std::ostream &os,
                File const &f) {
  for (auto const &ll : f.logical_lines()) {
    os << ll;
  }
}
