/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Procedure.hh
*/

#ifndef FLPR_PROCEDURE_HH
#define FLPR_PROCEDURE_HH 1

#include "flpr/Range_Partition.hh"
#include "flpr/Token_Text.hh"
#include <cassert>
#include <string>

//! A helper class for instantiating a Range_Partition
template <typename Cursor> class Prgm_Cursor_Tracker;

//! A view of a procedure in a Prgm_Tree
/*! This class takes a cursor to a procedure (function-subprogram,
    subroutine-subprogram, main-program, or separate-module-subprogram) and
    creates (possibly-empty) LL_Stmt ranges across important regions of the
    procedure. This form is a lot easier to manipulate than the Prgm_Tree.

    The enum Region_Tag lists the regions that Procedure tracks.
*/
template <typename PFile_T> class Procedure {

public:
  using Parse = typename PFile_T::Parse;
  using Prgm_Tree = typename PFile_T::Parse_Tree;
  using Prgm_Node = typename Prgm_Tree::node;
  using Prgm_Node_Iter = typename Prgm_Tree::iterator;
  using Prgm_Cursor = typename Prgm_Tree::cursor_t;
  using Prgm_Const_Cursor = typename Prgm_Tree::const_cursor_t;
  using Stmt_Tree = typename Prgm_Tree::value::Stmt_Tree;
  using Stmt_Cursor = typename Stmt_Tree::cursor_t;
  using Stmt_Const_Cursor = typename Stmt_Tree::const_cursor_t;
  using Stmt_Range = typename Prgm_Tree::value::Stmt_Range;
  using Stmt_Const_Range = SL_Const_Range<LL_Stmt>;
  using Stmt_Iterator = typename Stmt_Range::iterator;
  using Stmt_Const_Iterator = typename Stmt_Range::const_iterator;
  class Region_Iterator;
  class Region_Const_Iterator;

  enum Region_Tag {
    PROC_BEGIN,
    USES,
    IMPORTS,
    IMPLICITS,
    DECLS,
    EXECUTION_PART,
    CONTAINED,
    PROC_END,
    NUM_REGION_TAG
  };

public:
  explicit Procedure(PFile_T &file)
      : file_{file}, dirty_{false}, ranges_{NUM_REGION_TAG} {}
  Procedure(Procedure const &) = delete;
  Procedure(Procedure &&) = default;
  Procedure &operator=(Procedure const &) = delete;
  Procedure &operator=(Procedure &&) = default;

  //! True after a successful ingest.
  constexpr bool procedure_initialized() const noexcept {
    return !ranges_.empty(PROC_END);
  }
  constexpr operator bool() const { return procedure_initialized(); }

  //! Reset to a blank state
  void clear();

  //! Establish initial region ranges from the Prgm_Tree procedure root node
  bool ingest(Prgm_Cursor procedure_root);

  /* -------------------------- Range Functions -------------------------- */

  constexpr bool has_region(Region_Tag idx) const noexcept {
    return !ranges_.empty(idx);
  }
  Region_Iterator begin(Region_Tag idx) noexcept {
    return Region_Iterator(idx, ranges_.begin(idx));
  }
  Region_Const_Iterator begin(Region_Tag idx) const noexcept {
    return Region_Const_Iterator(idx, ranges_.cbegin(idx));
  }
  Region_Const_Iterator cbegin(Region_Tag idx) const noexcept {
    return Region_Const_Iterator(idx, ranges_.cbegin(idx));
  }
  //! Return the iterator to the last statement in the range
  Region_Iterator last(Region_Tag idx) noexcept {
    assert(!ranges_.empty(idx));
    return Region_Iterator(idx, std::prev(ranges_.end(idx)));
  }
  Region_Const_Iterator clast(Region_Tag idx) const noexcept {
    assert(!ranges_.empty(idx));
    return Region_Const_Iterator(idx, std::prev(ranges_.cend(idx)));
  }
  Region_Iterator end(Region_Tag idx) noexcept {
    return Region_Iterator(idx, ranges_.end(idx));
  }
  Region_Const_Iterator end(Region_Tag idx) const noexcept {
    return Region_Const_Iterator(idx, ranges_.cend(idx));
  }
  Region_Const_Iterator cend(Region_Tag idx) const noexcept {
    return Region_Const_Iterator(idx, ranges_.cend(idx));
  }

  //! Return the cursor for the node that "covers" the range
  constexpr Prgm_Const_Cursor range_cursor(size_t idx) const {
    return *(ranges_.get_tracker(idx));
  }

  Stmt_Range range(size_t idx) {
    return Stmt_Range(ranges_.begin(idx), ranges_.end(idx));
  }

  Stmt_Const_Range crange(size_t idx) const {
    return Stmt_Const_Range(ranges_.cbegin(idx), ranges_.cend(idx));
  }

  /* -------------------- Procedure Inquiries ------------------------ */

  //! returns true if this procedure is a main-program
  /*! main-program has the exceptional quality that the initial program-stmt is
      optional, so we need to work around this at times. */
  constexpr bool is_main_program() const noexcept {
    // Don't check procedure_initialized() here
    return (Syntax_Tags::PG_MAIN_PROGRAM == subprogram_tag_);
  }
  //! returns true if this is a main-program WITHOUT a program-stmt
  constexpr bool headless_main_program() const noexcept {
    return is_main_program() && ranges_.empty(PROC_BEGIN);
  }

  //! return the procedure name string
  std::string name() const;

  //! copy all statement labels out to destination, in order
  template <typename OutputIt> void scan_out_labels(OutputIt d_first) const;

  /* --------------------------- Modifiers ----------------------------- */

  //! rename the procedure
  bool rename(std::string const &new_name);

  //! ensure the end statement is fully specified
  /*! "END <PROCEDURE TYPE> <PROCEDURE NAME>".  Returns true if changed. */
  bool complete_end_stmt();

  //! emplace a single statement, stored in ll, before position pos
  /*! \param[in] new_syntag the Syntax_Tag for this statement
      \param[in] before_prefix if true, the new statement is inserted
                 _under_ the prefix of pos.  If false, it is inserted _above_
                 the prefix of pos.
  */
  Region_Iterator emplace_stmt(Region_Iterator pos, Logical_Line &&ll,
                               int new_syntag, bool before_prefix) {
    Stmt_Iterator iter;
    if (before_prefix)
      iter =
          file_.logical_file().emplace_ll_stmt(pos, std::move(ll), new_syntag);
    else
      iter = file_.logical_file().emplace_ll_stmt_after_prefix(
          pos, std::move(ll), new_syntag);
    ranges_.insert(pos.get_region(), pos, iter);
    return Region_Iterator(pos.get_region(), iter);
  }

  //! Fully replace a statement with new_text and new_syntag
  Region_Iterator replace_stmt(Region_Iterator pos, std::string const &new_text,
                               int new_syntag) {
    file_.logical_file().replace_stmt_text(pos, {new_text}, new_syntag);
    return pos;
  }

  //! Replace a portion of a statement, assuming the syntag is unchanged
  void replace_stmt_substr(Region_Iterator pos, LL_TT_Range const &token_range,
                           std::string const &new_text) {
    file_.logical_file().replace_stmt_substr(pos, token_range, new_text);
  }

public:
  class Region_Iterator : public Stmt_Iterator {
  public:
    Region_Iterator(Region_Tag region, Stmt_Iterator it)
        : Stmt_Iterator{it}, region_{region} {}
    Region_Tag get_region() const { return region_; }

  private:
    Region_Tag region_;
  };

  class Region_Const_Iterator : public Stmt_Const_Iterator {
  public:
    Region_Const_Iterator(Region_Tag region, Stmt_Const_Iterator it)
        : Stmt_Const_Iterator{it}, region_{region} {}
    Region_Tag get_region() const { return region_; }

  private:
    Region_Tag region_;
  };

private:
  PFile_T &file_;
  bool dirty_;
  int subprogram_tag_;
  Prgm_Cursor procedure_root_;

  using Tracker = Prgm_Cursor_Tracker<Prgm_Cursor>;

  Range_Partition<Stmt_Range, Tracker> ranges_;

private:
  constexpr void mark_prgm_tree_dirty_() noexcept {
    /* consider dropping the subtree anchored at procedure_root_ here */
    dirty_ = true;
  }
  constexpr Prgm_Cursor copy_up(Prgm_Cursor c) { return c.up(); }
  constexpr Prgm_Cursor &range_cursor_(size_t idx) {
    return *(ranges_.get_tracker(idx));
  }
};

/****************************************************************************
 NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

 With the possible exception of replace_fragment, only modify statements
 via Logical_File interfaces.  Making changes from here with low-level
 calls will corrupt Stmt_Trees.
****************************************************************************/

template <typename PFile_T> void Procedure<PFile_T>::clear() {
  ranges_.clear_partitions();
  procedure_root_.clear();
  subprogram_tag_ = Syntax_Tags::UNKNOWN;
}

template <typename PFile_T>
bool Procedure<PFile_T>::ingest(Prgm_Cursor procedure_root) {
  Prgm_Cursor pc{procedure_root};
  procedure_root_ = pc;
  subprogram_tag_ = pc->syntag();
  if (!(Syntax_Tags::PG_FUNCTION_SUBPROGRAM == subprogram_tag_ ||
        Syntax_Tags::PG_SUBROUTINE_SUBPROGRAM == subprogram_tag_ ||
        Syntax_Tags::PG_MAIN_PROGRAM == subprogram_tag_ ||
        Syntax_Tags::PG_SEPARATE_MODULE_SUBPROGRAM == subprogram_tag_)) {
    return false;
  }

  pc.down();

  /* Identify the statement that begins this procedure, remembering that
     program-stmt is optional for main-program */
  if (!(is_main_program() && Syntax_Tags::SG_PROGRAM_STMT != pc->syntag())) {
    assert(pc.is_leaf());
    ranges_.set_tracker(PROC_BEGIN, Tracker{pc});
    ranges_.append(PROC_BEGIN, pc->stmt_range());
    pc.next();
  }

  bool scan{true};
  do {
    switch (pc->syntag()) {
    case Syntax_Tags::PG_SPECIFICATION_PART: {
      Prgm_Cursor spc = pc;
      spc.down();
      do {
        switch (spc->syntag()) {
        case Syntax_Tags::SG_USE_STMT:
          ranges_.set_tracker(USES, Tracker(pc));
          ranges_.append(USES, spc->stmt_range());
          break;
        case Syntax_Tags::SG_IMPORT_STMT:
          ranges_.set_tracker(IMPORTS, Tracker(pc));
          ranges_.append(IMPORTS, spc->stmt_range());
          break;
        case Syntax_Tags::PG_IMPLICIT_PART:
          ranges_.set_tracker(IMPLICITS, Tracker(spc));
          ranges_.append(IMPLICITS, spc->stmt_range());
          break;
        case Syntax_Tags::PG_DECLARATION_CONSTRUCT:
          ranges_.set_tracker(DECLS, Tracker(pc));
          ranges_.append(DECLS, spc->stmt_range());
          break;
        }
      } while (spc.try_next());
    } break;
    case Syntax_Tags::PG_EXECUTION_PART:
      ranges_.set_tracker(EXECUTION_PART, Tracker(pc));
      ranges_.append(EXECUTION_PART, pc->stmt_range());
      break;
    case Syntax_Tags::PG_INTERNAL_SUBPROGRAM_PART:
      ranges_.set_tracker(CONTAINED, Tracker(pc));
      ranges_.append(CONTAINED, pc->stmt_range());
      break;
    default:
      scan = false;
      break;
    }
  } while (scan && pc.try_next());

  assert(scan == false); // we want an SG_END_*_STMT to terminate the scan
  assert(Syntax_Tags::SG_END_FUNCTION_STMT == pc->syntag() ||
         Syntax_Tags::SG_END_SUBROUTINE_STMT == pc->syntag() ||
         Syntax_Tags::SG_END_PROGRAM_STMT == pc->syntag() ||
         Syntax_Tags::SG_END_MP_SUBPROGRAM_STMT == pc->syntag());

  dirty_ = false;
  ranges_.set_tracker(PROC_END, Tracker(pc));
  ranges_.append(PROC_END, pc->stmt_range());
  assert(ranges_.validate());
  return true;
}

template <typename PFile_T> std::string Procedure<PFile_T>::name() const {
  assert(procedure_initialized());
  if (headless_main_program())
    return std::string();

  Stmt_Const_Cursor s = range_cursor(PROC_BEGIN)->stmt_tree().ccursor();
  s.down();
  if (Syntax_Tags::SG_PREFIX == s->syntag)
    s.next();
  s.next();
  if (Syntax_Tags::KW_PROCEDURE == s->syntag)
    s.next();
  assert(s->token_range.size() == 1);
  return s->token_range.front().text();
}

template <typename PFile_T>
bool Procedure<PFile_T>::rename(std::string const &new_name) {
  assert(procedure_initialized());
  if (new_name == name())
    return false;

  if (headless_main_program()) {
    std::string const program_stmt = "program " + new_name;
    emplace_stmt(end(PROC_BEGIN), Logical_Line(program_stmt),
                 Syntax_Tags::SG_PROGRAM_STMT, false);
    mark_prgm_tree_dirty_();
    return true;
  }

  /* fix the subprogram statement */
  Stmt_Cursor s = range_cursor_(PROC_BEGIN)->stmt_tree().cursor();
  s.down();
  if (Syntax_Tags::SG_PREFIX == s->syntag)
    s.next();
  s.next();
  if (Syntax_Tags::KW_PROCEDURE == s->syntag)
    s.next();
  assert(s->token_range.size() == 1);
  /* We can do this low-level manipulation ONLY because we are not changing the
     structure or meaning of the statement */
  s->token_range.ll().replace_fragment(s->token_range.begin(),
                                       Syntax_Tags::TK_NAME, new_name);

  /* fix the end statement */
  s = range_cursor_(PROC_END)->stmt_tree().cursor();
  s.down();
  if (s.try_next(2)) {
    assert(Syntax_Tags::is_name(s->syntag));
    /* We can do this low-level manipulation ONLY because we are not changing
       the structure or meaning of the statement */
    s->token_range.ll().replace_fragment(s->token_range.begin(),
                                         Syntax_Tags::TK_NAME, new_name);
  }
  mark_prgm_tree_dirty_();
  return true;
}

template <typename PFile_T> bool Procedure<PFile_T>::complete_end_stmt() {
  std::string suffix;

  Stmt_Cursor es = range_cursor_(PROC_END)->stmt_tree().cursor();
  es.down();
  assert(Syntax_Tags::KW_END == es->syntag);
  if (!es.has_next()) {
    /* needs subprogram type and name */
    switch (subprogram_tag_) {
    case Syntax_Tags::PG_FUNCTION_SUBPROGRAM:
      suffix = " function ";
      break;
    case Syntax_Tags::PG_SUBROUTINE_SUBPROGRAM:
      suffix = " subroutine ";
      break;
    case Syntax_Tags::PG_SEPARATE_MODULE_SUBPROGRAM:
      suffix = " procedure ";
      break;
    case Syntax_Tags::PG_MAIN_PROGRAM:
      suffix = " program ";
      break;
    }
    assert(!suffix.empty());
    suffix.append(name());
  } else {
    es.next();
    if (!es.has_next()) {
      /* needs name */
      if (!headless_main_program()) {
        suffix = " " + name();
      }
    }
  }
  if (!suffix.empty()) {
    file_.logical_file().append_stmt_text(
        range_cursor_(PROC_END)->ll_stmt_iter(), suffix);
    mark_prgm_tree_dirty_();
    return true;
  }
  /* everything was there */
  return false;
}

template <typename PFile_T>
template <typename OutputIt>
void Procedure<PFile_T>::scan_out_labels(OutputIt d_first) const {
  /* Because labels can appear almost anywhere in the body of a subprogram, we
     want to go to the subprogram root, as its stmt_range covers all the
     statements in the subprogram */
  Prgm_Cursor c = procedure_root_;
  for (auto const &s : c->stmt_range()) {
    if (s.has_label()) {
      *d_first++ = s.label();
    }
  }
}

template <typename Cursor> class Prgm_Cursor_Tracker {
  using iterator = typename Cursor::value_type::Stmt_Range::iterator;
  using diff_type = typename std::iterator_traits<iterator>::difference_type;

public:
  Prgm_Cursor_Tracker(Cursor const &c) : cursor_{c} {}
  constexpr Cursor *operator->() noexcept { return &cursor_; }
  constexpr Cursor &operator*() noexcept { return cursor_; }
  constexpr Cursor const *operator->() const noexcept { return &cursor_; }
  constexpr Cursor const &operator*() const noexcept { return cursor_; }

  /* FIX: not implemented yet.  This should update the begin() of any stmt_range
     in the Prgm_Tree at or above cursor_ that starts with old_begin, and
     updates all range sizes above that */
  void update_begin(iterator old_begin, iterator new_begin, diff_type size) {}
  /* FIX: not implemented yet.  This should update the end() of any stmt_range
     in the Prgm_Tree at or above cursor that ends with old_end.  The size of
     those ranges doesn't change. */
  void update_end(iterator old_end, iterator new_end) {}
  /* FIX: not implemented yet.  This should update the size() of any stmt_range
     in the Prgm_Tree at or above cursor. */
  void update_size(diff_type new_size) {}

private:
  Cursor cursor_;
};

} // namespace FLPR

#endif
