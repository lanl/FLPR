/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file utils.hh
*/

#ifndef FLPR_UTILS_HH
#define FLPR_UTILS_HH

#include <algorithm>
#include <cctype>
#include <iosfwd>
#include <string>
#include <vector>

namespace FLPR {
//! convert the given string to lower case
void tolower(std::string &);

//! convert the given string to upper case
void toupper(std::string &);

//! break the string s by the delimiter character
/*! Note that this skips repeated delimiters */
void simple_tokenize(std::string const &s, std::vector<std::string> &t,
                     const std::string::value_type delim = ' ');

//! remove whitespace from the begining and end of the string
inline std::string trim(const std::string &s) {
  const auto wsfront = std::find_if_not(s.begin(), s.end(),
                                        [](char c) { return std::isspace(c); });
  const auto wsback =
      std::find_if_not(s.rbegin(), std::string::const_reverse_iterator(wsfront),
                       [](char c) { return std::isspace(c); })
          .base();

  return std::string(wsfront, wsback);
}

//! remove whitespace from the end of the string (copy)
inline std::string trim_back_copy(std::string const &s) {
  std::string::size_type back = s.find_last_not_of(" \t");
  if (back == std::string::npos)
    return std::string();
  return std::string(s, 0, back + 1);
}

//! remove whitespace from the end of the string (in-place)
inline void trim_back(std::string &s) {
  std::string::size_type back = s.find_last_not_of(" \t");
  if (back == std::string::npos)
    s.clear();
  else
    s.erase(back + 1);
}

//! return the last non-blank character in s, or '\0'
inline char last_non_blank_char(std::string const &s) {
  std::string::size_type back = s.find_last_not_of(" \t");
  if (back == std::string::npos)
    return '\0';
  return s[back];
}

//! remove the capacity from a vector
template <typename T> inline void destroy(std::vector<T> &v) {
  std::vector<T> empty_vec;
  empty_vec.swap(v);
}

void print_ctrlchars(std::ostream &os, const std::string &s);
} // namespace FLPR
#endif
