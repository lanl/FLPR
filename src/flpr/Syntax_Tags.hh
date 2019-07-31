/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file include/flpr/Syntax_Tags.hh
  Definition of FLPR::Syntax_Tags, used to identify nodes in a
  parse tree.
*/

#ifndef FLPR_SYNTAX_TAGS_HH
#define FLPR_SYNTAX_TAGS_HH 1

#include "flpr/Syntax_Tags_Defs.hh"
#include <ostream>

namespace FLPR {

//! Define a class to hold the enums
/*! We aren't using an enum class here because we want to store the tags in the
  tree as an integer, so that the client can extend the tag values. */
struct Syntax_Tags {
  enum Tags { MAP(LISTIZE) };
  static constexpr char const *const strings[] = {MAP(STRINGIZE)};
  static constexpr int types[] = {MAP(TYPIZE)};
  static constexpr char const *label(int const tag) { return strings[tag]; }
  static constexpr int pg_begin_tag() { return PG_000_LB + 1; }
  static constexpr int pg_end_tag() { return PG_ZZZ_UB; }
  static constexpr bool is_name(int const tag) {
    return tag == TK_NAME || types[tag] == 4;
  }
  static std::ostream &print(std::ostream &os, int syntag) {
    if (syntag >= CLIENT_EXTENSION) {
      os << "<client-extension+" << syntag - CLIENT_EXTENSION << '>';
    } else {
      os << strings[syntag];
    }
    return os;
  }
  static constexpr bool is_keyword(int const syntag) {
    return (syntag < CLIENT_EXTENSION) ? (types[syntag] == 4) : true;
  }
  /* could add std::vectors for representing strings and types for extension
     classes: client could register TAG, STR, TYPE (for TAG >=
     CLIENT_EXTENSION), then the lookup routines could index as

     if(tag >= CLIENT_EXTENSION) return ext_strings[tag-CLIENT_EXTENSION];
  */
};

} // namespace FLPR

//! Print the name of the tag using syntag_str_
inline std::ostream &operator<<(std::ostream &os, FLPR::Syntax_Tags::Tags tag) {
  return os << FLPR::Syntax_Tags::label(tag);
}

#undef MAP
#undef LISTIZE
#undef STRINGIZE

#endif
