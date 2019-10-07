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

  The code is split up in an unusual way to allow you to easily use the format
  operations with your own FLPR syntax extensions: just copy this file, add your
  extensions in main(), and run.
*/

#include "flpr_format_base.hh"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *const argv[]) {

  /* Read the command-line arguments */
  std::vector<std::string> filenames;
  Options options;
  options.enable_all_filters();
  options[OPT(FIXED_TO_FREE)] = false;
  options[OPT(REINDENT)] = false;
  if (!parse_cmd_line(filenames, options, argc, argv)) {
    std::cerr << "exiting on error." << std::endl;
    return 1;
  }

  /* You could register FLPR syntax extensions here */

  /* The actual indentation pattern is selected on a file-by-file basis
     below. */
  FLPR::Indent_Table indents;

  /* Process each input file */
  for (auto const &fname : filenames) {
    File file;
    VERBOSE_BEGIN("read_file");
    file.read_file(fname);
    VERBOSE_END;
    /* Define the indentation pattern based on the input format. It would be
       nice if this was setup from an external configuration file */
    if (file.logical_file().is_fixed_format()) {
      indents.apply_constant_fixed_indent(4);
      indents.set_continued_offset(5);
    } else {
      indents.apply_emacs_indent();
    }
    if (flpr_format_file(file, options, indents)) {
      std::cerr << "Error formating file \"" << fname << "\"" << std::endl;
    } else {
      write_file(std::cout, file);
    }
  }
}
