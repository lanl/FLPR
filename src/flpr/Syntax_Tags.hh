/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

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
#include <string>
#include <vector>

namespace FLPR {

//! Define a class to hold the enums representing the syntax tags
/*! We aren't using an enum class here because we want to store the tags in the
  tree as an integer, so that the client can extend the tag values. */
struct Syntax_Tags {
  enum Tags { MAP(LISTIZE) };

public:
  static std::string label(int const syntag);
  static constexpr int pg_begin_tag() { return PG_000_LB + 1; }
  static constexpr int pg_end_tag() { return PG_ZZZ_UB; }
  static constexpr bool is_name(int const tag) {
    return tag == TK_NAME || types_[tag] == 4;
  }
  static int type(int const syntag);

  static std::ostream &print(std::ostream &os, int const syntag) {
    return os << label(syntag);
  }
  static bool is_keyword(int const syntag) { return type(syntag) == 4; }
  static bool register_ext(int const tag_idx, char const *const label,
                           int const type);

public:
  struct Ext_Record {
    std::string label;
    int type{-1};
    constexpr bool empty() const { return type == -1; }
  };

private:
  static constexpr char const *const strings_[] = {MAP(STRINGIZE)};
  static constexpr int types_[] = {MAP(TYPIZE)};
  static std::vector<Ext_Record> extensions_;
  static int get_ext_idx_(int const syntag) {
    if (syntag < CLIENT_EXTENSION)
      return -2;
    const int ext_idx = syntag - CLIENT_EXTENSION;
    if (ext_idx >= static_cast<int>(extensions_.size()) ||
        extensions_[ext_idx].empty())
      return -1;
    return ext_idx;
  }
};

} // namespace FLPR

#undef MAP
#undef LISTIZE
#undef STRINGIZE

#endif
