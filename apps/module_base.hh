/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file module_base.hh

  Demonstrating how to selectively insert a *use-stmt* into
  subprograms that contain a *call-stmt* to a particular name.  You
  may want functionality like this when moving old code into modules.
*/

#ifndef MODULE_BASE_HH
#define MODULE_BASE_HH 1

#include <flpr/flpr.hh>
#include <string>
#include <unordered_set>
#include <vector>

namespace FLPR_Module {

using File = FLPR::Parsed_File<>;
using Cursor = typename File::Parse_Tree::cursor_t;
using Stmt_Cursor = typename File::Stmt_Cursor;
using Stmt_Range = typename File::Parse_Tree::value::Stmt_Range;
using Stmt_Iter = typename Stmt_Range::iterator;
using Stmt_Const_Iter = typename Stmt_Range::const_iterator;
using Procedure = FLPR::Procedure<File>;

class Module_Action {
public:
  Module_Action(std::string &&module_name,
                std::vector<std::string> &&only_names)
      : module_name_{std::move(module_name)}, only_names_{
                                                  std::move(only_names)} {
    module_lc_ = module_name_;
    FLPR::tolower(module_lc_);
    assert(only_names_.empty());
  }
  void add_subroutine_name(std::string name) {
    FLPR::tolower(name);
    subroutine_names_.emplace(std::move(name));
  }

  bool operator()(File &file, Cursor c, bool const internal_procedure,
                  bool const module_procedure) const;

private:
  std::string module_name_;
  std::vector<std::string> only_names_;
  std::unordered_set<std::string> subroutine_names_;
  std::string module_lc_;
};

bool do_file(std::string const &filename, FLPR::File_Type file_type,
             Module_Action const &action);
void write_file(std::ostream &os, File const &f);
bool has_call_named(FLPR::LL_Stmt const &stmt,
                    std::unordered_set<std::string> const &lowercase_names);

Stmt_Cursor find_use_module_name(FLPR::LL_Stmt &stmt);
FLPR::File_Type file_type_from_ext(std::string const &filename);

} // namespace FLPR_Module

#endif
