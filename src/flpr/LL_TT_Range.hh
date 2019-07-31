/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_TT_Range.hh
*/

#ifndef FLPR_LL_TT_RANGE_HH
#define FLPR_LL_TT_RANGE_HH 1

#include "flpr/Logical_Line.hh"
#include "flpr/Token_Text.hh"

namespace FLPR {
//! Identify a range of elements in the fragments of a particular Logical_Line
class LL_TT_Range : public TT_Range {
public:
  using LL_IT = LL_SEQ::iterator;

  LL_TT_Range() : TT_Range(), ll_set_{false} {}
  LL_TT_Range(LL_TT_Range const &) = default;
  LL_TT_Range(LL_TT_Range &&) = default;
  LL_TT_Range &operator=(LL_TT_Range const &) = default;
  LL_TT_Range &operator=(LL_TT_Range &&) = default;

  LL_TT_Range(LL_IT const &line_ref, TT_Range const &r)
      : TT_Range{r}, line_ref_{line_ref}, ll_set_{true} {}
  LL_TT_Range(LL_IT const &line_ref, TT_Range::iterator const &beg,
              TT_Range::iterator const &end)
      : TT_Range{beg, end}, line_ref_{line_ref}, ll_set_{true} {}

  //! Access the iterator to the owning Logical_Line
  LL_IT it() const {
    assert(ll_set_);
    return line_ref_;
  }

  //! Access the owning Logical_Line
  Logical_Line &ll() { return *it(); }
  Logical_Line const &ll() const {
    assert(ll_set_);
    return *it();
  }

  //! Update the owning Logical_Line iterator
  /*! Used to move this TT_Range of tokens to a new LL */
  void set_it(LL_IT const it) {
    line_ref_ = it;
    ll_set_ = true;
  }

  //! Return true if in a multi-statement Logical_Line
  bool in_compound() const noexcept { return ll().is_compound(); }

  //! Return the line number associated with the first token
  constexpr int linenum() const {
    if (empty())
      return -1;
    return front().start_line;
  }

  //! Return the column number associated with the first token
  constexpr int colnum() const {
    if (empty())
      return -1;
    return front().start_pos;
  }

  constexpr bool equal(LL_TT_Range const &rhs) const {
    return TT_Range::equal(rhs) && line_ref_ == rhs.line_ref_;
  }

  virtual std::ostream &print(std::ostream &os) const;

protected:
  LL_IT line_ref_;

private:
  bool ll_set_;
};
} // namespace FLPR
#endif
