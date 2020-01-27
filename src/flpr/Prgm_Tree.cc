/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Prgm_Tree.cc
*/

#include "flpr/Prgm_Tree.hh"
#include "flpr/Prgm_Parsers.hh"

namespace FLPR {
namespace Prgm {

std::ostream &operator<<(std::ostream &os, Prgm_Node_Data const &pn) {
  if (pn.is_stmt()) {
    if (pn.stmt_tree()) {
      os << '[' << pn.stmt_tree() << ']';
    }
  } else {
    Syntax_Tags::print(os, pn.syntag());
  }
  return os;
}

// do an explicit instantiation of the default Prgm::Parsers
template struct Parsers<>;

} // namespace Prgm
} // namespace FLPR
