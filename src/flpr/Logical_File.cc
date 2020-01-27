/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Logical_File.cc
*/

#include "flpr/Logical_File.hh"
#include "flpr/File_Line.hh"
#include "flpr/LL_Stmt_Src.hh"
#include "flpr/utils.hh"

#include <cassert>
#include <cctype>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>

namespace FLPR {
void Logical_File::clear() {
  file_info.reset();
  lines.clear();
  ll_stmts.clear();
  has_flpr_pp = false;
  num_input_lines = 0;
}

bool Logical_File::read_and_scan(std::string const &filename,
                                 File_Type file_type) {
  std::ifstream is(filename.c_str());
  if (!is) {
    std::cerr << "Logical_File::read_and_scan: unable to open file \""
              << filename << "\" for reading\n";
    return false;
  }

  bool res = read_and_scan(is, filename, file_type);

  is.close();
  return res;
}

bool Logical_File::read_and_scan(std::istream &is,
                                 std::string const &stream_name,
                                 File_Type stream_type) {
  Line_Buf buf;
  buf.reserve(1024);
  for (std::string line; std::getline(is, line);) {
    buf.push_back(line);
  }
  return scan(buf, stream_name, stream_type);
}

bool Logical_File::scan(Line_Buf const &buf, std::string const &buffer_name,
                        File_Type buffer_type) {
  file_info = std::make_shared<File_Info>(buffer_name, buffer_type);

  bool res = false;
  switch (file_type()) {
  case File_Type::FIXEDFMT:
    res = scan_fixed(buf);
    break;
  case File_Type::FREEFMT:
    res = scan_free(buf);
    break;
  default:
    std::cerr << "FLPR::Logical_File::scan Error: "
                 "unsupported file type with "
              << file_type() << "\n";
  };
  return res;
}

bool Logical_File::scan_fixed(Line_Buf const &raw_lines) {
  const size_t N = raw_lines.size();
  num_input_lines = N;
  // Convert the raw text input into File_Lines
  std::vector<File_Line> fl(N);
  char prev_open_delim = '\0';
  for (size_t i = 0; i < N; ++i) {
    try {
      fl[i] =
          File_Line::analyze_fixed((int)i + 1, raw_lines[i], prev_open_delim);
    } catch (std::exception &e) {
      std::cerr << "At line " << i + 1 << " of \"" << file_info->filename
                << "\":\n"
                << raw_lines[i] << '\n'
                << "scan_fixed error: " << e.what() << std::endl;
      return false;
    }
    prev_open_delim = fl[i].open_delim;
  }

  /* Identify "logical lines": blocks of lines that represent a
     comment/whitespace block or a single statement. */
  size_t curr = 0;
  while (curr < N) {
    size_t start_line = curr;
    while (curr < N && fl[curr].is_trivial())
      curr += 1;
    if (curr > start_line) {
      // Have a trivial block [start_line..curr)
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      lines.back().file_info = file_info;
      continue;
    }

    if (curr < N && !fl[curr].is_fortran()) {
      /* Look for a continued preprocessor statement */
      assert(fl[curr].is_preprocessor() || fl[curr].is_flpr_pp() ||
             fl[curr].is_include());
      LineCat cat{LineCat::UNKNOWN};
      // Categorize this line
      if (fl[curr].is_flpr_pp()) {
        has_flpr_pp = true;
        cat = LineCat::FLPR_PP;
      } else if (fl[curr].is_include()) {
        cat = LineCat::INCLUDE;
      } else {
        cat = LineCat::MACRO;
      }

      /* absorb any continued preprocessor lines */
      size_t start_line = curr++;
      while (curr < N && last_non_blank_char(fl[curr - 1].left_txt) == '\\') {
        fl[curr].make_preprocessor();
        curr += 1;
      }
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      Logical_Line &ll = lines.back();
      ll.file_info = file_info;
      ll.cat = cat;
      continue;
    }

    if (curr < N) {
      if (!fl[curr].is_fortran()) {
        std::cerr << "scan_fixed confused on line " << curr + 1 << "of \""
                  << file_info->filename << "\": \n"
                  << fl[curr] << '\n';
        exit(1);
      }

      size_t last_code_line = curr;
      while (++curr < N) {
        if (fl[curr].is_continuation())
          last_code_line = curr;
        else if (fl[curr].is_trivial())
          continue;
        else
          break;
      }
      // The above routine will consume trailing trivial lines, so wind
      // curr back to the point just after the last fortran line.
      curr = last_code_line + 1;

      // code for this statement is now in [start_line..curr)
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      Logical_Line &ll = lines.back();
      ll.needs_reformat = true;
      ll.file_info = file_info;
    }
  }

  return true;
}

bool Logical_File::scan_free(std::vector<std::string> const &raw_lines) {
  const size_t N = raw_lines.size();
  num_input_lines = N;
  bool in_literal_block = false;
  // Convert the raw text input into File_Lines
  std::vector<File_Line> fl(N);
  char prev_open_delim = '\0';
  bool prev_line_cont = false;
  for (size_t i = 0; i < N; ++i) {
    try {
      fl[i] = File_Line::analyze_free((int)i + 1, raw_lines[i], prev_open_delim,
                                      prev_line_cont, in_literal_block);
    } catch (std::exception &e) {
      std::cerr << "At line " << i + 1 << " of \"" << file_info->filename
                << "\":\n"
                << raw_lines[i] << '\n'
                << "scan_free error: " << e.what() << std::endl;
      return false;
    }
    prev_open_delim = fl[i].open_delim;
    prev_line_cont = fl[i].is_continued();
  }

  // Identify "logical lines": blocks of lines that represent a
  // comment/whitespace block or a single statement.
  size_t curr = 0;
  while (curr < N) {
    // Look for a block of trivial lines
    size_t start_line = curr;
    while (curr < N && fl[curr].is_trivial())
      curr += 1;
    if (curr > start_line) {
      // Have a trivial block [start_line..curr)
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      Logical_Line &ll = lines.back();
      ll.file_info = file_info;
      continue;
    }

    // Look for a FLPR literal block
    start_line = curr;
    while (curr < N && fl[curr].is_flpr_lit())
      curr += 1;
    if (curr > start_line) {
      // Have a literal block [start_line..curr)
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      Logical_Line &ll = lines.back();
      ll.file_info = file_info;
      ll.cat = LineCat::LITERAL;
      continue;
    }

    if (curr < N && !fl[curr].is_fortran()) {
      /* Look for a continued preprocessor statement */
      assert(fl[curr].is_preprocessor() || fl[curr].is_flpr_pp() ||
             fl[curr].is_include());
      LineCat cat{LineCat::UNKNOWN};
      // Categorize this line
      if (fl[curr].is_flpr_pp()) {
        has_flpr_pp = true;
        cat = LineCat::FLPR_PP;
      } else if (fl[curr].is_include()) {
        cat = LineCat::INCLUDE;
      } else {
        cat = LineCat::MACRO;
      }

      /* absorb any continued lines */
      size_t start_line = curr++;
      while (curr < N && last_non_blank_char(fl[curr - 1].left_txt) == '\\') {
        fl[curr].make_preprocessor();
        curr += 1;
      }
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      Logical_Line &ll = lines.back();
      ll.file_info = file_info;
      ll.cat = cat;
      continue;
    }

    if (curr < N) {
      if (!fl[curr].is_fortran()) {
        std::cerr << "scan_free confused on line " << curr + 1 << "of \""
                  << file_info->filename << "\": \n"
                  << fl[curr] << '\n';
        exit(1);
      }

      size_t last_code_line = curr;
      while (curr < N) {
        if (!fl[curr].is_trivial() && !fl[curr].is_continued()) {
          last_code_line = curr;
          break;
        }
        curr += 1;
      }

      curr = last_code_line + 1;

      // code for this statement is now in [start_line..curr)
      lines.emplace_back(fl.begin() + start_line, fl.begin() + curr);
      lines.back().file_info = file_info;
    }
  }
  return true;
}

void Logical_File::make_stmts() {
  ll_stmts.clear();
  LL_Stmt_Src ss{lines, false};
  while (ss.advance()) {
    ll_stmts.emplace_back(ss.move());
  }
}

bool Logical_File::split_compound_before(LL_STMT_SEQ::iterator pos) {
  if (pos->is_compound() < 2)
    return false;
  /* pos is a compound stmt */
  LL_STMT_SEQ::iterator prev_stmt = std::prev(pos);
  assert(!prev_stmt->empty());
  assert(prev_stmt->is_compound() + 1 == pos->is_compound());
  assert(prev_stmt->it() == pos->it());

  auto ll_seq_orig = prev_stmt->it();
  size_t const num_stmts{ll_seq_orig->stmts().size()};

  /* create a new LL after the original to hold the pos stmt (and its subsequent
     compounds */
  auto ll_seq_new = lines.insert(std::next(ll_seq_orig), Logical_Line());
  bool res = ll_seq_orig->split_after(prev_stmt->last(), *ll_seq_new);
  assert(res);

  /* build the Logical_Line statement list */
  ll_seq_orig->init_stmts();
  assert(!ll_seq_orig->stmts().empty());
  ll_seq_new->init_stmts();
  assert(!ll_seq_new->stmts().empty());
  assert(ll_seq_orig->stmts().size() + ll_seq_new->stmts().size() == num_stmts);

  /* update statements inplace for the new Logical_Line */
  LL_Stmt_Src ss{ll_seq_new, false};
  size_t num_changed{0};
  LL_STMT_SEQ::iterator update_stmt = pos;
  while (ss.advance()) {
    assert(update_stmt->it() == ll_seq_orig);
    int hold = update_stmt->stmt_tag(false);
    update_stmt->update_range(ss.move());
    assert(hold == update_stmt->stmt_tag(false));
    num_changed += 1;
    assert(update_stmt->it() == ll_seq_new);
    std::advance(update_stmt, 1);
  }
  assert(num_changed == ll_seq_new->stmts().size());
  return true;
}

bool Logical_File::isolate_stmt(LL_STMT_SEQ::iterator pos) {
  bool res = split_compound_before(pos);
  LL_STMT_SEQ::iterator next_stmt = std::next(pos);
  if (next_stmt != ll_stmts.end()) {
    res |= split_compound_before(next_stmt);
  }
  return res;
}

LL_STMT_SEQ::iterator Logical_File::emplace_ll_stmt(LL_STMT_SEQ::iterator pos,
                                                    Logical_Line &&ll,
                                                    int new_syntag) {
  assert(ll.has_stmts());
  assert(ll.stmts().size() == 1);

  /* see if we need to split the pos->ll() in order to insert a new LL above
     pos */
  if (pos->is_compound() > 1)
    split_compound_before(pos);

  /* Either this statement is on its own, or is the first compound.  Either
     way, we can insert a Logical_Line directly above the prefix */
  LL_SEQ::iterator ll_insert_pos = pos->prefix_ll_begin();

  /* Insert a Logical_Line to hold the new statement at the correct position */
  LL_SEQ::iterator ll_new = lines.emplace(ll_insert_pos, std::move(ll));

  /* Insert and record the iterator to the new statement */
  LL_Stmt_Src ss{ll_new, true};
  auto result = ll_stmts.emplace(pos, ss.move());
  result->set_stmt_syntag(new_syntag);

  return result;
}

LL_STMT_SEQ::iterator
Logical_File::emplace_ll_stmt_after_prefix(LL_STMT_SEQ::iterator pos,
                                           Logical_Line &&ll, int new_syntag) {
  assert(ll.has_stmts());
  assert(ll.stmts().size() == 1);

  if (pos->prefix_lines.empty())
    return emplace_ll_stmt(pos, std::move(ll), new_syntag);

  /* As this stmt has a prefix, it must be on its own */
  assert(pos->is_compound() < 2);
  LL_SEQ::iterator ll_insert_pos = pos->prefix_ll_end();
  /* Insert a Logical_Line to hold the new statement at the correct position */
  LL_SEQ::iterator ll_new = lines.emplace(ll_insert_pos, std::move(ll));

  /* Insert and record the iterator to the first new statement */
  LL_Stmt_Src ss{ll_new, true};
  auto result = ll_stmts.emplace(pos, ss.move());
  result->set_stmt_syntag(new_syntag);

  /* Now transfer the prefix from the old to the new */
  assert(pos->prefix_ll_end() == ll_new);
  result->prefix_lines = pos->prefix_lines;
  pos->prefix_lines.clear();
  return result;
}

void Logical_File::replace_stmt_text(LL_STMT_SEQ::iterator stmt,
                                     std::vector<std::string> const &new_text,
                                     int new_syntag) {
  /* put the stmt on its own line */
  isolate_stmt(stmt);

  stmt->ll().replace_main_text(new_text);
  assert(stmt->ll().has_stmts());
  assert(stmt->ll().stmts().size() == 1);
  stmt->assign_range(stmt->ll().stmts()[0]);
  stmt->set_stmt_syntag(new_syntag);
  stmt->unhook();
}

void Logical_File::replace_stmt_substr(LL_STMT_SEQ::iterator stmt,
                                       LL_TT_Range const &orig_tt_const,
                                       std::string const &new_text) {

  LL_TT_Range &orig_tt{const_cast<LL_TT_Range &>(orig_tt_const)};

  /* convert orig_tt into offsets because the isolate_stmt call is going to
     change the fragments list. */
  auto const beg_off = std::distance(stmt->begin(), orig_tt.begin());
  auto const end_off = std::distance(stmt->begin(), orig_tt.end());

  isolate_stmt(stmt);

  /* find the updated TT_Range using the offsets */
  auto new_beg_it = std::next(stmt->ll().fragments().begin(), beg_off);
  auto new_end_it = std::next(stmt->ll().fragments().begin(), end_off);

  stmt->ll().replace_stmt_substr(TT_Range{new_beg_it, new_end_it}, new_text);
  assert(stmt->ll().has_stmts());
  assert(stmt->ll().stmts().size() == 1);
  stmt->assign_range(stmt->ll().stmts()[0]);
  stmt->drop_stmt_tree();
  stmt->unhook();
}

void Logical_File::append_stmt_text(LL_STMT_SEQ::iterator stmt,
                                    std::string const &new_text) {
  /* put the stmt on its own line */
  isolate_stmt(stmt);

  stmt->ll().insert_text_before(stmt->end(), new_text);
  assert(stmt->ll().has_stmts());
  assert(stmt->ll().stmts().size() == 1);
  stmt->assign_range(stmt->ll().stmts()[0]);
  stmt->drop_stmt_tree();
  stmt->unhook();
}

bool Logical_File::set_stmt_label(LL_STMT_SEQ::iterator stmt, int label) {
  assert(!(label < 0));
  if (!stmt->has_label()) {
    if (label == 0)
      return false;
    split_compound_before(stmt); // can't label something in a compound
  }
  bool retval = stmt->ll().set_label(label);
  stmt->cache_new_label_value(stmt->ll().label);
  return retval;
}

bool Logical_File::convert_fixed_to_free() {
  bool changed{false};
  for (Logical_Line &ll : lines) {
    if (ll.layout().empty())
      continue;
    if (!ll.layout().front().is_fixed_format())
      continue;

    using FL_VEC = typename Logical_Line::FL_VEC;

    FL_VEC &layout{ll.layout()};
    const size_t N_FL{layout.size()};

    for (size_t i = 0; i < N_FL; ++i) {
      layout[i].unset_classification(File_Line::class_flags::fixed_format);
    }

    if (layout[0].is_fortran()) {
      /* Count the number of File_Lines that contain Fortran statements.  We use
         this to determine where to put trailing continuation marks. */
      size_t total_fortran_lines{0};
      for (size_t scan = 0; scan < N_FL; ++scan) {
        if (layout[scan].is_fortran())
          total_fortran_lines += 1;
      }

      /* Scan the tokens to find multiline tokens (continued strings or tokens
         that are split across two lines */
      std::deque<size_t> needs_front_continuation;
      for (auto const &tt : ll.fragments()) {
        if (tt.is_split_token_()) {
          for (int fli = tt.mt_begin_line_ + 1; fli <= tt.mt_end_line_; ++fli) {
            needs_front_continuation.push_back(static_cast<size_t>(fli));
          }
        }
      }

      size_t curr_fortran_line{0};
      for (size_t curr_idx = 0; curr_idx < N_FL; ++curr_idx) {
        if (layout[curr_idx].is_fortran()) {
          File_Line &fl = layout[curr_idx];
          if (curr_fortran_line + 1 < total_fortran_lines) {
            /* Need to add a trailing continuation mark */
            if (fl.right_txt.empty()) {
              fl.right_txt = "&";
            } else {
              fl.right_txt.insert(0, "& ");
              if (fl.open_delim == '\0' && fl.right_space.size() > 2) {
                fl.right_space.erase(0, 2);
              }
            }
            layout[curr_idx].set_classification(
                File_Line::class_flags::continued);
          }

          /* Adjust left_txt and left_spaces */
          if (curr_fortran_line == 0) {
            if (fl.left_txt.empty()) {
              fl.left_space.insert(0, 6, ' ');
            }
          } else {
            /* Need to adjust the control column 6 continuation mark */
            assert(fl.left_txt.size() == 6);

            if (!needs_front_continuation.empty() &&
                needs_front_continuation.front() == curr_idx) {
              needs_front_continuation.pop_front();
              fl.left_txt[5] = '&';
            } else {
              layout[curr_idx].unset_classification(
                  File_Line::class_flags::continuation);
              assert(fl.left_txt[0] == ' ');
              assert(fl.left_txt[1] == ' ');
              assert(fl.left_txt[2] == ' ');
              assert(fl.left_txt[3] == ' ');
              assert(fl.left_txt[4] == ' ');
              fl.left_txt.clear();
              fl.left_space.insert(0, 6, ' ');
            }
          }
          curr_fortran_line += 1;
        } else if (layout[curr_idx].is_comment()) {
          /* The only thing that we have to change is a comment initiated by a
             character in control column one... anything else that has been
             identified as a comment line must already begin with a '!' */
          File_Line &fl = layout[curr_idx];
          assert(!fl.left_txt.empty());
          if (fl.left_txt[0] != ' ')
            fl.left_txt[0] = '!';
        }
      }
    } else {
      /* This is a non-Fortran Logical_Line, so we just have to adjust comments,
         and not worry about continuations. */
      for (size_t curr_idx = 0; curr_idx < N_FL; ++curr_idx) {
        if (layout[curr_idx].is_comment()) {
          File_Line &fl = layout[curr_idx];
          assert(!fl.left_txt.empty());
          if (fl.left_txt[0] != ' ')
            fl.left_txt[0] = '!';
        }
      }
    }

    changed = true;
  } // foreach ll
  if (changed) {
    if (file_info)
      file_info->file_type = File_Type::FREEFMT;
  }

  return changed;
}

} // namespace FLPR
