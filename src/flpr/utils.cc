/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file utils.cc
*/

#include "flpr/utils.hh"
#include <ios>
#include <ostream>

namespace FLPR {
/* ------------------------------------------------------------------------ */
void tolower(std::string &foo) {
  for (std::string::size_type i = 0; i < foo.size(); ++i)
    foo[i] = std::tolower(foo[i]);
}

/* ------------------------------------------------------------------------ */
void toupper(std::string &foo) {
  for (std::string::size_type i = 0; i < foo.size(); ++i)
    foo[i] = std::toupper(foo[i]);
}

/* ------------------------------------------------------------------------ */
void simple_tokenize(std::string const &s, std::vector<std::string> &t,
                     const std::string::value_type delim) {
  std::string::const_iterator sp = s.begin();
  do {
    std::string::const_iterator tbeg = sp;
    while (s.end() != sp && delim != *sp)
      ++sp;
    if (tbeg != sp)
      t.push_back(std::string(tbeg, sp));
  } while (s.end() != sp++);
}

/* ------------------------------------------------------------------------ */
void print_ctrlchars(std::ostream &os, const std::string &s) {
  for (auto c : s) {
    if (c < 32)
      os << std::hex << std::showbase << int(c);
    else
      os << c;
  }
}

} // namespace FLPR
