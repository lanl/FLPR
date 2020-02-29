/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Logical_File.hh
*/

#ifndef FLPR_LOGICAL_FILE_HH
#define FLPR_LOGICAL_FILE_HH 1

#include "flpr/File_Info.hh"
#include "flpr/LL_Stmt.hh"
#include "flpr/Logical_Line.hh"
#include "flpr/Safe_List.hh"
#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace FLPR {
/*! \brief A sequence of Logical_Lines and LL_Stmts that make up a file, plus
  other identifying information. */
class Logical_File {
public:
  //! Container for raw text lines of a file
  using Line_Buf = std::vector<std::string>;
  //! Type for the container of lines
  using const_iterator = typename LL_SEQ::const_iterator;
  using iterator = typename LL_SEQ::iterator;

  constexpr Logical_File() : has_flpr_pp{false}, num_input_lines{0} {}
  Logical_File(Logical_File &&) = default;
  Logical_File(Logical_File const &) = delete;
  Logical_File &operator=(Logical_File const &) = delete;
  Logical_File &operator=(Logical_File &&) = delete;
  ~Logical_File() = default;

  //! Read in the contents of the named file and scan
  bool read_and_scan(std::string const &filename, int const last_fixed_col,
                     File_Type file_type = File_Type::UNKNOWN);

  //! Read in the contents of a stream and scan
  bool read_and_scan(std::istream &is, std::string const &stream_name,
                     int const last_fixed_col,
                     File_Type file_type = File_Type::UNKNOWN);

  //! Scan a list of raw lines
  bool scan(Line_Buf const &line_buffer, std::string const &buffer_name,
            int const last_fixed_col, File_Type file_type = File_Type::UNKNOWN);

  //! Scan the file assuming F77-style fixed format
  bool scan_fixed(Line_Buf const &fl, int const last_col);

  //! Scan the file assuming F90-style free format
  bool scan_free(Line_Buf const &fl);

  File_Type file_type() const {
    if (file_info)
      return file_info->file_type;
    return File_Type::UNKNOWN;
  }
  bool is_fixed_format() const { return file_type() == File_Type::FIXEDFMT; }
  //! Populate ll_stmts: call after lines are loaded
  void make_stmts();

  //! Break a compound into two Logical_Lines before this statement
  /*! If pos refers to the first statement of a compound line, nothing happens
      and false is returned.  Otherwise a new Logical_Line is created following
      pos->ll(), pos (and any subsequent compounds) are moved to the new line,
      and the Logical_Line and ll_stmts are updated.  Note that the ll_stmts are
      updated in-place, so any Safe_List iterators to them are still good. */
  bool split_compound_before(LL_STMT_SEQ::iterator pos);

  //! Make sure that this statement is the only one in a Logical_Line
  bool isolate_stmt(LL_STMT_SEQ::iterator pos);

  //! emplace a one-stmt Logical_Line before the prefix of statement pos
  LL_STMT_SEQ::iterator emplace_ll_stmt(LL_STMT_SEQ::iterator pos,
                                        Logical_Line &&ll, int new_syntag);

  //! emplace a one-stmt Logical_Line after the prefix of statement pos
  LL_STMT_SEQ::iterator emplace_ll_stmt_after_prefix(LL_STMT_SEQ::iterator pos,
                                                     Logical_Line &&ll,
                                                     int new_syntag);

  /* "regen" means re-parse the statement, and fix up ll_stmts */

  //! Replace all of the text of a statement and regen
  void replace_stmt_text(LL_STMT_SEQ::iterator stmt,
                         std::vector<std::string> const &new_text,
                         int new_syntag);

  //! Replace the text covered by orig_tt with new_txt and regen
  void replace_stmt_substr(LL_STMT_SEQ::iterator stmt,
                           LL_TT_Range const &orig_tt,
                           std::string const &new_txt);

  //! Append some text to the end of stmt and regen
  void append_stmt_text(LL_STMT_SEQ::iterator stmt,
                        std::string const &new_text);

  //! Label a statement (label == 0 will unlabel it)
  bool set_stmt_label(LL_STMT_SEQ::iterator stmt, int label);

  //! Convert fixed format to free
  bool convert_fixed_to_free();

public:
  //! Basic information about the input file
  std::shared_ptr<File_Info> file_info;
  //! The scanned Logical_Lines
  LL_SEQ lines;
  //! The LL_Stmt's associated with lines
  LL_STMT_SEQ ll_stmts;
  //! True if read_and_scan found FLPR preprocessor lines
  bool has_flpr_pp;
  //! Number of scanned line
  size_t num_input_lines;

private:
  //! Clear the contents of this structure
  void clear();
};

} // namespace FLPR
#endif
