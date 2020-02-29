/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Logical_Line.hh
*/

#ifndef FLPR_LOGICAL_LINE_HH
#define FLPR_LOGICAL_LINE_HH 1

#include "flpr/File_Info.hh"
#include "flpr/File_Line.hh"
#include "flpr/Line_Accum.hh"
#include "flpr/Safe_List.hh"
#include "flpr/Token_Text.hh"
#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace FLPR {
//! Specific categorization of a Logical_Line
enum LineCat {
  INCLUDE, //!< A Fortan include line
  LITERAL, //!< Unscanned line copied to output
  MACRO,   //!< General preprocessor line (e.g. CPP)
  FLPR_PP, //!< FLPR preprocessor line
  UNKNOWN
};

//! The contents of a single (possibly continued) line of input.
/*!
  A contiguous set of lines that belong together, such as (continued)
  statement(s), or a comment block. This structure maintains the format of the
  original input, so that it can be re-emitted in a similar form.  If parsing
  free-format files, the left_text vector will likely to only be used for
  labels.

  NOTE: there may be more than one statement in a Logical_Line, if they have
  been compounded with semicolons.
*/
class Logical_Line {
public:
  using iterator = TT_SEQ::iterator;
  using const_iterator = TT_SEQ::const_iterator;
  using FL_VEC = std::vector<File_Line>;
  using STMT_VEC = std::vector<TT_Range>;

  std::shared_ptr<File_Info> file_info;

  int label;           //!< Fortran numerical line label
  LineCat cat;         //!< what sort of line is this (special)
  bool suppress;       //!< don't output this line
  bool needs_reformat; //!< true->reformat before output

  Logical_Line() noexcept;

  // We need to update Logical_Line::stmts after copies
  Logical_Line(Logical_Line const &src) noexcept;
  Logical_Line &operator=(Logical_Line const &src) noexcept;

  // .. but not after moves
  Logical_Line(Logical_Line &&) = default;
  Logical_Line &operator=(Logical_Line &&) = default;

  //! Create a Logical_Line from a range of File_Lines (MOVE operator!)
  template <typename Iter>
  Logical_Line(Iter first, Iter last) noexcept
      : label{0}, cat{LineCat::UNKNOWN}, suppress{false}, needs_reformat{false},
        num_semicolons_{-1} {
    std::move(first, last, std::back_inserter(layout_));
    init_from_layout();
  }

  //! Make a trivial Logical_Line from a free-format raw string
  explicit Logical_Line(std::string const &raw_text);

  //! Make a complicated Logical_Line from a vector of free-format strings
  /*! You can use the std::vector initializer_list constructor to easily use
    this constructor. */
  explicit Logical_Line(std::vector<std::string> const &raw_text);

  //! Make a trivial Logical_Line from a fixed-format raw string
  Logical_Line(std::string const &raw_text, int const last_col);

  //! Make a complicated Logical_Line from a vector of fixed-format strings
  Logical_Line(std::vector<std::string> const &raw_text, int const last_col);

  //! Resets the contents
  void clear() noexcept;

  //! Initialize structure from contents of the layout member.
  void init_from_layout() noexcept;

  //! Non-const layout accessor
  constexpr FL_VEC &layout() noexcept { return layout_; }

  //! Const layout accessor
  constexpr FL_VEC const &layout() const noexcept { return layout_; }

  //! Non-const fragments accessor
  constexpr TT_SEQ &fragments() noexcept { return fragments_; }

  //! Const fragments accessor
  constexpr TT_SEQ const &fragments() const noexcept { return fragments_; }

  //! Another const fragments accessor
  constexpr TT_SEQ const &cfragments() const noexcept { return fragments_; }

  //! Const statements accessor
  constexpr STMT_VEC const &stmts() const noexcept { return stmts_; }

  //! physical start line index in file (index 1)
  int start_line() const noexcept {
    if (layout_.empty())
      return -1;
    return layout_.front().linenum;
  }

  //! physical end line index in file (index 1, +1)
  int end_line() const noexcept {
    if (layout_.empty())
      return -1;
    return layout_.back().linenum + 1;
  }

  //! Returns true if this has multiple non-empty statements
  bool is_compound() const { return stmts_.size() > 1; };

  //! Generate new text from the fragments
  void text_from_frags() noexcept;

  //! Replace the syntax_tag and text in a fragment, updating everything
  void replace_fragment(typename TT_SEQ::iterator frag, int const new_syntag,
                        std::string const &new_text);

  //! Remove the fragment, updating everything
  void remove_fragment(typename TT_SEQ::iterator frag);

  //! Replace the main_txt in a Logical_Line
  void replace_main_text(std::vector<std::string> const &new_text);

  void replace_stmt_substr(TT_Range const &orig, std::string const &new_text);

  //! Remove empty statements (e.g. ";b=a; ;a=b;" -> "b=a; a=b"
  bool remove_empty_statements();

  //! Break a Logical_Line in two after frag text.
  bool split_after(typename TT_SEQ::iterator frag, Logical_Line &new_ll);

  //! Add new text before frag, and reinitialize
  void insert_text_before(typename TT_SEQ::iterator frag,
                          std::string const &new_text);

  //! Add new text after frag, and reinitialize
  void insert_text_after(typename TT_SEQ::iterator frag,
                         std::string const &new_text);

  //! Standard output
  std::ostream &print(std::ostream &os) const;

  //! Diagnostic output
  std::ostream &dump(std::ostream &os) const;

  //! Appends an eol comment to first line.  Don't put a leading '!' in text
  void append_comment(std::string const &text);

  //! Returns true if this Logical_Line contains some Fortran statements
  /*! Logical_Lines _can_ be all comments, or preprocessor, or include, etc. */
  bool has_fortran() const noexcept {
    bool has_f = false;
    for (auto const &fl : layout_) {
      has_f |= fl.is_fortran();
    }
    return !suppress && has_f;
  }

  //! Returns true if there are empty statements
  /*! For example: "a=1;", or ";a=1", or "a=1;;b=2" */
  constexpr bool has_empty_statements() const noexcept {
    assert(num_semicolons_ > -1);
    return num_semicolons_ > 0 &&
           num_semicolons_ >= static_cast<int>(stmts_.size());
  }

  constexpr int num_semicolons() const noexcept {
    assert(num_semicolons_ > -1);
    return num_semicolons_;
  }

  //! Initialize the stmts member
  void init_stmts();

  //! Clear all stmts data
  void clear_stmts() noexcept {
    num_semicolons_ = -1;
    stmts_.clear();
  }

  //! Return true if stmts are initialized
  constexpr bool has_stmts() const noexcept { return num_semicolons_ != -1; }

  //! Sets layout_ spacing if needed
  /*! Returns true if spacing changed, false otherwise */
  bool set_leading_spaces(int const spaces, int continued_offset);
  int get_leading_spaces() const noexcept {
    return layout_[0].get_leading_spaces();
  }

  //! Sets a new label (new_label == 0 clears it)
  bool set_label(int new_label);

private:
  //! Perform lexical analysis, creating fragments from the combined text
  void tokenize(Line_Accum const &);

  //! If the last token in fragments is actually two (no space), split it
  /*! This handles the exception to the free-format spacing rules found
    in section 6.3.2.2 of the standard */
  void unsmash();

  //! Append a token to main_txt if it fits within max_len characters
  bool append_tt_if_(std::string &main_txt, size_t max_len,
                     Token_Text const &tt, bool first);

  void erase_stmt_text_(int stln, int stcol, int eln, int ecol);

private:
  int num_semicolons_; // set by init_stmts, -1 -> not initialized

  //! The layout of the physical lines associated with this Logical_Line
  /*! This vector of File_Line captures the location and contents of the
      non-Fortran text associated with this Logical_Line, including control
      columns, indentation/spacing, comments and continuation characters.  On
      output, the physical lines are created by applying this layout to the
      fragments. This approach was taken to allow independent manipulation of
      the layout_ and the Fortran contents, without worrying about trying to
      keep them internally consistant. */
  FL_VEC layout_;

  //! tokenized version of the Fortran text
  TT_SEQ fragments_;

  //! Ranges of Token_Text making up individual non-empty Fortran statements
  /*! This variable is established by init_stmts().  Note that stmts.size() == 1
    does NOT mean that there aren't semicolons; it just means that there is one
    non-empty statement. */
  STMT_VEC stmts_;
};

//! The type for a sequence of Logical_Line
using LL_SEQ = Safe_List<Logical_Line>;

std::ostream &operator<<(std::ostream &os, Logical_Line const &r);
std::ostream &operator<<(std::ostream &os, const LineCat &l);
} // namespace FLPR
#endif
