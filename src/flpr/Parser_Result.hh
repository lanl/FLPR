/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Parser_Result.hh
*/

#ifndef FLPR_PARSER_RESULT_HH
#define FLPR_PARSER_RESULT_HH 1

#include <ostream>

namespace FLPR {
namespace details_ {
//! Utility class for parser results
template <typename TT> class Parser_Result {
public:
  using tree_type = TT;
  Parser_Result() : parse_tree{}, match{false} {}
  Parser_Result(Parser_Result const &) = delete;
  Parser_Result(Parser_Result &&) = default;
  Parser_Result &operator=(Parser_Result const &) = delete;
  Parser_Result &operator=(Parser_Result &&) = default;
  Parser_Result(tree_type &&pt, bool match) noexcept
      : parse_tree{std::move(pt)}, match{match} {}
  operator tree_type() { return std::move(parse_tree); }

  /*! @name result_data The data returned by a parser
   * These are public to allow for structured bindings.  A parser, such as
   * Optional_Parser, may "match" a non-existent term, so we can't rely on the
   * state of the tree alone.
   */
  //! The (possibly bad) parse tree matched by the parser
  tree_type parse_tree;
  //! Whether the parser considered this a success or not.
  bool match;
};

template <typename TT>
std::ostream &operator<<(std::ostream &os, Parser_Result<TT> const &r) {
  if (r.match)
    os << *(r.parse_tree);
  else
    os << "()";
  return os;
}

template <typename F, typename... Args>
constexpr bool and_fold(F f, Args... args) noexcept {
  return (... && f(args));
}

template <typename F, typename... Args>
constexpr bool or_fold(F f, Args... args) noexcept {
  return (... || f(args));
}

} // namespace details_
} // namespace FLPR

#endif
