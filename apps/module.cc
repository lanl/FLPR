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


#include <flpr/flpr.hh>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

/*--------------------------------------------------------------------------*/

using File = FLPR::Parsed_File<>;
#define TAG(T) FLPR::Syntax_Tags::T
using Cursor = typename File::Parse_Tree::cursor_t;
using Stmt_Cursor = typename File::Stmt_Cursor;
using Stmt_Range = typename File::Parse_Tree::value::Stmt_Range;
using Stmt_Iter = typename Stmt_Range::iterator;
using Stmt_Const_Iter = typename Stmt_Range::const_iterator;
using Procedure = FLPR::Procedure<File>;

class Module_Action {
public:
  Module_Action(std::string &&module_name,
                std::vector<std::string> &&only_names) :
      module_name_{std::move(module_name)},
      only_names_{std::move(only_names)}
  {
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


void print_usage(std::ostream &os) {
  os << "Usage: module (-o <only name>)* (-f <filename> | <call name>) "
    "<module name> <filename> ... \n";
  os << "\t-o <only name>\t(optional) a name in use-stmt only-list\n";
  os << "\t-f <filename>\tname of file containing call names\n";
  os << "\t<call name>\tthe subroutine name that triggers module addition\n";
  os << "\t<module name>\tthe module for which an use-stmt will be added\n";
  os << "\t<filename>\tthe Fortran source file to operate on\n";
}

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
  if(call_name_filename.empty()) {
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

  Module_Action action(std::string{argv[mod_index]}, std::move(only_names));
  if(call_name_filename.empty()) {
    action.add_subroutine_name(std::string{argv[mod_index-1]});
  } else {
    std::ifstream is(call_name_filename.c_str());
    std::string name;
    while(is >> name) {
      action.add_subroutine_name(name);
    }
    is.close();
  }
  
  for (int i = mod_index + 1; i < argc; ++i) {
    std::string const filename{argv[i]};
    /* you could change to an alternative file_type_from_ext function here */
    do_file(filename, FLPR::file_type_from_extension(filename), action);
  }
  
  return 0;
}

/*--------------------------------------------------------------------------*/

bool do_file(std::string const &filename, FLPR::File_Type file_type,
             Module_Action const &visit_action) {

  File file(filename, file_type);
  if (!file)
    exit(1);

  FLPR::Procedure_Visitor puv(file, visit_action);

  bool const changed = puv.visit();

  if (changed) {
    std::string bak{filename + ".bak"};
    if(!rename(filename.c_str(), bak.c_str())) {
      std::ofstream os(filename);
      write_file(os, file);
      os.close();
    } else {
      std::cerr << "Unable to rename \"" << filename << "\" to \"" <<
        bak << std::endl;
      exit(2);
    }
  }
  return changed;
}

/*--------------------------------------------------------------------------*/

bool
Module_Action::operator()(File &file, Cursor c, bool const internal_procedure,
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
    if(use_c->token_range.front().lower() == module_lc_)
      found = true;
  }

  if(!found) {
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

  if (TAG(SG_IF_STMT) == stmt_tag) {
    assert(TAG(SG_ACTION_STMT) == c->syntag);
    c.down();
    assert(TAG(SG_IF_STMT) == c->syntag);
    c.down();
    assert(TAG(KW_IF) == c->syntag);
    c.next(4);
  }

  assert(TAG(SG_ACTION_STMT) == c->syntag);
  c.down(1);
  if (TAG(SG_CALL_STMT) != c->syntag)
    return false;
  assert(TAG(SG_CALL_STMT) == c->syntag);
  c.down(1);
  assert(TAG(KW_CALL) == c->syntag);
  c.next(1);
  assert(TAG(SG_PROCEDURE_DESIGNATOR) == c->syntag);
  c.down();
  assert(TAG(SG_DATA_REF));
  c.down();
  assert(TAG(SG_PART_REF));
  c.down();
  assert(TAG(TK_NAME));
  std::string lname = c->token_range.front().lower();
  
  return (lowercase_names.count(lname) == 1);

}


Stmt_Cursor find_use_module_name(FLPR::LL_Stmt &stmt) {
  Stmt_Cursor c{stmt.stmt_tree().cursor()};
  assert(TAG(SG_USE_STMT) == c->syntag);
  c.down(1);
  assert(TAG(KW_USE) == c->syntag);
  c.next(1);
  if(TAG(TK_COMMA) == c->syntag) 
    c.next(2);
  if(TAG(TK_DBL_COLON) == c->syntag)
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
