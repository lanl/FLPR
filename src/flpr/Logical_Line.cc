/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Logical_Line.cc
*/

#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "flpr/Logical_Line.hh"
#include "flpr/Syntax_Tags.hh"
#include "flpr/utils.hh"

#include "Smash_Hash.hh"
#include "scan_fort.hh"

// Defined in scan_toks.l
extern int curr_line_pos;

namespace FLPR {
/* ------------------------------------------------------------------------ */
Logical_Line::Logical_Line() noexcept { clear(); }

/* ------------------------------------------------------------------------ */
Logical_Line::Logical_Line(Logical_Line const &src) noexcept
    : file_info{src.file_info}, label{src.label}, cat{src.cat},
      suppress{src.suppress}, needs_reformat{src.needs_reformat},
      num_semicolons_{src.num_semicolons_}, layout_{src.layout_},
      fragments_{src.fragments_}, stmts_{src.stmts_} {
  // Now we need to update the iterators in stmts_ to point to new fragments
  TT_SEQ::iterator dstb{fragments_.begin()};
  TT_SEQ::const_iterator srcb{src.fragments_.cbegin()};
  std::transform(src.stmts_.begin(), src.stmts_.end(), stmts_.begin(),
                 [srcb, dstb](auto r) { return rebase(srcb, r, dstb); });
}

/* ------------------------------------------------------------------------ */
Logical_Line &Logical_Line::operator=(Logical_Line const &src) noexcept {
  layout_ = src.layout_;
  fragments_ = src.fragments_;
  stmts_ = src.stmts_;
  file_info = src.file_info;
  label = src.label;
  cat = src.cat;
  suppress = src.suppress;
  needs_reformat = src.needs_reformat;
  num_semicolons_ = src.num_semicolons_;

  // Now we need to update the iterators in stmts to point to new fragments
  TT_SEQ::iterator dstb{fragments_.begin()};
  TT_SEQ::const_iterator srcb{src.fragments_.cbegin()};
  std::transform(src.stmts_.begin(), src.stmts_.end(), stmts_.begin(),
                 [srcb, dstb](auto r) { return rebase(srcb, r, dstb); });
  return *this;
}

/* ------------------------------------------------------------------------ */
Logical_Line::Logical_Line(std::string const &raw_text,
                           bool const free_format) {
  clear();
  if (free_format) {
    layout_.emplace_back(File_Line::analyze_free(raw_text));
  } else {
    layout_.emplace_back(File_Line::analyze_fixed(raw_text));
  }
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
Logical_Line::Logical_Line(std::vector<std::string> const &raw_text,
                           bool const free_format) {
  clear();
  if (free_format) {
    char prev_open_delim = '\0';
    bool prev_line_cont = false;
    int line_no = 1;
    bool in_literal_block{false};
    for (auto const &l : raw_text) {
      layout_.emplace_back(File_Line::analyze_free(
          line_no++, l, prev_open_delim, prev_line_cont, in_literal_block));
      prev_open_delim = layout_.back().open_delim;
      prev_line_cont = layout_.back().is_continued();
    }
  } else {
    char prev_open_delim = '\0';
    int line_no = 1;
    for (auto const &l : raw_text) {
      layout_.emplace_back(
          File_Line::analyze_fixed(line_no++, l, prev_open_delim));
      prev_open_delim = layout_.back().open_delim;
    }
  }
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::clear() noexcept {
  suppress = false;
  layout_.clear();
  fragments_.clear();
  label = 0;
  cat = LineCat::UNKNOWN;
  needs_reformat = false;
  clear_stmts();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::init_from_layout() noexcept {
  Line_Accum la;

  for (auto &fl : layout_) {
    if (!fl.is_trivial()) {
      int const first_col = fl.main_first_col();

      /* need to add the left and right space sizes here so that continued lines
         have the correct breaks between lexemes. */
      la.add_line(fl.linenum, fl.left_space.size(), first_col, fl.main_txt,
                  fl.right_space.size());
    }
  }

  if (layout_.front().has_label()) {
    assert(!layout_.front().left_txt.empty());
    std::size_t pos;
    label = std::stoi(layout_.front().left_txt, &pos);
    if (pos < layout_.front().left_txt.size()) {
      std::cerr << "Label \"" << layout_.front().left_txt
                << "\" not fully converted to integer" << std::endl;
    }
  } else
    label = 0;

  tokenize(la);
}

namespace {
size_t max_main_txt_len(File_Line const &fl) noexcept {
  const size_t MAX_CHARS{76};
  if (fl.is_comment())
    return 0;

  size_t mlen = fl.left_txt.size() + fl.left_space.size();
  const size_t rlen = fl.right_space.size() + fl.right_txt.size();

  // We will try to format around the right space and right txt, as
  // long as we have some room to do so.  Note that this will get us
  // in trouble if right_text is a continuation that gets pushed way
  // over by fl.right_space
  if (mlen + rlen < MAX_CHARS / 2)
    mlen += rlen;
  assert(mlen < MAX_CHARS);
  return MAX_CHARS - mlen;
}

// Continue the current line
Logical_Line::FL_VEC::iterator
continue_fl(Logical_Line::FL_VEC &text, Logical_Line::FL_VEC::iterator curr) {
  curr->make_continued();

  size_t spaces = curr->left_txt.size() + curr->left_space.size();
  if (curr == text.begin())
    spaces += 2;
  // Add (or move to) the next line in text
  do
    std::advance(curr, 1);
  while (curr != text.end() && curr->is_comment());

  if (curr == text.end())
    curr = text.insert(text.end(), File_Line());
  else {
    curr->left_txt.clear();
    curr->make_uncontinued();
    curr->main_txt.clear();
  }
  curr->left_space.assign(spaces, ' ');
  return curr;
}

} // namespace

/* ------------------------------------------------------------------------ */
bool Logical_Line::append_tt_if_(std::string &main_txt, size_t max_len,
                                 Token_Text const &tt, bool first) {
  size_t len = tt.text().size();
  if (!first && tt.pre_spaces_ > 0)
    len += tt.pre_spaces_;
  // Save room to end the line on a comma
  if (tt.token != Syntax_Tags::TK_COMMA)
    max_len -= 1;
  if (main_txt.size() + len > max_len)
    return false;
  if (!first && tt.pre_spaces_ > 0)
    main_txt.append(tt.pre_spaces_, ' ');
  main_txt.append(tt.text());
  return true;
}

/* ------------------------------------------------------------------------ */
void Logical_Line::text_from_frags() noexcept {
  if (layout_.empty())
    return;
  auto fline_it = layout_.begin();
  if (!fline_it->is_fortran())
    return;
  fline_it->main_txt.clear();
  fline_it->make_uncontinued();

  bool line_start{true};
  size_t max_llen = max_main_txt_len(*fline_it);
  for (auto tt_it = fragments_.begin(); tt_it != fragments_.end(); ++tt_it) {
    int redo = 0;
    do {
      if (append_tt_if_(fline_it->main_txt, max_llen, *tt_it, line_start)) {
        //	    std::cout << "APPEND " << redo << ' ' << *tt_it << '\n';
        redo = 0;
        line_start = false;
        continue;
      }
      //	std::cout << "FAIL " << redo << ' ' << line_start << ' ' <<
      //*tt_it << '\n';
      // Couldn't append the token to the current line
      bool add_line{true};
      if (line_start) {
        // I couldn't add the current token as the first one on a
        // line!
        size_t token_size = tt_it->text().size();
        size_t splits = token_size / max_llen + 1;
        size_t split_size = token_size / splits + 1;
        size_t pos{0}, count{split_size};

        for (size_t i = 0; i < splits; ++i) {
          if (i > 0)
            fline_it->main_txt.append(1, '&');
          fline_it->main_txt.append(tt_it->text().substr(pos, count));
          // Move the continuation ampersand in close
          if (i + 1 != splits) {
            fline_it->make_continued();
            fline_it->right_space.clear();
            fline_it = continue_fl(layout_, fline_it);
          }
          pos = pos + count;
        }
        line_start = false;
        add_line = false;
        redo = 0;
      }
      if (add_line) {
        //	    std::cout << "NEW LINE" << std::endl;
        // Setup a new File_Line
        fline_it = continue_fl(layout_, fline_it);
        max_llen = max_main_txt_len(*fline_it);
        line_start = true;
        // Try again
        redo += 1;
      }
    } while (redo > 0 && redo < 4);
    if (redo == 4) {
      std::cerr << "text_from_frags: couldn't add " << *tt_it
                << " to any File_Line\n";
      dump(std::cerr);
      exit(1);
    }
  }

  if (!fragments_.empty())
    std::advance(fline_it, 1);

  bool blanked{false};
  while (fline_it != layout_.end()) {
    if (!fline_it->is_comment()) {
      fline_it->make_uncontinued();
      if (fline_it->right_txt.empty()) {
        fline_it->set_classification(File_Line::class_flags::blank);
        blanked = true;
      } else {
        fline_it->main_txt.clear();
        fline_it->set_classification(File_Line::class_flags::comment);
      }
    }
    std::advance(fline_it, 1);
  }
  if (blanked) {
    auto newend =
        std::remove_if(layout_.begin(), layout_.end(),
                       [](File_Line const &fl) { return fl.is_blank(); });
    layout_.erase(newend, layout_.end());
    if (layout_.empty())
      suppress = true;
  }
  init_stmts();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::replace_fragment(typename TT_SEQ::iterator frag,
                                    int const new_syntag,
                                    std::string const &new_text) {

  auto const old_text_len = frag->text().size();
  int len_change = (int)new_text.size() - (int)old_text_len;

  frag->token = new_syntag;
  frag->mod_text() = new_text;

  assert(!frag->is_split_token_());
  int const layout_line = frag->mt_begin_line_;
  std::string &main_txt = layout_[layout_line].main_txt;
  main_txt.replace(frag->mt_begin_col_, old_text_len, new_text);

  // Update the mt_begin_col_ for all fragments following on this line
  for (frag = std::next(frag);
       frag != fragments_.end() && frag->mt_begin_line_ == layout_line;
       frag = std::next(frag)) {
    frag->mt_begin_col_ += len_change;
    if (!frag->is_split_token_())
      frag->mt_end_col_ += len_change;
  }
}

/* ------------------------------------------------------------------------ */
void Logical_Line::remove_fragment(typename TT_SEQ::iterator frag) {

  auto const old_text_len = frag->text().size();

  /* this isn't setup to do tokens that are split across continuations */
  assert(!frag->is_split_token_());
  int const layout_line = frag->mt_begin_line_;
  std::string &main_txt = layout_[layout_line].main_txt;
  main_txt.erase(frag->mt_begin_col_, old_text_len);

  int const len_change = -(int)(old_text_len);

  // Update the mt_begin_col_ for all fragments following on this line
  for (frag = fragments_.erase(frag);
       frag != fragments_.end() && frag->mt_begin_line_ == layout_line;
       frag = std::next(frag)) {
    frag->mt_begin_col_ += len_change;
    if (!frag->is_split_token_())
      frag->mt_end_col_ += len_change;
  }

  init_stmts();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::replace_main_text(std::vector<std::string> const &new_text) {
  if (new_text.empty())
    return;
  assert(!layout_.empty());
  assert(!is_compound());

  size_t const old_size = layout_.size();

  /* find the last is_fortran() line in layout_ to use as a prototype for any
     needed new File_Lines, and to mark as continued if appending new text */
  size_t ref_line = old_size - 1;
  while (ref_line > 0 && !layout_[ref_line].is_fortran()) {
    ref_line -= 1;
  }
  assert(layout_[ref_line].is_fortran());
  assert(!layout_[ref_line].is_continued());

  size_t num_orig_fortran_lines{0};
  for (auto const &fl : layout_) {
    if (fl.is_fortran())
      num_orig_fortran_lines += 1;
  }

  /* Currently, this routine will destroy embedded comment lines.  Prevent this
     from happening until it is fixed. */
  assert(num_orig_fortran_lines == layout_.size());

  if (layout_.size() < new_text.size()) {
    int const indent = layout_[ref_line].main_first_col() - 1;
    File_Line ref_fl = layout_[ref_line];
    ref_fl.make_continued();
    ref_fl.left_txt.clear();
    ref_fl.right_txt.clear();
    ref_fl.set_leading_spaces(indent);
    layout_.insert(layout_.end(), new_text.size() - old_size, ref_fl);
  }

  /* copy the new text into place */
  size_t fl_idx{0};
  for (std::string const &s : new_text) {
    layout_[fl_idx++].main_txt = s;
  }
  assert(ref_line < fl_idx);

  /* fix continuations */
  if (ref_line < fl_idx - 1) {
    layout_[ref_line].make_continued();
  }
  layout_[fl_idx - 1].make_uncontinued();

  /* clear any excess original lines, preserving comments */
  for (size_t i = fl_idx; i < layout_.size(); ++i) {
    layout_[i].make_comment_or_blank();
  }
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::erase_stmt_text_(int stln, int stcol, int eln, int ecol) {
  assert(stln <= eln);
  assert(stln >= 0);
  assert(eln <= static_cast<int>(layout_.size()));
  assert(layout_[stln].is_fortran());
  bool const multiline = stln < eln;
  int const hold_stln = stln;
  if (multiline) {
    assert(layout_[eln].is_fortran());
    layout_[stln].main_txt.erase(stcol);
    while (++stln < eln) {
      if (!layout_[stln].is_fortran())
        continue;
      layout_[stln].make_comment_or_blank();
    }
    stcol = 0;
  }
  assert(stcol <= ecol);
  assert(stcol >= 0);
  assert(ecol <= static_cast<int>(layout_[eln].main_txt.size()));
  layout_[stln].main_txt.erase(stcol, ecol - stcol);
  if (layout_[stln].main_txt.find_first_not_of(' ') == std::string::npos) {
    layout_[stln].main_txt.clear();
    if (multiline)
      layout_[stln].make_comment_or_blank();
  }

  int count_fortran{0};
  for (int i = hold_stln; i <= eln; ++i)
    if (layout_[i].is_fortran())
      count_fortran += 1;
  if (count_fortran == 1)
    layout_[hold_stln].make_uncontinued();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::replace_stmt_substr(TT_Range const &orig,
                                       std::string const &new_text) {

  int const sl = orig.front().mt_begin_line_;
  int const sc = orig.front().mt_begin_col_;
  int const el = orig.back().mt_end_line_;
  int const ec = orig.back().mt_end_col_;
  erase_stmt_text_(sl, sc, el, ec);
  layout_[sl].main_txt.insert(sc, new_text);
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::insert_text_before(typename TT_SEQ::iterator frag,
                                      std::string const &new_text) {
  int sl, sc;
  if (frag == fragments_.end()) {
    auto const &tmp = fragments_.back();
    sl = tmp.mt_end_line_;
    sc = tmp.mt_end_col_;
  } else {
    sl = frag->mt_begin_line_;
    sc = frag->mt_begin_col_;
  }
  assert(sl < static_cast<int>(layout_.size()));
  assert(sc <= static_cast<int>(layout_[sl].main_txt.size()));
  layout_[sl].main_txt.insert(sc, new_text);
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::insert_text_after(typename TT_SEQ::iterator frag,
                                     std::string const &new_text) {
  int el, ec;
  assert(frag != fragments_.end());
  el = frag->mt_end_line_;
  ec = frag->mt_end_col_;

  assert(el < static_cast<int>(layout_.size()));
  assert(ec <= static_cast<int>(layout_[el].main_txt.size()));
  layout_[el].main_txt.insert(ec, new_text);
  init_from_layout();
}

/* ------------------------------------------------------------------------ */
bool Logical_Line::split_after(typename TT_SEQ::iterator frag,
                               Logical_Line &new_ll) {
  if (frag == fragments_.end())
    return false;
  int const split_line = frag->mt_end_line_;
  assert(split_line < static_cast<int>(fragments_.size()));

  auto const num_left_sp = layout_[0].main_first_col() - 1;

  /* collect the range of tokens that follow frag on the same layout_ line */
  auto lr_beg = std::next(frag);
  auto lr_end = lr_beg;
  while (lr_end != fragments_.end() && lr_end->mt_begin_line_ == split_line) {
    lr_end = std::next(lr_end);
  }

  /* eliminate any semicolons at the begining of the remainder sequence */
  while (lr_beg != lr_end && Syntax_Tags::TK_SEMICOLON == lr_beg->token) {
    lr_beg = std::next(lr_beg);
  }

  FL_VEC::iterator export_beg;

  /* if the range isn't empty, insert a new layout_ line, move the remainder
     text to the new line, and update all mt_begin_line_ following frag. */
  if (lr_beg != lr_end) {
    /* insert a duplicate */
    auto layout_insert = layout_.begin() + split_line + 1;
    export_beg = layout_.insert(layout_insert, layout_[split_line]);

    /* cleanup the source: if there is a right_space, expand it to cover the
     characters being erased from main_txt, in order to preserve trailing
     comment alignment */
    size_t erase_start_pos = frag->mt_end_col_;
    if (!layout_[split_line].right_txt.empty()) {
      size_t const new_size = layout_[split_line].right_space.size() +
                              layout_[split_line].main_txt.size() -
                              erase_start_pos;
      layout_[split_line].right_space.assign(new_size, ' ');
    }
    /* cleanup the source: remove main_txt past the end of frag */
    layout_[split_line].main_txt.erase(erase_start_pos);

    /* cleanup the source: remove any trailing ampersand continuations */
    layout_[split_line].make_uncontinued();

    /* clean new: remove main_txt before start of frag */
    export_beg->main_txt.erase(0, lr_beg->mt_begin_col_);

    /* clean new: remove right_txt comment, and recover trailing ampersand, if
       needed */
    if (export_beg->is_continued()) {
      export_beg->right_txt = '&';
      size_t const new_rs =
          export_beg->right_txt.size() + lr_beg->mt_begin_col_ - 1;
      if (export_beg->right_space.size() != new_rs) {
        export_beg->right_space.assign(new_rs, ' ');
      }
    } else {
      export_beg->right_space.clear();
      export_beg->right_txt.clear();
    }

    /* clean new: remove left_txt */
    export_beg->left_txt.clear();
    /* clean new: indent column */
    export_beg->set_leading_spaces(num_left_sp);

    /* now, shuffle all the mt_begin_col_ down on [lr_beg, lr_end) */
    int const offset = lr_beg->mt_begin_col_;
    for (auto tt = lr_beg; tt != lr_end; ++tt) {
      tt->mt_begin_col_ -= offset;
      if (!tt->is_split_token_())
        tt->mt_end_col_ -= offset;
    }

    /* finally, push ALL remainder token lines down one */
    for (auto tt = lr_beg; tt != fragments_.end(); ++tt) {
      tt->mt_begin_line_ += 1;
      tt->mt_end_line_ += 1;
    }

  } else {
    /* if there is a non-trivial line after split-line, make sure that it is not
       continuation and adjust indent */
    export_beg = layout_.begin() + split_line + 1;

    /* cleanup the source: remove any trailing ampersand continuations */
    layout_[split_line].make_uncontinued();

    /* skip comments */
    auto non_trivial = export_beg;
    while (non_trivial != layout_.end() && non_trivial->is_trivial()) {
      if (non_trivial->left_txt.empty()) {
        non_trivial->left_space.assign(num_left_sp, ' ');
      }
      non_trivial = std::next(non_trivial);
    }
    if (non_trivial != layout_.end()) {
      non_trivial->left_txt.clear();
      non_trivial->set_leading_spaces(num_left_sp);
    }
  }

  if (export_beg == layout_.end())
    return false;

  new_ll.clear();

  /* Export all of the remainder lines out to new_ll */
  new_ll.layout_.insert(new_ll.layout_.end(),
                        std::make_move_iterator(export_beg),
                        std::make_move_iterator(layout_.end()));

  /* Move the remainder fragments starting at lr_beg (not next(frag), because
     those might be semicolons). */
  new_ll.fragments_.insert(new_ll.fragments_.end(),
                           std::make_move_iterator(lr_beg),
                           std::make_move_iterator(fragments_.end()));

  /* update the main_txt line numbers for the new fragments  */
  int const move_up = new_ll.fragments_.front().mt_begin_line_;
  for (auto &tt : new_ll.fragments_) {
    tt.mt_begin_line_ -= move_up;
    tt.mt_end_line_ -= move_up;
  }

  /* Trim down this layout_ */
  layout_.resize(split_line + 1);

  /* Statements is now invalid */
  clear_stmts();

  /* The fragments starting after frag need to be cleaned up for this line */
  fragments_.erase(std::next(frag), fragments_.end());

  return true;
}

/* ------------------------------------------------------------------------ */
bool Logical_Line::remove_empty_statements() {
  bool changed{false};

  TT_SEQ::iterator tt = fragments_.begin();
  while (tt != fragments_.end() && tt->token == Syntax_Tags::TK_SEMICOLON)
    ++tt;
  if (tt != fragments_.begin()) {
    changed = true;
    fragments_.erase(fragments_.begin(), tt);
  }

  bool back_changed{false};
  while (!fragments_.empty() &&
         fragments_.back().token == Syntax_Tags::TK_SEMICOLON) {
    // If we get here, there is at least one non-semi token, otherwise
    // the previous loop would have cleaned them out
    fragments_.pop_back();
    back_changed = true;
  }

  if (stmts_.size() > 1) {
    bool internal_empty{false};
    for (tt = fragments_.begin(); tt != fragments_.end(); ++tt) {
      if (tt->token != Syntax_Tags::TK_SEMICOLON)
        continue;
      auto next_tt = std::next(tt);
      if (next_tt != fragments_.end() &&
          next_tt->token == Syntax_Tags::TK_SEMICOLON) {
        // Clear the second, third, whatever semis, as the first one
        // is the end to a statement.
        next_tt->mod_text().clear();
        internal_empty = true;
      }
    }
    if (internal_empty) {
      fragments_.remove_if([](Token_Text const &tt) {
        return (tt.token == Syntax_Tags::TK_SEMICOLON) && tt.text().empty();
      });
      changed = true;
    }
  }
  if (back_changed) {
    // Fix up the end of the last statement
    stmts_.back() = TT_Range(stmts_.back().begin(), fragments_.end());
    changed = true;
  }

  if (changed)
    text_from_frags();
  return changed;
}

/* ------------------------------------------------------------------------ */
void Logical_Line::init_stmts() {
  bool stmt_beg_init{false}; // "statement begin initialized"
  iterator s_beg, s_end;     // statement begin, statement end
  stmts_.clear();
  num_semicolons_ = 0; // this indicates that num_semicolons_ is initialized

  for (auto f_it = fragments_.begin(); f_it != fragments_.end(); ++f_it) {
    // If the current statement begin is not set, make it so.
    if (!stmt_beg_init) {
      stmt_beg_init = true;
      // Need this formulation in the case of compound statements
      s_beg = f_it;
    }
    // Close a statement
    if (f_it->token == Syntax_Tags::TK_SEMICOLON) {
      num_semicolons_ += 1;
      // Only record it if not empty
      if (s_beg != f_it)
        stmts_.emplace_back(s_beg, f_it);
      stmt_beg_init = false;
    }
  }
  // Close off the open statement
  if (stmt_beg_init) {
    s_end = fragments_.end();
    if (s_beg != s_end)
      stmts_.emplace_back(s_beg, s_end);
  }
}

/* ------------------------------------------------------------------------ */
bool Logical_Line::set_leading_spaces(int const spaces, int continued_offset) {
  /* continued_offset only applies to Fortran lines (not comments, etc) */
  if (!has_fortran())
    continued_offset = 0;
  bool changed = false;

  // Re-indent the first File_Line
  changed |= layout_[0].set_leading_spaces(spaces);
  // Re-indent any continuation File_Lines
  size_t const N = layout_.size();
  for (size_t i = 1; i < N; ++i) {
    changed |= layout_[i].set_leading_spaces(spaces + continued_offset);
  }
  return changed;
}

/* ------------------------------------------------------------------------ */
bool Logical_Line::set_label(int new_label) {
  if (new_label == label)
    return false;
  layout_[0].set_label(new_label);
  label = new_label;
  return true;
}

/* ------------------------------------------------------------------------ */
void Logical_Line::tokenize(Line_Accum const &la) {
  // Clear out any previous tokens
  fragments_.clear();

  // The index into la.accum()
  curr_line_pos = 0;

  // Feed the la.accum() to the flex parser to generate
  // the Logical_Line token fragment data.
  YY_BUFFER_STATE bs = yy_scan_string(la.accum().c_str());
  const int N = la.accum().size();
  int tok_start_col = curr_line_pos;
  int next_pre_sp = 0;
  int space_between;

  for (int result_tok = yylex(); result_tok != Syntax_Tags::EOL;
       result_tok = yylex()) {
    /* tok_start is an index into la.accum().  Convert this into a file line and
     column number */
    int li, ci, tli, tci;
    la.linecolno(tok_start_col, li, ci, tli, tci);
    fragments_.emplace_back(std::string(yytext, yyleng), result_tok, li, ci);
    /* Break up keywords with no space. */
    if (result_tok == Syntax_Tags::TK_NAME)
      unsmash();

    /* calculate the "end" address (one character beyond the last character of
       the token */

    int end_file_line_idx, end_file_col_idx, end_text_line_idx,
        end_text_col_idx;

    la.linecolno(curr_line_pos - 1, end_file_line_idx, end_file_col_idx,
                 end_text_line_idx, end_text_col_idx);
    end_text_col_idx += 1;

    /* yylex advances curr_line_pos to the end of the last token recognized, but
       we want where the next token begins */
    tok_start_col = curr_line_pos;
    space_between = 0;
    while (tok_start_col < N && std::isspace(la.accum()[tok_start_col])) {
      tok_start_col += 1;
      space_between += 1;
    }

    Token_Text &tt{fragments_.back()};
    tt.mt_begin_line_ = tli;
    tt.mt_begin_col_ = tci;
    tt.mt_end_line_ = end_text_line_idx;
    tt.mt_end_col_ = end_text_col_idx;
    tt.pre_spaces_ = next_pre_sp;
    tt.post_spaces_ = space_between;

    next_pre_sp = space_between;
  }
  yy_delete_buffer(bs);
  init_stmts();
}

/* ------------------------------------------------------------------------ */
void Logical_Line::append_comment(std::string const &comment_text) {
  if (comment_text.empty())
    return;
  if (layout_[0].right_txt.empty()) {
    int lline_len = layout_[0].main_first_col() + layout_[0].main_txt.size();
    int c_len = 2 + comment_text.size();
    if (72 - c_len > lline_len)
      layout_[0].right_space = std::string(72 - c_len - lline_len, ' ');
    else
      layout_[0].right_space = "    ";
    layout_[0].right_txt = "! ";
    layout_[0].right_txt.append(comment_text);
  } else {
    if (layout_[0].right_txt.find('!') == std::string::npos)
      layout_[0].right_txt.append(" ! ");
    else
      layout_[0].right_txt.append(" : ");
    layout_[0].right_txt.append(comment_text);
  }
}

/* ------------------------------------------------------------------------ */
void Logical_Line::unsmash() {
  using details_::Smash_Hash;
  using details_::Smashed;
  if (fragments_.empty())
    return;
  if (fragments_.back().token != Syntax_Tags::TK_NAME)
    return;

  Smashed const *ptr = Smash_Hash::in_word_set(
      fragments_.back().lower().c_str(), fragments_.back().text().size());
  if (!ptr)
    return;
  Token_Text new2{fragments_.back()};

  // Alter the first token in place
  Token_Text &new1{fragments_.back()};
  new1.token = ptr->tok1;
  new1.mod_text().erase(ptr->splitpos);
  new1.post_spaces_ = 0;

  // Add a new second token
  new2.token = ptr->tok2;
  new2.mod_text().erase(0, ptr->splitpos);
  new2.start_pos += ptr->splitpos;
  new2.pre_spaces_ = 0;
  fragments_.emplace_back(std::move(new2));
}

/* ------------------------------------------------------------------------ */
std::ostream &operator<<(std::ostream &os, const LineCat &l) {
  switch (l) {
  case LineCat::INCLUDE:
    os << "INCLUDE";
    break;
  case LineCat::LITERAL:
    os << "LITERAL";
    break;
  case LineCat::MACRO:
    os << "MACRO";
    break;
  case LineCat::FLPR_PP:
    os << "FLPR_PP";
    break;
  case LineCat::UNKNOWN:
    os << "UNKNOWN";
    break;
  default:
    os << "*** UNHANDLED LineCat ENUM *** " << static_cast<int>(l);
    break;
  }
  return os;
}

std::ostream &Logical_Line::print(std::ostream &os) const {
  if (!suppress) {
    for (auto const &fl : layout_) {
      os << fl << '\n';
    }
  }
  return os;
};

/* ------------------------------------------------------------------------ */
std::ostream &operator<<(std::ostream &os, Logical_Line const &ll) {
  return ll.print(os);
}

std::ostream &Logical_Line::dump(std::ostream &os) const {
  for (auto const &tt : fragments_)
    Syntax_Tags::print(os, tt.token) << ' ';
  os << '\n';
  for (auto const &fl : layout_) {
    os << '\t';
    fl.dump(os) << '\n';
  }
  return os;
}

} // namespace FLPR
