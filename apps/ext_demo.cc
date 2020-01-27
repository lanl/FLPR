/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Demonstrate how to extend the action-stmt parsers.
*/

#include "flpr/Logical_File.hh"
#include "flpr/Prgm_Parsers.hh"
#include "flpr/Stmt_Parser_Exts.hh"
#include "flpr/parse_stmt.hh"
#include <iostream>

/* Use the default program parsers */
using Parse = FLPR::Prgm::Parsers<>;

/* Define a handy shortcut */
using Parse_Tree = Parse::Prgm_Tree;

void make_one_line_file(FLPR::Logical_File &lf, char const *const text);
bool parse_executable_construct(FLPR::Logical_File &lf);
namespace FLPR::Stmt {
Stmt_Tree write_comma_stmt(TT_Stream &ts);
Stmt_Tree write_comma_stmt_mytag(TT_Stream &ts);
} // namespace FLPR::Stmt

// If you want to create your own Syntax_Tags, set them up this way.
enum My_Syntax_Tags {
  MY_FOO_STMT = FLPR::Syntax_Tags::CLIENT_EXTENSION,
  MY_WRITE_STMT,
  MY_BAR_STMT
};

int main() {

  /* According to the standard, there should NOT be a comma after the
     io-control-spec-list in a write statement.  The standard parser accepts a
     properly structured write statement... */
  FLPR::Logical_File lf_standard;
  make_one_line_file(lf_standard, "write(*,100) a,b");
  std::cout << "Default ";
  parse_executable_construct(lf_standard);

  /* ... but does NOT accept a comma after the io-control-spec-list, even though
     some compilers do. */
  FLPR::Logical_File lf_comma;
  make_one_line_file(lf_comma, "write(*,100), a,b");
  std::cout << "Default ";
  parse_executable_construct(lf_comma);

  /* If you have a lot of those kind of commas in your code base, you can create
     an action-stmt grammar extension that accepts that format. */
  auto &exts = FLPR::Stmt::get_parser_exts();
  exts.register_action_stmt(FLPR::Stmt::write_comma_stmt);
  std::cout << "Extended ";
  parse_executable_construct(lf_comma);

  /* The previous rule said that the syntax tag was "SG_WRITE_STMT", but you may
     want to give it something unique, in case you need to act on it
     specially */
  exts.clear();
  exts.register_action_stmt(FLPR::Stmt::write_comma_stmt_mytag);

  /* If you are printing out the syntax tree, as in this example, you can
     register a label and a type (see Syntax_Tags_Defs.hh for a description of
     types) that make the output prettier.  If you don't do this optional step,
     your extension tag will come with a default label of
     "<client-extension+N>", where N is an integer offset from the
     Syntax_Tags::CLIENT_EXTENSION tag. Syntax tag extension registration looks
     like: */
  FLPR::Syntax_Tags::register_ext(MY_WRITE_STMT, "my-write-stmt", 5);
  std::cout << "Extended ";
  parse_executable_construct(lf_comma);

  return 0;
}

void make_one_line_file(FLPR::Logical_File &lf, char const *const text) {
  FLPR::Logical_File::Line_Buf buf;
  buf.emplace_back(std::string{text});
  lf.scan_free(buf);
}

bool parse_executable_construct(FLPR::Logical_File &lf) {
  lf.make_stmts();
  Parse::State state(lf.ll_stmts);
  auto result{Parse::executable_construct(state)};
  if (result.match) {
    /* descend down to where we can grab the syntax tag for the write-stmt.  We
       start in the program tree: executable-construct */
    auto prgm_cursor{result.parse_tree.ccursor()};
    /* descend down executable-construct -> action-stmt */
    prgm_cursor.down();
    /* we're now at a leaf of the program parse tree, so we switch from the
       program tree to the statement tree : action-stmt */
    auto stmt_cursor{prgm_cursor->stmt_tree().ccursor()};
    /* descend down action-stmt -> write-stmt */
    stmt_cursor.down();

    std::cout << "parser recognizes \"";
    FLPR::Syntax_Tags::print(std::cout, stmt_cursor->syntag);
    std::cout << "\" from \"" << lf.ll_stmts.front() << "\"\n";
  } else {
    std::cout << "parser DOES NOT recognize \"" << lf.ll_stmts.front()
              << "\"\n";
  }
  return result.match;
}

/* It is much easier to define this in the FLPR::Stmt namespace */
namespace FLPR::Stmt {
/* An extended write-stmt parser that allows a comma after io-control-spec-list.
   This code is a copy of FLPR::Stmt::write_stmt (in parse_stmt.cc) with one
   extra line to accept the comma. */
Stmt_Tree write_comma_stmt(TT_Stream &ts) {
  constexpr auto p = seq(
      Syntax_Tags::SG_WRITE_STMT, tok(Syntax_Tags::KW_WRITE),
      // This fakes the io-control-spec-list
      tag_if(Syntax_Tags::SG_IO_CONTROL_SPEC_LIST, rule(consume_parens)),
      tok(Syntax_Tags::TK_COMMA),
      opt(list(Syntax_Tags::SG_OUTPUT_ITEM_LIST, rule(output_item))), eol());
  return p(ts);
}

/* Same as above, but we're going to tag it with a client-extension syntax
   tag */
Stmt_Tree write_comma_stmt_mytag(TT_Stream &ts) {
  constexpr auto p = seq(
      MY_WRITE_STMT, tok(Syntax_Tags::KW_WRITE),
      // This fakes the io-control-spec-list
      tag_if(Syntax_Tags::SG_IO_CONTROL_SPEC_LIST, rule(consume_parens)),
      tok(Syntax_Tags::TK_COMMA),
      opt(list(Syntax_Tags::SG_OUTPUT_ITEM_LIST, rule(output_item))), eol());
  return p(ts);
}

} // namespace FLPR::Stmt
