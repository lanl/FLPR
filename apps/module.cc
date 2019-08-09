/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file module.cc

  Demonstrating how to selectively insert a *use-stmt* into
  subprograms that contain a *call-stmt* to a particular name.  You
  may want functionality like this when moving old code into modules.
*/

#include "module_base.hh"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

/*--------------------------------------------------------------------------*/

void print_usage(std::ostream &os);

/*--------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

  std::vector<std::string> only_names;
  std::string call_name_filename;

  int ch;
  while ((ch = getopt(argc, argv, "f:o:")) != -1) {
    switch (ch) {
    case 'o':
      only_names.emplace_back(std::string{optarg});
      break;
    case 'f':
      call_name_filename = std::string{optarg};
      break;
    default:
      print_usage(std::cerr);
      exit(1);
    }
  }

  int const mod_index = optind;
  if (call_name_filename.empty()) {
    if (argc < 4) {
      print_usage(std::cerr);
      exit(1);
    }
  } else {
    if (argc < 3) {
      print_usage(std::cerr);
      exit(1);
    }
  }

  FLPR_Module::Module_Action action(std::string{argv[mod_index]},
                                    std::move(only_names));
  if (call_name_filename.empty()) {
    action.add_subroutine_name(std::string{argv[mod_index - 1]});
  } else {
    std::ifstream is(call_name_filename.c_str());
    std::string name;
    while (is >> name) {
      action.add_subroutine_name(name);
    }
    is.close();
  }

  for (int i = mod_index + 1; i < argc; ++i) {
    std::string const filename{argv[i]};
    /* you could change to an alternative file_type_from_ext function here */
    FLPR_Module::do_file(filename, FLPR::file_type_from_extension(filename),
                         action);
  }

  return 0;
}

void print_usage(std::ostream &os) {
  os << "Usage: module (-o <only name>)* (-f <filename> | <call name>) "
        "<module name> <filename> ... \n";
  os << "\t-o <only name>\t(optional) a name in use-stmt only-list\n";
  os << "\t-f <filename>\tname of file containing call names\n";
  os << "\t<call name>\tthe subroutine name that triggers module addition\n";
  os << "\t<module name>\tthe module for which an use-stmt will be added\n";
  os << "\t<filename>\tthe Fortran source file to operate on\n";
}
