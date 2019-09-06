/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  \file flpr_show_cst.cc

  Diagnostic utility to parse lines from the standard input and, if successful,
  print out the concrete syntax tree.
*/

#include "flpr/flpr.hh"
#include <iostream>

int main() {
  /* Setup options */
  FLPR::File_Type file_type = FLPR::File_Type::FREEFMT;

  /* Read lines from standard input */
  using Raw_Lines = FLPR::Logical_File::Line_Buf;
  Raw_Lines raw_lines;
  std::string line;
  /* Print banner etc to cerr to allow for easy redirect of output */
  std::cerr << '\n'
            << "==============================\n"
            << "FLPR Show Concrete Syntax Tree\n"
            << "==============================\n\n"
            << "Enter free-form Fortran statements, blank line or "
               "Ctrl-D/EOF to end input: "
            << std::endl;
  while (std::getline(std::cin, line)) {
    if (!line.empty()) {
      raw_lines.emplace_back(line);
    } else
      break;
  }
  if (raw_lines.empty()) {
    std::cerr << "No lines entered." << std::endl;
    return 1;
  }

  /* Scan text and produce a Logical_File (a sequence of Logical_Lines) */
  FLPR::Logical_File logical_file;
  /* Set a fake filename just to make the output look pretty.  This isn't
     required. */
  logical_file.file_info =
      std::make_shared<FLPR::File_Info>(std::string("Line"), file_type);

  bool scan_okay{false};

  switch (file_type) {
  case FLPR::File_Type::FIXEDFMT:
    scan_okay = logical_file.scan_fixed(raw_lines);
    break;
  case FLPR::File_Type::FREEFMT:
    scan_okay = logical_file.scan_free(raw_lines);
    break;
  default:
    std::cerr << "Unhandled file form type." << std::endl;
  }
  if (!scan_okay) {
    std::cerr << "scan failed, exiting." << std::endl;
    return 2;
  }

  /* Create statements from logical lines */
  logical_file.make_stmts();

  /* Iterate across the LL_Stmts, seeing which parsers match.  Note that
     multiple parsers can match.  For example, text accepted by assignment-stmt
     will also match against action-stmt and forall-assignment-stmt. */
  for (auto &stmt : logical_file.ll_stmts) {
    int results{0};
    std::cerr << "--------------------------------------------------------\n"
              << "Parsing statement: \"" << stmt << "\"\n"
              << "--------------------------------------------------------\n";
    /* Build a token stream for this statement */
    FLPR::TT_Stream ts(stmt);
    /* Iterate across all the statement grammar (SG) parsers... */
    for (int sg_id = FLPR::Syntax_Tags::SG_000_LB + 1;
         sg_id < FLPR::Syntax_Tags::SG_ZZZ_UB; ++sg_id) {
      /* ...looking for top-level stmt parsers... */
      if (FLPR::Syntax_Tags::type(sg_id) == 5) {
        FLPR::Stmt::Stmt_Tree st = FLPR::Stmt::parse_stmt_dispatch(sg_id, ts);
        /* ... that accept this input. */
        if (st) {
          results += 1;
          std::cout << results << ": " << st << '\n';
          /* do this so that subsequent parsers can attempt to match. */
          ts.rewind();
        }
      }
    }
    if (!results) {
      std::cout << "Unrecognized: " << stmt << '\n';
    }
  }
  return 0;
}
