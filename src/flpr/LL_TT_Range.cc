/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_TT_Range.cc

  Definition of non-inline LL_TT_Range member functions.
*/

#include "flpr/LL_TT_Range.hh"

namespace FLPR {
std::ostream &LL_TT_Range::print(std::ostream &os) const {
  if (empty()) {
    os << "EMPTY";
  } else {
    for (auto const &t : *this) {
      os << t << ' ';
    }
  }
  return os;
}

} // namespace FLPR
