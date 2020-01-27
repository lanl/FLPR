/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Stmt_Parsers.hh
*/
#ifndef FLPR_STMT_PARSERS_HH
#define FLPR_STMT_PARSERS_HH 1

#include "flpr/Parser_Result.hh"
#include "flpr/Stmt_Tree.hh"
#include "flpr/TT_Stream.hh"
#include "flpr/utils.hh"
#include <tuple>

#define TRACE_TOKEN_PARSER 0

#if TRACE_TOKEN_PARSER
#include <iostream>
#endif

namespace FLPR {
namespace Stmt {

//! The result from each parser in Stmt
using SP_Result = FLPR::details_::Parser_Result<Stmt_Tree>;

//! Return a  Stmt::Stmt_Tree with syntag if any of the alternatives match
/*! This "covers" the result of a matching parser with a new syntag node */
template <typename... Ps> class Alternatives_Parser {
public:
  Alternatives_Parser(Alternatives_Parser const &) = default;
  constexpr explicit Alternatives_Parser(int const syntag, Ps &&... ps) noexcept
      : syntag_{syntag}, parsers_{std::forward<Ps>(ps)...} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    Stmt_Tree root;
    auto assign_if = [&ts, &root ](auto const &p) constexpr {
      auto [st, match] = p(ts);
      if (match) {
        root = std::move(st);
      }
      return match;
    };
    auto fold = [&assign_if](auto... as) constexpr {
      return FLPR::details_::or_fold(assign_if, as...);
    };
    bool const match = std::apply(fold, parsers_);
    if (match) {
      Stmt_Tree new_root{this->syntag_};
      hoist_back(new_root, std::move(root));
      cover_branches(*new_root);
      return SP_Result{std::move(new_root), true};
    }
    return SP_Result{std::move(root), false};
  }

private:
  int const syntag_;
  std::tuple<Ps...> const parsers_;
};

//! Generate an Alternatives_Parser
template <typename... Ps>
inline constexpr Alternatives_Parser<Ps...> alts(int const syntag,
                                                 Ps &&... ps) noexcept {
  return Alternatives_Parser<Ps...>{syntag, std::forward<Ps>(ps)...};
}

//! Generate an hoisted Alternatives_Parser
template <typename... Ps>
inline constexpr Alternatives_Parser<Ps...> h_alts(Ps &&... ps) noexcept {
  return Alternatives_Parser<Ps...>{Syntax_Tags::HOIST,
                                    std::forward<Ps>(ps)...};
}

//! Match a TK_NAME that is one character long
class Letter_Parser {
public:
  SP_Result operator()(TT_Stream &ts) const noexcept {
    if (!Syntax_Tags::is_name(ts.peek()))
      return SP_Result{Stmt_Tree{}, false};
    if (ts.peek_tt().text().size() != 1)
      return SP_Result{Stmt_Tree{}, false};
    return SP_Result{Stmt_Tree{Syntax_Tags::TK_NAME, ts.digest(1)}, true};
  }
};

//! Generate a Letter_Parser
inline constexpr Letter_Parser letter() noexcept { return Letter_Parser{}; }

//! Match a TK_NAME with a particular string value
class Literal_Parser {
public:
  Literal_Parser(Literal_Parser const &) = default;
  Literal_Parser(char const *const s) : lc_text_{s} { tolower(lc_text_); }
  SP_Result operator()(TT_Stream &ts) const noexcept {
    if (!Syntax_Tags::is_name(ts.peek()))
      return SP_Result{Stmt_Tree{}, false};
    if (ts.peek_tt().lower() != lc_text_)
      return SP_Result{Stmt_Tree{}, false};
    return SP_Result{Stmt_Tree{Syntax_Tags::TK_NAME, ts.digest(1)}, true};
  }

private:
  std::string lc_text_;
};

//! Generate a Literal_Parser
inline auto literal(char const *const s) { return Literal_Parser{s}; }

//! Match a Fortran \<name\>, converting keywords if necessary
class Name_Parser {
public:
  SP_Result operator()(TT_Stream &ts) const noexcept {
#if TRACE_TOKEN_PARSER
    std::cerr << "TP: match <name> vs. ";
    Syntax_Tags::print(std::cerr, ts.peek()) << "?\n";
#endif
    if (!Syntax_Tags::is_name(ts.peek()))
      return SP_Result{Stmt_Tree{}, false};
    // If you unkeyword the stream here, it won't be accurate for
    // any stream rewinds.
    return SP_Result{Stmt_Tree{Syntax_Tags::TK_NAME, ts.digest(1)}, true};
  }
};

//! Generate a Name_Parser
inline constexpr auto name() { return Name_Parser{}; }

//! Flips the match of the given parser
template <typename P> class Negated_Parser {
public:
  Negated_Parser(Negated_Parser const &) = default;
  constexpr explicit Negated_Parser(P &&p) noexcept
      : parser_{std::forward<P>(p)} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    auto [st, match] = parser_(ts);
    return SP_Result{std::move(st), !match};
  }

private:
  P const parser_;
};

//! Generate an Negated_Parser
template <typename P> inline constexpr Negated_Parser<P> neg(P &&p) noexcept {
  return Negated_Parser<P>{std::forward<P>(p)};
}

//! Forwards the Stmt_Tree of a parser, always setting match to true
template <typename P> class Optional_Parser {
public:
  Optional_Parser(Optional_Parser const &) = default;
  constexpr explicit Optional_Parser(P &&p) noexcept
      : parser_{std::forward<P>(p)} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    auto [st, match] = parser_(ts);
    return SP_Result{std::move(st), true};
  }

private:
  P const parser_;
};

//! Generate an Optional_Parser
template <typename P> inline constexpr Optional_Parser<P> opt(P &&p) noexcept {
  return Optional_Parser<P>{std::forward<P>(p)};
}

//! Returns true if the next token in the stream matches (not consumed)
class Peek_Parser {
public:
  Peek_Parser(Peek_Parser const &) = default;
  constexpr explicit Peek_Parser(int const token_tag) noexcept
      : token_tag_{token_tag} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    return SP_Result{Stmt_Tree{}, token_tag_ == ts.peek()};
  }

private:
  int const token_tag_;
};

//! Generate a Peek_Parser
inline constexpr Peek_Parser peek(int const token_tag) noexcept {
  return Peek_Parser{token_tag};
}

//! Match if the TT_Stream is exhausted
/*! This is used to ensure that there is nothing dangling after a complete
 * Fortran statement has been matched */
inline constexpr Peek_Parser eol() noexcept {
  return Peek_Parser{Syntax_Tags::BAD};
}

//! Return the Stmt_Tree generated by a function, match means tree is good
class Rule_Parser {
public:
  using parser_function = Stmt_Tree (*)(TT_Stream &ts);
  Rule_Parser(Rule_Parser const &) = default;
  constexpr explicit Rule_Parser(parser_function f) noexcept : f_{f} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    auto ts_rewind_point = ts.mark();
    Stmt_Tree st = f_(ts);
    if (!st)
      ts.rewind(ts_rewind_point);
    return SP_Result{std::move(st), static_cast<bool>(st)};
  }

private:
  parser_function const f_;
};

//! Generate a Rule_Parser
inline constexpr auto rule(Rule_Parser::parser_function f) {
  return Rule_Parser{f};
};

//! Match if all specified parsers match
/*! On match, this returns a Stmt_Tree with a root \c syntag that covers all the
 * Stmt_Trees from the sub parsers */
template <typename... Ps> class Sequence_Parser {
public:
  Sequence_Parser(Sequence_Parser const &) = default;
  constexpr explicit Sequence_Parser(int const syntag, Ps &&... ps) noexcept
      : syntag_{syntag}, parsers_{std::forward<Ps>(ps)...} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    auto ts_rewind_point = ts.mark();
    Stmt_Tree root(true);
    auto attach_if = [&ts, &root ](auto const &p) constexpr {
      auto [st, match] = p(ts);
      if (st) {
        hoist_back(root, std::move(st));
      }
      return match;
    };
    auto fold = [&attach_if](auto... as) constexpr {
      return FLPR::details_::and_fold(attach_if, as...);
    };
    bool const match = std::apply(fold, parsers_);
    if (!match) {
      ts.rewind(ts_rewind_point);
      root.clear();
    } else {
      (*root)->syntag = syntag_;
      cover_branches(*root);
    }
    return SP_Result{std::move(root), match};
  }

private:
  int const syntag_;
  std::tuple<Ps...> const parsers_;
};

//! Generate a Sequence_Parser
template <typename... Ps>
inline constexpr Sequence_Parser<Ps...> seq(int const syntag,
                                            Ps &&... ps) noexcept {
  return Sequence_Parser<Ps...>{syntag, std::forward<Ps>(ps)...};
}

//! Generate a special Sequence_Parser with syntag \c HOIST
/*! \c HOIST basically means that the branches represent a list that gets
    "hoisted" into the branches of a new root, rather than grafted as a tree */
template <typename... Ps>
inline constexpr Sequence_Parser<Ps...> h_seq(Ps &&... ps) noexcept {
  return Sequence_Parser<Ps...>{Syntax_Tags::HOIST, std::forward<Ps>(ps)...};
}

//! Kleene star: Zero or more matches of the parser
template <typename P> class Star_Parser {
public:
  Star_Parser(Star_Parser const &) = default;
  constexpr explicit Star_Parser(P &&p) noexcept
      : parser_{std::forward<P>(p)} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    Stmt_Tree root(Syntax_Tags::HOIST);
    auto attach_if = [&ts, &root ](auto const &p) constexpr {
      auto [st, match] = p(ts);
      if (st) {
        hoist_back(root, std::move(st));
      }
      return match;
    };
    while (attach_if(parser_))
      ;
    cover_branches(*root);
    // The list root may have no children, but this always matches
    return SP_Result{std::move(root), true};
  }

private:
  P const parser_;
};

//! Generate a Star_Parser
template <typename P> inline constexpr Star_Parser<P> star(P &&p) noexcept {
  return Star_Parser<P>{std::forward<P>(p)};
}

//! "Tag" a Stmt_Tree with a new root
/*! If \c parser_ matches, generate a new Stmt_Tree with the root given the
  specified \c syntag and the parser_ result Stmt_Tree as the one branch. This
  is used to give a different context for the result tree.  For example, a \c
  name can be tagged with \c dummy-arg to indicate that it is a dummy argument,
  not just some arbitrary \c name. */
template <typename P> class Tag_Parser {
public:
  Tag_Parser(Tag_Parser const &) = default;
  constexpr explicit Tag_Parser(int const syntag, P &&p) noexcept
      : syntag_{syntag}, parser_{std::forward<P>(p)} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    SP_Result res = parser_(ts);
    if (res.parse_tree) {
      Stmt_Tree new_root{syntag_, (*(res.parse_tree))->token_range};
      hoist_back(new_root, std::move(res.parse_tree));
      cover_branches(*new_root);
      return SP_Result{std::move(new_root), res.match};
    }
    return res;
  }

private:
  int const syntag_;
  P const parser_;
};

//! Generate a Tag_Parser
template <typename P>
inline constexpr Tag_Parser<P> tag_if(int const syntag, P &&p) noexcept {
  return Tag_Parser<P>{syntag, std::forward<P>(p)};
}

//! Match a token with the given Syntax_Tags::Tags value
class Token_Parser {
public:
  Token_Parser(Token_Parser const &) = default;
  constexpr explicit Token_Parser(int const token_tag) noexcept
      : token_tag_{token_tag} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
#if TRACE_TOKEN_PARSER
    Syntax_Tags::print(std::cerr << "TP: match ", token_tag_) << " vs. ";
    Syntax_Tags::print(std::cerr, ts.peek()) << "?\n";
#endif
    if (token_tag_ != ts.peek())
      return SP_Result{Stmt_Tree{}, false};
    return SP_Result{Stmt_Tree{token_tag_, ts.digest(1)}, true};
  }

private:
  int const token_tag_;
};

//! Generate a Token_Parser
inline constexpr auto tok(int const token_tag) {
  return Token_Parser{token_tag};
}

/*************************** DERIVED PARSERS **********************************/

//! R401: xyz-list (4.1.3)
/*! This is a parser for the assumed syntax rule "xyz (, xyz)*" */
template <typename P> class List_Parser {
public:
  List_Parser(List_Parser const &) = default;
  constexpr explicit List_Parser(int const syntag, P &&p) noexcept
      : syntag_{syntag}, parser_{std::forward<P>(p)} {}
  SP_Result operator()(TT_Stream &ts) const noexcept {
    auto p =
        seq(syntag_, parser_,
            star(seq(Syntax_Tags::HOIST, tok(Syntax_Tags::TK_COMMA), parser_)));
    // The cover_branches was provided by seq
    return p(ts);
  }

private:
  int const syntag_;
  P const parser_;
};

//! Generate a List_Parser
template <typename P>
inline constexpr List_Parser<P> list(int const syntag, P &&p) noexcept {
  return List_Parser<P>{syntag, std::forward<P>(p)};
}

template <typename P> inline constexpr List_Parser<P> h_list(P &&p) noexcept {
  return list(Syntax_Tags::HOIST, std::forward<P>(p));
}

template <typename... Ps>
inline constexpr auto parens(int const syntag, Ps &&... ps) noexcept {
  return seq(syntag, tok(Syntax_Tags::TK_PARENL), std::forward<Ps>(ps)...,
             tok(Syntax_Tags::TK_PARENR));
}

template <typename... Ps> inline constexpr auto h_parens(Ps &&... ps) noexcept {
  return seq(Syntax_Tags::HOIST, tok(Syntax_Tags::TK_PARENL),
             std::forward<Ps>(ps)..., tok(Syntax_Tags::TK_PARENR));
}

template <typename... Ps>
inline constexpr auto brackets(int const syntag, Ps &&... ps) noexcept {
  return seq(syntag, tok(Syntax_Tags::TK_BRACKETL), std::forward<Ps>(ps)...,
             tok(Syntax_Tags::TK_BRACKETR));
}

template <typename... Ps>
inline constexpr auto h_brackets(Ps &&... ps) noexcept {
  return seq(Syntax_Tags::HOIST, tok(Syntax_Tags::TK_BRACKETL),
             std::forward<Ps>(ps)..., tok(Syntax_Tags::TK_BRACKETR));
}

} // namespace Stmt
} // namespace FLPR
#undef TRACE_TOKEN_PARSER
#endif
