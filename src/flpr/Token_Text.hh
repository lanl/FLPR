/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file include/flpr/Token_Text.hh
*/

#ifndef FLPR_TOKEN_TEXT_HH
#define FLPR_TOKEN_TEXT_HH 1

#include "flpr/Safe_List.hh"
#include "flpr/Syntax_Tags.hh"
#include <ostream>
#include <string>

namespace FLPR {
class Logical_Line;
class Logical_File;

//! A token, and it's corresponding text, as discovered by the lexer.
/*!
  This class records some information that you may not find in a compiler:
  file positions and spacing information.  The file positions, \p
  start_line and \p start_pos are the (index 1) positions of the first
  character of the lexeme.  These are used for error and warning
  reporting.  The spacing information is used to capture the number of
  spaces before and after the lexeme.  This is used to recreate the
  original input.  Keeping the spacing information with the tokens
  gives an efficient way of analyzing and editing a sequence of tokens.
 */
class Token_Text {
public:
  friend class Logical_Line;
  friend class Logical_File;
  Token_Text();
  Token_Text(std::string &&txti, int toki, int sli, int spi)
      : token(toki), start_line(sli), start_pos(spi), text_(std::move(txti)) {}

  int token;      //!< A token identifer from scan_toks.h
  int start_line; //!< The file line number of the start of this token
  int start_pos;  //!< The file character position of the start of this token

  //! Const access to the text
  std::string const &text() const noexcept { return text_; }
  //! Provide a modifiable reference to text: clears lower_
  std::string &mod_text() noexcept {
    lower_.clear();
    return text_;
  }
  //! Access the lowercase version of text
  std::string const &lower() const;

  int pre_spaces() const noexcept { return pre_spaces_; }
  int post_spaces() const noexcept { return post_spaces_; }

  /************* Accessors for testing purposes only *******************/
  int main_txt_line() const noexcept { return mt_begin_line_; }
  int main_txt_col() const noexcept { return mt_begin_col_; }

private:
  std::string text_;          //!< The matched text (lexeme)
  mutable std::string lower_; //!< Lowercase version of the text

  /*********************************************************************/
  /*                   For use by Logical_Line friend                  */
  /*********************************************************************/
  //! Index of the Logical_Line::layout_ line the first character is on
  int mt_begin_line_;
  //! Index of starting column in main_txt
  int mt_begin_col_;
  //! Index of the Logical_Line::layout_ line the last+1 character is on
  int mt_end_line_;
  //! Index of last (+1) column in main_txt
  int mt_end_col_;
  //! The number of spaces before this token
  int pre_spaces_;
  //! The number of spaces after this token
  int post_spaces_;
  /*********************************************************************/

private:
  constexpr bool is_split_token_() const {
    return mt_begin_line_ != mt_end_line_;
  }
};

//! The type for a sequence of Token_Text
/*!
  Note that this container must have the same iterator invalidation
  rules as std::list (e.g. iterators remain valid for all sequence
  modifications, unless you have an iterator to an element that gets
  erased).
*/
using TT_SEQ = FLPR::Safe_List<Token_Text>;
//! A range of Token_Text
using TT_Range = FLPR::SL_Range<Token_Text>;

void unkeyword(TT_SEQ::iterator beg, const TT_SEQ::iterator end,
               int first_N = -1);
std::ostream &operator<<(std::ostream &os, Token_Text const &tt);

void render(std::ostream &os, TT_SEQ::const_iterator beg,
            TT_SEQ::const_iterator end);
} // namespace FLPR
#endif
