/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file File_Line.cc
*/

#include "flpr/File_Line.hh"
#include "flpr/utils.hh"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <vector>

#define SET_CLASS(A) bits.set(static_cast<int>(class_flags::A))
#define IS_CLASS(A) bits.test(static_cast<int>(class_flags::A))
#define UNSET_CLASS(A) bits.reset(static_cast<int>(class_flags::A))

namespace {
bool is_include_line(std::string const &txt, std::string::size_type non_blank);
bool is_flpr_literal(std::string const &txt,
                     std::string::size_type comment_pos);

bool is_flpr_directive(std::string const &txt,
                       std::string::size_type comment_pos);
std::string::size_type
find_trailing_fixed(std::string const &txt,
                    std::string::size_type const start_idx,
                    char const previous_open_delim, char &open_delim);

std::string::size_type
find_trailing_free(std::string const &txt,
                   std::string::size_type const start_idx,
                   char const previous_open_delim, char &open_delim);

constexpr char filler(bool b, char c) {
  if (b)
    return c;
  return '_';
}

} // namespace

namespace FLPR {
File_Line::File_Line(const int ln, BITS const &c, std::string const &lt,
                     std::string const &ls, std::string const &mt,
                     std::string const &rs, std::string const &rt,
                     const char od)
    : linenum(ln), left_txt(lt), left_space(ls), main_txt(mt), right_space(rs),
      right_txt(rt), open_delim(od), classification_(c) {
  assert(open_delim == '\0' || open_delim == '\"' || open_delim == '\'');
}

File_Line::File_Line(int ln, BITS const &c, std::string const &lt)
    : linenum(ln), left_txt(lt), open_delim('\0'), classification_(c) {}

File_Line File_Line::analyze_fixed(const int ln, std::string const &raw_txt_in,
                                   const char prev_open_delim) {
  using ST = std::string::size_type;
  BITS bits;
  std::string left_text, left_sp, main_text, right_sp, right_text;

  SET_CLASS(fixed_format);
  if (raw_txt_in.empty()) {
    SET_CLASS(blank);
    return File_Line(ln, bits, raw_txt_in);
  }

  // expand tabs in the control columns.  They shouldn't be here,
  // but... if there is a tab in the control columns, expand it and
  // any adjacent tabs into 6-space blocks
  std::string raw_txt{raw_txt_in};
  {
    const ST tab_begin = raw_txt.find_first_of("\t");
    if (tab_begin < 6) {
      ST tab_end = tab_begin + 1;
      while (tab_end < raw_txt.size() && raw_txt[tab_end] == '\t')
        ++tab_end;
      int num_tabs = tab_end - tab_begin;
      for (ST i = tab_begin; i < tab_end; ++i)
        raw_txt[i] = ' ';
      raw_txt.insert(tab_end, 5 * num_tabs, ' ');
    }
  }

  // Find the first non-blank character
  ST ri = raw_txt.find_first_not_of(" \t\r");
  if (ri == std::string::npos) {
    SET_CLASS(blank);
    return File_Line(ln, bits, raw_txt);
  }

  const char c = std::toupper(raw_txt[ri]);

  if (ri != 5) // continuation trumps everything
  {
    // Check for standard preprocessor commands
    if (ri == 0 && c == '#') {
      SET_CLASS(preprocessor);
      return File_Line(ln, bits, raw_txt);
    }
    // How about a Fortran include directive? (Section 6.4 of the standard)
    if (is_include_line(raw_txt, ri)) {
      SET_CLASS(include);
      return File_Line(ln, bits, raw_txt);
    }
    // Look for comment lines (or flpr preprocessor directives)
    if (c == '!' || (ri == 0 && (c == '*' || c == 'C'))) {
      if (is_flpr_literal(raw_txt, ri))
        // This is because the formatting and continuation style has
        // to match the generated f90 format
        throw std::runtime_error("FLPR literals are not supported "
                                 "in fixed format files");
      else if (is_flpr_directive(raw_txt, ri))
        SET_CLASS(flpr_pp);
      else {
        SET_CLASS(comment);
        return File_Line(ln, bits, raw_txt);
      }
      return File_Line(ln, bits, raw_txt);
    }
  }

  // Now we have a standard Fortran line, no comments, preprocessors or blanks

  ST indent_begin = 6;

  // Identify things before the main body
  if (ri < 6) {
    if (ri == 5) {
      ri += 1;
      left_text = raw_txt.substr(0, ri);
      if (raw_txt[5] != '0')
        SET_CLASS(continuation);
    } else if (std::isdigit(c)) {
      // Read a <= 5-digit integer statement label
      const ST N = raw_txt.size();
      while (ri < N && ri < 6 && std::isdigit(raw_txt[ri]))
        ri += 1;
      left_text = raw_txt.substr(0, ri);
      SET_CLASS(label);
    }

    // If we set anything in the left text, we need to advance to the
    // next non-blank, UNLESS we are in a character context
    if (!left_text.empty() && prev_open_delim == '\0') {
      while (ri < raw_txt.size() && std::isspace(raw_txt[ri]))
        ri += 1;
      if (ri == raw_txt.size()) {
        SET_CLASS(blank);
        return File_Line(ln, bits, raw_txt);
      }
    }
  }

  // Now record any indent (whitespace between first character of
  // Fortran text and the end of the control columns
  if (ri > indent_begin)
    left_sp = raw_txt.substr(indent_begin, ri - indent_begin);

  // We're at the first character of Fortran text.
  char open_delim_char(0);
  ST trailing_begin =
      find_trailing_fixed(raw_txt, ri, prev_open_delim, open_delim_char);

  if (trailing_begin == std::string::npos) {
    main_text = raw_txt.substr(ri);
  } else {
    main_text = raw_txt.substr(ri, trailing_begin - ri);
    right_text = raw_txt.substr(trailing_begin);
  }

  if (main_text.empty()) {
    throw std::runtime_error("Didn't find expected main body");
  }

  // Move any trailing whitespace from the end of main_text to right_space
  if (!open_delim_char) {
    ST last_char = main_text.find_last_not_of(" \t");
    if (last_char + 1 < main_text.size()) {
      right_sp =
          main_text.substr(last_char + 1, main_text.size() - last_char - 1);
      main_text.erase(last_char + 1);
    }
  }

  return File_Line(ln, bits, left_text, left_sp, main_text, right_sp,
                   right_text, open_delim_char);
}

File_Line File_Line::analyze_free(const int ln, std::string const &raw_txt,
                                  const char prev_open_delim,
                                  const bool prev_line_cont,
                                  bool &in_literal_block) {
  using ST = std::string::size_type;
  BITS bits;
  std::string left_text, left_sp, main_text, right_sp, right_text;

  // Find the first non-blank character
  ST ri = raw_txt.find_first_not_of(" \t\r");

  if (in_literal_block) {
    SET_CLASS(flpr_lit);
    if (std::string::npos != ri && raw_txt[ri] == '!') {
      if (is_flpr_literal(raw_txt, ri))
        in_literal_block = false;
    }
    return File_Line(ln, bits, raw_txt);
  }

  // There isn't one...
  if (std::string::npos == ri) {
    SET_CLASS(blank);
    return File_Line(ln, bits, raw_txt);
  }

  const char c = std::toupper(raw_txt[ri]);

  // Check for standard preprocessor commands
  if (ri == 0 && c == '#') {
    SET_CLASS(preprocessor);
    return File_Line(ln, bits, raw_txt);
  }
  // How about a Fortran include directive? (Section 6.4 of the standard)
  if (is_include_line(raw_txt, ri)) {
    SET_CLASS(include);
    return File_Line(ln, bits, raw_txt);
  }

  // Look for comment lines (or flpr preprocessor directives)
  if (c == '!') {
    if (is_flpr_literal(raw_txt, ri)) {
      SET_CLASS(flpr_lit);
      in_literal_block = true;
    } else if (is_flpr_directive(raw_txt, ri))
      SET_CLASS(flpr_pp);
    else {
      /* This is kind of weird, but it keeps the continuation going through an
         interspersed comment line */
      if (prev_line_cont)
        SET_CLASS(continued);
      SET_CLASS(comment);
      return File_Line(ln, bits, trim_back_copy(raw_txt));
    }
    return File_Line(ln, bits, raw_txt);
  }

  // Now we have a standard Fortran line

  ST indent_begin = 0;

  // Identify things before the main body
  if (!prev_line_cont && std::isdigit(c)) {
    // Read a <= 5-digit integer statement label
    ST label_begin = ri;
    const ST N = raw_txt.size();
    while (ri < N && (ri - label_begin < 6) && std::isdigit(raw_txt[ri]))
      ri += 1;
    indent_begin = ri;
    left_text = raw_txt.substr(0, ri);
    SET_CLASS(label);
  } else if ('&' == c) {
    ri += 1;
    left_text = raw_txt.substr(0, ri);
    SET_CLASS(continuation);
    indent_begin = ri;
  }

  /* If we set anything in the left text, we need to advance to the next
     non-blank, UNLESS this is a continuation */
  if (!left_text.empty() && !IS_CLASS(continuation))
    while (std::isspace(raw_txt[ri]))
      ri += 1;

  /* Now record any indent (whitespace between first character of Fortran text
     and the last feature [begining of line, end of label, or prefix
     continuation]) */
  if (ri > indent_begin)
    left_sp = raw_txt.substr(indent_begin, ri - indent_begin);

  // We're at the first character of Fortran text.
  char open_delim;
  ST trailing_begin =
      find_trailing_free(raw_txt, ri, prev_open_delim, open_delim);

  if (trailing_begin == std::string::npos) {
    main_text = raw_txt.substr(ri);
  } else {
    main_text = raw_txt.substr(ri, trailing_begin - ri);
    right_text = raw_txt.substr(trailing_begin);
    if (right_text[0] == '&')
      SET_CLASS(continued);
  }

  // Move any trailing whitespace from the end of main_text to right_space
  if (!open_delim) {
    ST last_char = main_text.find_last_not_of(" \t");
    if (last_char + 1 < main_text.size()) {
      right_sp =
          main_text.substr(last_char + 1, main_text.size() - last_char - 1);
      main_text.erase(last_char + 1);
    }
  }
  trim_back(right_text);
  if (right_text.empty())
    right_sp.clear();

  return File_Line(ln, bits, left_text, left_sp, main_text, right_sp,
                   right_text, open_delim);
}

void File_Line::swap(File_Line &other) {
  std::swap(linenum, other.linenum);
  left_txt.swap(other.left_txt);
  left_space.swap(other.left_space);
  main_txt.swap(other.main_txt);
  right_space.swap(other.right_space);
  right_txt.swap(other.right_txt);
  std::swap(open_delim, other.open_delim);
  std::swap(classification_, other.classification_);
}

void File_Line::unspace_main() {
  auto fnb = main_txt.find_first_not_of(' ');
  if (fnb == std::string::npos) {
    // main_txt is empty?
    set_classification(class_flags::blank);
    return;
  } else if (fnb > 0) {
    main_txt.erase(0, fnb);
  }
  auto lnb = main_txt.find_last_not_of(' ');
  assert(lnb != std::string::npos);
  auto ftb = lnb + 1;
  if (ftb < main_txt.size()) {
    size_t num_blanks = main_txt.size();
    main_txt.erase(ftb);
    num_blanks -= main_txt.size();
    right_space.insert(right_space.end(), num_blanks, ' ');
  }
}

void File_Line::make_uncontinued() {
  auto &bits = classification_;
  UNSET_CLASS(continued);
  size_t pos = 0;
  if (!right_txt.empty()) {
    if (right_txt[0] == '&')
      right_txt[0] = ' ';
    pos = right_txt.find_first_not_of(' ');
    right_txt.erase(0, pos);
  }
  if (right_txt.empty()) {
    right_space.clear();
  } else {
    /* pad right_space so that right_txt comment doesn't move */
    right_space.append(pos, ' ');
  }
}

void File_Line::make_continued() {
  auto &bits = classification_;
  SET_CLASS(continued);
  if (right_space.empty())
    right_space.assign(1, ' ');
  if (right_txt.empty())
    right_txt.assign(1, '&');
  else if (right_txt[0] != '&') {
    right_txt.insert(0, "& ");
    /* If there is room, adjust right_space so that we don't change
       the start of a trailing comment */
    if (right_space.size() > 2)
      right_space.erase(0, 2);
  }
}

void File_Line::make_preprocessor() {
  size_t const ff{static_cast<size_t>(class_flags::fixed_format)};
  size_t const pp{static_cast<size_t>(class_flags::preprocessor)};
  bool const is_fixed_format = classification_[ff];
  classification_.reset();
  classification_[ff] = is_fixed_format;
  classification_[pp] = true;
  left_txt += left_space + main_txt + right_space + right_txt;
  left_space.clear();
  main_txt.clear();
  right_space.clear();
  right_txt.clear();
}

void File_Line::make_blank() {
  size_t const ff{static_cast<size_t>(class_flags::fixed_format)};
  size_t const blank{static_cast<size_t>(class_flags::blank)};
  bool const is_fixed_format = classification_[ff];
  classification_.reset();
  classification_[ff] = is_fixed_format;
  classification_[blank] = true;
  left_txt.clear();
  left_space.clear();
  main_txt.clear();
  right_space.clear();
  right_txt.clear();
}

void File_Line::make_comment_or_blank() {
  size_t const ff{static_cast<size_t>(class_flags::fixed_format)};
  size_t const com{static_cast<size_t>(class_flags::comment)};
  size_t const trail{static_cast<size_t>(class_flags::continued)};
  size_t const blank{static_cast<size_t>(class_flags::blank)};

  if (classification_[com])
    return;
  if (classification_[blank]) {
    make_blank(); // force the clearing of the strings
    return;
  }
  if (classification_[trail]) {
    assert(right_txt[0] == '&');
    right_txt[0] = ' ';
  }
  auto comment_start = right_txt.find('!');
  if (comment_start != std::string::npos && comment_start > 0) {
    /* transfer any leading blanks from right_txt to right_space */
    right_space.append(comment_start, ' ');
    right_txt.erase(0, comment_start);
    /* set each of the earlier text fields to be blank of the same length */
    left_txt.assign(left_txt.size(), ' ');
    right_txt.assign(right_txt.size(), ' ');
    bool const is_fixed_format = classification_[ff];
    classification_.reset();
    classification_[ff] = is_fixed_format;
    classification_[com] = true;
  } else {
    make_blank();
  }
}

size_t File_Line::size() const noexcept {
  return left_txt.size() + left_space.size() + main_txt.size() +
         right_space.size() + right_txt.size();
}

bool File_Line::set_leading_spaces(int const spaces) {
  if (is_comment()) {
    std::string::size_type pos = left_txt.find('!');
    if (pos != std::string::npos) {
      left_space.clear();
      main_txt.clear();
      right_space.clear();
      right_txt.clear();
      if (static_cast<int>(pos) == spaces)
        return false; // no change needed

      /* If the comment began in the first column, it could have been from
         fixed-format.  Remove any empty control columns. */
      if (pos == 0) {
        std::string::size_type tpos = left_txt.find_first_not_of(" \t", 1);
        if (tpos != std::string::npos) {
          if (static_cast<int>(tpos) > 5) {
            if (static_cast<int>(tpos) >= spaces + 5) {
              left_txt.erase(1, spaces + 4);
            } else {
              left_txt.erase(1, 4);
            }
          }
        }
      } else {
        left_txt.erase(0, pos);
      }
      left_txt.insert(0, spaces, ' ');
      return true;
    }
    size_t leading_space = left_txt.size() + left_space.size();
    pos = main_txt.find('!');
    if (pos != std::string::npos) {
      if (pos > 0) {
        main_txt.erase(0, pos);
        left_space.append(pos, ' ');
        leading_space += pos;
      }
      if (static_cast<int>(leading_space) == spaces)
        return false;
      /* I don't know why this would be true, but... */
      if (static_cast<int>(left_txt.size()) < spaces) {
        left_space.assign(spaces - left_txt.size(), ' ');
        return true;
      }
      return false;
    }
    // Don't do anything to a right_txt comment
    return false;

  } else if (is_fortran()) {
    int orig_spaces{-1};
    int offset;
    if (left_txt.empty()) {
      if (static_cast<int>(left_space.size()) == spaces)
        return false; // no change needed
      orig_spaces = left_space.size();
      offset = spaces - left_space.size();
      left_space.assign(spaces, ' ');
    } else {
      int const lt_size = static_cast<int>(left_txt.size());
      orig_spaces = lt_size + left_space.size();

      /* Work around statement labels.*/
      if (has_label()) {
        if (lt_size < spaces) {
          /* If the label fits in the indent region, properly format it */
          int const new_ls = spaces - lt_size;
          if (static_cast<int>(left_space.size()) == new_ls)
            return false; // no change needed
          offset = new_ls - left_space.size();
          left_space.assign(new_ls, ' ');
        } else {
          /* Otherwise, use left_space to introduce a single space */
          if (left_space.size() == 1)
            return false; // no change needed
          offset = 1 - left_space.size();
          left_space.assign(1, ' ');
        }
      } else if (is_continuation() && !is_fixed_format()) {
        if (orig_spaces == spaces)
          return false; // no change needed
        if (orig_spaces < spaces) {
          offset = spaces - orig_spaces;
          left_txt.insert(0, spaces - orig_spaces, ' ');
        } else {
          /* see if there are some spaces to pull out before the continuation
             character */
          std::string::size_type pos = left_txt.find_first_not_of(' ');
          assert(pos != std::string::npos);
          size_t const desired = orig_spaces - spaces;
          size_t const remove_count = std::min(pos, desired);
          offset = -remove_count;
          left_txt.erase(0, remove_count);
        }
      }
    }
    /* See if we can adjust spacing to leave right_txt comments in their
       original position */
    if (!right_txt.empty() && offset) {
      std::string::size_type pos = right_txt.find('!');
      if (pos != std::string::npos) {
        if (offset < 0) {
          /* The main_txt moved left, so we can introduce more spaces on the
             right */
          right_space.insert(pos, -offset, ' ');
        } else {
          /* The main_txt moved right, so see if we can squeeze right_space */
          int const min_size = (right_space.empty()) ? 0 : 1;
          int const squeeze = right_space.size() - offset;
          size_t const new_size = std::max(min_size, squeeze);
          if (right_space.size() != new_size) {
            right_space.assign(new_size, ' ');
          }
        }
      }
    }
    return true;
  }
  return false; // wasn't a comment or fortran line
}

int File_Line::get_leading_spaces() const noexcept {
  int retval{0};
  if (is_comment()) {
    std::string::size_type pos = left_txt.find('!');
    if (pos != std::string::npos) {
      retval = pos;
    } else {
      pos = main_txt.find('!');
      if (pos != std::string::npos) {
        retval = left_txt.size() + left_space.size() + pos;
      } else {
        pos = right_txt.find('!');
        if (pos != std::string::npos) {
          retval = left_txt.size() + left_space.size() + main_txt.size() +
                   right_txt.size() + pos;
        }
      }
    }
  } else if (is_fortran()) {
    retval = main_first_col() - 1;
  }
  return retval;
}

bool File_Line::set_label(int new_label) {
  assert(!(new_label < 0 || new_label > 99999));
  assert(is_fortran());
  assert(!is_continuation());

  auto &bits = classification_;

  if (new_label == 0 && !has_label())
    return false;
  if (is_fixed_format()) {
    if (left_txt.empty()) {
      left_txt = std::to_string(new_label);
      left_txt.append(6 - left_txt.size(), ' ');
      if (left_space.size() > 6)
        left_space.erase(0, 6);
    } else {
      assert(has_label());
      left_txt = std::to_string(new_label);
      left_txt.append(6 - left_txt.size(), ' ');
    }
  } else {
    /* free-format */
    size_t const old_size = left_txt.size();
    left_txt = std::to_string(new_label);
    size_t const new_size = left_txt.size();
    if (new_size < old_size) {
      left_space.append(old_size - new_size, ' ');
    } else if (new_size > old_size) {
      size_t const diff = new_size - old_size;
      if (diff + 1 < left_space.size()) {
        left_space.erase(0, diff);
      } else {
        left_space.assign(1, ' ');
      }
    }
  }
  SET_CLASS(label);
  return true;
}

std::ostream &File_Line::dump(std::ostream &os) const {
  print_classbits(os) << ' ';
  if (!is_blank())
    os << '<' << left_txt << "> <" << left_space << "> <" << main_txt << "> <"
       << right_space << "> <" << right_txt << '>';
  return os;
}

std::ostream &File_Line::print_classbits(std::ostream &os) const {
  os << filler(is_blank(), 'b');
  os << filler(is_comment(), 'c');
  os << filler(is_continued(), 'T');
  os << filler(is_continuation(), 'L');
  os << filler(has_label(), 'l');
  os << filler(is_preprocessor(), 'p');
  os << filler(is_flpr_pp(), 'f');
  return os;
}

std::ostream &operator<<(std::ostream &os, FLPR::File_Line const &fl) {
  if (fl.is_fortran() && fl.is_fixed_format()) {
    auto flags = os.setf(std::ios::left);
    os << std::setw(6) << fl.left_txt << fl.left_space << fl.main_txt
       << fl.right_space << fl.right_txt;
    os.setf(flags);
  } else
    os << fl.left_txt << fl.left_space << fl.main_txt << fl.right_space
       << fl.right_txt;
  return os;
}

} // namespace FLPR

namespace {

bool is_include_line(std::string const &txt, std::string::size_type non_blank) {
  if (non_blank >= txt.size())
    return false;
  if (txt[non_blank] != 'i' && txt[non_blank] != 'I')
    return false;
  std::string maybe_inc{txt.substr(non_blank, 7)};
  FLPR::tolower(maybe_inc);
  if (maybe_inc == "include") {
    non_blank = non_blank + 7;
    while (non_blank < txt.size() && txt[non_blank] == ' ')
      non_blank += 1;
    if (non_blank < txt.size() &&
        (txt[non_blank] == '\'' || txt[non_blank] == '"'))
      return true;
  }
  return false;
}

bool is_flpr_directive(std::string const &txt,
                       std::string::size_type comment_pos) {
  return (txt.size() > comment_pos + 5 && txt[comment_pos + 1] == '#' &&
          txt[comment_pos + 2] == 'f' && txt[comment_pos + 3] == 'l' &&
          txt[comment_pos + 4] == 'p' && txt[comment_pos + 5] == 'r');
}

bool is_flpr_literal(std::string const &txt,
                     std::string::size_type comment_pos) {
  return (is_flpr_directive(txt, comment_pos) &&
          txt.size() > comment_pos + 5 + 8 && txt[comment_pos + 9] == ' ' &&
          txt[comment_pos + 10] == 'l' && txt[comment_pos + 11] == 'i' &&
          txt[comment_pos + 12] == 't' && txt[comment_pos + 13] == 'e' &&
          txt[comment_pos + 14] == 'r' && txt[comment_pos + 15] == 'a' &&
          txt[comment_pos + 16] == 'l');
}

/* Find trailing comments in fixed format.  Note that start_idx needs to be past
 any prefixed labels, continuations, or control blocks. */
std::string::size_type
find_trailing_fixed(std::string const &txt,
                    std::string::size_type const start_idx,
                    char const previous_open_delim, char &open_delim) {
  using ST = std::string::size_type;
  assert(previous_open_delim == '\0' || previous_open_delim == '\"' ||
         previous_open_delim == '\'');

  const ST N = txt.size();
  char char_context = previous_open_delim;
  open_delim = '\0';

  for (ST i = start_idx; i < N; ++i) {
    const char c = txt[i];
    if (!char_context) {
      /* Note that you shouldn't use this technique to find the extent of
         strings, as they are allowed to contain doubled delimiters as escapes.
         For example: 'Paul''s code' is equivalent to "Paul's code", not two
         strings.  For our purpose, this doesn't matter. */
      if (c == '\'' || c == '"')
        char_context = c;
      else if (c == '!')
        return i;
    } else {
      if (c == char_context)
        char_context = 0;
    }
  }
  open_delim = char_context;
  return std::string::npos;
}

/* Find trailing continuation and/or comments.  Note that start_idx needs to be
   past any prefixed labels or leading continuations.  This version is more
   complicated than find_trailing_fixed() because we have to handle trailing
   continuations, and the possibility that they can appear inside a character
   context. */
std::string::size_type
find_trailing_free(std::string const &txt,
                   std::string::size_type const start_idx,
                   char const previous_open_delim, char &open_delim) {
  using ST = std::string::size_type;
  assert(previous_open_delim == '\0' || previous_open_delim == '\"' ||
         previous_open_delim == '\'');

  const ST N = txt.size();
  char char_context = previous_open_delim;

  for (ST i = start_idx; i < N; ++i) {
    const char c = txt[i];
    if (!char_context) {
      /* Note that you shouldn't use this technique to find the extent of
         strings, as they are allowed to contain doubled delimiters as escapes.
         For example: 'Paul''s code' is equivalent to "Paul's code", not two
         strings.  For our purpose, this doesn't matter. */
      if (c == '\'' || c == '"')
        char_context = c;
      else if (c == '!' || c == '&') {
        open_delim = '\0';
        return i;
      }
    } else {
      if (c == char_context)
        char_context = 0;
      else if (c == '&') {
        // In F90 free-format, an ampersand with no tailing
        // non-blank characters can signal a continuation of a
        // character context.
        bool ws{true};
        for (ST j = i + 1; j < N && ws; ++j)
          ws = std::isspace(txt[j]);

        if (ws) {
          open_delim = char_context;
          return i;
        }
      }
    }
  }

  // We've reached the end of the line, shouldn't be in a character context
  if (char_context != '\0')
    throw std::runtime_error("find_trailing_free: finished line "
                             "in character context");

  open_delim = '\0';
  return std::string::npos;
}

} // namespace
