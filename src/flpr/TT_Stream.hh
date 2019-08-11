/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file TT_Stream.hh
*/

#ifndef FLPR_TT_STREAM_HH
#define FLPR_TT_STREAM_HH 1

#include "flpr/LL_TT_Range.hh"
#include "flpr/Syntax_Tags.hh"
#include <string>

namespace FLPR {

//! Represent a stream of Token_Text
class TT_Stream {
public:
  //! Represent a range of Token_Text in a stream
  struct Capture {
    using iterator = TT_Range::iterator;
    using const_iterator = TT_Range::const_iterator;

    Capture() : complete_{false} {}
    explicit Capture(iterator beg) : complete_(false), beg_(beg) {}
    void close(iterator end) {
      complete_ = true;
      end_ = end;
    }
    constexpr bool empty() const { return (!complete_ || (beg_ == end_)); }
    constexpr int size() const {
      return (complete_) ? std::distance(beg_, end_) : 0;
    }
    iterator begin() { return beg_; }
    iterator end() { return end_; }
    const_iterator begin() const { return beg_; }
    const_iterator end() const { return end_; }

    friend TT_Stream;

  private:
    bool complete_;
    iterator beg_, end_;
  };

public:
  explicit TT_Stream(LL_TT_Range &ll_tt)
      : ll_tt_range_{ll_tt}, next_tok_{ll_tt_range_.begin()} {}
  explicit TT_Stream(LL_TT_Range &&ll_tt)
      : ll_tt_range_{std::move(ll_tt)}, next_tok_{ll_tt_range_.begin()} {}

  /* ---------  Token stream query manipulation functions ---------- */

  //! Returns the current Token
  inline int curr() const;
  //! The current Token_Text
  inline Token_Text const &curr_tt() const;
  //! The curr+1 token, or Syntax_Tags::BAD if EOL
  inline int peek() const;
  //! The curr+offset token or Syntax_Tags::BAD if EOL
  inline int peek(int const offset) const;
  //! Return the last token of the line
  inline int peek_back() const;
  //! The curr+offset token_text or one with token==Syntax_Tags::BAD if EOL
  inline Token_Text const &peek_tt(int const offset = 1) const;
  //! Advance to next token
  inline void consume(int const advance = 1);
  //! Advance to the last token
  inline void consume_until_eol();
  //! Advance a number of tokens, returning a LL_TT_Range that covers them
  inline LL_TT_Range digest(int const advance = 1);
  //! Move backwards in the stream (unconsume)
  inline void put_back();
  //! seek back to before the first token
  inline void rewind();
  //! seek back to a specified token
  inline void rewind(TT_Range::iterator it);
  //! mark a point that can be returned to with reset()
  inline TT_Range::iterator mark();
  //! returns true if there are no more tokens in the line
  inline bool is_eol();
  //! returns the number of tokens that have been consumed
  inline int num_consumed() const;
  //! return the iterator to the next token
  inline TT_Range::iterator next_iterator() const { return next_tok_; }
  /* ------------ Expect a particular token in the stream ----------- */

  //! Consume tok or fail with an error message
  inline void expect_tok(int tok);
  //! Fail if not at end-of-line
  inline void expect_eol();
  //! Consume & return id or fail
  inline std::string const &expect_id();
  //! Consume & return lowercase id or fail
  inline std::string const &expect_id_low();
  //! Consume & return int or fail
  inline int expect_integer();

  /* ----------------------- Utilities --------------------------- */

  //! Advance through a paren-delimited expression with subparen expressions
  /*! Assuming peek() is TOK_PARENL, this will move curr() forward to
      the matching TOK_PARENR (ignoring any other paren expressions). */
  bool move_to_close_paren();
  //! Advance until peek() is the close-paren
  bool move_before_close_paren();
  //! Reverse through a paren-delimited expression with subparen expressions
  /*! Assuming curr() it TOK_PARENR, this will move curr() BACKWARDS
    to the matching TOK_PARENL (ignoring any other paren expressions). */
  bool move_to_open_paren();
  //! Advance through a bracket-delimited expression with bracket expressions
  bool ignore_bracket_expr();
  //! General error message formatted the same as "expect" errors
  void e_general(char const *const errmsg) const;
  //! General warning message formatted the same as "expect" errors
  void w_general(char const *const errmsg) const;
  //! Convert keyword tokens in the rest of the line (or num_toks) to IDs
  inline void unkeyword(const int num_toks = -1);

  //! Start capturing token indices with the next consumed token
  Capture capture_begin() const;
  //! Mark the next token as the non-inclusive end of the capture
  void capture_end(Capture &cap_rec) const;
  //! Convert a Capture structure to a string of concatenated tt.text strings
  void capture_text(Capture const &, std::string &) const;
  inline LL_TT_Range capture_to_range(TT_Stream::Capture const &cap);

  void debug_print(std::ostream &os) const;
  constexpr LL_TT_Range const &source() const { return ll_tt_range_; }

private:
  /* ------------  Error reporting functions ----------------------- */
  void e_expect_tok(int tok_found, int tok_expect) const;
  void e_expect_eol() const;
  void e_expect_id(int tok_found) const;

private:
  LL_TT_Range ll_tt_range_;
  TT_Range::iterator next_tok_;
};

inline TT_Stream::Capture TT_Stream::capture_begin() const {
  return Capture(next_tok_);
}

inline void TT_Stream::capture_end(TT_Stream::Capture &cap_rec) const {
  cap_rec.close(next_tok_);
}

inline void TT_Stream::capture_text(TT_Stream::Capture const &cap,
                                    std::string &output) const {
  for (auto curr = cap.beg_; curr != cap.end_; ++curr)
    output.append(curr->text());
}

inline LL_TT_Range TT_Stream::capture_to_range(TT_Stream::Capture const &cap) {
  return LL_TT_Range(ll_tt_range_.it(), cap.beg_, cap.end_);
}

inline void TT_Stream::unkeyword(const int num_toks) {
  FLPR::unkeyword(next_tok_, ll_tt_range_.end(), num_toks);
}

inline Token_Text const &TT_Stream::curr_tt() const {
  static const Token_Text bad_tt;
  if (next_tok_ != ll_tt_range_.begin())
    return *(std::prev(next_tok_));
  else
    return bad_tt;
}

// FRAGS[curr_tok_].token or Syntax_Tags::BAD
inline int TT_Stream::curr() const { return curr_tt().token; }

// FRAGS[curr_tok_+1].token or Syntax_Tags::BAD
inline Token_Text const &TT_Stream::peek_tt(int const offset) const {
  static const Token_Text bad_tt;
  // the -1 is because next_token_ is already advanced one.
  auto tmp = std::next(next_tok_, offset - 1);
  if (tmp != ll_tt_range_.end())
    return *tmp;
  else
    return bad_tt;
}

// FRAGS[curr_tok_+1].token or Syntax_Tags::BAD
inline int TT_Stream::peek() const {
  return (next_tok_ == ll_tt_range_.end()) ? Syntax_Tags::BAD
                                           : next_tok_->token;
}

// FRAGS[curr_tok_+1].token or Syntax_Tags::BAD
inline int TT_Stream::peek(int const offset) const {
  return peek_tt(offset).token;
}

// FRAGS[curr_tok_+1].token or Syntax_Tags::BAD
inline int TT_Stream::peek_back() const {
  if (ll_tt_range_.empty())
    return Syntax_Tags::BAD;
  return ll_tt_range_.back().token;
}

inline int TT_Stream::num_consumed() const {
  return static_cast<int>(
      std::distance(ll_tt_range_.begin(), TT_Range::const_iterator(next_tok_)));
}

// Note that this will consume a semicolon
inline bool TT_Stream::is_eol() {
  if (next_tok_ != ll_tt_range_.end()) {
    if (Syntax_Tags::TK_SEMICOLON == next_tok_->token) {
      next_tok_++;
      return true;
    } else
      return false;
  }
  return true;
}

inline void TT_Stream::consume(int const advance) {
  std::advance(next_tok_, advance);
}

inline void TT_Stream::consume_until_eol() { next_tok_ = ll_tt_range_.end(); }

inline LL_TT_Range TT_Stream::digest(int const advance) {
  TT_Range::iterator beg = next_tok_;
  std::advance(next_tok_, advance);
  return LL_TT_Range{ll_tt_range_.it(), beg, next_tok_};
}

// curr_ = max(-1, curr_tok_ - 1)
inline void TT_Stream::put_back() {
  if (next_tok_ != ll_tt_range_.begin())
    next_tok_--;
}

inline void TT_Stream::rewind() { next_tok_ = ll_tt_range_.begin(); }

inline void TT_Stream::rewind(TT_Range::iterator it) { next_tok_ = it; }

inline TT_Range::iterator TT_Stream::mark() { return next_tok_; }

inline void TT_Stream::expect_tok(const int tok) {
  const int next_tok = peek();
  consume();
  if (next_tok != tok) {
    e_expect_tok(next_tok, tok);
  }
}

inline void TT_Stream::expect_eol() {
  if (!is_eol())
    e_expect_eol();
}

inline int TT_Stream::expect_integer() {
  const int next_tok = peek();
  if (Syntax_Tags::SG_INT_LITERAL_CONSTANT != next_tok) {
    e_expect_tok(next_tok, Syntax_Tags::SG_INT_LITERAL_CONSTANT);
  }
  consume();
  return std::stoi(curr_tt().text());
}

inline std::string const &TT_Stream::expect_id() {
  const int next_tok = peek();
  if (Syntax_Tags::TK_NAME != next_tok) {
    e_expect_id(next_tok);
  }
  consume();
  return curr_tt().text();
}

inline std::string const &TT_Stream::expect_id_low() {
  const int next_tok = peek();
  if (Syntax_Tags::TK_NAME != next_tok) {
    e_expect_id(next_tok);
  }
  consume();
  return curr_tt().lower();
}
} // namespace FLPR
#endif
