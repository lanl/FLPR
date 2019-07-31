/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#ifndef LL_HELPER_HH
#define LL_HELPER_HH 1

#include "flpr/LL_Stmt_Src.hh"
#include "flpr/Logical_File.hh"
#include "flpr/TT_Stream.hh"
#include <ostream>

//! Logical_Line helper
/*! This forms a FLPR::Logical_File structure by parsing the strings given in \c
buf member of the constructor.  The file contents can then be accessed through
an FLPR::LL_Stmt_Src provided by src().  For convenience, \c stream1() returns a
FLPR::TT_Stream for the first statement. */
class LL_Helper {
public:
  using Raw_Lines = FLPR::Logical_File::Line_Buf;
  LL_Helper(Raw_Lines &&buf, const bool is_free_format = true) noexcept;
  //! Return a TT_Stream for the first statement
  FLPR::TT_Stream stream1() noexcept {
    return FLPR::TT_Stream{text_.ll_stmts.front()};
  }
  constexpr FLPR::LL_SEQ &lines() { return text_.lines; }
  constexpr FLPR::LL_STMT_SEQ &ll_stmts() { return text_.ll_stmts; }
  //! dump the contents of the Logical_File
  std::ostream &print(std::ostream &os) const;
  constexpr FLPR::Logical_File &logical_file() { return text_; }

private:
  FLPR::Logical_File text_;
};

#endif
