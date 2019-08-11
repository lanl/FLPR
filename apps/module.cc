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

  A backup copy of the original file contents will be made '.bak' extension,
  then the changes will be made under the original file name.

  The code is split up in an unusual way to allow you to easily use the module
  operations with your own syntax extensions: just copy this file, add your
  extensions in main(), and run.
*/

#include "module_base.hh"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

using vec_str = std::vector<std::string>;

/*--------------------------------------------------------------------------*/

void parse_cmd_line(int argc, char *const argv[], vec_str &only_names,
                    std::string &call_name, bool &call_name_is_file,
                    std::string &module_name, vec_str &fortran_filenames);
void print_usage(std::ostream &os);

/*--------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

  vec_str only_names, fortran_filenames;
  std::string call_name, module_name;
  bool call_name_is_file{false};

  parse_cmd_line(argc, argv, only_names, call_name, call_name_is_file,
                 module_name, fortran_filenames);

  /* You could register FLPR syntax extensions here */

  FLPR_Module::Module_Action action(std::move(module_name),
                                    std::move(only_names));

  if (call_name_is_file) {
    /* The list of CALL procedure-designators that trigger the USE <module_name>
       insertion is given in a file */
    std::ifstream is(call_name.c_str());
    std::string name;
    while (is >> name) {
      action.add_subroutine_name(name);
    }
    is.close();
  } else {
    /* There is only one name that triggers the action */
    action.add_subroutine_name(call_name);
  }

  for (std::string const &filename : fortran_filenames) {
    /* you could change to an alternative file_type_from_ext function here */
    FLPR_Module::do_file(filename, FLPR::file_type_from_extension(filename),
                         action);
  }

  return 0;
}

/*--------------------------------------------------------------------------*/

void parse_cmd_line(int argc, char *const argv[], vec_str &only_names,
                    std::string &call_name, bool &call_name_is_file,
                    std::string &module_name, vec_str &fortran_filenames) {

  call_name_is_file = false;

  int ch;
  while ((ch = getopt(argc, argv, "f:o:")) != -1) {
    switch (ch) {
    case 'o':
      only_names.emplace_back(std::string{optarg});
      break;
    case 'f':
      call_name = std::string{optarg};
      call_name_is_file = true;
      break;
    default:
      print_usage(std::cerr);
      exit(1);
    }
  }

  int idx = optind;
  if (!call_name_is_file) {
    if (argc < 4) {
      print_usage(std::cerr);
      exit(1);
    }
    call_name = std::string{argv[idx++]};
  } else {
    if (argc < 3) {
      print_usage(std::cerr);
      exit(1);
    }
  }

  module_name = std::string{argv[idx]};
  for (int i = idx+1; i < argc; ++i) {
    fortran_filenames.emplace_back(std::string{argv[i]});
  }
}

/*--------------------------------------------------------------------------*/

void print_usage(std::ostream &os) {
  os << "Usage: module (-o <only name>)* (-f <filename> | <call name>) "
        "<module name> <filename> ... \n";
  os << "\t-o <only name>\t(optional) a name in use-stmt only-list\n";
  os << "\t-f <filename>\tname of file containing call names\n";
  os << "\t<call name>\tthe subroutine name that triggers module addition\n";
  os << "\t<module name>\tthe module for which an use-stmt will be added\n";
  os << "\t<filename>\tthe Fortran source file to operate on\n";
}
