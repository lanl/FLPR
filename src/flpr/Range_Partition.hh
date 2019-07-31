/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Range_Partition.hh
*/

#include "flpr/Safe_List.hh"
#include <optional>
#include <vector>

namespace FLPR {

template <typename Range, typename Chg_Trkr> class Range_Partition {
public:
  using iterator = typename Range::iterator;
  using const_iterator = typename Range::const_iterator;
  using index_type = size_t;
  using const_index = index_type const;
  using difference_type =
      typename std::iterator_traits<iterator>::difference_type;

public:
  constexpr explicit Range_Partition(index_type count)
      : parts_{count + 1}, end_idx_{count} {
    assert(count > 0);
    for (index_type i = 0; i < parts_.size(); ++i) {
      parts_[i].deactivate(i);
    }
  }

  void clear_partitions() {
    for (index_type i = 0; i < parts_.size(); ++i) {
      parts_[i].deactivate(i);
      parts_[i].chg_trkr.reset();
    }
  }

  constexpr bool empty() const noexcept { return active_empty_(); }
  constexpr iterator begin() noexcept { return succ_(end_idx_).begin; }
  constexpr const_iterator begin() const noexcept {
    return succ_(end_idx_).begin;
  }
  constexpr const_iterator cbegin() const noexcept {
    return succ_(end_idx_).begin;
  }
  constexpr iterator end() noexcept { return parts_[end_idx_].begin; }
  constexpr const_iterator end() const noexcept {
    return parts_[end_idx_].begin;
  }
  constexpr const_iterator cend() const noexcept {
    return parts_[end_idx_].begin;
  }

  constexpr bool empty(const_index idx) const noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    return cbegin(idx) == cend(idx);
  }
  constexpr iterator begin(const_index idx) noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    activate_(idx);
    return parts_[idx].begin;
  }
  constexpr const_iterator begin(const_index idx) const noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    return parts_[idx].begin;
  }
  constexpr const_iterator cbegin(const_index idx) const noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    return parts_[idx].begin;
  }
  constexpr iterator end(const_index idx) noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    activate_(idx);
    return succ_(idx).begin;
  }
  constexpr const_iterator end(const_index idx) const noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    return succ_(idx).begin;
  }
  constexpr const_iterator cend(const_index idx) const noexcept {
    assert(valid_idx_(idx)); // doesn't have to be active
    return succ_(idx).begin;
  }

  constexpr void set_tracker(const_index idx, Chg_Trkr const &trkr) {
    assert(valid_idx_(idx));
    parts_[idx].chg_trkr = trkr;
  }
  constexpr void set_tracker(const_index idx, Chg_Trkr &&trkr) {
    assert(valid_idx_(idx));
    parts_[idx].chg_trkr = std::move(trkr);
  }

  constexpr bool has_tracker(const_index idx) const noexcept {
    assert(valid_idx_(idx));
    return parts_[idx].chg_trkr.has_value();
  }

  Chg_Trkr &get_tracker(const_index idx) {
    assert(valid_idx_(idx));
    return *(parts_[idx].chg_trkr);
  }
  Chg_Trkr const &get_tracker(const_index idx) const {
    assert(valid_idx_(idx));
    return *(parts_[idx].chg_trkr);
  }

  void append(const_index idx, Range const &r_const) {
    Range &r{const_cast<Range &>(r_const)};
    append(idx, r.begin(), r.end());
  }
  void append(const_index idx, Range &&r) { append(idx, r.begin(), r.end()); }
  void append(const_index idx, iterator range_begin, iterator range_end) {
    assert(valid_idx_(idx));

    if (range_begin == range_end)
      return;

    assert(active_empty_() || range_begin == end());

    if (active_empty_() || last_active_idx_() < idx) {
      activate_(idx);
      parts_[idx].begin = range_begin;
    } else {
      assert(last_active_idx_() == idx);
    }
    parts_.back().begin = range_end;
  }

  void insert(const_index idx, iterator new_begin, iterator insert_pos) {
    assert(active_idx_(idx));
    difference_type const size_change = std::distance(new_begin, insert_pos);
    iterator old_begin = begin();
    if (insert_pos == old_begin) {
      /* inserting before the body changes our begin, and we tell the tracker
         for the previous partition that their end has changed. */
      parts_[idx].begin = new_begin;
      if (has_tracker(idx))
        parts_[idx].chg_trkr->update_begin(old_begin, new_begin, size_change);
      if (valid_idx_(idx - 1) && has_tracker(idx - 1)) {
        parts_[idx - 1].chg_trkr->update_end(old_begin, new_begin);
      }
    } else {
      /* inserting anywhere in the body only changes the length of tracked
         ranges */
      if (has_tracker(idx))
        parts_[idx].chg_trkr->update_size(size_change);
    }
  }

  bool validate() const;

private:
  struct Partition {
    static const_index bad_idx{20000000};
    Partition() : pred{bad_idx}, succ{bad_idx} {}
    iterator begin;
    index_type pred, succ;
    std::optional<Chg_Trkr> chg_trkr;

  public:
    constexpr bool active() const noexcept { return pred != succ; }
    constexpr void deactivate(const_index my_idx) noexcept {
      pred = my_idx;
      succ = my_idx;
    }
    constexpr bool okay() const noexcept {
      return pred != bad_idx && succ != bad_idx;
    }
  };

  std::vector<Partition> parts_;
  index_type end_idx_;

private:
  constexpr bool valid_idx_(const_index idx) const noexcept {
    return /* idx >= 0 && */ idx < end_idx_ && parts_[idx].okay();
  }
  constexpr bool active_idx_(const_index idx) const noexcept {
    return valid_idx_(idx) && parts_[idx].active();
  }
  constexpr index_type first_active_idx_() const noexcept {
    return parts_.back().succ;
  }
  constexpr index_type last_active_idx_() const noexcept {
    return parts_.back().pred;
  }
  constexpr bool active_empty_() const noexcept {
    return parts_.back().pred == end_idx_;
  }
  constexpr index_type next_active_idx_(index_type idx) const noexcept {
    if (parts_[idx].active())
      return parts_[idx].succ;
    do {
      idx += 1;
    } while (valid_idx_(idx) && !parts_[idx].active());
    /* note that this could be end_idx_ */
    return idx;
  }

  constexpr Partition &pred_(const_index idx) noexcept {
    return parts_[parts_[idx].pred];
  }
  constexpr Partition const &pred_(const_index idx) const noexcept {
    return parts_[parts_[idx].pred];
  }

  constexpr Partition &succ_(const_index idx) noexcept {
    return parts_[parts_[idx].succ];
  }
  constexpr Partition const &succ_(const_index idx) const noexcept {
    return parts_[parts_[idx].succ];
  }

  constexpr void activate_(const_index idx) noexcept {
    if (!parts_[idx].active()) {
      const_index succ = next_active_idx_(idx);
      const_index pred = parts_[succ].pred;
      parts_[pred].succ = idx;
      parts_[succ].pred = idx;
      parts_[idx].begin = parts_[succ].begin;
      parts_[idx].pred = pred;
      parts_[idx].succ = succ;
    }
  }
  constexpr void deactivate_(const_index idx) noexcept {
    if (parts_[idx].active()) {
      const_index succ = parts_[idx].succ;
      const_index pred = parts_[idx].pred;
      parts_[pred].succ = succ;
      parts_[succ].pred = pred;
      parts_[idx].deactivate_(idx);
    }
  }
};

template <typename Range, typename Chg_Trkr>
bool Range_Partition<Range, Chg_Trkr>::validate() const {

  assert(end_idx_ + 1 == parts_.size());
  auto const my_bad = parts_.size() + 1;

  /* check the active linked list entries */
  auto first_idx = my_bad;
  auto last_idx = end_idx_;
  auto next_idx = parts_[end_idx_].succ;
  for (index_type i = 0; i < end_idx_; ++i) {
    assert(valid_idx_(i));
    if ((parts_[i].succ == i) && (parts_[i].pred == i)) {
      assert(!parts_[i].active());
    } else {
      assert(active_idx_(i));
      if (first_idx == my_bad) {
        first_idx = i;
      }
      assert(i == next_idx);
      assert(parts_[i].pred == last_idx);
      last_idx = i;
      next_idx = parts_[i].succ;
      assert(next_idx > i);
    }
  }
  return true;
}
