/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Generate a free-format Fortran file after applying a set of filters to either
  a fixed-format or free-format input file.

  From a FLPR programming perspective, this is a demonstration of augmenting the
  Prgm_Tree node data to store client application data.
*/

#include "flpr_format_base.hh"
#include <array>
#include <fstream>
#include <iostream>
#include <unistd.h>

/* Define a handy shortcut */
using Parse_Tree = typename File::Parse_Tree;

/* -------------------------------------------------------------------------- */

bool elaborate_end_stmts(File &f);
bool remove_empty_stmts(typename FLPR::LL_SEQ &ll_seq);
bool split_compound_stmts(typename FLPR::LL_SEQ &ll_seq);

/* -------------------------------------------------------------------------- */

int flpr_format_file(File &file, Options const &options,
                     FLPR::Indent_Table const &indents) {
  if (file) {
    bool do_write = false;

    /* first, do any transformations that just work on the sequence of
       Logical_Lines, accessed through file.logical_lines(). These are the most
       primitive transformations.  Doing these later requires updating
       higher-level data structures, which is unnecessary overhead. */

    /* this one is implemented in Logical_File, so we'll call it first */
    if (options[OPT(FIXED_TO_FREE)]) {
      VERBOSE_BEGIN("fixed_to_free");
      do_write |= file.logical_file().convert_fixed_to_free();
      VERBOSE_END;
    }

    /* remove_empty_stmts doesn't even need the sequence: it just changes the
       internals of a logical_line.  We pass the sequence just to move the
       iteration out of main() */
    if (options[OPT(REMOVE_EMPTY_STMTS)] &&
        !options[OPT(SPLIT_COMPOUND_STMTS)]) {
      VERBOSE_BEGIN("remove_empty_stmts");
      do_write |= remove_empty_stmts(file.logical_lines());
      VERBOSE_END;
    }

    /* split_compound_stmts can create new Logical_Lines, so it needs to be
       able to insert items into the sequence */
    if (options[OPT(SPLIT_COMPOUND_STMTS)]) {
      VERBOSE_BEGIN("split_compound_stmts");
      do_write |= split_compound_stmts(file.logical_lines());
      VERBOSE_END;
    }

    /* Now do transformations that require the sequence of LL_Stmts, accessed
       via file.statements().  NOTE: any operations on LL_Stmts that will modify
       the underlying Logical_Line sequence should be performed via
       logical_file(), NOT directly on logical_lines().  The operations in
       logical_file() maintain the consistency between the LL_Stmts and the
       Logical_Lines. */

    /* A note on Parsed_File: it uses lazy evaluation for more complicated
       higher-level data structures, including the list of statements() and the
       parse_tree().  If you call parse_tree(), it will build whatever it needs:
       you don't have to explicitly build any lower-level structure.

       In this demo, I wanted to be able to put timers around the statement
       creation and parse_tree creation, so I am explicitly building.  THIS
       ISN'T NECESSARY IN NORMAL APPLICATIONS. */

    /* prebuild statements just for timing (see above) */
    VERBOSE_BEGIN("make_stmts");
    file.prefetch_statements();
    VERBOSE_END;

    /* Now do transformations that require the concrete syntax tree, accessed
       via file.parse_tree(). */

    /* prebuild parse_tree just for timing (see above) */
    VERBOSE_BEGIN("build_parse_tree");
    file.prefetch_parse_tree();
    VERBOSE_END;

    /* indent last, because why bother until all statements are in the correct
       position? */
    if (options[OPT(REINDENT)]) {
      VERBOSE_BEGIN("indent");
      do_write |= file.indent(indents);
      VERBOSE_END;
    }

    /* now, if any transformation made a change, write the output. */
    if (options.do_output() || (do_write && !options.quiet())) {
      VERBOSE_BEGIN("write");
      if (options.write_inplace()) {
        std::cerr << "write in place not implemented yet" << '\n';
      } else {
        write_file(std::cout, file);
      }
      VERBOSE_END;
    } else {
      if (options.verbose())
        std::cerr << "nothing changed" << std::endl;
    }
  }
  return 0;
}

bool remove_empty_stmts(typename FLPR::LL_SEQ &ll_seq) {
  bool changed = false;
  for (FLPR::Logical_Line &ll : ll_seq) {
    if (!ll.has_fortran() && ll.has_empty_statements())
      changed |= ll.remove_empty_statements();
  }
  return changed;
}

/* This needs to be updated to take advantage of newer features. As it is, it
   probably corrupts the Token_Text layout position information.  DON'T USE AS
   AN EXAMPLE! */
bool split_compound_stmts(typename FLPR::LL_SEQ &ll_seq) {
  bool changed = false;
  bool cleanup_split = false;

  for (FLPR::LL_SEQ::iterator ll_it = ll_seq.begin(); ll_it != ll_seq.end();
       ++ll_it) {

    if (!ll_it->has_fortran())
      continue;
    FLPR::Logical_Line &ll{*ll_it};
    if (ll.stmts().size() < 2) {
      if (cleanup_split) {
        ll.fragments().erase(ll.stmts()[0].end(), ll.fragments().end());
        ll.text_from_frags();
        cleanup_split = false;
      } else if (ll.has_empty_statements()) {
        changed |= ll.remove_empty_statements();
      }
    } else {
      assert(ll_it->stmts().size() > 1);
      /* Insert a copy above the current LL. The Logical_Line copy constructor
         updates the new stmt TT_Ranges to reference the new copy of lines. */
      auto new_ll_it = ll_seq.insert(ll_it, *ll_it);
      /* Fix it up to contain only one statement */
      FLPR::Logical_Line &new_ll{*new_ll_it};
      /* Remove trailing fragments from new LL (the semicolon and the rest of
         the compound statments) */
      new_ll.fragments().erase(new_ll.stmts()[0].end(),
                               new_ll.fragments().end());
      /* Remove leading fragments from new LL (this handles the case of there
         being empty statements before the beginning of this one). */
      new_ll.fragments().erase(new_ll.fragments().begin(),
                               new_ll.stmts()[0].begin());
      /* Fix-up the File_Lines (text) in the new LL */
      new_ll.text_from_frags();

      /* Fix-up the stmts list */
      new_ll.init_stmts();

      /* Remove leading fragments from old LL */
      ll.fragments().erase(ll.fragments().begin(), ll.stmts()[1].begin());
      cleanup_split = true; /* this will reformat the old LL once it gets down
                               to one statement */

      /* Fix-up the stmts list */
      ll.init_stmts();

      ll_it = new_ll_it; // revisit the source line
      changed = true;
    }
  }
  return changed;
}

/* Adds the name and type to procedure END statements */
bool elaborate_end_stmts(File &f) {
  if (f.parse_tree().empty())
    return false;
  return false;
}

void write_file(std::ostream &os, File const &f) {
  for (auto const &ll : f.logical_lines()) {
    os << ll;
  }
}

void print_usage(std::ostream &os) {
  os << "usage: flpr-format [-foqtv] file ...\n";
  os << "\t-e\telaborate procedure END statements\n";
  os << "\t-f\tdo fixed-format to free-format conversion\n";
  os << "\t-i\treindent\n";
  os << "\t-o\tforce output, even if no changes\n";
  os << "\t-q\tquiet: no output of any kind \n";
  os << "\t-t\ttime each phase\n";
  os << "\t-v\tshow transformation phases\n";
}

bool parse_cmd_line(std::vector<std::string> &filenames, Options &options,
                    int argc, char *const argv[]) {
  int ch;
  while ((ch = getopt(argc, argv, "efioqtv")) != -1) {
    switch (ch) {
    case 'e':
      options[Options::ELABORATE_END_STMTS] = true;
      break;
    case 'f':
      options[Options::FIXED_TO_FREE] = true;
      break;
    case 'i':
      options[Options::REINDENT] = true;
      break;
    case 'o':
      options.set_do_output(true);
      break;
    case 'q': /* really just for testing */
      options.set_quiet(true);
      options.set_verbose(false);
      options.set_do_timing(false);
      options.set_do_output(false);
      break;
    case 't':
      options.set_do_timing(true);
      options.set_verbose(true);
      break;
    case 'v':
      options.set_verbose(true);
      break;
    default:
      std::cerr << "unknown option\n";
      print_usage(std::cerr);
      return false;
    }
  }
  argc -= optind;
  argv += optind;
  for (int i = 0; i < argc; ++i) {
    filenames.emplace_back(std::string{argv[i]});
  }
  if (filenames.empty()) {
    print_usage(std::cerr);
    return false;
  }
  return true;
}
