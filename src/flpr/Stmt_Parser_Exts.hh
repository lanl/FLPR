/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Stmt_Parser_Exts.hh
*/

#ifndef FLPR_STMT_PARSER_EXTS_HH
#define FLPR_STMT_PARSER_EXTS_HH 1

#include "flpr/Stmt_Parsers.hh"
#include "flpr/Stmt_Tree.hh"
#include <vector>

namespace FLPR {
namespace Stmt {
//! Manage extensions for the statement parsers
/*! The Parser_Exts class is (optionally) used to provide application-specific
    statement parsers.  This allows the client application to extend the
    language being recognized in Prgm::Parsers */
class Parser_Exts {
public:
  //! The statement parser function signature
  /*! Note that this is the same as what you find in parse_stmt.hh */
  using stmt_parser = Stmt_Tree (*)(TT_Stream &ts);

public:
  //! Register an action-stmt extension
  void register_action_stmt(stmt_parser ext) noexcept;
  //! Register an other-specification-stmt extension
  void register_other_specification_stmt(stmt_parser ext) noexcept;
  //! Clear all registered extensions
  void clear() noexcept;

  //@{
  /*! This is called by a driver routine in parse_stmt.cc and is not intended
      for client use */
  SP_Result parse_action_stmt(TT_Stream &ts);
  SP_Result parse_other_specification_stmt(TT_Stream &ts);
  //@}
private:
  std::vector<stmt_parser> action_exts_;
  std::vector<stmt_parser> other_specification_exts_;
};

//! Access the Parser_Exts singleton
Parser_Exts &get_parser_exts() noexcept;

} // namespace Stmt
} // namespace FLPR
#endif
