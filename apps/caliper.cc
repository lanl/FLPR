/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/**
  /file caliper.cc

  /brief Add fictitious function calls at entry and exit point of each external
  and module subprogram in a file.

  Demonstration program to insert fictitious performance caliper calls in each
  external subprogram and module subprogram (not internal subprograms).  The
  caliper calls include the subprogram name as an actual parameter, and mark the
  begining and end of each executable section.  The executable section is
  scanned for conditional return statements: if they exist, a labeled continue
  statement is introduced about the end caliper, and the return is replaced with
  a branch to that continue.
*/

#include "flpr/flpr.hh"
#include <cassert>
#include <iostream>
#include <set>

/*--------------------------------------------------------------------------*/

using File = FLPR::Parsed_File<>;
#define TAG(T) FLPR::Syntax_Tags::T
using Cursor = typename File::Parse_Tree::cursor_t;
using Stmt_Range = typename File::Parse_Tree::value::Stmt_Range;
using Stmt_Iter = typename Stmt_Range::iterator;
using Stmt_Const_Iter = typename Stmt_Range::const_iterator;
using Procedure = FLPR::Procedure<File>;

bool caliper_procedure(File &file, Cursor c, bool internal_procedure,
                       bool module_procedure);
bool exclude_procedure(Procedure const &subp);
void count_return_stmts(Procedure const &proc, int &num_if_stmt_returns,
                        int &num_internal_returns, int &num_final_returns);
void convert_return_stmts(Procedure &proc, int label);
bool caliper_file(std::string const &filename);
void write_file(std::ostream &os, File const &f);

/*--------------------------------------------------------------------------*/

/**
 *  \brief Main caliper code driver
 *  \param[in] argc Argument count
 *  \param[in] argv List of argument values
 *  \returns  Status code
 */
int main(int argc,
         char const *argv[]) {
  int err {0};
  if (argc != 2) {
    std::cerr << "Usage: caliper <filename>" << std::endl;
    err = 1;
  } else {
    if (!caliper_file(std::string{argv[1]})) {
      err = 2;
    }
  }
  return err;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Apply caliper transform to source file
 *  \param[in] filename Name of target file
 *  \returns Changed status; true if target file has been changed
 */
bool caliper_file(std::string const &filename) {
  File file(filename);
  if (!file)
    return false;

  FLPR::Procedure_Visitor puv(file, caliper_procedure);
  bool const changed = puv.visit();
  if (changed) {
    write_file(std::cout, file);
  }
  return changed;
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Apply caliper transform to a given procedure.
 *
 *  Caliper transform will be applied to module or standalone procedures which
 *  can be `ingest()`ed and are not internal procedures, have no executable
 *  body, or are otherwise explicitly excluded from processing.
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
bool caliper_procedure(File &file,
                       Cursor c,
                       bool internal_procedure,
                       bool module_procedure) {
  Procedure proc(file);
  if (!proc.ingest(c)) {
    std::cerr << "\n******** Unable to ingest procedure *******\n" << std::endl;
    return false;
  }

  if (internal_procedure) {
    std::cerr << "skipping " << proc.name() << ": internal procedure"
              << std::endl;
    return false;
  }

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

  int num_if_stmt_returns  {0};
  int num_internal_returns {0};
  int num_final_returns    {0};
  count_return_stmts(proc, num_if_stmt_returns, num_internal_returns,
                     num_final_returns);

//  std::string const continue_stmt = std::string{"continue"};
  std::string const beg_stmt = std::string{"call caliper_begin('"}
                               + proc.name() + "')";
  std::string const end_stmt = std::string{"call caliper_end('"}
                               + proc.name() + "')";

  /* insert the caliper_begin statement at the begining of the execution-part */
  FLPR::LL_STMT_SEQ::iterator beg_it;

  beg_it =
      proc.emplace_stmt(proc.begin(Procedure::EXECUTION_PART),
                        FLPR::Logical_Line{beg_stmt}, TAG(SG_CALL_STMT), false);
  beg_it->set_leading_spaces(std::next(beg_it)->get_leading_spaces(), 2);

  /* place the caliper_end statement, either replacing the final return
     statement, or inserted as the last statement of the execution-part */
  FLPR::LL_STMT_SEQ::iterator end_it;

  if (num_final_returns) {
    /* replace the final return */
    end_it = proc.replace_stmt(proc.last(Procedure::EXECUTION_PART), end_stmt,
                               TAG(SG_CALL_STMT));
  } else {
    /* insert a new line */
    end_it = proc.emplace_stmt(proc.end(Procedure::EXECUTION_PART),
                               FLPR::Logical_Line{end_stmt}, TAG(SG_CALL_STMT),
                               true);
    end_it->set_leading_spaces(std::prev(end_it)->get_leading_spaces(), 2);
  }

  /* if we have other return statements in the execution-part, we need to
     convert them into goto statements */
  if (num_if_stmt_returns || num_internal_returns) {

    int new_label = 9999;
    if (end_it->has_label()) {
      /* this statement was already labeled (e.g. it was a labeled return-stmt),
         so just reuse that label to not upset other branch statements */
      new_label = end_it->label();
    } else {
      /* scan for existing labels */
      std::set<int> labels;
      proc.scan_out_labels(std::inserter(labels, labels.end()));

      /* pick one that is not currently in use */
      while (new_label > 0 && labels.count(new_label))
        new_label -= 1;
      assert(new_label > 0);

      /* apply the new label */
      file.logical_file().set_stmt_label(end_it, new_label);
    }
    convert_return_stmts(proc, new_label);
  }
  return true;
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
 *  \brief Count number of return statements in a procedure
 *  \param[in] proc Procedure to check
 *  \param[out] num_if_stmt_returns Number of return statements found within
 *  `if` conditonals
 *  \param[out] num_internal_stmt_returns Number of return statements within
 *  the body of a procedure
 *  \param[out] num_final_stmt_returns Number of return statements found
 *  at end of procedures
 */
void count_return_stmts(Procedure const &proc,
                        int &num_if_stmt_returns,
                        int &num_internal_returns,
                        int &num_final_returns) {
  num_if_stmt_returns = 0;
  num_internal_returns = 0;
  num_final_returns = 0;

  auto ebegin = proc.cbegin(Procedure::EXECUTION_PART);
  auto eend = proc.cend(Procedure::EXECUTION_PART);

  for (auto si = ebegin; si != eend; ++si) {
    assert(si->has_hook());
    int stmt_tag = si->stmt_tag(true);
    if (stmt_tag < 0) {
      stmt_tag *= -1;
      if (TAG(SG_RETURN_STMT) == stmt_tag) {
        num_if_stmt_returns += 1;
      }
    } else if (TAG(SG_RETURN_STMT) == stmt_tag) {
      if (std::next(si) == eend) {
        num_final_returns += 1;
      } else {
        num_internal_returns += 1;
      }
    }
  }
  assert(num_final_returns == 0 || num_final_returns == 1);
}

/*--------------------------------------------------------------------------*/

/**
 *  \brief Convert return statements in a procedure to `go to`
 *  \param[inout] proc Procedure to modify
 *  \param[in] label Integer line label of procedure exit point
 */
void convert_return_stmts(Procedure &proc,
                          int label) {
  /* for each return-stmt that we want to replace, we need the LL_Stmt iterator
     that holds it, and the LL_TT_Range of the return_stmt. */

  std::string const goto_stmt = "go to " + std::to_string(label);

  Procedure::Region_Iterator ebegin = proc.begin(Procedure::EXECUTION_PART);
  Procedure::Region_Iterator eend = proc.end(Procedure::EXECUTION_PART);

  for (Procedure::Region_Iterator llsi = ebegin; llsi != eend; ++llsi) {
    int stmt_tag = llsi->stmt_tag(true);
    if (stmt_tag < 0) {
      /* a negative tag means that we looked inside of an if-stmt */
      stmt_tag *= -1;
      if (TAG(SG_RETURN_STMT) == stmt_tag) {
        auto scursor = llsi->stmt_tree().cursor();
        scursor.down(2);
        assert(TAG(KW_IF) == scursor->syntag);
        scursor.next(4);
        assert(TAG(SG_ACTION_STMT) == scursor->syntag);
        scursor.down();
        assert(TAG(SG_RETURN_STMT) == scursor->syntag);
        proc.replace_stmt_substr(llsi, scursor->token_range, goto_stmt);
      }
    } else if (TAG(SG_RETURN_STMT) == stmt_tag) {
      assert(std::next(llsi) != eend); // should be gone already
      proc.replace_stmt(llsi, goto_stmt, TAG(SG_GOTO_STMT));
    }
  }
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
