/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/**
  /file mark_executable.hh

  /brief Declaration of function interfaces exposed by mark_executable.cc

  Demonstration program to insert a `continue` line at the beginning of the
  executable section of each subprogram as an aid to visualizing the boundary
  between declaration and executable portions of a routine.
*/

#ifndef MARK_EXECUTABLE_HH
#define MARK_EXECUTABLE_HH 1

#include "flpr/flpr.hh"
#include <iostream>
#include <string>

/*--------------------------------------------------------------------------*/

using File = FLPR::Parsed_File<>;
using Cursor = typename File::Parse_Tree::cursor_t;
using Procedure = FLPR::Procedure<File>;

bool markexe_procedure(File &file,
                       Cursor c,
                       bool internal_procedure,
                       bool module_procedure);
bool exclude_procedure(Procedure const &subp);
bool markexe_file(std::string const &filename);
void write_file(std::ostream &os,
                File const &f);

#endif
