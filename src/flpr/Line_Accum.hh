/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Line_Accum.hh
*/

#ifndef FLPR_LINE_ACCUM_HH
#define FLPR_LINE_ACCUM_HH 1

#include <iosfwd>
#include <string>
#include <vector>

namespace FLPR {
//! A helper class for determining token offsets
/*! For a given Logical_Line, the flex scanner is given a string that is the
  concatenation of the main_txt field of File_Lines.  Flex reports the starting
  position of tokens as an offset into its input string. This class accumulates
  the string for the scanner, and provides translations from the flex starting
  location back to file and layout_ line and column numbers. */
class Line_Accum {
public:
  //! Add a main_txt string to the accumulator.
  void add_line(int const file_lineno, int const num_left_spaces,
                int const main_txt_file_colno, std::string const &main_txt,
                int const num_right_spaces);

  //! Return the file line and column
  /*!
    \param[in]  offset   The offset returned by yylex
    \param[out] lineno   The (index 1) line number in the input file
    \param[out] colno    The (index 1) column number in the input file
  */
  bool linecolno(int accum_offset, int &lineno, int &colno) const;

  //! Return the file line and column, and offsets into main_txt
  /*!
    \param[in]  offset     The offset returned by yylex
    \param[out] lineno     The (index 1) line number in the input file
    \param[out] colno      The (index 1) column number in the input file
    \param[out] txt_lineno The (index 0) main_txt line number
    \param[out] txt_colno  The (index 0) main_txt column number
  */
  bool linecolno(int accum_offset, int &lineno, int &colno, int &txt_lineno,
                 int &txt_colno) const;

  std::string const &accum() const { return accum_; }
  std::ostream &print(std::ostream &os) const;

private:
  std::string accum_;
  /* lli means "local line index", and it is the (index 0) line number into the
     implicit list of File_Lines that are added to this accumulator. */
  std::vector<int> lli_to_accum_offset_;
  std::vector<int> lli_to_file_line_num_;
  std::vector<int> lli_to_file_column_num_;
};
} // namespace FLPR
#endif
