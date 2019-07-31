/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file LL_Stmt_Src.hh
*/

#ifndef FLPR_LL_STMT_SRC_HH
#define FLPR_LL_STMT_SRC_HH 1

#include "flpr/LL_Stmt.hh"
#include "flpr/Logical_Line.hh"
#include <cassert>

namespace FLPR {
//! Presents an LL_SEQ as a sequence of statements
/*! This is a fairly strange hybrid between an istream and an
    InputIterator.  The similarity to istream comes from the fact that
    we don't know how many elements are going to be presented, so we
    advance and check the "goodness" of the source, rather than having
    a begin/end pair.  It is similar to an InputIterator in that you
    can dereference it without copying, because it constructs a buffer
    of LL_Stmt.
 */
class LL_Stmt_Src {
public:
  using value_type = LL_Stmt;
  using reference = LL_Stmt &;
  using const_reference = LL_Stmt const &;
  using pointer = LL_Stmt *;
  using const_pointer = LL_Stmt const *;

public:
  LL_Stmt_Src() = delete;
  LL_Stmt_Src(LL_Stmt_Src const &) = default;
  LL_Stmt_Src(LL_Stmt_Src &&) = default;
  LL_Stmt_Src &operator=(LL_Stmt_Src const &) = default;
  LL_Stmt_Src &operator=(LL_Stmt_Src &&) = default;

  //! Construct a source for the given Logical_Line sequence
  explicit LL_Stmt_Src(LL_SEQ &ll, bool const do_advance = true)
      : it_{ll}, curr_{buf_.end()} {
    if (do_advance)
      advance();
  }

  //! Construct a source for a single Logical_Line
  explicit LL_Stmt_Src(LL_SEQ::iterator ll, bool const do_advance = true)
      : it_{ll}, curr_{buf_.end()} {
    if (do_advance)
      advance();
  }

  //! Test to see if the source is still good
  explicit operator bool() {
    if (curr_ != buf_.end()) {
      return true;
    }
    return more_avail_();
  }
  //! Advance to the next LL_Stmt.
  /*! \returns false if there are none, true otherwise */
  bool advance() {
    bool retval{true};
    if (curr_ != buf_.end()) {
      std::advance(curr_, 1);
    }
    if (curr_ == buf_.end()) {
      buf_.clear();
      retval = more_avail_();
    }
    return retval;
  }

  LL_Stmt &&move() {
    assert(!buf_.empty());
    assert(curr_ != buf_.end());
    return std::move(*curr_);
  }

private:
  //! Return true if there are more statements available
  bool more_avail_() {
    while (buf_.empty() && it_)
      refill_();
    return !buf_.empty();
  }
  //! Attempt to refill the buffer from the next non-trivial LL
  void refill_();
  //! Where we are in the input sequence
  SL_Range_Iterator<Logical_Line> it_;
  using BUF_T = std::list<value_type>;
  //! Storage for the local LL_Stmt
  BUF_T buf_;
  //! Where we are in the buffer
  typename BUF_T::iterator curr_;
};
} // namespace FLPR
#endif
