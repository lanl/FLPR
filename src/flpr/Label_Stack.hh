/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Label_Stack.hh
*/
#ifndef FLPR_LABEL_STACK_HH
#define FLPR_LABEL_STACK_HH 1

#include <vector>

namespace FLPR {

class Label_Stack {
public:
  //! add a new label to the top of the stack
  void push(int const label) { label_stack_.emplace_back(Label_Rec_(label)); }

  //! remove the entry at the top of the stack
  void pop() { label_stack_.pop_back(); }

  //! Return true if label matches the top stack entry
  bool is_top(int const label) const {
    return (!label_stack_.empty() && label_stack_.back().label == label);
  }

  //! if label matches the top stack entry, return the level, else -1
  inline int level(int const label);

  //! return true if empty
  constexpr bool empty() const { return label_stack_.empty(); }

  //! return the number of elements in the stack
  constexpr size_t size() const { return label_stack_.size(); }

private:
  /* The meaning of the "level" field
     -1 : levels not yet computed
      0 : 1 of 1 of this label
      1 : 1 of N of this label (outer-most)
        ...
      N : N of N of this label (inner-most)
  */
  struct Label_Rec_ {
  public:
    Label_Rec_() = default;
    explicit Label_Rec_(int const l) : label(l), level(-1) {}

  public:
    int label, level;
  };
  using Stack_T = std::vector<Label_Rec_>;
  Stack_T label_stack_;
};

inline int Label_Stack::level(int const label) {
  /* If the stack is empty or label doesn't match the top entry, return -1 */
  if (label_stack_.empty())
    return -1;
  if (label_stack_.back().label != label)
    return -1;

  /* If the levels have not been assigned, do so */
  if (label_stack_.back().level == -1) {
    Stack_T::reverse_iterator it = label_stack_.rbegin();
    while (it != label_stack_.rend() && it->label == label)
      std::advance(it, 1);
    auto count = std::distance(label_stack_.rbegin(), it);
    assert(count > 0);
    if (count == 1) {
      /* If there is only one of this label at the top of the stack, it
         gets a level == 0 */
      std::advance(it, -1);
      it->level = 0;
    } else {
      /* If there are more than one of this label in a sequence at the top of
         the stack, they are assigned a label of "i" (of count).  For example,
         if count == 2, the top of the stack entry will get a level of two, and
         the second entry on the stack will get a level of one. */
      for (int i = 0; i < count; ++i) {
        std::advance(it, -1);
        it->level = i + 1;
      }
    }
  }

  return label_stack_.back().level;
}

} // namespace FLPR

#endif
