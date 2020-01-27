/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file File_Line.hh
*/

#ifndef FLPR_FILE_LINE_HH
#define FLPR_FILE_LINE_HH

#include <bitset>
#include <string>
#include <vector>

#define GET_CLASS(A) classification_[static_cast<int>(class_flags::A)]

namespace FLPR {
/*!
  \brief An input line partitioned into fields

  A representation of the textual layout of a single source line.  This
  separates the line into "fields", which describe parts of the line that
  Fortran treats specially (e.g. labels, continuations, trailing comments, etc).
*/
class File_Line {
public:
  //! Characteristics of a line
  /*! Note that we distinguish between a blank (or all whitespace) line and a
      comment, although the standard says that they are the same. This is
      because we want to preserve the structure of comment lines, but blanks
      don't matter. */
  enum class class_flags {
    blank,        //!< The line is entirely empty (or whitespace)
    comment,      //!< The line only contains a comment
    continued,    //!< trailing continuation ampersand
    continuation, //!< leading continuation ampersand
    label,        //!< this is a labeled statement
    preprocessor, //!< Traditional C-preprocessor directives
    include,      //!< Fortran include directive
    flpr_pp,      //!< FLPR-specific preprocessor directive
    flpr_lit,     //!< FLPR literal section
    fixed_format, //!< Came from a fixed_format file
    zzz_num       //!< must be last!
  };

  //! The (index origin=1) line number in the source
  int linenum;
  //! Labels, continuation symbols, preprocessor statements, and comments
  std::string left_txt;
  //! The whitespace between left_txt and main_txt
  std::string left_space;
  //! The body of a fortran line, trimmed of whitespace on both ends
  std::string main_txt;
  //! The whitespace between main_txt and right_txt
  std::string right_space;
  //! Trailing comments and/or continuation symbols.
  std::string right_txt;
  //! Any open character context
  /*! If this line ends while in a character context, this is the
      character that must be matched in order to close the context.
      The only legitimate values are '\0' (not in an open context),
      '\'', or '\"'. */
  char open_delim;

public:
  File_Line() : linenum(-1), open_delim(0) {}

  //! These are classification flag manipulation and query functions
  //@{
  constexpr bool has_label() const { return (GET_CLASS(label)); }
  constexpr bool is_blank() const { return (GET_CLASS(blank)); }
  constexpr bool is_comment() const { return (GET_CLASS(comment)); }
  constexpr bool is_continuation() const { return (GET_CLASS(continuation)); }
  constexpr bool is_continued() const { return (GET_CLASS(continued)); }
  constexpr bool is_flpr_lit() const { return (GET_CLASS(flpr_lit)); }
  constexpr bool is_flpr_pp() const { return (GET_CLASS(flpr_pp)); }
  constexpr bool is_include() const { return (GET_CLASS(include)); }
  constexpr bool is_preprocessor() const { return (GET_CLASS(preprocessor)); }
  constexpr bool is_trivial() const {
    return (GET_CLASS(comment) | GET_CLASS(blank));
  }
  constexpr bool is_fortran() const {
    return !is_trivial() && !is_preprocessor() && !is_flpr_pp() &&
           !is_flpr_lit() && !is_include();
  }
  constexpr bool is_fixed_format() const { return GET_CLASS(fixed_format); }
  void set_classification(class_flags f) {
    classification_.set(static_cast<int>(f));
  }
  void unset_classification(class_flags f) {
    classification_.reset(static_cast<int>(f));
  }
  //@}

  /*!
    \brief Apportion the text to the different fields assuming fixed-format
           input.

    \param[in] linenum         The (index origin = 1) line number in the file
    \param[in,out] raw_txt     The character data for one file line
    \param[in] prev_open_delim The `open_delim` from the previous File_Line
  */
  static File_Line analyze_fixed(const int linenum, std::string const &raw_txt,
                                 const char prev_open_delim);
  static File_Line analyze_fixed(std::string const &raw_txt,
                                 int const linenum = -1) {
    return analyze_fixed(linenum, raw_txt, '\0');
  }

  /*!
    \brief Apportion the text to the different fields assuming free-format
    input.

    \param[in] linenum         The (index origin = 1) line number in the file
    \param[in] raw_txt         The character data for one file line
    \param[in] prev_open_delim The `open_delim` from the previous File_Line
    \param[in] prev_line_cont  True if the previous line is continued
    \param[in,out] in_literal_block True if we are in (or have just entered)
                                    a FLPR literal block.
  */
  static File_Line analyze_free(const int linenum, std::string const &raw_txt,
                                const char prev_open_delim,
                                const bool prev_line_cont,
                                bool &in_literal_block);

  static File_Line analyze_free(std::string const &raw_txt,
                                int const linenum = -1) {
    bool in_lit{false};
    return analyze_free(linenum, raw_txt, '\0', false, in_lit);
  }

  //! Similar to std::swap
  void swap(File_Line &other);

  //! Return the (index 1) character column number of main_txt
  int main_first_col() const {
    if (main_txt.empty())
      return 0;
    int val = 1;
    if (is_fixed_format())
      val += 6;
    else
      val += (int)left_txt.size();
    val += (int)left_space.size();
    return val;
  }

  //! Transfer any leading or trailing spaces from main_txt to left and right
  void unspace_main();

  //! If continued, unset continued flag and remove trailing amp
  void make_uncontinued();

  //! If not continued, set continued flag and add trailing amp
  void make_continued();

  //! Force into being a preprocessor line
  void make_preprocessor();

  //! Convert into a blank line
  /*! Note that this will destroy all text on the line, including comments */
  void make_blank();

  //! Remove main_txt and convert to comment or blank as appropriate
  /*! No action if this line is_comment().  If there is a comment in right_txt,
      this line is converted into a comment (all other fields blank), else
      make_blank() is called. */
  void make_comment_or_blank();

  //! Return the number of characters across
  size_t size() const noexcept;

  //! Set the number of spaces aligning the main_txt or comment
  /*! Note that this will make no changes if not is_fortran() or is_comment().
      This function returns true if the spacing was altered, false otherwise. */
  bool set_leading_spaces(int const spaces);
  int get_leading_spaces() const noexcept;

  //! Set a new label (new_label == 0 clears it)
  bool set_label(int new_label);

  std::ostream &print_classbits(std::ostream &os) const;
  std::ostream &dump(std::ostream &os) const;

private:
  using BITS = std::bitset<static_cast<size_t>(class_flags::zzz_num)>;
  BITS classification_;

private:
  File_Line(const int ln, BITS const &c, std::string const &lt,
            std::string const &ls, std::string const &mt, std::string const &rs,
            std::string const &rt, const char od);
  File_Line(int ln, BITS const &c, std::string const &lt);
};

//! Used for diagnostic output
std::ostream &operator<<(std::ostream &os, File_Line const &fl);
} // namespace FLPR

#undef GET_CLASS

#endif
