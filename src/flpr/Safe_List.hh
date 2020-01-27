/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file include/flpr/Safe_List.hh
*/

#ifndef FLPR_SAFE_LIST_HH
#define FLPR_SAFE_LIST_HH

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <list>
#include <utility>

namespace FLPR {
//! Define a list container with a concrete end() element
/*!
  While the C++ standard states that an iterator to an element of a
  std::list remains valid for all list operations except for the
  deletion of that particular element, it makes no such guarantees for
  the end() iterator. This class adds a default constructed element at
  the end of the list which is used as "end()", and modifies std::list
  operations to hide that fact.  This makes it safe to store "end()"
  (or any other iterator) in another data structure.

  *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE ***

  If a class C is holding iterators into a member Safe_List, be very
  careful with op= and copy-ctors of C : those iterators need to be
  updated to refer to the new Safe_List.

  *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE ***
*/
template <class T, class Alloc = std::allocator<T>> class Safe_List {
public:
  using base_type = std::list<T, Alloc>;
  using value_type = typename base_type::value_type;
  using reference = typename base_type::reference;
  using const_reference = typename base_type::const_reference;
  using iterator = typename base_type::iterator;
  using const_iterator = typename base_type::const_iterator;
  using pointer = typename base_type::pointer;
  using const_pointer = typename base_type::const_pointer;
  using size_type = typename base_type::size_type;

  /*********************** Altered functions *************************/
  constexpr Safe_List() : list_{} { append_sentry_(); }
  constexpr Safe_List(size_type count, const T &value) : list_(count, value) {
    append_sentry_();
  }
  constexpr explicit Safe_List(size_type count) : list_(count) {
    append_sentry_();
  }
  constexpr Safe_List(std::initializer_list<T> init) : list_(init) {
    append_sentry_();
  }
  constexpr Safe_List(Safe_List const &src) : list_{src.list_} {
    my_end_ = std::prev(list_.end());
  }
  constexpr Safe_List(Safe_List &&src) : list_{std::move(src.list_)} {
    my_end_ = std::prev(list_.end());
  }
  constexpr Safe_List &operator=(Safe_List const &src) {
    list_ = src.list_;
    my_end_ = std::prev(list_.end());
    return *this;
  }
  constexpr Safe_List &operator=(Safe_List &&src) {
    if (this != &src) {
      list_ = std::move(src.list_);
      my_end_ = std::prev(list_.end());
    }
    return *this;
  }
  ~Safe_List() = default;

  constexpr reference back() { return *(std::prev(my_end_)); }
  constexpr const_reference back() const { return *(std::prev(my_end_)); }
  constexpr iterator end() noexcept { return my_end_; }
  constexpr const_iterator end() const noexcept { return my_end_; }
  constexpr const_iterator cend() const noexcept { return my_end_; }
  constexpr void clear() {
    list_.clear();
    append_sentry_();
  }
  constexpr size_type size() const noexcept { return list_.size() - 1; };
  constexpr bool empty() const noexcept { return list_.size() < 2; };
  template <class... Args> constexpr reference emplace_back(Args &&... args) {
    list_.emplace(my_end_, std::forward<Args>(args)...);
    return back();
  }
  constexpr void push_back(const T &value) { list_.insert(my_end_, value); }
  constexpr void push_back(T &&value) { list_.insert(my_end_, value); }
  constexpr void pop_back() { list_.erase(std::prev(my_end_)); }
  template <class UnaryPredicate> constexpr void remove_if(UnaryPredicate p) {
    list_.remove_if(p);
    // If the predicate removed our sentry, replace it
    if (list_.empty() && std::prev(list_.end()) != my_end_)
      append_sentry_();
  }

  /*********************** Pass-thru functions ***********************/
  constexpr reference front() { return list_.front(); }
  constexpr const_reference front() const { return list_.front(); }
  constexpr iterator begin() noexcept { return list_.begin(); }
  constexpr const_iterator begin() const noexcept { return list_.begin(); }
  constexpr const_iterator cbegin() const noexcept { return list_.begin(); }
  template <class... Args>
  constexpr iterator emplace(const_iterator pos, Args &&... args) {
    return list_.emplace(pos, std::forward<Args>(args)...);
  }
  template <class... Args> constexpr reference emplace_front(Args &&... args) {
    list_.emplace_front(std::forward<Args>(args)...);
    return front();
  }
  constexpr iterator insert(const_iterator pos, const T &value) {
    return list_.insert(pos, value);
  }
  constexpr iterator insert(const_iterator pos, T &&value) {
    return list_.insert(pos, value);
  }
  template <class InputIt>
  constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
    return list_.insert(pos, first, last);
  }
  constexpr iterator erase(const_iterator pos) { return list_.erase(pos); }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    return list_.erase(first, last);
  }

private:
  base_type list_;  //!< The underlying std::list
  iterator my_end_; //!< The hidden element that serves as end()

private:
  //! Append the hidden element to an existing list
  /*! Note, this should only be used after some ctor initialization of list_ */
  constexpr void append_sentry_() { my_end_ = list_.insert(list_.end(), T()); }
};

//! Define a range of elements in a Safe_List
template <class T> class SL_Range {
public:
  using SL_SEQ = Safe_List<T>;
  using value_type = typename SL_SEQ::value_type;
  using iterator = typename SL_SEQ::iterator;
  using const_iterator = typename SL_SEQ::const_iterator;
  using reference = typename SL_SEQ::reference;
  using const_reference = typename SL_SEQ::const_reference;
  using pointer = typename SL_SEQ::pointer;
  using const_pointer = typename SL_SEQ::const_pointer;

public:
  constexpr SL_Range() : size_{0}, bad_{true} {}
  constexpr explicit SL_Range(SL_SEQ &seq)
      : begin_{seq.begin()}, size_{seq.size()}, end_{seq.end()}, bad_{false} {}
  constexpr SL_Range(iterator begin, iterator end)
      : begin_{begin}, size_{}, end_{end}, bad_{false} {
    size_ = static_cast<size_t>(std::distance(begin, end));
  }
  constexpr SL_Range(iterator only)
      : begin_{only}, size_{1}, end_{std::next(only)}, bad_{false} {}

  SL_Range(SL_Range const &) = default;
  SL_Range(SL_Range &&) = default;
  SL_Range &operator=(SL_Range const &) = default;
  SL_Range &operator=(SL_Range &&) = default;
  virtual ~SL_Range() = default;

  constexpr iterator begin() noexcept { return begin_; }
  constexpr const_iterator begin() const noexcept { return begin_; }
  constexpr const_iterator cbegin() const noexcept { return begin_; }

  //! Return the last iterator in the sequence
  constexpr iterator last() noexcept {
    assert(!empty());
    return std::next(begin_, size_ - 1);
  }
  constexpr const_iterator last() const noexcept {
    assert(!empty());
    return std::next(begin_, size_ - 1);
  }

  constexpr iterator end() noexcept { return end_; }
  constexpr const_iterator end() const noexcept { return end_; }
  constexpr const_iterator cend() const noexcept { return end_; }

  constexpr reference front() {
    assert(!empty());
    return *begin_;
  };
  constexpr const_reference front() const {
    assert(!empty());
    return *begin_;
  };
  constexpr reference back() {
    assert(!empty());
    return *last();
  };
  constexpr const_reference back() const {
    assert(!empty());
    return *last();
  };
  constexpr void clear() noexcept {
    size_ = 0;
    bad_ = true;
  }
  /* after this, begin() and end() return empty_end, and size() == 0 */
  constexpr void clear(iterator empty_end) noexcept {
    begin_ = end_ = empty_end;
    size_ = 0;
    bad_ = false;
  }
  constexpr bool empty() const noexcept {
    assert(bad_ || std::next(begin_, size_) == end_);
    return empty_();
  }
  constexpr inline bool equal(SL_Range const &rhs) const;
  constexpr void push_front(SL_Range const &adj) {
    if (adj.empty())
      return;
    if (empty()) {
      *this = adj;
    } else {
      if (adj.begin_ != begin_) {
        assert(begin_ == adj.end());
        begin_ = adj.begin_;
        size_ += adj.size();
      }
    }
  }
  constexpr void push_back(SL_Range const &adj) {
    if (adj.empty())
      return;
    if (empty()) {
      *this = adj;
    } else {
      if (adj.end() != end()) {
        assert(end() == adj.begin_);
        end_ = adj.end_;
        size_ += adj.size();
      }
    }
  }
  constexpr void update_end(const_iterator it) {
    if (empty_()) {
      bad_ = false;
      begin_ = end_ = it;
    } else {
      assert(std::next(begin_, size_) == it);
      end_ = it;
    }
  }
  //! make this particular function easily available to derived classes
  constexpr void assign_range(SL_Range const &r) { operator=(r); }
  constexpr void assign_range(SL_Range &&r) { operator=(std::move(r)); }
  constexpr size_t size() const noexcept { return bad_ ? 0 : size_; }

private:
  //! the iterator to the start location in the SL_SEQ
  iterator begin_;

  //! the number of elements in this range
  size_t size_;
  //! the iterator to the end location
  /*! Note that this is redundant with size_, but it provides a way of checking
  if (A) something invalidated end_ by inserting something before it, or (B) the
  user invalidated size_ by not doing a this->push_back() after an insert before
  end_. */
  iterator end_;

  /*! We use this when SL_Range is default constructed or clear(), to
    differentiate from the empty() state of size_ == 0, as begin is
    uninitialized (therefore begin() and end() are bad)*/
  bool bad_;

private:
  constexpr bool empty_() const noexcept { return bad_ || size_ == 0; }
};

template <class T>
constexpr inline bool SL_Range<T>::equal(SL_Range<T> const &rhs) const {
  // Don't comparare begin_ & end_ if these are bad
  return bad_ == rhs.bad_ &&
         (bad_ || (begin_ == rhs.begin_ && size_ == rhs.size_));
}

//! Transfer a range to a copy of a sequence
/*!
  This creates a new range that is equivalent to \p src_range, but is
  relative to \p cpy_seq rather than \p src_seq.

  \param src_seq    the original SL_SEQ
  \param src_range  a range relative to \p src_seq
  \param cpy_seq    a (deep) copy of \p src_seq
*/
template <class T>
constexpr SL_Range<T> rebase(typename Safe_List<T>::const_iterator src_seq_beg,
                             SL_Range<T> const &src_range,
                             typename Safe_List<T>::iterator cpy_seq_beg) {
  return SL_Range<T>(
      std::next(cpy_seq_beg, std::distance(src_seq_beg, src_range.cbegin())),
      std::next(cpy_seq_beg, std::distance(src_seq_beg, src_range.cend())));
}

//! Define a range of const elements in a Safe_List
template <class T> class SL_Const_Range {
public:
  using SL_SEQ = Safe_List<T>;
  using value_type = typename SL_SEQ::value_type;
  using const_iterator = typename SL_SEQ::const_iterator;
  using const_reference = typename SL_SEQ::const_reference;
  using const_pointer = typename SL_SEQ::const_pointer;

public:
  constexpr explicit SL_Const_Range(SL_SEQ const &seq)
      : begin_{seq.cbegin()}, size_{seq.size()}, end_{seq.cend()} {}
  constexpr SL_Const_Range(const_iterator begin, const_iterator end)
      : begin_{begin}, size_{}, end_{end} {
    size_ = static_cast<size_t>(std::distance(begin, end));
  }
  constexpr SL_Const_Range(const_iterator only)
      : begin_{only}, size_{1}, end_{std::next(only)} {}

  SL_Const_Range(SL_Const_Range const &) = default;
  SL_Const_Range(SL_Const_Range &&) = default;
  SL_Const_Range &operator=(SL_Const_Range const &) = default;
  SL_Const_Range &operator=(SL_Const_Range &&) = default;

  constexpr const_iterator begin() const noexcept { return begin_; }
  constexpr const_iterator cbegin() const noexcept { return begin_; }

  //! Return the last iterator in the sequence
  constexpr const_iterator last() const noexcept {
    assert(!empty());
    return std::next(begin_, size_ - 1);
  }

  constexpr const_iterator end() const noexcept { return end_; }
  constexpr const_iterator cend() const noexcept { return end_; }

  constexpr const_reference front() const {
    assert(!empty());
    return *begin_;
  };
  constexpr const_reference back() const {
    assert(!empty());
    return *last();
  };
  constexpr bool empty() const noexcept {
    assert(std::next(begin_, size_) == end_);
    return size_ == 0;
  }
  constexpr size_t size() const noexcept { return size_; }

private:
  //! the iterator to the start location in the SL_SEQ
  const_iterator begin_;

  //! the number of elements in this range
  size_t size_;
  //! the iterator to the end location
  /*! Note that this is redundant with size_, but it provides a way of checking
  if (A) something invalidated end_ by inserting something before it, or (B) the
  user invalidated size_ by not doing a this->push_back() after an insert before
  end_. */
  const_iterator end_;
};

//! An iterator with a contained end().
template <class T> class SL_Range_Iterator : public SL_Range<T> {
public:
  using SL_SEQ = Safe_List<T>;
  using value_type = typename SL_Range<T>::value_type;
  using iterator = typename SL_Range<T>::iterator;
  using const_iterator = typename SL_Range<T>::const_iterator;
  using reference = typename SL_Range<T>::reference;
  using const_reference = typename SL_Range<T>::const_reference;
  using pointer = typename SL_Range<T>::pointer;
  using const_pointer = typename SL_Range<T>::const_pointer;

  constexpr explicit SL_Range_Iterator(SL_Range<T> &r)
      : SL_Range<T>{r}, curr_{r.begin()} {}
  constexpr explicit SL_Range_Iterator(SL_SEQ &seq)
      : SL_Range<T>{seq}, curr_{seq.begin()} {}
  constexpr explicit SL_Range_Iterator(typename SL_SEQ::iterator it)
      : SL_Range<T>{it}, curr_{it} {}
  constexpr explicit operator bool() const noexcept {
    return curr_ != SL_Range<T>::cend();
  }
  constexpr bool operator!() const noexcept {
    return curr_ == SL_Range<T>::cend();
  }
  constexpr typename SL_SEQ::iterator iter() { return curr_; }
  constexpr operator iterator() noexcept { return curr_; }
  constexpr operator const_iterator() const noexcept { return curr_; }
  constexpr pointer operator->() { return &(*curr_); }
  constexpr const_pointer operator->() const { return &(*curr_); }
  constexpr reference operator*() { return *curr_; }
  constexpr const_reference operator*() const { return *curr_; }
  constexpr bool advance() {
    if (curr_ != SL_Range<T>::end()) {
      std::advance(curr_, 1);
    }
    return curr_ != SL_Range<T>::end();
  }

private:
  iterator curr_;
};

template <class T>
inline bool operator==(Safe_List<T> const &lhs, Safe_List<T> const &rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
} // namespace FLPR
#endif
