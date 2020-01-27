/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Line_Accum.cc
*/

#include "flpr/Line_Accum.hh"
#include <cassert>
#include <iostream>
#include <ostream>

namespace FLPR {
void Line_Accum::add_line(int const file_lineno, int const num_left_spaces,
                          int const main_txt_file_colno,
                          std::string const &main_txt,
                          int const num_right_spaces) {
  /* main_txt starts at lli_to_accum_offset_[i], and relates to file line
     numbers lli_to_file_line_num_[i], and column number
     lli_to_file_column_num_[i]. */
  if (!lli_to_accum_offset_.empty() && num_left_spaces > 0) {
    accum_ += ' ';
  }
  lli_to_accum_offset_.push_back(accum_.size());
  lli_to_file_line_num_.push_back(file_lineno);
  lli_to_file_column_num_.push_back(main_txt_file_colno);
  accum_ += main_txt;
  if (num_right_spaces > 0) {
    accum_ += ' ';
  }
}

bool Line_Accum::linecolno(int accum_offset, int &lineno, int &colno) const {
  if (lli_to_accum_offset_.empty())
    return false;

  /* find the LLI */
  size_t const N = lli_to_accum_offset_.size();
  size_t lli{0};
  while (lli + 1 < N && lli_to_accum_offset_[lli + 1] <= accum_offset) {
    lli += 1;
  }

  assert(lli < lli_to_accum_offset_.size());
  assert(lli_to_accum_offset_[lli] <= accum_offset);

  /* adjust to be the offset within the main_txt for this LLI */
  accum_offset -= lli_to_accum_offset_[lli];
  lineno = lli_to_file_line_num_[lli];
  colno = lli_to_file_column_num_[lli] + accum_offset;

  assert(colno > 0);

  return true;
}

bool Line_Accum::linecolno(int accum_offset, int &lineno, int &colno,
                           int &txt_lineno, int &txt_colno) const {
  if (lli_to_accum_offset_.empty())
    return false;

  /* find the LLI */
  size_t const N = lli_to_accum_offset_.size();
  size_t lli{0};
  while (lli + 1 < N && lli_to_accum_offset_[lli + 1] <= accum_offset) {
    lli += 1;
  }

  assert(lli < lli_to_accum_offset_.size());
  assert(lli_to_accum_offset_[lli] <= accum_offset);

  /* adjust to be the offset within the main_txt for this LLI */
  accum_offset -= lli_to_accum_offset_[lli];
  lineno = lli_to_file_line_num_[lli];
  colno = lli_to_file_column_num_[lli] + accum_offset;
  txt_lineno = (int)lli;
  txt_colno = accum_offset;

  assert(colno > 0);
  assert(txt_lineno >= 0);
  assert(txt_lineno < static_cast<int>(lli_to_file_line_num_.size()));
  assert(txt_colno >= 0);

  return true;
}

std::ostream &Line_Accum::print(std::ostream &os) const {
  os << '\"' << accum_ << '\"' << '\n';
  for (auto const &i : lli_to_accum_offset_)
    os << i << ' ';
  os << '\n';
  for (auto const &i : lli_to_file_line_num_)
    os << i << ' ';
  os << '\n';
  for (auto const &i : lli_to_file_column_num_)
    os << i << ' ';
  os << '\n';
  return os;
}
} // namespace FLPR
