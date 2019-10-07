/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#ifndef FLPR_FORMAT_BASE_HH
#define FLPR_FORMAT_BASE_HH 1

#include "Timer.hh"
#include <array>
#include <flpr/Indent_Table.hh>
#include <flpr/flpr.hh>
#include <ostream>

/* Manage the transformation options */
struct Options {
  enum Filter_Tags : size_t {
    ELABORATE_END_STMTS,
    FIXED_TO_FREE,
    REINDENT,
    REMOVE_EMPTY_STMTS,
    SPLIT_COMPOUND_STMTS,
    NUM_FILTERS /* must be last */
  };

  Options() noexcept
      : write_inplace_{false}, verbose_{false}, do_timing_{false}, quiet_{
                                                                       false} {
    disable_all_filters();
  }
  void disable_all_filters() noexcept { filters_.fill(false); }
  void enable_all_filters() noexcept { filters_.fill(true); }
  constexpr bool &operator[](Filter_Tags const tag) noexcept {
    return filters_[tag];
  }
  constexpr bool operator[](Filter_Tags const tag) const noexcept {
    return filters_[tag];
  }
  constexpr void set_write_inplace(bool const val) noexcept {
    write_inplace_ = val;
  }
  constexpr bool write_inplace() const noexcept { return write_inplace_; }
  constexpr void set_verbose(bool const val) noexcept { verbose_ = val; }
  constexpr bool verbose() const noexcept { return verbose_; }
  constexpr void set_do_timing(bool const val) noexcept { do_timing_ = val; }
  constexpr bool do_timing() const noexcept { return do_timing_; }
  constexpr void set_quiet(bool const val) noexcept { quiet_ = val; }
  constexpr bool quiet() const noexcept { return quiet_; }
  constexpr void set_do_output(bool const val) noexcept { do_output_ = val; }
  constexpr bool do_output() const noexcept { return do_output_; }

private:
  bool write_inplace_;
  bool verbose_;
  bool do_timing_;
  bool quiet_;
  bool do_output_;
  std::array<bool, NUM_FILTERS> filters_;
};

/* This class contains and manages the Logical_Lines, LL_Stmts, Parse_Tree
   related to one input file */
using File = FLPR::Parsed_File<>;

bool parse_cmd_line(std::vector<std::string> &filenames, Options &options,
                    int argc, char *const argv[]);
int flpr_format_file(File &file, Options const &options,
                     FLPR::Indent_Table const &indents);
void write_file(std::ostream &os, File const &file);

#define OPT(T) Options::T
/* -------------------------------------------------------------------------- */

#define VERBOSE_BEGIN(OP)                                                      \
  {                                                                            \
    Timer t;                                                                   \
    if (options.verbose()) {                                                   \
      std::cerr << "Performing " << OP << "... " << std::flush;                \
      if (options.do_timing())                                                 \
        t.start();                                                             \
    }
#define VERBOSE_END                                                            \
  if (options.verbose()) {                                                     \
    if (options.do_timing()) {                                                 \
      t.stop();                                                                \
      std::cerr << "done (" << t << ")." << std::endl;                         \
    } else {                                                                   \
      std::cerr << "done." << std::endl;                                       \
    }                                                                          \
  }                                                                            \
  }

#endif
