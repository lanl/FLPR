/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Parsed_File.hh
*/

#ifndef FLPR_PARSED_FILE_HH
#define FLPR_PARSED_FILE_HH 1

#include "flpr/Indent_Table.hh"
#include "flpr/LL_Stmt_Src.hh"
#include "flpr/Logical_File.hh"
#include "flpr/Prgm_Parsers.hh"
#include "flpr/Prgm_Tree.hh"
#include <ostream>
#include <string>

namespace FLPR {

//! A lazy-evaluation container for all FLPR constructs related to a file
template <typename PG_NODE_DATA = Prgm::Prgm_Node_Data> class Parsed_File {
public:
  using Parse = FLPR::Prgm::Parsers<PG_NODE_DATA>;
  using Parse_Tree = typename Parse::Prgm_Tree;
  using Prgm_Cursor = typename Parse_Tree::cursor_t;
  using Stmt_Cursor = typename Parse_Tree::value::Stmt_Tree::cursor_t;
  using Stmt_Const_Cursor =
      typename Parse_Tree::value::Stmt_Tree::const_cursor_t;

public:
  //! Read and scan a file
  /*! This routine assembles the physical lines of a file into a sequence of
      Logical_Lines, accessable as `logical_lines()`.  No other structures are
      created.  The file is consumed by the time this call returns. The filename
      will be parsed for File_Type extensions IF file_type is not specified. */
  Parsed_File(std::string const &filename,
              File_Type file_type = File_Type::UNKNOWN);

  //! Read and scan an std::istream
  /*! This routine assembles the physical lines of an istream into a sequence of
      Logical_Lines, accessable as `logical_lines()`.  No other structures are
      created.  The stream is consumed by the time that this call returns. The
      stream_name will be parsed for File_Type extensions IF stream_type is not
      specified.*/
  explicit Parsed_File(std::istream &is, std::string const &stream_name,
                       File_Type stream_type = File_Type::UNKNOWN);

  Parsed_File() = default;
  Parsed_File(Parsed_File &&) = default;
  Parsed_File(Parsed_File const &) = delete;
  Parsed_File &operator=(Parsed_File &&) = default;
  Parsed_File &operator=(Parsed_File const &) = delete;

  bool read_file(std::string const &filename,
                 File_Type file_type = File_Type::UNKNOWN);

  constexpr operator bool() const noexcept { return !bad_state_; }

  constexpr Logical_File &logical_file() noexcept { return logical_file_; }
  constexpr Logical_File const &logical_file() const noexcept {
    return logical_file_;
  }
  constexpr LL_SEQ &logical_lines() noexcept { return logical_file_.lines; }
  constexpr LL_SEQ const &logical_lines() const noexcept {
    return logical_file_.lines;
  }

  //! Build the LL_Stmts (Logical Line Statements) sequence, if needed.
  bool prefetch_statements() {
    if (!stmts_ok_)
      build_stmts_();
    return stmts_ok_;
  }

  //! Build/return the LL_Stmts (Logical Line Statements) sequence
  LL_STMT_SEQ &statements() {
    prefetch_statements();
    return logical_file_.ll_stmts;
  }

  //! Build the parse tree, if needed.
  bool prefetch_parse_tree() {
    if (!tree_ok_)
      build_tree_();
    return tree_ok_;
  }

  //! Build/return the parse tree
  Parse_Tree &parse_tree() {
    prefetch_parse_tree();
    return parse_tree_;
  }

  //! Indent the statements according to the provided Indent_Table.
  bool indent(Indent_Table const &indents) {
    if (parse_tree().empty())
      return false;
    return indent_recurse_(*(parse_tree()), indents, 0);
  }

  //! Return a Prgm_Tree cursor for a given LL_Stmt
  Prgm_Cursor stmt_to_node_cursor(LL_STMT_SEQ::iterator stmt_it) noexcept {
    if (stmt_it->has_hook()) {
      return Prgm_Cursor(
          static_cast<typename Parse_Tree::pointer>(stmt_it->get_hook())
              ->self());
    }
    return Prgm_Cursor();
  }

private:
  mutable Logical_File logical_file_;
  mutable Parse_Tree parse_tree_;
  bool from_stream_{false};
  mutable bool bad_state_{true}, stmts_ok_{false}, tree_ok_{false};

private:
  void link_stmts_recurse_(typename Parse_Tree::node &n);
  void build_stmts_() {
    if (!bad_state_) {
      logical_file_.make_stmts();
      stmts_ok_ = true;
    }
  }
  void build_tree_();
  bool indent_recurse_(typename Parse_Tree::node &n,
                       Indent_Table const &indents, int curr_spaces);
};

template <typename PG_NODE_DATA>
Parsed_File<PG_NODE_DATA>::Parsed_File(std::string const &filename,
                                       File_Type file_type) {
  if (logical_file_.read_and_scan(filename, file_type)) {
    bad_state_ = false;
  }
}

template <typename PG_NODE_DATA>
Parsed_File<PG_NODE_DATA>::Parsed_File(std::istream &is,
                                       std::string const &stream_name,
                                       File_Type stream_type)
    : from_stream_{true} {
  if (logical_file_.read_and_scan(is, stream_name, stream_type)) {
    bad_state_ = false;
  }
}

template <typename PG_NODE_DATA>
bool Parsed_File<PG_NODE_DATA>::read_file(std::string const &filename,
                                          File_Type file_type) {
  assert(bad_state_);
  if (logical_file_.read_and_scan(filename, file_type)) {
    bad_state_ = false;
  }
  return bad_state_;
}

template <typename PG_NODE_DATA> void Parsed_File<PG_NODE_DATA>::build_tree_() {
  if (bad_state_)
    return;

  if (statements().empty()) {
    parse_tree_ = Parse_Tree{};
  } else {

    typename Parse::State state(statements());
    auto result{Parse::program(state)};
    if (!result.match) {
      std::cerr << "\tparsing FAILED" << std::endl;
      bad_state_ = true;
    }
    parse_tree_.swap(result.parse_tree);
    link_stmts_recurse_(*parse_tree_);
  }
  tree_ok_ = true;
}

template <typename PG_NODE_DATA>
void Parsed_File<PG_NODE_DATA>::link_stmts_recurse_(
    typename Parse_Tree::node &n) {
  if (n.is_leaf()) {
    if (n->is_stmt()) {
      n->ll_stmt().set_hook(&n); // set an uplink from LL_Stmt to this node
    }
  } else {
    for (auto &b : n.branches()) {
      link_stmts_recurse_(b);
    }
  }
}

template <typename PG_NODE_DATA>
bool Parsed_File<PG_NODE_DATA>::indent_recurse_(typename Parse_Tree::node &n,
                                                Indent_Table const &indents,
                                                int curr_spaces) {
  bool changed = false;

  if (n.is_leaf()) {
    if (n->is_stmt()) {
      /* Do the actual indent only if you're the first or only statement on a
         line.  Note that the other statements on a compound line have their
         'spaces' member set correctly, so you can uncompound them easily. */
      if (n->ll_stmt().is_compound() < 2) {
        changed = n->ll_stmt().set_leading_spaces(curr_spaces,
                                                  indents.continued_offset());
      }
    }
  } else {
    if (Indent_Table::begin_end_construct(n->syntag())) {
      /* This handles a construct where there is a "begin" statement
         (e.g. select-case-stmt) and and "end" statement (e.g. end-select-stmt)
         that are not indented, and some arbitrarily complex set of statements
         between them that are indented */
      assert(n.branches().size() >= 2);
      size_t const middle_branches = n.branches().size() - 2;

      // Indent the begin stmt/first branch
      auto b_it = n.branches().begin();
      changed |= indent_recurse_(*b_it++, indents, curr_spaces);
      // Increase the indent
      curr_spaces += indents[n->syntag()];
      // Indent the middle statements
      for (size_t i = 0; i < middle_branches; ++i)
        changed |= indent_recurse_(*b_it++, indents, curr_spaces);
      // Decrease the indent
      curr_spaces -= indents[n->syntag()];
      // Indent the end stmt/last branch
      changed |= indent_recurse_(*b_it++, indents, curr_spaces);
      assert(b_it == n.branches().end());
    } else {
      // Simple case: indent all branches the same
      curr_spaces += indents[n->syntag()];
      for (auto &b : n.branches()) {
        changed |= indent_recurse_(b, indents, curr_spaces);
      }
    }
  }
  return changed;
}

} // namespace FLPR
#endif
