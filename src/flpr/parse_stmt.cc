/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file parse_stmt.cc
*/

#include "flpr/parse_stmt.hh"
#include "flpr/Stmt_Parser_Exts.hh"
#include "flpr/Stmt_Parsers.hh"

#define TRACE_SG 0

#include <iostream>

namespace FLPR {
namespace Stmt {

#define FAIL                                                                   \
  {                                                                            \
    ts.rewind(ts_rewind_mark);                                                 \
    return Stmt_Tree{};                                                        \
  }

#define INIT_FAIL auto ts_rewind_mark = ts.mark()

#if TRACE_SG
#define RULE(T)                                                                \
  constexpr auto rule_tag{Syntax_Tags::T};                                     \
  Syntax_Tags::print(std::cerr << "SGTRACE >  ", Syntax_Tags::T) << '\n'

#define EVAL(T, E)                                                             \
  Stmt_Tree res_ = E;                                                          \
  if (!res_.tree_initialized())                                                \
    Syntax_Tags::print(std::cerr << "SGTRACE <! ", Syntax_Tags::T) << '\n';    \
  else                                                                         \
    std::cerr << "SGTRACE <= " << res_ << '\n';                                \
  return res_;
#else
#define RULE(T)                                                                \
  constexpr auto rule_tag { Syntax_Tags::T }
#define EVAL(T, E) return E
#endif

#define TAG(X) Syntax_Tags::X
#define TOK(X) tok(Syntax_Tags::X)

//! Subparser to consume until the end of an expression
Stmt_Tree consume_until_break(TT_Stream &ts, int const rule_tag) {
  // FIX: may need a pattern more like consume_parens
  INIT_FAIL;
  int nesting_depth = 1;
  auto cap = ts.capture_begin();
  while (nesting_depth > 0) {
    if (Syntax_Tags::TK_PARENL == ts.peek()) {
      nesting_depth += 1;
    } else if (Syntax_Tags::TK_BRACKETL == ts.peek()) {
      nesting_depth += 1;
    } else if (Syntax_Tags::TK_PARENR == ts.peek()) {
      nesting_depth -= 1;
      if (!nesting_depth)
        break;
    } else if (Syntax_Tags::TK_BRACKETR == ts.peek()) {
      nesting_depth -= 1;
      if (!nesting_depth)
        break;
    } else if (nesting_depth == 1) {
      // Other things you can run into that terminate an expression
      if (Syntax_Tags::BAD == ts.peek())
        break;
      if (Syntax_Tags::TK_COMMA == ts.peek())
        break;
      if (Syntax_Tags::TK_COLON == ts.peek())
        break;
      if (Syntax_Tags::TK_EQUAL == ts.peek()) {
        FAIL;
        break;
      }
    }
    ts.consume();
  }
  ts.capture_end(cap);
  if (cap.empty())
    return Stmt_Tree{};
  return Stmt_Tree{rule_tag, ts.capture_to_range(cap)};
}

//! helper function to consume until the matching PARENR
Stmt_Tree consume_parens(TT_Stream &ts) {
  int nesting_depth = 1;
  if (Syntax_Tags::TK_PARENL != ts.peek())
    return Stmt_Tree{};

  Stmt_Tree root{Syntax_Tags::HOIST};
  root.graft_back(Stmt_Tree{ts.peek(), ts.digest(1)});
  while (nesting_depth > 0) {
    if (Syntax_Tags::TK_PARENL == ts.peek()) {
      nesting_depth += 1;
    } else if (Syntax_Tags::TK_PARENR == ts.peek()) {
      nesting_depth -= 1;
    } else {
      assert(Syntax_Tags::BAD != ts.peek());
    }
    root.graft_back(Stmt_Tree{ts.peek(), ts.digest(1)});
  }
  return root;
}

//! helper function for identifying names in lists
/*! The name() parser is too aggressive when it is used in a list of alts, as it
  will gobble the keyword at the begining of other rules.  This helper only
  matches "<name>[',',')','=>',<eol>]"
*/
Stmt_Tree list_name(TT_Stream &ts) {
  if (Syntax_Tags::is_name(ts.peek(1))) {
    int const next = ts.peek(2);
    if (TAG(BAD) == next || TAG(TK_ARROW) == next || TAG(TK_COLON) == next ||
        TAG(TK_COMMA) == next || TAG(TK_PARENR) == next) {
      return Stmt_Tree{Syntax_Tags::TK_NAME, ts.digest(1)};
    }
  }
  return Stmt_Tree{};
}

//! helper: bind_c (local)
Stmt_Tree bind_c(TT_Stream &ts) {
  auto p = h_seq(TOK(KW_BIND), TOK(TK_PARENL), literal("c"), TOK(TK_PARENR));
  return p(ts);
}

/**********************************************************************/

// clang-format off

/* I:A */

//! R828: access-id (8.6.1)
Stmt_Tree access_id(TT_Stream &ts) {
  RULE(SG_ACCESS_ID);
  constexpr auto p =
    alts(rule_tag,
         rule(generic_spec),
         name()   // access-name
         );
  EVAL(SG_ACCESS_ID, p(ts));
}

//! R807: access-spec (8.5.2)
Stmt_Tree access_spec(TT_Stream &ts) {
  RULE(SG_ACCESS_SPEC);
  constexpr auto p = alts(rule_tag, TOK(KW_PUBLIC), TOK(KW_PRIVATE));
  EVAL(SG_ACCESS_SPEC, p(ts));
}

//! R827: access-stmt (8.6.1)
Stmt_Tree access_stmt(TT_Stream &ts) {
  RULE(SG_ACCESS_STMT);
  constexpr auto p =
    seq(rule_tag,
        rule(access_spec),
        opt(h_seq(opt(TOK(TK_DBL_COLON)),
                  list(TAG(SG_ACCESS_ID_LIST),
                       rule(access_id)))),
        eol());
  EVAL(SG_ACCESS_STMT, p(ts));
}

//! R515: action-stmt (5.1)
/*! While a little meta compared to other statement parsers, this rule needs to
    be available to statement rules like if-stmt.  Please keep is_action_stmt in
    sync with this. */
Stmt_Tree action_stmt(TT_Stream &ts) {
  RULE(SG_ACTION_STMT);
  constexpr auto p =
    alts(rule_tag,
         rule(allocate_stmt),
         rule(assignment_stmt),
         rule(backspace_stmt),
         rule(call_stmt),
         rule(close_stmt),
         rule(continue_stmt),
         rule(cycle_stmt),
         rule(deallocate_stmt),
         rule(endfile_stmt),
         rule(error_stop_stmt),
         rule(event_post_stmt),
         rule(event_wait_stmt),
         rule(exit_stmt),
         rule(fail_image_stmt),
         rule(flush_stmt),
         rule(form_team_stmt),
         rule(goto_stmt),
         rule(if_stmt),
         rule(inquire_stmt),
         rule(lock_stmt),
         rule(nullify_stmt),
         rule(open_stmt),
         rule(pointer_assignment_stmt),
         rule(print_stmt),
         rule(read_stmt),
         rule(return_stmt),
         rule(rewind_stmt),
         rule(stop_stmt),
         rule(sync_all_stmt),
         rule(sync_images_stmt),
         rule(sync_memory_stmt),
         rule(sync_team_stmt),
         rule(unlock_stmt),
         rule(wait_stmt),
         rule(where_stmt),
         rule(write_stmt),
         rule(computed_goto_stmt),
         rule(arithmetic_if_stmt),
         rule(forall_stmt),
         rule(macro_stmt));
  auto res = p(ts);
  if(!res.match) {
    res = get_parser_exts().parse_action_stmt(ts);
  }
  EVAL(SG_ACTION_STMT, res);
}

//! R1524: actual-arg (15.5.1)
Stmt_Tree actual_arg(TT_Stream &ts) {
  RULE(SG_ACTUAL_ARG);
  /* procedure-name and proc-component-ref reduce to data-ref in variable */
  constexpr auto p =
    seq(rule_tag,
        rule(expr)
        );
  EVAL(SG_ACTUAL_ARG, p(ts));
}

//! R1523: actual-arg-spec (15.5.1)
Stmt_Tree actual_arg_spec(TT_Stream &ts) {
  RULE(SG_ACTUAL_ARG_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_EQUAL))),
        rule(actual_arg));
  EVAL(SG_ACTUAL_ARG_SPEC, p(ts));
}

//! R1009: add-op (6.2.4)
Stmt_Tree add_op(TT_Stream &ts) {
  RULE(SG_ADD_OP);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_PLUS),
         TOK(TK_MINUS));
  EVAL(SG_ADD_OP, p(ts));
}

//! R928: alloc-opt (9.7.1.1)
Stmt_Tree alloc_opt(TT_Stream &ts) {
  RULE(SG_ALLOC_OPT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_ERRMSG), TOK(TK_EQUAL), rule(variable)),
         h_seq(TOK(KW_MOLD), TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(KW_SOURCE), TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(KW_STAT), TOK(TK_EQUAL), rule(variable)));
  EVAL(SG_ALLOC_OPT, p(ts));
}

//! R829: allocatable-stmt (8.6.2)
Stmt_Tree allocatable_stmt(TT_Stream &ts) {
  RULE(SG_ALLOCATABLE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ALLOCATABLE),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_ALLOCATABLE_DECL_LIST),
             rule(allocatable_decl)),
        eol());
  EVAL(SG_ALLOCATABLE_STMT, p(ts));
}

//! R830: allocatable-decl (8.6.2)
Stmt_Tree allocatable_decl(TT_Stream &ts) {
  RULE(SG_ALLOCATABLE_DECL);
  constexpr auto p =
    seq(rule_tag, name(),
        opt(h_parens(rule(array_spec))),
        opt(h_brackets(rule(coarray_spec))));
  EVAL(SG_ALLOCATABLE_DECL, p(ts));
}

//! R936: allocate-coarray-spec (9.7.1.1)
Stmt_Tree allocate_coarray_spec(TT_Stream &ts) {
  RULE(SG_ALLOCATE_COARRAY_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(list(TAG(SG_ALLOCATE_COSHAPE_SPEC_LIST),
                       rule(allocate_coshape_spec)),
                  TOK(TK_COMMA))),
        opt(h_seq(rule(lower_bound_expr),
                  TOK(TK_COLON))),
        TOK(TK_ASTERISK));
  EVAL(SG_ALLOCATE_COARRAY_SPEC, p(ts));
}

//! R937: allocate-coshape-spec (9.7.1.1)
Stmt_Tree allocate_coshape_spec(TT_Stream &ts) {
  RULE(SG_ALLOCATE_COSHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(rule(lower_bound_expr), TOK(TK_COLON))),
        rule(upper_bound_expr));
  EVAL(SG_ALLOCATE_COSHAPE_SPEC, p(ts));
}

//! R932: allocate-object (9.7.1.1)
Stmt_Tree allocate_object(TT_Stream &ts) {
  RULE(SG_ALLOCATE_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         rule(structure_component),  // check the longer form first
         h_seq(name(), neg(peek(TAG(TK_EQUAL)))) // don't eat options
         );
  EVAL(SG_ALLOCATE_OBJECT, p(ts));
}

//! R933: allocate-shape-spec (9.7.1.1)
Stmt_Tree allocate_shape_spec(TT_Stream &ts) {
  RULE(SG_ALLOCATE_SHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(rule(lower_bound_expr), TOK(TK_COLON))),
        rule(upper_bound_expr));
  EVAL(SG_ALLOCATE_SHAPE_SPEC, p(ts));
}

//! R927: allocate-stmt (9.7.1.1)
Stmt_Tree allocate_stmt(TT_Stream &ts) {
  RULE(SG_ALLOCATE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ALLOCATE),
        h_parens(h_seq(opt(h_seq(rule(type_spec), TOK(TK_DBL_COLON))),
                       list(TAG(SG_ALLOCATION_LIST),
                            rule(allocation)),
                       opt(h_seq(TOK(TK_COMMA),
                                 list(TAG(SG_ALLOC_OPT_LIST),
                                      rule(alloc_opt)))))),
        eol());
  EVAL(SG_ALLOCATE_STMT, p(ts));
}

//! R931: allocation (9.7.1.1)
Stmt_Tree allocation(TT_Stream &ts) {
  RULE(SG_ALLOCATION);
  constexpr auto p =
    seq(rule_tag,
        rule(allocate_object), neg(peek(TAG(TK_EQUAL))),
        opt(h_parens(list(TAG(SG_ALLOCATE_SHAPE_SPEC_LIST),
                          rule(allocate_shape_spec)))),
        opt(h_brackets(rule(allocate_coarray_spec)))
        );
  EVAL(SG_ALLOCATION, p(ts));
}

//! DELETED FEATURE: arithmetic-if-stmt (B.2)
Stmt_Tree arithmetic_if_stmt(TT_Stream &ts) {
  RULE(SG_ARITHMETIC_IF_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_IF),
        h_parens(rule(expr)),
        rule(label), TOK(TK_COMMA),
        rule(label), TOK(TK_COMMA),
        rule(label),
        eol());
  EVAL(SG_ARITHMETIC_IF_STMT, p(ts));
}

//! This is to remove ambiguity between different shape/size specs
Stmt_Tree array_spec_helper(TT_Stream &ts) {
  constexpr auto p =
    h_seq(opt(rule(expr)),
          opt(TOK(TK_COLON)),
          opt(h_alts(rule(expr),
                     TOK(TK_ASTERISK))));
  return p(ts);
}

//! R917: array-element (9.5.3.1)
Stmt_Tree array_element(TT_Stream &ts) {
  RULE(SG_ARRAY_ELEMENT);
  /* R917: array-element is data-ref */
  /* C924: every part-ref shall have rank zero and the last part-ref shall
     contain a subscript-list */
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(),
                  opt(h_parens(list(TAG(SG_SECTION_SUBSCRIPT_LIST),
                                    rule(expr)))),
                  opt(rule(image_selector)),
                  peek(TAG(TK_PERCENT)),
                  star(h_seq(TOK(TK_PERCENT),
                             name(),
                             opt(h_parens(list(TAG(SG_SECTION_SUBSCRIPT_LIST),
                                               rule(expr)))),
                             opt(rule(image_selector)),
                             peek(TAG(TK_PERCENT)))),
                  TOK(TK_PERCENT))),
        name(),
        h_parens(list(TAG(SG_SECTION_SUBSCRIPT_LIST), rule(expr))),
        opt(rule(image_selector))
        );
  EVAL(SG_ARRAY_ELEMENT, p(ts));
}

//! TRUNCATED R807: array-spec (8.5.8)
Stmt_Tree array_spec(TT_Stream &ts) {
  RULE(SG_ARRAY_SPEC);
  constexpr auto p =
    alts(rule_tag,
         /*
           These rules require a lot of checking to ensure they fully execute
         list(TAG(SG_EXPLICIT_SHAPE_SPEC_LIST), rule(explicit_shape_spec)),
         list(TAG(SG_ASSUMED_SHAPE_SPEC_LIST), rule(assumed_shape_spec)),
         // Syntactically, this will be picked up by the above
         // list(TAG(SG_DEFERRED_SHAPE_SPEC_LIST), rule(deferred_shape_spec)),
         rule(assumed_size_spec),
         rule(implied_shape_spec),
         tag_if(TAG(SG_IMPLIED_SHAPE_OR_ASSUMED_SIZE_SPEC),
         rule(assumed_implied_spec)),
         */
         list(TAG(SG_ARRAY_SPEC_LIST), rule(array_spec_helper)),
         tag_if(TAG(SG_ASSUMED_RANK_SPEC), TOK(TK_DBL_DOT))
         );
  EVAL(SG_ARRAY_SPEC, p(ts));
}

//! R1032: assignment-stmt (10.2.1.1)
Stmt_Tree assignment_stmt(TT_Stream &ts) {
  RULE(SG_ASSIGNMENT_STMT);
  constexpr auto p =
    seq(rule_tag, rule(variable), TOK(TK_EQUAL), rule(expr), eol());
  EVAL(SG_ASSIGNMENT_STMT, p(ts));
}

//! R1103: associate-stmt (11.1.3.1)
Stmt_Tree associate_stmt(TT_Stream &ts) {
  RULE(SG_ASSOCIATE_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_ASSOCIATE),
        h_parens(list(TAG(SG_ASSOCIATION_LIST),
                      rule(association))),
        eol());
  EVAL(SG_ASSOCIATE_STMT, p(ts));
}

//! R1104: association (11.1.3.1)
Stmt_Tree association(TT_Stream &ts) {
  RULE(SG_ASSOCIATION);
  constexpr auto p =
    seq(rule_tag,
        name(),  // associate-name
        TOK(TK_ARROW),
        rule(selector));
  EVAL(SG_ASSOCIATION, p(ts));
}

//! R821: assumed-implied-spec (8.5.8.5)
Stmt_Tree assumed_implied_spec(TT_Stream &ts) {
  RULE(SG_ASSUMED_IMPLIED_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(rule(expr), TOK(TK_COLON))),
        TOK(TK_ASTERISK));
  EVAL(SG_ASSUMED_IMPLIED_SPEC, p(ts));
}

//! R819: assumed-shape-spec (8.5.8.3)
Stmt_Tree assumed_shape_spec(TT_Stream &ts) {
  RULE(SG_ASSUMED_SHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(rule(expr)),
        TOK(TK_COLON),
        h_alts(peek(TAG(TK_PARENR)),
               peek(TAG(TK_COMMA)))
                    
        );
  EVAL(SG_ASSUMED_SHAPE_SPEC, p(ts));
}

//! R822: assumed-size-spec (8.5.8.5)
Stmt_Tree assumed_size_spec(TT_Stream &ts) {
  RULE(SG_ASSUMED_SIZE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        list(TAG(SG_EXPLICIT_SHAPE_SPEC_LIST), rule(explicit_shape_spec)),
        TOK(TK_COMMA),
        rule(assumed_shape_spec));
  EVAL(SG_ASSUMED_SIZE_SPEC, p(ts));
}

//! R831: asynchronous-stmt (8.6.3)
Stmt_Tree asynchronous_stmt(TT_Stream &ts) {
  RULE(SG_ASYNCHRONOUS_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ASYNCHRONOUS),
        opt(TOK(TK_DBL_COLON)),
        h_list(name()), // object-name-list
        eol());
  EVAL(SG_ASYNCHRONOUS_STMT, p(ts));
}

//! R802: attr-spec (8.2)
Stmt_Tree attr_spec(TT_Stream &ts) {
  RULE(SG_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(access_spec),
         TOK(KW_ALLOCATABLE),
         TOK(KW_ASYNCHRONOUS),
         h_seq(TOK(KW_CODIMENSION), h_brackets(rule(coarray_spec))),
         TOK(KW_CONTIGUOUS),
         h_seq(TOK(KW_DIMENSION), h_parens(rule(array_spec))),
         TOK(KW_EXTERNAL),
         h_seq(TOK(KW_INTENT), h_parens(rule(intent_spec))),
         TOK(KW_INTRINSIC),
         rule(language_binding_spec),
         TOK(KW_OPTIONAL),
         TOK(KW_PARAMETER),
         TOK(KW_POINTER),
         TOK(KW_PROTECTED),
         TOK(KW_SAVE),
         TOK(KW_TARGET),
         TOK(KW_VALUE),
         TOK(KW_VOLATILE));
  EVAL(SG_ATTR_SPEC, p(ts));
}

/* I:B */

//! TRUNCATED R1224: backspace-stmt (12.8.1)
Stmt_Tree backspace_stmt(TT_Stream &ts) {
  RULE(SG_BACKSPACE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_BACKSPACE),
        h_alts(rule(int_expr),
               rule(consume_parens)),
        eol());
  EVAL(SG_BACKSPACE_STMT, p(ts));
}

//! R833: bind-entity (8.6.4)
Stmt_Tree bind_entity(TT_Stream &ts) {
  RULE(SG_BIND_ENTITY);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(TK_SLASHF),
               name(),             // common-block-name
               TOK(TK_SLASHF)),
         name());                  // entity-name
  EVAL(SG_BIND_ENTITY, p(ts));
}

//! R832: bind-stmt (8.6.4)
Stmt_Tree bind_stmt(TT_Stream &ts) {
  RULE(SG_BIND_STMT);
  constexpr auto p =
    seq(rule_tag,
        rule(language_binding_spec),
        opt(TOK(TK_DBL_COLON)),
        h_list(rule(bind_entity)),
        eol());
  EVAL(SG_BIND_STMT, p(ts));
}

//! R752: binding-attr (7.5.5)
Stmt_Tree binding_attr(TT_Stream &ts) {
  RULE(SG_BINDING_ATTR);
  constexpr auto p =
    alts(rule_tag,
         rule(access_spec),
         TOK(KW_DEFERRED),
         TOK(KW_NON_OVERRIDABLE),
         TOK(KW_NOPASS),
         h_seq(TOK(KW_PASS), opt(h_parens(name()))) // arg-name
         );
  EVAL(SG_BINDING_ATTR, p(ts));
}

//! R747: binding-private-stmt (7.5.5)
Stmt_Tree binding_private_stmt(TT_Stream &ts) {
  RULE(SG_BINDING_PRIVATE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PRIVATE),
        eol());
  EVAL(SG_BINDING_PRIVATE_STMT, p(ts));
}

//! R1108: block-stmt (11.1.4)
Stmt_Tree block_stmt(TT_Stream &ts) {
  RULE(SG_BLOCK_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_BLOCK),
        eol());
  EVAL(SG_BLOCK_STMT, p(ts));
}

//! R1036: bounds-remapping (10.2.2.2)
Stmt_Tree bounds_remapping(TT_Stream &ts) {
  RULE(SG_BOUNDS_REMAPPING);
  constexpr auto p =
    seq(rule_tag,
        rule(lower_bound_expr),
        TOK(TK_COLON),
        rule(upper_bound_expr));
  EVAL(SG_BOUNDS_REMAPPING, p(ts));
}

//! R1035: bounds-spec (R1034)
Stmt_Tree bounds_spec(TT_Stream &ts) {
  RULE(SG_BOUNDS_SPEC);
  constexpr auto p =
    seq(rule_tag,
        rule(lower_bound_expr),
        TOK(TK_COLON));
  EVAL(SG_BOUNDS_SPEC, p(ts));
}

/* I:C */

//! R1521: call-stmt (15.5.1)
Stmt_Tree call_stmt(TT_Stream &ts) {
  RULE(SG_CALL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_CALL),
        rule(procedure_designator),
        opt(h_parens(opt(list(TAG(SG_ACTUAL_ARG_SPEC_LIST),
                              rule(actual_arg_spec))))),
        eol()
        );
  EVAL(SG_CALL_STMT, p(ts));
}

//! R1145: case-selector (11.1.9.1)
Stmt_Tree case_selector(TT_Stream &ts) {
  RULE(SG_CASE_SELECTOR);
  constexpr auto p =
    alts(rule_tag,
         h_parens(list(TAG(SG_CASE_VALUE_RANGE_LIST),
                       rule(case_value_range))),
         TOK(KW_DEFAULT));
  EVAL(SG_CASE_SELECTOR, p(ts));
}

//! R1142: case-stmt (11.1.9.1)
Stmt_Tree case_stmt(TT_Stream &ts) {
  RULE(SG_CASE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_CASE),
        rule(case_selector),
        opt(name()), // case-construct-name
        eol());
  EVAL(SG_CASE_STMT, p(ts));
}

//! R1146: case-value-range (11.1.9.1)
Stmt_Tree case_value_range(TT_Stream &ts) {
  RULE(SG_CASE_VALUE_RANGE);
  constexpr auto p =
    alts(rule_tag,
         h_seq(rule(expr), TOK(TK_COLON), rule(expr)),
         h_seq(rule(expr), TOK(TK_COLON)),
         h_seq(TOK(TK_COLON), rule(expr)),
         h_seq(rule(expr)));
  EVAL(SG_CASE_VALUE_RANGE, p(ts));
}

//! R723: char-length (7.4.4.2)
Stmt_Tree char_length(TT_Stream &ts) {
  RULE(SG_CHAR_LENGTH);
  constexpr auto p =
    alts(rule_tag,
         h_parens(rule(type_param_value)),
         TOK(SG_INT_LITERAL_CONSTANT),
         /* this is an extension! */
         name());
  EVAL(SG_CHAR_LENGTH, p(ts));
}

//! R721: char-selector (7.4.4.2)
Stmt_Tree char_selector(TT_Stream &ts) {
  RULE(SG_CHAR_SELECTOR);
  constexpr auto p =
    alts(rule_tag,
         rule(length_selector),
         h_parens(TOK(KW_LEN), TOK(TK_EQUAL), rule(type_param_value),
                 TOK(TK_COMMA), TOK(KW_KIND), TOK(TK_EQUAL),
                 rule(int_constant_expr)),
         h_parens(rule(type_param_value), TOK(TK_COMMA),
                 opt(h_seq(TOK(KW_KIND), TOK(TK_EQUAL))),
                 rule(int_constant_expr)),
         h_parens(TOK(KW_KIND), TOK(TK_EQUAL), rule(int_constant_expr),
                 opt(h_seq(TOK(TK_COMMA),
                           TOK(KW_LEN), TOK(TK_EQUAL), rule(type_param_value))))
         );
  EVAL(SG_CHAR_SELECTOR, p(ts));
}

//! TRUNCATED R1208: close-stmt (12.5.7.2)
Stmt_Tree close_stmt(TT_Stream &ts) {
  RULE(SG_CLOSE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_CLOSE),
        rule(consume_parens),
        eol());
  EVAL(SG_CLOSE_STMT, p(ts));
}

//! R809: coarray-spec (8.5.6)
Stmt_Tree coarray_spec(TT_Stream &ts) {
  RULE(SG_COARRAY_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(explicit_coshape_spec),
         list(TAG(SG_DEFERRED_COSHAPE_SPEC_LIST),
              tag_if(TAG(SG_DEFERRED_COSHAPE_SPEC), TOK(TK_COLON)))
         );
  EVAL(SG_COARRAY_SPEC, p(ts));
}

//! R835: codimension-decl (8.6.5)
Stmt_Tree codimension_decl(TT_Stream &ts) {
  RULE(SG_CODIMENSION_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(), // coarray-name
        h_brackets(rule(coarray_spec)));
  EVAL(SG_CODIMENSION_DECL, p(ts));
}

//! R834: codimension-stmt (8.6.5)
Stmt_Tree codimension_stmt(TT_Stream &ts) {
  RULE(SG_CODIMENSION_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_CODIMENSION),
        opt(TOK(TK_DBL_COLON)),
        h_list(rule(codimension_decl)),
        eol());
  EVAL(SG_CODIMENSION_STMT, p(ts));
}

//! R914: coindexed-named-object (9.4.3)
Stmt_Tree coindexed_named_object(TT_Stream &ts) {
  RULE(SG_COINDEXED_NAMED_OBJECT);
  /* R914: coindexed-named-object is data-ref */
  /* C921: The data-ref should contain exactly one part-ref, and it must contain
     an image-selector */   
  constexpr auto p =
    seq(rule_tag,
        name(),
        opt(h_parens(list(TAG(SG_SECTION_SUBSCRIPT_LIST),
                          rule(section_subscript)))),
        rule(image_selector),
        neg(peek(TAG(TK_PERCENT))));
  EVAL(SG_COINDEXED_NAMED_OBJECT, p(ts));
}

//! R874: common-block-object (8.10.2.1)
Stmt_Tree common_block_object(TT_Stream &ts) {
  RULE(SG_COMMON_BLOCK_OBJECT);
  constexpr auto p =
    seq(rule_tag, name(), opt(h_parens(rule(array_spec))));
  EVAL(SG_COMMON_BLOCK_OBJECT, p(ts));
}

//! R873: common-stmt (8.10.2.1)
Stmt_Tree common_stmt(TT_Stream &ts) {
  RULE(SG_COMMON_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_COMMON),
        opt(h_alts(h_seq(TOK(TK_SLASHF), opt(name()), TOK(TK_SLASHF)),
                   TOK(TK_CONCAT))),
        list(TAG(SG_COMMON_BLOCK_OBJECT_LIST), rule(common_block_object)),
        star(h_seq(opt(TOK(TK_COMMA)),
                   h_alts(h_seq(TOK(TK_SLASHF), opt(name()), TOK(TK_SLASHF)),
                          TOK(TK_CONCAT)),
                   list(TAG(SG_COMMON_BLOCK_OBJECT_LIST),
                        rule(common_block_object)))),
        eol()
        );
  EVAL(SG_COMMON_STMT, p(ts));
}

//! TRUNCATED: R740: component-array-spec (7.5.4.1)
Stmt_Tree component_array_spec(TT_Stream &ts) {
  RULE(SG_COMPONENT_ARRAY_SPEC);
  constexpr auto p =
    alts(rule_tag,
         list(TAG(SG_COMPONENT_ARRAY_SPEC_LIST), rule(array_spec_helper)));
  EVAL(SG_COMPONENT_ARRAY_SPEC, p(ts));
}

//! R738: component-attr-spec (7.5.4.1)
Stmt_Tree component_attr_spec(TT_Stream &ts) {
  RULE(SG_COMPONENT_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(access_spec),
         TOK(KW_ALLOCATABLE),
         h_seq(TOK(KW_CODIMENSION), h_brackets(rule(coarray_spec))),
         TOK(KW_CONTIGUOUS),
         h_seq(TOK(KW_DIMENSION), h_parens(rule(component_array_spec))),
         TOK(KW_POINTER));
  EVAL(SG_COMPONENT_ATTR_SPEC, p(ts));
}

//! R758: component-data-source (7.5.10)
Stmt_Tree component_data_source(TT_Stream &ts) {
  RULE(SG_COMPONENT_DATA_SOURCE);
  /*
    All of these alternatives get covered by proc_target:
    alts(rule_tag, rule(expr), rule(data_target), rule(proc_target));
  */
  constexpr auto p =
    tag_if(rule_tag, rule(proc_target));
  EVAL(SG_COMPONENT_DATA_SOURCE, p(ts));
}

//! R739: component-decl (7.5.4.1)
Stmt_Tree component_decl(TT_Stream &ts) {
  RULE(SG_COMPONENT_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(), // component-name
        opt(h_parens(rule(component_array_spec))),
        opt(h_parens(rule(coarray_spec))),
        opt(h_seq(TOK(TK_ASTERISK), rule(char_length))),
        opt(rule(component_initialization)));
  EVAL(SG_COMPONENT_DECL, p(ts));
}

//! R736: component-def-stmt (7.5.4.1)
Stmt_Tree component_def_stmt(TT_Stream &ts) {
  RULE(SG_COMPONENT_DEF_STMT);
  constexpr auto p =
    alts(rule_tag,
         rule(data_component_def_stmt),
         rule(proc_component_def_stmt));
  EVAL(SG_COMPONENT_DEF_STMT, p(ts));
}

//! TRUNCATED R743: component-initialization (7.5.4.6)
Stmt_Tree component_initialization(TT_Stream &ts) {
  RULE(SG_COMPONENT_INITIALIZATION);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(TK_ARROW), rule(expr)));
  EVAL(SG_COMPONENT_INITIALIZATION, p(ts));
}

//! R757: component-spec (7.5.10)
Stmt_Tree component_spec(TT_Stream &ts) {
  RULE(SG_COMPONENT_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_EQUAL))),
        rule(component_data_source));
  EVAL(SG_COMPONENT_SPEC, p(ts));
}

//! R1158: computed-goto-stmt (11.2.3)
Stmt_Tree computed_goto_stmt(TT_Stream &ts) {
  RULE(SG_COMPUTED_GOTO_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_GO),
        TOK(KW_TO),
        h_parens(list(TAG(SG_LABEL_LIST), rule(label))),
        opt(TOK(TK_COMMA)),
        rule(expr),
        eol());
  EVAL(SG_COMPUTED_GOTO_STMT, p(ts));
}

//! R1126: concurrent-control (11.1.7.2)
Stmt_Tree concurrent_control(TT_Stream &ts) {
  RULE(SG_CONCURRENT_CONTROL);
  constexpr auto p =
    seq(rule_tag,
        name(), // index-name
        TOK(TK_EQUAL),
        tag_if(TAG(SG_CONCURRENT_LIMIT), rule(int_expr)),
        TOK(TK_COLON),
        tag_if(TAG(SG_CONCURRENT_LIMIT), rule(int_expr)),
        opt(h_seq(TOK(TK_COLON),
                  tag_if(TAG(SG_CONCURRENT_STEP), rule(int_expr))))
        );
  EVAL(SG_CONCURRENT_CONTROL, p(ts));
}

//! R1125: concurrent-header (11.1.7.2)
Stmt_Tree concurrent_header(TT_Stream &ts) {
  RULE(SG_CONCURRENT_HEADER);
  constexpr auto p =
    parens(rule_tag,
           opt(h_seq(rule(integer_type_spec), TOK(TK_DBL_COLON))),
           list(TAG(SG_CONCURRENT_CONTROL_LIST),
                rule(concurrent_control)),
           opt(h_seq(TOK(TK_COMMA),
                     rule(logical_expr)))
           );
  EVAL(SG_CONCURRENT_HEADER, p(ts));
}

//! R1129: concurrent-locality (11.1.7.2)
Stmt_Tree concurrent_locality(TT_Stream &ts) {
  RULE(SG_CONCURRENT_LOCALITY);
  constexpr auto p =
    seq(rule_tag, 
        star(rule(locality_spec))
        );
  EVAL(SG_CONCURRENT_LOCALITY, p(ts));
}

//! R1543: contains-stmt (15.6.2.8)
Stmt_Tree contains_stmt(TT_Stream &ts) {
  RULE(SG_CONTAINS_STMT);
  constexpr auto p = seq(rule_tag, TOK(KW_CONTAINS), eol());
  EVAL(SG_CONTAINS_STMT, p(ts));
}

//! R1159: continue-stmt (11.3)
Stmt_Tree continue_stmt(TT_Stream &ts) {
  RULE(SG_CONTINUE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_CONTINUE), eol());
  EVAL(SG_CONTINUE_STMT, p(ts));
}

//! R925: cosubscript (9.6)
Stmt_Tree cosubscript(TT_Stream &ts) {
  RULE(SG_COSUBSCRIPT);
  constexpr auto p =
    tag_if(rule_tag, rule(int_expr));
  EVAL(SG_COSUBSCRIPT, p(ts));
}

//! R1133: cycle-stmt (11.1.7.4.4)
Stmt_Tree cycle_stmt(TT_Stream &ts) {
  RULE(SG_CYCLE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_CYCLE), opt(name()), eol());
  EVAL(SG_CYCLE_STMT, p(ts));
}

/* I:D */

//! R737: data-component-def-stmt (7.5.4.1)
Stmt_Tree data_component_def_stmt(TT_Stream &ts) {
  RULE(SG_DATA_COMPONENT_DEF_STMT);
  constexpr auto p =
    seq(rule_tag,
        rule(declaration_type_spec),
        opt(h_seq(opt(h_seq(TOK(TK_COMMA),
                            list(TAG(SG_COMPONENT_ATTR_SPEC_LIST),
                                 rule(component_attr_spec)))),
                  TOK(TK_DBL_COLON))),
        list(TAG(SG_COMPONENT_DECL_LIST), rule(component_decl)),
        eol());
  EVAL(SG_DATA_COMPONENT_DEF_STMT, p(ts));
}

//! TRUNCATED: R840: data-implied-do (8.6.7)
Stmt_Tree data_implied_do(TT_Stream &ts) {
  RULE(SG_DATA_IMPLIED_DO);
  constexpr auto p =
    tag_if(rule_tag, rule(consume_parens));
  EVAL(SG_DATA_IMPLIED_DO, p(ts));
}

//! R1034: data-pointer-object (10.2.2.2)
Stmt_Tree data_pointer_object(TT_Stream &ts) {
  RULE(SG_DATA_POINTER_OBJECT);
    /* The real rule is:
     scalar-variable % data-pointer-component-name
     But without the ability to do left recursion, the variable clause 
     consumes the entire thing.
  */
  constexpr auto p =
    alts(rule_tag,
         rule(variable),
         //         h_seq(rule(variable)
         //     tag_if(TAG(SG_DATA_POINTER_COMPONENT_NAME), name())),
         tag_if(TAG(SG_VARIABLE_NAME), h_seq(name(), neg(TOK(TK_PERCENT))))
         );
  EVAL(SG_DATA_POINTER_OBJECT, p(ts));
}

//! R911: data-ref (9.4.2)
Stmt_Tree data_ref(TT_Stream &ts) {
  RULE(SG_DATA_REF);
  constexpr auto p =
    seq(rule_tag,
        rule(part_ref),
        star(h_seq(TOK(TK_PERCENT), rule(part_ref)))
        );
  EVAL(SG_DATA_REF, p(ts));
}

//! R837: data-stmt (8.6.7)
Stmt_Tree data_stmt(TT_Stream &ts) {
  RULE(SG_DATA_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_DATA),
        list(TAG(SG_DATA_STMT_SET_LIST),
             rule(data_stmt_set)),
        eol()
        );
  EVAL(SG_DATA_STMT, p(ts));
}

//! R845: data-stmt-constant (8.6.7)
Stmt_Tree data_stmt_constant(TT_Stream &ts) {
  RULE(SG_DATA_STMT_CONSTANT);
  constexpr auto p =
    alts(rule_tag,
         TOK(SG_CHAR_LITERAL_CONSTANT),
         h_seq(opt(rule(sign)), TOK(SG_REAL_LITERAL_CONSTANT)),
         h_seq(opt(rule(sign)), TOK(SG_INT_LITERAL_CONSTANT)),
         rule(logical_literal_constant),
         tag_if(TAG(SG_NULL_INIT), rule(function_reference)),
         tag_if(TAG(SG_INITIAL_DATA_TARGET), rule(designator)),
         rule(structure_constructor)
         );
  EVAL(SG_DATA_STMT_CONSTANT, p(ts));
}

//! R838: data-stmt-object (8.6.7)
Stmt_Tree data_stmt_object(TT_Stream &ts) {
  RULE(SG_DATA_STMT_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         rule(data_implied_do),
         rule(variable));
  EVAL(SG_DATA_STMT_OBJECT, p(ts));
}

//! R844: data-stmt-repeat (8.6.7)
Stmt_Tree data_stmt_repeat(TT_Stream &ts) {
  RULE(SG_DATA_STMT_REPEAT);
  constexpr auto p =
    alts(rule_tag,
         TOK(SG_INT_LITERAL_CONSTANT),  // integer-constant
         name(),
         rule(designator)  // int-constant-subobject
         );
  EVAL(SG_DATA_STMT_REPEAT, p(ts));
}

//! R837: data-stmt-set (8.6.7)
Stmt_Tree data_stmt_set(TT_Stream &ts) {
  RULE(SG_DATA_STMT_SET);
  constexpr auto p =
    seq(rule_tag,
        list(TAG(SG_DATA_STMT_OBJECT_LIST),
             rule(data_stmt_object)),
        TOK(TK_SLASHF),
        list(TAG(SG_DATA_STMT_VALUE_LIST),
             rule(data_stmt_value)),
        TOK(TK_SLASHF)
        );
  EVAL(SG_DATA_STMT_SET, p(ts));
}

//! R843: data-stmt-value (8.6.7)
Stmt_Tree data_stmt_value(TT_Stream &ts) {
  RULE(SG_DATA_STMT_VALUE);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(rule(data_stmt_repeat),
                  TOK(TK_ASTERISK))),
        rule(data_stmt_constant));
  EVAL(SG_DATA_STMT_VALUE, p(ts));
}

//! R941: dealloc-opt (9.7.3.1)
Stmt_Tree dealloc_opt(TT_Stream &ts) {
  RULE(SG_DEALLOC_OPT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_ERRMSG), TOK(TK_EQUAL), rule(variable)),
         h_seq(TOK(KW_STAT), TOK(TK_EQUAL), rule(variable)));
  EVAL(SG_DEALLOC_OPT, p(ts));
}

//! R940: deallocate-stmt (9.7.3.1)
Stmt_Tree deallocate_stmt(TT_Stream &ts) {
  RULE(SG_DEALLOCATE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_DEALLOCATE),
        h_parens(list(TAG(SG_ALLOCATE_OBJECT_LIST),
                      rule(allocate_object)),
                 opt(h_seq(TOK(TK_COMMA),
                           list(TAG(SG_DEALLOC_OPT_LIST),
                                rule(dealloc_opt))))
                 ),
        eol()
        );
  EVAL(SG_DEALLOCATE_STMT, p(ts));
}

//! R703: declaration-type-spec (7.3.2.1)
Stmt_Tree declaration_type_spec(TT_Stream &ts) {
  RULE(SG_DECLARATION_TYPE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(intrinsic_type_spec),
         h_seq(TOK(KW_TYPE),  h_parens(rule(intrinsic_type_spec))),
         h_seq(TOK(KW_TYPE),  h_parens(rule(derived_type_spec))),
         h_seq(TOK(KW_CLASS), h_parens(rule(derived_type_spec))),
         h_seq(TOK(KW_CLASS), h_parens(TOK(TK_ASTERISK))),
         h_seq(TOK(KW_TYPE),  h_parens(TOK(TK_ASTERISK)))
         );
  EVAL(SG_DECLARATION_TYPE_SPEC, p(ts));
}

//! Special version that doesn't allow kind parameters on intrinsic types
Stmt_Tree declaration_type_spec_no_kind(TT_Stream &ts) {
  RULE(SG_DECLARATION_TYPE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(intrinsic_type_spec_no_kind),
         h_seq(TOK(KW_TYPE),  h_parens(rule(intrinsic_type_spec))),
         h_seq(TOK(KW_TYPE),  h_parens(rule(derived_type_spec))),
         h_seq(TOK(KW_CLASS), h_parens(rule(derived_type_spec))),
         h_seq(TOK(KW_CLASS), h_parens(TOK(TK_ASTERISK))),
         h_seq(TOK(KW_TYPE),  h_parens(TOK(TK_ASTERISK)))
         );
  EVAL(SG_DECLARATION_TYPE_SPEC, p(ts));
}

//! R1025: default-char-expr (10.1.9.1)
Stmt_Tree default_char_expr(TT_Stream &ts) {
  RULE(SG_DEFAULT_CHAR_EXPR);
  constexpr auto p = tag_if(rule_tag, rule(expr));
  EVAL(SG_DEFAULT_CHAR_EXPR, p(ts));
}

//! R1509: defined-io-generic-spec (15.4.3.2)
Stmt_Tree defined_io_generic_spec(TT_Stream &ts) {
  RULE(SG_DEFINED_IO_GENERIC_SPEC);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_READ),  h_parens(TOK(KW_FORMATTED))),
         h_seq(TOK(KW_READ),  h_parens(TOK(KW_UNFORMATTED))),
         h_seq(TOK(KW_WRITE), h_parens(TOK(KW_FORMATTED))),
         h_seq(TOK(KW_WRITE), h_parens(TOK(KW_UNFORMATTED)))
         );
  EVAL(SG_DEFINED_IO_GENERIC_SPEC, p(ts));
}

//! R609: defined-operator (6.2.4)
Stmt_Tree defined_operator(TT_Stream &ts) {
  RULE(SG_DEFINED_OPERATOR);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_DEF_OP),  // can't tell diff. here between unary/binary
         rule(extended_intrinsic_op));
  EVAL(SG_DEFINED_OPERATOR, p(ts));
}

//! R754: derived-type-spec (7.5.9)
Stmt_Tree derived_type_spec(TT_Stream &ts) {
  RULE(SG_DERIVED_TYPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        name(),
        opt(h_parens(list(TAG(SG_TYPE_PARAM_SPEC_LIST),
                         rule(type_param_spec))))
        );
  EVAL(SG_DERIVED_TYPE_SPEC, p(ts));
}

//! R727: derived-type-stmt (7.5.2.1)
Stmt_Tree derived_type_stmt(TT_Stream &ts) {
  RULE(SG_DERIVED_TYPE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_TYPE),
        opt(h_seq(opt(h_seq(TOK(TK_COMMA),
                            list(TAG(SG_TYPE_ATTR_SPEC_LIST),
                                 rule(type_attr_spec)))),
                  TOK(TK_DBL_COLON))),
        name(), // type-name
        opt(h_parens(list(TAG(SG_TYPE_PARAM_NAME_LIST),
                          tag_if(TAG(SG_TYPE_PARAM_NAME), name())))),
        eol());
  EVAL(SG_DERIVED_TYPE_STMT, p(ts));
}

//! R901: designator (9.1)
Stmt_Tree designator(TT_Stream &ts) {
  RULE(SG_DESIGNATOR);
  /*
    should be:
       { object-name | array-element | array-section | 
         coindexed-named-object | complex-part-designator |
         structure-component | substring}
    but most reduce syntactically to data-ref.  array-section and substring
    can be followed by a (substring-range)
  */
  constexpr auto p =
    seq(rule_tag,
        rule(data_ref),
        opt(h_parens(rule(substring_range)))
        );
  EVAL(SG_DESIGNATOR, p(ts));
}

//! R848: dimension-stmt (8.6.8)
Stmt_Tree dimension_stmt(TT_Stream &ts) {
  RULE(SG_DIMENSION_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_DIMENSION),
        opt(TOK(TK_DBL_COLON)),
        /* This rule is a mess.  Rather than hoisting everything up onto one
           line, we should really introduce a tag for each "name(array-spec)"
           sequence... the standard doesn't give one. */
        h_list(h_seq(name(),
                     h_parens(rule(array_spec))
                     )
               ),
        eol()
        );
  EVAL(SG_DIMENSION_STMT, p(ts));
}

//! R1120: do-stmt (11.1.7.2)
Stmt_Tree do_stmt(TT_Stream &ts) {
  RULE(SG_DO_STMT);
  constexpr auto p =
    seq(rule_tag,
        h_alts(rule(label_do_stmt),
               rule(nonlabel_do_stmt)),
        eol());
  EVAL(SG_DO_STMT, p(ts));
}


//! R1536: dummy-arg (15.6.2.3)
Stmt_Tree dummy_arg(TT_Stream &ts) {
  RULE(SG_DUMMY_ARG);
  constexpr auto p =
    alts(rule_tag,
         name(), // dummy-arg-name
         TOK(TK_ASTERISK));
  EVAL(SG_DUMMY_ARG, p(ts));
}

/* I:E */

//! R1137: else-stmt (11.1.8.1)
Stmt_Tree else_stmt(TT_Stream &ts) {
  RULE(SG_ELSE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ELSE),
        opt(name()),
        eol());
  EVAL(SG_ELSE_STMT, p(ts));
}

//! R1136: else-if-stmt (11.1.8.1)
Stmt_Tree else_if_stmt(TT_Stream &ts) {
  RULE(SG_ELSE_IF_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ELSE),
        TOK(KW_IF),
        h_parens(rule(logical_expr)),
        TOK(KW_THEN),
        opt(name()),
        eol());
  EVAL(SG_ELSE_IF_STMT, p(ts));
}

//! R1048: elsewhere-stmt (10.2.3.1)
Stmt_Tree elsewhere_stmt(TT_Stream &ts) {
  RULE(SG_ELSEWHERE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ELSE), TOK(KW_WHERE), opt(name()), eol());
  EVAL(SG_ELSEWHERE_STMT, p(ts));
}

//! R1106: end-associate-stmt (11.1.3.2)
Stmt_Tree end_associate_stmt(TT_Stream &ts) {
  RULE(SG_END_ASSOCIATE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        TOK(KW_ASSOCIATE),
        opt(name()),
        eol());
  EVAL(SG_END_ASSOCIATE_STMT, p(ts));
}

//! R1108: end-block-stmt (11.1.4)
Stmt_Tree end_block_stmt(TT_Stream &ts) {
  RULE(SG_END_BLOCK_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        TOK(KW_BLOCK),
        opt(name()),
        eol());
  EVAL(SG_END_BLOCK_STMT, p(ts));
}

//! R1131: end-do (11.1.7.2)
Stmt_Tree end_do(TT_Stream &ts) {
  RULE(SG_END_DO);
  constexpr auto p =
    alts(rule_tag,
         rule(end_do_stmt),
         rule(continue_stmt));
  EVAL(SG_END_DO, p(ts));
}

//! R1132: end-do-stmt (11.1.7.2)
Stmt_Tree end_do_stmt(TT_Stream &ts) {
  RULE(SG_END_DO_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_END), TOK(KW_DO), opt(name()), eol());
  EVAL(SG_END_DO_STMT, p(ts));
}

//! R763: end-enum-stmt (7.6)
Stmt_Tree end_enum_stmt(TT_Stream &ts) {
  RULE(SG_END_ENUM_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_END), TOK(KW_ENUM), eol());
  EVAL(SG_END_ENUM_STMT, p(ts));
}

//! R1054: end-forall-stmt (10.2.4.1)
Stmt_Tree end_forall_stmt(TT_Stream &ts) {
  RULE(SG_END_FORALL_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_END), TOK(KW_FORALL),
        opt(name()), eol());
  EVAL(SG_END_FORALL_STMT, p(ts));
}

//! R1533: end-function-stmt (15.6.2.2)
Stmt_Tree end_function_stmt(TT_Stream &ts) {
  RULE(SG_END_FUNCTION_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_FUNCTION),
                  opt(name())) ),
        eol()
        );
  EVAL(SG_END_FUNCTION_STMT, p(ts));
}

//! R1138: end-if-stmt (11.1.8.1)
Stmt_Tree end_if_stmt(TT_Stream &ts) {
  RULE(SG_END_IF_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_END), TOK(KW_IF), opt(name()), eol());
  EVAL(SG_END_IF_STMT, p(ts));
}

//! R1504: end-interface-stmt (15.4.3.2)
Stmt_Tree end_interface_stmt(TT_Stream &ts) {
  RULE(SG_END_INTERFACE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_END), TOK(KW_INTERFACE),
        opt(rule(generic_spec)), eol());
  EVAL(SG_END_INTERFACE_STMT, p(ts));
}

//! R1406: end-module-stmt (14.2.1)
Stmt_Tree end_module_stmt(TT_Stream &ts) {
  RULE(SG_END_MODULE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_MODULE),  opt(name())) ),
        eol()
        );
  EVAL(SG_END_MODULE_STMT, p(ts));
}

//! R1540: end-mp-subprogram-stmt (15.6.2.5)
Stmt_Tree end_mp_subprogram_stmt(TT_Stream &ts) {
  RULE(SG_END_MP_SUBPROGRAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_PROCEDURE),
                  opt(name())) ),
        eol()
        );
  EVAL(SG_END_MP_SUBPROGRAM_STMT, p(ts));
}

//! R1403: end-program-stmt (14.1)
Stmt_Tree end_program_stmt(TT_Stream &ts) {
  RULE(SG_END_PROGRAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_PROGRAM),  opt(name())) ),
        eol()
        );
  EVAL(SG_END_PROGRAM_STMT, p(ts));
}

//! R1143: end-select-stmt (11.1.9.1)
Stmt_Tree end_select_stmt(TT_Stream &ts) {
  RULE(SG_END_SELECT_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END), TOK(KW_SELECT),
        opt(name()), // case-constuct-name
        eol());
  EVAL(SG_END_SELECT_STMT, p(ts));
}

//! R1151: end-select-rank-stmt (11.1.10.1)
Stmt_Tree end_select_rank_stmt(TT_Stream &ts) {
  RULE(SG_END_SELECT_RANK_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END), TOK(KW_SELECT),
        opt(name()), // select-construct-name
        eol());
  EVAL(SG_END_SELECT_RANK_STMT, p(ts));
}

//! R1155: end-select-type-stmt (11.1.11.1)
Stmt_Tree end_select_type_stmt(TT_Stream &ts) {
  RULE(SG_END_SELECT_TYPE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END), TOK(KW_SELECT),
        opt(name()), // select-construct-name
        eol());
  EVAL(SG_END_SELECT_TYPE_STMT, p(ts));
}

//! R1419: end-submodule-stmt (14.2.3)
Stmt_Tree end_submodule_stmt(TT_Stream &ts) {
  RULE(SG_END_SUBMODULE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_SUBMODULE),
                  opt(name())) ),
        eol()
        );
  EVAL(SG_END_SUBMODULE_STMT, p(ts));
}

//! R1537: end-subroutine-stmt (15.6.2.3)
Stmt_Tree end_subroutine_stmt(TT_Stream &ts) {
  RULE(SG_END_SUBROUTINE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        opt(h_seq(TOK(KW_SUBROUTINE),
                  opt(name())) ),
        eol()
        );
  EVAL(SG_END_SUBROUTINE_STMT, p(ts));
}

//! R730: end-type-stmt (7.5.2.1)
Stmt_Tree end_type_stmt(TT_Stream &ts) {
  RULE(SG_END_TYPE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END),
        TOK(KW_TYPE),
        opt(name()),  // type-name
        eol());
  EVAL(SG_END_TYPE_STMT, p(ts));
}

//! R1049: end-where-stmt (10.2.3.1)
Stmt_Tree end_where_stmt(TT_Stream &ts) {
  RULE(SG_END_WHERE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END), TOK(KW_WHERE), opt(name()), eol());
  EVAL(SG_END_WHERE_STMT, p(ts));
}

//! TRUNCATED R1225: endfile-stmt (12.8.1)
Stmt_Tree endfile_stmt(TT_Stream &ts) {
  RULE(SG_ENDFILE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_END), TOK(KW_FILE),
        h_alts(rule(int_expr),
               rule(consume_parens)),
        eol());
  EVAL(SG_ENDFILE_STMT, p(ts));
}

//! R803: entity-decl (8.2)
Stmt_Tree entity_decl(TT_Stream &ts) {
  RULE(SG_ENTITY_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(),
        opt(h_parens(rule(array_spec))),
        opt(h_brackets(rule(coarray_spec))),
        opt(h_seq(TOK(TK_ASTERISK), rule(char_length))),
        opt(rule(initialization)));
  EVAL(SG_ENTITY_DECL, p(ts));
}

//! R1541: entry-stmt (15.6.2.6)
Stmt_Tree entry_stmt(TT_Stream &ts) {
  RULE(SG_ENTRY_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ENTRY),
        name(),
        opt(h_seq(h_parens(opt(h_list(rule(dummy_arg)))),
                  opt(rule(suffix)))),
        eol());
  EVAL(SG_ENTRY_STMT, p(ts));
}

//! R762: enumerator (7.6)
Stmt_Tree enumerator(TT_Stream &ts) {
  RULE(SG_ENUMERATOR);
  constexpr auto p =
    seq(rule_tag,
        name(), // named-constant
        opt(h_seq(TOK(TK_EQUAL), rule(int_expr)))
        );
  EVAL(SG_ENUMERATOR, p(ts));
}

//! R760: enumerator-def-stmt (7.6)
Stmt_Tree enumerator_def_stmt(TT_Stream &ts) {
  RULE(SG_ENUMERATOR_DEF_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ENUMERATOR),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_ENUMERATOR_LIST),
             rule(enumerator)),
        eol());
  EVAL(SG_ENUMERATOR_DEF_STMT, p(ts));
}

//! R759: enum-def-stmt (7.6)
Stmt_Tree enum_def_stmt(TT_Stream &ts) {
  RULE(SG_ENUM_DEF_STMT);
  auto p =
    seq(rule_tag,
        TOK(KW_ENUM), TOK(TK_COMMA), rule(bind_c), eol());
  EVAL(SG_ENUM_DEF_STMT, p(ts));
}

//! R1021: equiv-op (6.2.4)
Stmt_Tree equiv_op(TT_Stream &ts) {
  RULE(SG_EQUIV_OP);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_REL_EQ),   // the lexer merges ".EQ." and "=="
         TOK(TK_REL_NE));
  EVAL(SG_EQUIV_OP, p(ts));
}

//! R872: equivalence-object (8.10.1.1)
Stmt_Tree equivalence_object(TT_Stream &ts) {
  RULE(SG_EQUIVALENCE_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         rule(array_element),
         rule(substring),
         tag_if(TAG(SG_VARIABLE_NAME), name()));
  EVAL(SG_EQUIVALENCE_OBJECT, p(ts));
}

//! R871: equivalence-set (8.10.1.1)
Stmt_Tree equivalence_set(TT_Stream &ts) {
  RULE(SG_EQUIVALENCE_SET);
  constexpr auto p =
    parens(rule_tag,
           rule(equivalence_object),
           TOK(TK_COMMA),
           list(TAG(SG_EQUIVALENCE_OBJECT_LIST),
                rule(equivalence_object)));
  EVAL(SG_EQUIVALENCE_SET, p(ts));
}

//! R870: equivalence-stmt (8.10.1.1)
Stmt_Tree equivalence_stmt(TT_Stream &ts) {
  RULE(SG_EQUIVALENCE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_EQUIVALENCE),
        list(TAG(SG_EQUIVALENCE_SET_LIST),
             rule(equivalence_set)),
        eol());
  EVAL(SG_EQUIVALENCE_STMT, p(ts));
}

//! R1161: error-stop-stmt (11.4)
Stmt_Tree error_stop_stmt(TT_Stream &ts) {
  RULE(SG_ERROR_STOP_STMT);
  constexpr auto p =
    seq(rule_tag,
         TOK(KW_ERROR),
         TOK(KW_STOP),
         opt(rule(default_char_expr)),
         opt(h_seq(TOK(TK_COMMA),
                   TOK(KW_QUIET),
                   TOK(TK_EQUAL),
                   rule(logical_expr))),
         eol());
  EVAL(SG_ERROR_STOP_STMT, p(ts));
}

//! R1170: event-post-stmt (11.6.7)
Stmt_Tree event_post_stmt(TT_Stream &ts) {
  RULE(SG_EVENT_POST_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_EVENT),
        TOK(KW_POST),
        h_parens(rule(variable), // event-variable
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(rule(sync_stat))))),
        eol());
  EVAL(SG_EVENT_POST_STMT, p(ts));
}

//! R1172: event-wait-stmt (11.6.8)
Stmt_Tree event_wait_stmt(TT_Stream &ts) {
  RULE(SG_EVENT_WAIT_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_EVENT),
        TOK(KW_WAIT),
        h_parens(rule(variable), // event-variable
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(h_alts(h_seq(TOK(KW_UNTIL_COUNT),
                                               TOK(TK_EQUAL),
                                               rule(expr)), 
                                         rule(sync_stat)))))),
        eol());
  EVAL(SG_EVENT_WAIT_STMT, p(ts));
}

//! R811: explicit-coshape-spec (8.5.6.3)
Stmt_Tree explicit_coshape_spec(TT_Stream &ts) {
  RULE(SG_EXPLICIT_COSHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(list(TAG(HOIST),
                       h_seq(opt(h_seq(rule(expr), TOK(TK_COLON))),
                             rule(expr))),
                  TOK(TK_COMMA))),
        h_seq(opt(h_seq(rule(expr), TOK(TK_COLON))),
              TOK(TK_ASTERISK))
        );
  EVAL(SG_EXPLICIT_COSHAPE_SPEC, p(ts));
}

//! R816: explicit-shape-spec (8.5.8.2)
Stmt_Tree explicit_shape_spec(TT_Stream &ts) {
  RULE(SG_EXPLICIT_SHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(rule(expr), TOK(TK_COLON))),
        rule(expr));
  EVAL(SG_EXPLICIT_SHAPE_SPEC, p(ts));
}

//! R1156: exit-stmt (11.1.12)
Stmt_Tree exit_stmt(TT_Stream &ts) {
  RULE(SG_EXIT_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_EXIT), opt(name()), eol());
  EVAL(SG_EXIT_STMT, p(ts));
}

//! TRUNCATED R1022: expr (10.1.2.8)
Stmt_Tree expr(TT_Stream &ts) {
  RULE(SG_EXPR);
  if(TAG(TK_ASTERISK) == ts.peek() ||
     TAG(TK_SLASHF) == ts.peek())
    return Stmt_Tree{};
  EVAL(SG_EXPR, consume_until_break(ts, rule_tag));
}

//! R610: extended-intrinsic-op (6.2.4)
Stmt_Tree extended_intrinsic_op(TT_Stream &ts) {
  RULE(SG_EXTENDED_INTRINSIC_OP);
  constexpr auto p =
    tag_if(rule_tag,
           rule(intrinsic_operator));
  EVAL(SG_EXTENDED_INTRINSIC_OP, p(ts));
}

//! R1511: external-stmt (15.4.3.5)
Stmt_Tree external_stmt(TT_Stream &ts) {
  RULE(SG_EXTERNAL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_EXTERNAL), opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_EXTERNAL_NAME_LIST), name()),
        eol());
  EVAL(SG_EXTERNAL_STMT, p(ts));
}

/* I:F */

//! R1163: fail-image-stmt (11.5)
Stmt_Tree fail_image_stmt(TT_Stream &ts) {
  RULE(SG_FAIL_IMAGE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_FAIL),
        TOK(KW_IMAGE),
        eol());
  EVAL(SG_FAIL_IMAGE_STMT, p(ts));
}

//! R1175: form-team-stmt (11.6.9)
Stmt_Tree form_team_stmt(TT_Stream &ts) {
  RULE(SG_FORM_TEAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_FORM),
        TOK(KW_TEAM),
        h_parens(h_seq(rule(expr), TOK(TK_COMMA), rule(variable),
                       opt(h_seq(TOK(TK_COMMA),
                                 h_list(
                                     h_alts(h_seq(TOK(KW_NEW_INDEX),
                                                  TOK(TK_EQUAL),
                                                  rule(expr)),
                                            rule(sync_stat))
                                        )
                                 )
                           )
                       )
                 ),
        eol());
  EVAL(SG_FORM_TEAM_STMT, p(ts));
}


//! R753: final-procedure-stmt (7.5.6.1)
Stmt_Tree final_procedure_stmt(TT_Stream &ts) {
  RULE(SG_FINAL_PROCEDURE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_FINAL),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_FINAL_SUBROUTINE_NAME_LIST), name()),
        eol());
  EVAL(SG_FINAL_PROCEDURE_STMT, p(ts));
}

//! TRUNCATED R1228: flush-stmt (12.9)
Stmt_Tree flush_stmt(TT_Stream &ts) {
  RULE(SG_FLUSH_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_FLUSH),
        h_alts(rule(int_expr),
               rule(consume_parens)),
        eol());
  EVAL(SG_FLUSH_STMT, p(ts));
}

//! R1053: forall-assignment-stmt (10.2.4.1)
Stmt_Tree forall_assignment_stmt(TT_Stream &ts) {
  RULE(SG_FORALL_ASSIGNMENT_STMT);
  constexpr auto p =
    alts(rule_tag,
         rule(assignment_stmt),
         rule(pointer_assignment_stmt));
  // no eol() here as the alternatives are both full statements
  EVAL(SG_FORALL_ASSIGNMENT_STMT, p(ts));
}

//! R1051: forall-construct-stmt (10.2.4.1)
Stmt_Tree forall_construct_stmt(TT_Stream &ts) {
  RULE(SG_FORALL_CONSTRUCT_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), // forall-construct-name
                  TOK(TK_COLON))),
        TOK(KW_FORALL),
        rule(concurrent_header),
        eol());
  EVAL(SG_FORALL_CONSTRUCT_STMT, p(ts));
}

//! R1055: forall-stmt (10.2.4.3)
Stmt_Tree forall_stmt(TT_Stream &ts) {
  RULE(SG_FORALL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_FORALL),
        rule(concurrent_header),
        rule(forall_assignment_stmt),
        eol());
  EVAL(SG_FORALL_STMT, p(ts));
}

//! R1215: format (12.6.2.2)
Stmt_Tree format(TT_Stream &ts) {
  RULE(SG_FORMAT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(rule(expr), neg(peek(TAG(TK_EQUAL)))),
         // label: handled by the expr above
         TOK(TK_ASTERISK));
  EVAL(SG_FORMAT, p(ts));
}

//! TRUNCATED R1302: format-specification (13.2.1)
Stmt_Tree format_specification(TT_Stream &ts) {
  RULE(SG_FORMAT_SPECIFICATION);
  constexpr auto p =
    tag_if(rule_tag, rule(consume_parens));
  EVAL(SG_FORMAT_SPECIFICATION, p(ts));
}

//! R1301: format-stmt (13.2.1)
Stmt_Tree format_stmt(TT_Stream &ts) {
  RULE(SG_FORMAT_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_FORMAT), rule(format_specification), eol());
  EVAL(SG_FORMAT_STMT, p(ts));
}

//! R1520: function-reference (15.5.1)
Stmt_Tree function_reference(TT_Stream &ts) {
  RULE(SG_FUNCTION_REFERENCE);
  constexpr auto p =
    seq(rule_tag,
        rule(procedure_designator),
        h_parens(opt(list(TAG(SG_ACTUAL_ARG_SPEC_LIST),
                          rule(actual_arg_spec))))
        );
  EVAL(SG_FUNCTION_REFERENCE, p(ts));
}

//! R1530: function-stmt (15.6.2.2)
Stmt_Tree function_stmt(TT_Stream &ts) {
  RULE(SG_FUNCTION_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(rule(prefix)),
        TOK(KW_FUNCTION), name(),
        opt(h_seq(h_parens(
                      opt(list(TAG(SG_DUMMY_ARG_NAME_LIST),
                               name()))
                           )
                  )
            ),
        opt(rule(suffix)),
        eol()
        );
  EVAL(SG_FUNCTION_STMT, p(ts));
}

/* I:G */

//! R1508: generic-spec (15.4.3.2)
Stmt_Tree generic_spec(TT_Stream &ts) {
  RULE(SG_GENERIC_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(list_name), 
         h_seq(TOK(KW_OPERATOR), h_parens(rule(defined_operator))),
         h_seq(TOK(KW_ASSIGNMENT), h_parens(TOK(TK_EQUAL))),
         rule(defined_io_generic_spec)
         );  
  EVAL(SG_GENERIC_SPEC, p(ts));
}

//! R1510: generic-stmt (15.4.3.3)
Stmt_Tree generic_stmt(TT_Stream &ts) {
  RULE(SG_GENERIC_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_GENERIC),
        opt(h_seq(TOK(TK_COMMA), rule(access_spec))),
        TOK(TK_DBL_COLON),
        rule(generic_spec),
        TOK(TK_ARROW),
        h_list(name()), // specific-procedure-list
        eol());
  EVAL(SG_GENERIC_STMT, p(ts));
}       

//! R1157: goto-stmt (11.2.2)
Stmt_Tree goto_stmt(TT_Stream &ts) {
  RULE(SG_GOTO_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_GO), TOK(KW_TO), rule(label), eol());
  EVAL(SG_GOTO_STMT, p(ts));
}
/* I:H */

/* I:I */

//! R1139: if-stmt (11.1.8.4)
Stmt_Tree if_stmt(TT_Stream &ts) {
  RULE(SG_IF_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_IF),
        h_parens(rule(logical_expr)),
        rule(action_stmt),
        eol());
  EVAL(SG_IF_STMT, p(ts));
}

//! R1135: if-then-stmt (11.1.8.1)
Stmt_Tree if_then_stmt(TT_Stream &ts) {
  RULE(SG_IF_THEN_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_IF),
        h_parens(rule(logical_expr)),
        TOK(KW_THEN),
        eol());
  EVAL(SG_IF_THEN_STMT, p(ts));
}

//! R924: image-selector (9.6)
Stmt_Tree image_selector(TT_Stream &ts) {
  RULE(SG_IMAGE_SELECTOR);
  constexpr auto p =
    brackets(rule_tag,
             list(TAG(SG_COSUBSCRIPT_LIST),
                  rule(cosubscript)),
             opt(h_seq(TOK(TK_COMMA),
                       list(TAG(SG_IMAGE_SELECTOR_SPEC_LIST),
                            rule(image_selector_spec))))
             );
  EVAL(SG_IMAGE_SELECTOR, p(ts));
}

//! R926: image-selector-spec (9.6)
Stmt_Tree image_selector_spec(TT_Stream &ts) {
  RULE(SG_IMAGE_SELECTOR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_STAT), TOK(TK_EQUAL), rule(variable)),
         h_seq(TOK(KW_TEAM), TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(KW_TEAM_NUMBER), TOK(TK_EQUAL), rule(expr))
         );
  EVAL(SG_IMAGE_SELECTOR_SPEC, p(ts));
}

//! R866: implicit-none-spec (8.7)
Stmt_Tree implicit_none_spec(TT_Stream &ts) {
  RULE(SG_IMPLICIT_NONE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         TOK(KW_EXTERNAL),
         TOK(KW_TYPE)
         );
  EVAL(SG_IMPLICIT_NONE_SPEC, p(ts));
}

//! R864: implicit-spec (8.7)
/*! This is ugly because INTEGER, REAL, CHARACTER and COMPLEX can take
  a parenthesized kind-selector, which can gobble a letter-spec. */
Stmt_Tree implicit_spec(TT_Stream &ts) {
  RULE(SG_IMPLICIT_SPEC);
  constexpr auto p =
    alts(rule_tag,
         // Try to match the longest sequence first <TYPE>(..)(..)
         h_seq(rule(declaration_type_spec),
               h_parens(list(TAG(SG_LETTER_SPEC_LIST), rule(letter_spec)))),
         // How about <TYPE>(..)?
         h_seq(rule(declaration_type_spec_no_kind),
               h_parens(list(TAG(SG_LETTER_SPEC_LIST), rule(letter_spec))))
         );
  EVAL(SG_IMPLICIT_SPEC, p(ts));
}

//! R863: implicit-stmt (8.7)
Stmt_Tree implicit_stmt(TT_Stream &ts) {
  RULE(SG_IMPLICIT_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_IMPLICIT), TOK(KW_NONE),
               opt(h_parens(opt(list(TAG(SG_IMPLICIT_NONE_SPEC_LIST),
                                    rule(implicit_none_spec))))),
               eol()),
         h_seq(TOK(KW_IMPLICIT), list(TAG(SG_IMPLICIT_SPEC_LIST),
                                      rule(implicit_spec)),
               eol())
         );
  EVAL(SG_IMPLICIT_STMT, p(ts));
}

//! R824: implied-shape-spec (8.5.8.6)
Stmt_Tree implied_shape_spec(TT_Stream &ts) {
  RULE(SG_IMPLIED_SHAPE_SPEC);
  constexpr auto p =
    seq(rule_tag,
        /* Odd sort of rule requires a list of at least two
           assumed-implied-spec */
        rule(assumed_implied_spec),
        TOK(TK_COMMA),
        list(TAG(SG_ASSUMED_IMPLIED_SPEC_LIST), rule(assumed_implied_spec))
        );
  EVAL(SG_IMPLIED_SHAPE_SPEC, p(ts));
}

//! R867: import-stmt (8.8)
Stmt_Tree import_stmt(TT_Stream &ts) {
  RULE(SG_IMPORT_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_IMPORT),
               opt(h_seq(opt(TOK(TK_DBL_COLON)),
                         list(TAG(SG_IMPORT_NAME_LIST), name()))), eol()),
         h_seq(TOK(KW_IMPORT), TOK(TK_COMMA), TOK(KW_ONLY), TOK(TK_COLON),
               list(TAG(SG_IMPORT_NAME_LIST), name()), eol()),
         h_seq(TOK(KW_IMPORT), TOK(TK_COMMA),
               h_alts(TOK(KW_NONE), TOK(KW_ALL))), eol());
  EVAL(SG_IMPORT_STMT, p(ts));
}

//! TRUNCATED R805: initialization (8.2)
Stmt_Tree initialization(TT_Stream &ts) {
  RULE(SG_INITIALIZATION);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(TK_ARROW), rule(expr)));
  EVAL(SG_INITIALIZATION, p(ts));
}

//! R1216: input-item (12.6.3)
Stmt_Tree input_item(TT_Stream &ts) {
  RULE(SG_INPUT_ITEM);
  constexpr auto p =
    alts(rule_tag,
         rule(io_implied_do),
         rule(variable));
  EVAL(SG_INPUT_ITEM, p(ts));
}

//! TRUNCATED R1230: inquire-stmt (12.10.1)
Stmt_Tree inquire_stmt(TT_Stream &ts) {
  RULE(SG_INQUIRE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_INQUIRE),
        rule(consume_parens),
        opt(list(TAG(SG_OUTPUT_ITEM_LIST), rule(output_item))),
        eol());
  EVAL(SG_INQUIRE_STMT, p(ts));
}

//! R1031: int-constant-expr (10.1.12)
Stmt_Tree int_constant_expr(TT_Stream &ts) {
  RULE(SG_INT_CONSTANT_EXPR);
  constexpr auto p = tag_if(rule_tag, rule(int_expr));
  EVAL(SG_INT_CONSTANT_EXPR, p(ts));
}

//! R1026: int-expr (10.1.9.1)
Stmt_Tree int_expr(TT_Stream &ts) {
  RULE(SG_INT_EXPR);
  constexpr auto p = tag_if(rule_tag, rule(expr));
  EVAL(SG_INT_EXPR, p(ts));
}

//! R705: integer-type-spec (7.4.1)
Stmt_Tree integer_type_spec(TT_Stream &ts) {
  RULE(SG_INTEGER_TYPE_SPEC);
  constexpr auto p =
    seq(rule_tag, TOK(KW_INTEGER), opt(rule(kind_selector)));
  EVAL(SG_INTEGER_TYPE_SPEC, p(ts));
}

//! R826: intent-spec (8.5.10)
Stmt_Tree intent_spec(TT_Stream &ts) {
  RULE(SG_INTENT_SPEC);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_IN), TOK(KW_OUT)),
         TOK(KW_IN),
         TOK(KW_OUT),
         TOK(KW_INOUT));
  EVAL(SG_INTENT_SPEC, p(ts));
}

//! R849: intent-stmt (8.6.9)
Stmt_Tree intent_stmt(TT_Stream &ts) {
  RULE(SG_INTENT_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_INTENT),
        h_parens(rule(intent_spec)),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_DUMMY_ARG_NAME_LIST),
             name()),
        eol());
  EVAL(SG_INTENT_STMT, p(ts));
}

//! R1503: interface-stmt (15.4.3.2)
Stmt_Tree interface_stmt(TT_Stream &ts) {
  RULE(SG_INTERFACE_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_INTERFACE), opt(rule(generic_spec)), eol()),
         h_seq(TOK(KW_ABSTRACT), TOK(KW_INTERFACE)), eol());
  EVAL(SG_INTERFACE_STMT, p(ts));
}

//! R608: intrinsic-operator (6.2.4)
Stmt_Tree intrinsic_operator(TT_Stream &ts) {
  RULE(SG_INTRINSIC_OPERATOR);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_POWER_OP),
         rule(mult_op),
         rule(add_op),
         TOK(TK_CONCAT),
         rule(rel_op),
         TOK(TK_NOT_OP),
         TOK(TK_AND_OP),
         TOK(TK_OR_OP),
         rule(equiv_op));
  EVAL(SG_INTRINSIC_OPERATOR, p(ts));
}

//! R1519: intrinsic-stmt (15.4.3.7)
Stmt_Tree intrinsic_stmt(TT_Stream &ts) {
  RULE(SG_INTRINSIC_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_INTRINSIC),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_INTRINSIC_PROCEDURE_NAME_LIST),
             name()),
        eol());
  EVAL(SG_INTRINSIC_STMT, p(ts));
}

//! R704: intrinsic-type-spec (7.4.1)
Stmt_Tree intrinsic_type_spec(TT_Stream &ts) {
  RULE(SG_INTRINSIC_TYPE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(integer_type_spec),
         h_seq(TOK(KW_REAL), opt(rule(kind_selector))),
         h_seq(TOK(KW_COMPLEX), opt(rule(kind_selector))),
         h_seq(TOK(KW_LOGICAL), opt(rule(kind_selector))),
         seq(TAG(KW_DOUBLEPRECISION), TOK(KW_DOUBLE), TOK(KW_PRECISION)),
         TOK(KW_DOUBLEPRECISION),
         h_seq(TOK(KW_CHARACTER), opt(rule(char_selector)))
         );
  EVAL(SG_INTRINSIC_TYPE_SPEC, p(ts));
}

//! Special version that doesn't consume a trailing paren expression
Stmt_Tree intrinsic_type_spec_no_kind(TT_Stream &ts) {
  RULE(SG_INTRINSIC_TYPE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         TOK(KW_INTEGER),
         TOK(KW_REAL),
         TOK(KW_COMPLEX),
         TOK(KW_LOGICAL),
         seq(TAG(KW_DOUBLEPRECISION), TOK(KW_DOUBLE), TOK(KW_PRECISION)),
         TOK(KW_DOUBLEPRECISION),
         TOK(KW_CHARACTER)
         );
  EVAL(SG_INTRINSIC_TYPE_SPEC, p(ts));
}

//! R1218: io-implied-do (12.6.3)
Stmt_Tree io_implied_do(TT_Stream &ts) {
  RULE(SG_IO_IMPLIED_DO);
  constexpr auto p =
    parens(rule_tag,
           h_seq(list(TAG(SG_IO_IMPLIED_DO_OBJECT_LIST),
                      rule(io_implied_do_object)),
                 TOK(TK_COMMA),
                 rule(io_implied_do_control))
           );
  EVAL(SG_IO_IMPLIED_DO, p(ts));
}

//! R1220: io-implied-do-control (12.6.3)
Stmt_Tree io_implied_do_control(TT_Stream &ts) {
  RULE(SG_IO_IMPLIED_DO_CONTROL);
  constexpr auto p =
    seq(rule_tag,
        rule(variable), TOK(TK_EQUAL),
        rule(expr), TOK(TK_COMMA), rule(expr), 
        opt(h_seq(TOK(TK_COMMA), rule(expr)))
        );
  EVAL(SG_IO_IMPLIED_DO_CONTROL, p(ts));
}

//! R1219: io-implied-do-object (12.6.3)
Stmt_Tree io_implied_do_object(TT_Stream &ts) {
  RULE(SG_IO_IMPLIED_DO_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(rule(input_item), neg(peek(TAG(TK_EQUAL)))),
         h_seq(rule(output_item), neg(peek(TAG(TK_EQUAL))))
         );
  EVAL(SG_IO_IMPLIED_DO_OBJECT, p(ts));
}

/* I:J */

/* I:K */

//! R706: kind-selector (7.4.1)
Stmt_Tree kind_selector(TT_Stream &ts) {
  RULE(SG_KIND_SELECTOR);
  constexpr auto p =
    alts(rule_tag,
         h_parens(opt(h_seq(TOK(KW_KIND), TOK(TK_EQUAL))),
                 rule(int_constant_expr)),
         // obsolete F77 style
         h_seq(TOK(TK_ASTERISK), TOK(SG_INT_LITERAL_CONSTANT))
         );
  EVAL(SG_KIND_SELECTOR, p(ts));
}

/* I:L */

//! R611: label (6.2.5)
Stmt_Tree label(TT_Stream &ts) {
  RULE(SG_LABEL);
  /* This is overly permissive in that it would allow a kind specifier.  The
     actual rule is "digit [digit [digit [ digit [ digit ] ] ] ]" */
  constexpr auto p =
    tag_if(rule_tag, TOK(SG_INT_LITERAL_CONSTANT));
  EVAL(SG_LABEL, p(ts));
}

//! R1121: label-do-stmt (11.1.7.2)
Stmt_Tree label_do_stmt(TT_Stream &ts) {
  RULE(SG_LABEL_DO_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_DO), rule(label),
        opt(rule(loop_control)),
        eol());
  EVAL(SG_LABEL_DO_STMT, p(ts));
}

//! R808: language-binding-spec (8.5.5)
Stmt_Tree language_binding_spec(TT_Stream &ts) {
  RULE(SG_LANGUAGE_BINDING_SPEC);
  auto p =
    seq(rule_tag,
        TOK(KW_BIND),
        TOK(TK_PARENL),
        literal("c"),
        opt(h_seq(TOK(TK_COMMA), TOK(KW_NAME), TOK(TK_EQUAL),
                  rule(default_char_expr))
            ),
        TOK(TK_PARENR)
        );
  EVAL(SG_LANGUAGE_BINDING_SPEC, p(ts));
}

//! R722: length-selector (7.4.4.2)
Stmt_Tree length_selector(TT_Stream &ts) {
  RULE(SG_LENGTH_SELECTOR);
  constexpr auto p =
    alts(rule_tag,
         h_parens(opt(h_seq(TOK(KW_LEN), TOK(TK_EQUAL))),
                 rule(type_param_value)),
         h_seq(TOK(TK_ASTERISK), rule(char_length)));
  EVAL(SG_LENGTH_SELECTOR, p(ts));
}

//! R865: letter_spec (8.7)
Stmt_Tree letter_spec(TT_Stream &ts) {
  RULE(SG_LETTER_SPEC);
  constexpr auto p =
    seq(rule_tag,
        letter(), opt(h_seq(TOK(TK_MINUS), letter()))
        );
  EVAL(SG_LETTER_SPEC, p(ts));
}

//! R1130: locality-spec (11.1.7.2)
Stmt_Tree locality_spec(TT_Stream &ts) {
  RULE(SG_LOCALITY_SPEC);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_LOCAL),
               h_parens(list(TAG(SG_VARIABLE_NAME_LIST),
                             tag_if(TAG(SG_VARIABLE_NAME), name())))),
         h_seq(TOK(KW_LOCAL_INIT),
               h_parens(list(TAG(SG_VARIABLE_NAME_LIST),
                             tag_if(TAG(SG_VARIABLE_NAME), name())))),
         h_seq(TOK(KW_SHARED),
               h_parens(list(TAG(SG_VARIABLE_NAME_LIST),
                             tag_if(TAG(SG_VARIABLE_NAME), name())))),
         h_seq(TOK(KW_DEFAULT), TOK(TK_PARENL), TOK(KW_NONE), TOK(TK_PARENR))
         );
  EVAL(SG_LOCALITY_SPEC, p(ts));
}

//! R1179: lock-stmt (11.6.10)
Stmt_Tree lock_stmt(TT_Stream &ts) {
  RULE(SG_LOCK_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_LOCK),
        h_parens(rule(variable), // lock-variable
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(h_alts(h_seq(TOK(KW_ACQUIRED_LOCK),
                                               TOK(TK_EQUAL),
                                               rule(variable)),
                                         rule(sync_stat)))))),
        eol());
  EVAL(SG_LOCK_STMT, p(ts));
}

//! R1024: logical-expr (10.1.9.1)
Stmt_Tree logical_expr(TT_Stream &ts) {
  RULE(SG_LOGICAL_EXPR);
  constexpr auto p =
    tag_if(rule_tag, rule(expr));
  EVAL(SG_LOGICAL_EXPR, p(ts));
}

//! R725: logical-literal-constant (7.4.5)
Stmt_Tree logical_literal_constant(TT_Stream &ts) {
  RULE(SG_LOGICAL_LITERAL_CONSTANT);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_FALSE_CONSTANT),
         TOK(TK_TRUE_CONSTANT));
  EVAL(SG_LOGICAL_LITERAL_CONSTANT, p(ts));
}
 
//! R1123: loop-control (11.1.7.1)
Stmt_Tree loop_control(TT_Stream &ts) {
  RULE(SG_LOOP_CONTROL);
  constexpr auto p =
    seq(rule_tag,
        opt(TOK(TK_COMMA)),
        h_alts(h_seq(name(),
                     TOK(TK_EQUAL),
                     rule(int_expr),
                     TOK(TK_COMMA),
                     rule(int_expr),
                     opt(h_seq(TOK(TK_COMMA), rule(int_expr)))
                     ),
               h_seq(TOK(KW_WHILE), h_parens(rule(logical_expr))),
               h_seq(TOK(KW_CONCURRENT),
                     rule(concurrent_header),
                     rule(concurrent_locality))
               )
        );
  EVAL(SG_LOOP_CONTROL, p(ts));
}

//! R934: lower-bound-expr (9.7.1.1)
Stmt_Tree lower_bound_expr(TT_Stream &ts) {
  RULE(SG_LOWER_BOUND_EXPR);
  constexpr auto p =
    tag_if(rule_tag, rule(expr));
  EVAL(SG_LOWER_BOUND_EXPR, p(ts));
}

/* I:M */

//! LOCAL: macro-stmt (local)
/*! FIX This handles patterns of the form "name(.*)", which we assume to be
    unexpanded macros. This should be moved to an extension. */
Stmt_Tree macro_stmt(TT_Stream &ts) {
  RULE(SG_MACRO_STMT);
  constexpr auto p =
    seq(rule_tag,
        neg(peek(TAG(KW_ASSOCIATE))),
        neg(peek(TAG(KW_WHERE))),
        neg(peek(TAG(KW_FORALL))),
        neg(peek(TAG(KW_CASE))),
        name(),
        rule(consume_parens),
        eol());
  EVAL(SG_MACRO_STMT, p(ts));
}

//! R1047: masked-elsewhere-stmt (10.2.3.1)
Stmt_Tree masked_elsewhere_stmt(TT_Stream &ts) {
  RULE(SG_MASKED_ELSEWHERE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_ELSE), TOK(KW_WHERE),
        h_parens(rule(logical_expr)),
        opt(name()),
        eol());
  EVAL(SG_MASKED_ELSEWHERE_STMT, p(ts));
}

//! R1410: module-nature (14.2.2)
Stmt_Tree module_nature(TT_Stream &ts) {
  RULE(SG_MODULE_NATURE);
  constexpr auto p =
    alts(rule_tag, TOK(KW_INTRINSIC), TOK(KW_NON_INTRINSIC));
  EVAL(SG_MODULE_NATURE, p(ts));
}

//! R1405: module-stmt (14.2.1)
Stmt_Tree module_stmt(TT_Stream &ts) {
  RULE(SG_MODULE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_MODULE), name(), eol());
  EVAL(SG_MODULE_STMT, p(ts));
}

//! R1539: mp-subprogram-stmt (15.6.2.5)
Stmt_Tree mp_subprogram_stmt(TT_Stream &ts) {
  RULE(SG_MP_SUBPROGRAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_MODULE), TOK(KW_PROCEDURE),
        name(), eol());
  EVAL(SG_MP_SUBPROGRAM_STMT, p(ts));
}

//! R1008: mult-op (6.2.4)
Stmt_Tree mult_op(TT_Stream &ts) {
  RULE(SG_MULT_OP);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_ASTERISK),
         TOK(TK_SLASHF));
  EVAL(SG_MULT_OP, p(ts));
}

/* I:N */

//! R852: named-constant-def (8.6.11)
Stmt_Tree named_constant_def(TT_Stream &ts) {
  RULE(SG_NAMED_CONSTANT_DEF);
  constexpr auto p =
    seq(rule_tag, name(), TOK(TK_EQUAL), rule(expr));
  EVAL(SG_NAMED_CONSTANT_DEF, p(ts));
}

//! R868: namelist-stmt (8.9)
Stmt_Tree namelist_stmt(TT_Stream &ts) {
  RULE(SG_NAMELIST_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_NAMELIST), TOK(TK_SLASHF),
        name(), // namelist-group-name
        TOK(TK_SLASHF),
        list(TAG(SG_NAMELIST_GROUP_OBJECT_LIST),
             name()), // namelist-group-object
        star(h_seq(opt(TOK(TK_COMMA)),
                   TOK(TK_SLASHF),
                   name(), // namelist-group-name
                   TOK(TK_SLASHF),
                   list(TAG(SG_NAMELIST_GROUP_OBJECT_LIST),
                        name()))), // namelist-group-object
        eol());
  EVAL(SG_NAMELIST_STMT, p(ts));
}

//! R1122: nonlabel-do-stmt (11.1.7.2)
Stmt_Tree nonlabel_do_stmt(TT_Stream &ts) {
  RULE(SG_NONLABEL_DO_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_DO),
        opt(rule(loop_control)),
        eol());
  EVAL(SG_NONLABEL_DO_STMT, p(ts));
}

//! R938: nullify-stmt (9.7.2)
Stmt_Tree nullify_stmt(TT_Stream &ts) {
  RULE(SG_NULLIFY_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_NULLIFY),
        h_parens(list(TAG(SG_POINTER_OBJECT_LIST),
                      rule(pointer_object))),
        eol());
  EVAL(SG_NULLIFY_STMT, p(ts));
}

/* I:O */

//! R1412: only (14.2.2)
Stmt_Tree only(TT_Stream &ts) {
  RULE(SG_ONLY);
  constexpr auto p =
    alts(rule_tag,
         rule(rename), // this should be first to get the long sequence
         rule(generic_spec),
         rule(list_name));
  EVAL(SG_ONLY, p(ts));
}

//! TRUNCATED R1204: open-stmt (12.5.6.2)
Stmt_Tree open_stmt(TT_Stream &ts) {
  RULE(SG_OPEN_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_OPEN),
        rule(consume_parens),
        eol());
  EVAL(SG_OPEN_STMT, p(ts));
}

//! R850: optional-stmt (8.6.10)
Stmt_Tree optional_stmt(TT_Stream &ts) {
  RULE(SG_OPTIONAL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_OPTIONAL), opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_DUMMY_ARG_NAME_LIST), name()),
        eol());
  EVAL(SG_OPTIONAL_STMT, p(ts));
}

//! R513: other-specification-stmt (5.1)
Stmt_Tree other_specification_stmt(TT_Stream &ts) {
  RULE(SG_OTHER_SPECIFICATION_STMT);
  constexpr auto p =
    alts(rule_tag,
         rule(access_stmt),
         rule(allocatable_stmt),
         rule(asynchronous_stmt),
         rule(bind_stmt),
         rule(codimension_stmt),
         rule(dimension_stmt),
         rule(external_stmt),
         rule(intent_stmt),
         rule(intrinsic_stmt),
         rule(namelist_stmt),
         rule(optional_stmt),
         rule(pointer_stmt),
         rule(protected_stmt),
         rule(save_stmt),
         rule(target_stmt),
         rule(volatile_stmt),
         rule(value_stmt),
         rule(common_stmt),
         rule(equivalence_stmt)
         );
  auto res = p(ts);
  if(!res.match) {
    res = get_parser_exts().parse_other_specification_stmt(ts);
  }

  EVAL(SG_OTHER_SPECIFICATION_STMT, res);
}

//! R1217: output-item (12.6.3)
Stmt_Tree output_item(TT_Stream &ts) {
  RULE(SG_OUTPUT_ITEM);
  constexpr auto p =
    alts(rule_tag,
         rule(io_implied_do),
         rule(expr));
  EVAL(SG_OUTPUT_ITEM, p(ts));
}

/* I:P */

//! R851: parameter-stmt (8.6.11)
Stmt_Tree parameter_stmt(TT_Stream &ts) {
  RULE(SG_PARAMETER_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_PARAMETER),
        h_parens(list(TAG(SG_NAMED_CONSTANT_DEF_LIST),
                     rule(named_constant_def))),
        eol()
        );
  EVAL(SG_PARAMETER_STMT, p(ts));
}

//! R909: parent-string (9.4.1)
Stmt_Tree parent_string(TT_Stream &ts) {
  RULE(SG_PARENT_STRING);
  constexpr auto p =
    alts(rule_tag,
         rule(array_element),
         rule(coindexed_named_object),
         rule(structure_component),
         TOK(SG_CHAR_LITERAL_CONSTANT),
         name()  // scalar-variable-name
         );
  EVAL(SG_PARENT_STRING, p(ts));
}

//! R912: part-ref (9.4.2)
Stmt_Tree part_ref(TT_Stream &ts) {
  RULE(SG_PART_REF);
  constexpr auto p =
    seq(rule_tag,
        name(),
        opt(h_parens(list(TAG(SG_SECTION_SUBSCRIPT_LIST),
                          rule(section_subscript)))),
        opt(rule(image_selector))
        );
  EVAL(SG_PART_REF, p(ts));
}

//! R1033: pointer-assignment-stmt (10.2.2.2)
Stmt_Tree pointer_assignment_stmt(TT_Stream &ts) {
  RULE(SG_POINTER_ASSIGNMENT_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(rule(data_pointer_object),
               h_parens(list(TAG(SG_BOUNDS_REMAPPING_LIST),
                             rule(bounds_remapping))),
               TOK(TK_ARROW),
               tag_if(TAG(SG_DATA_TARGET), rule(expr)),
               eol()),
         h_seq(rule(data_pointer_object),
               opt(h_parens(list(TAG(SG_BOUNDS_SPEC_LIST),
                                 rule(bounds_spec)))),
               TOK(TK_ARROW),
               tag_if(TAG(SG_DATA_TARGET), rule(expr)),
               eol()),
         h_seq(rule(proc_pointer_object),
               TOK(TK_ARROW),
               rule(proc_target),
               eol())
         );
  EVAL(SG_POINTER_ASSIGNMENT_STMT, p(ts));
}

//! R854: pointer-decl (8.6.12)
Stmt_Tree pointer_decl(TT_Stream &ts) {
  RULE(SG_POINTER_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(),  // object-name or proc-entity-name
        opt(h_parens(h_list(TOK(TK_COLON)))) // deferred-shape-spec-list
        );
  EVAL(SG_POINTER_DECL, p(ts));
}

//! R939: pointer-object (9.7.2)
Stmt_Tree pointer_object(TT_Stream &ts) {
  RULE(SG_POINTER_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         rule(structure_component),  // check the long rule first
         name()  // variable-name
         // proc-pointer-name (equiv to previous alt)
         );
  EVAL(SG_POINTER_OBJECT, p(ts));
}

//! R853: pointer-stmt (8.6.12)
Stmt_Tree pointer_stmt(TT_Stream &ts) {
  RULE(SG_POINTER_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_POINTER),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_POINTER_DECL_LIST),
             rule(pointer_decl)),
        eol()
        );
  EVAL(SG_POINTER_STMT, p(ts));
}

//! R1526: prefix (15.6.2.1)
Stmt_Tree prefix(TT_Stream &ts) {
  RULE(SG_PREFIX);
  constexpr auto p =
    tag_if(rule_tag, star(rule(prefix_spec)));
  EVAL(SG_PREFIX, p(ts));
}

//! R1527: prefix-spec (15.6.2.1)
Stmt_Tree prefix_spec(TT_Stream &ts) {
  RULE(SG_PREFIX_SPEC);
  constexpr auto p =
    alts(rule_tag,
         TOK(KW_ELEMENTAL),
         TOK(KW_IMPURE),
         TOK(KW_MODULE),
         TOK(KW_NON_RECURSIVE),
         TOK(KW_PURE),
         TOK(KW_RECURSIVE),
         rule(declaration_type_spec)
         );
  EVAL(SG_PREFIX_SPEC, p(ts));
}

//! R1212: print-stmt (12.6.1)
Stmt_Tree print_stmt(TT_Stream &ts) {
  RULE(SG_PRINT_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_PRINT), rule(format),
        opt(h_seq(TOK(TK_COMMA), list(TAG(SG_OUTPUT_ITEM_LIST),
                                      rule(output_item)))),
        eol());
  EVAL(SG_PRINT_STMT, p(ts));
}

//! R745: private-components-stmt (7.5.4)
Stmt_Tree private_components_stmt(TT_Stream &ts) {
  RULE(SG_PRIVATE_COMPONENTS_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PRIVATE),
        eol());
  EVAL(SG_PRIVATE_COMPONENTS_STMT, p(ts));
}

//! R729: private-or-sequence (7.5.2.1)
Stmt_Tree private_or_sequence(TT_Stream &ts) {
  RULE(SG_PRIVATE_OR_SEQUENCE);
  constexpr auto p =
    alts(rule_tag,
         rule(private_components_stmt),
         rule(sequence_stmt));
  EVAL(SG_PRIVATE_OR_SEQUENCE, p(ts));
}

//! R1514: proc-attr-spec (15.4.3.6)
Stmt_Tree proc_attr_spec(TT_Stream &ts) {
  RULE(SG_PROC_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(access_spec),
         rule(proc_language_binding_spec),
         h_seq(TOK(KW_INTENT), h_parens(rule(intent_spec))),
         TOK(KW_OPTIONAL),
         TOK(KW_POINTER),
         TOK(KW_PROTECTED),
         TOK(KW_SAVE));
  EVAL(SG_PROC_ATTR_SPEC, p(ts));
}

//! R742: proc-component-attr-spec (7.5.4.1)
Stmt_Tree proc_component_attr_spec(TT_Stream &ts) {
  RULE(SG_PROC_COMPONENT_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(access_spec),
         TOK(KW_NOPASS),
         h_seq(TOK(KW_PASS), opt(h_parens(name()))), // arg-name
         TOK(KW_POINTER));
  EVAL(SG_PROC_COMPONENT_ATTR_SPEC, p(ts));
}

//! R741: proc-component-def-stmt (7.5.4.1)
Stmt_Tree proc_component_def_stmt(TT_Stream &ts) {
  RULE(SG_PROC_COMPONENT_DEF_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PROCEDURE),
        h_parens(opt(rule(proc_interface))),
        TOK(TK_COMMA),
        list(TAG(SG_PROC_COMPONENT_ATTR_SPEC_LIST),
             rule(proc_component_attr_spec)),
        TOK(TK_DBL_COLON),
        list(TAG(SG_PROC_DECL_LIST),
             rule(proc_decl)),
        eol());
  EVAL(SG_PROC_COMPONENT_DEF_STMT, p(ts));
}

//! R1039: proc-component-ref (10.2.2.2)
Stmt_Tree proc_component_ref(TT_Stream &ts) {
  RULE(SG_PROC_COMPONENT_REF);
  /* The real rule is:
     scalar-variable % procedure-component-name
     But without the ability to do left recursion, the variable clause 
     consumes the entire thing.
  */
  constexpr auto p =
    seq(rule_tag,
        rule(variable) // scalar-variable
        // TOK(TK_PERCENT),
        // procedure-component-name
        );
  EVAL(SG_PROC_COMPONENT_REF, p(ts));
}

//! R1515: proc-decl (15.4.3.6)
Stmt_Tree proc_decl(TT_Stream &ts) {
  RULE(SG_PROC_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(), // procedure-entity-name
        opt(h_seq(TOK(TK_ARROW),
                  rule(proc_pointer_init)))
        );
  EVAL(SG_PROC_DECL, p(ts));
}

//! R1513: proc-interface (15.4.3.6)
Stmt_Tree proc_interface(TT_Stream &ts) {
  RULE(SG_PROC_INTERFACE);
  constexpr auto p =
    alts(rule_tag,
         name(), // interface-name
         rule(declaration_type_spec));
  EVAL(SG_PROC_INTERFACE, p(ts));
}

//! R1528: proc-language-binding-spec (15.6.2.1)
Stmt_Tree proc_language_binding_spec(TT_Stream &ts) {
  RULE(SG_PROC_LANGUAGE_BINDING_SPEC);
  constexpr auto p =
    tag_if(rule_tag, rule(language_binding_spec));
  EVAL(SG_PROC_LANGUAGE_BINDING_SPEC, p(ts));
}

//! R1517: proc-pointer-init (15.4.3.6)
Stmt_Tree proc_pointer_init(TT_Stream &ts) {
  RULE(SG_PROC_POINTER_INIT);
  constexpr auto p =
    alts(rule_tag,
         tag_if(TAG(SG_NULL_INIT), rule(function_reference)),
         name() // initial-proc-target
         );
  EVAL(SG_PROC_POINTER_INIT, p(ts));
}

//! R1038: proc-pointer-object (10.2.2.2)
Stmt_Tree proc_pointer_object(TT_Stream &ts) {
  RULE(SG_PROC_POINTER_OBJECT);
  constexpr auto p =
    alts(rule_tag,
         tag_if(TAG(SG_PROC_POINTER_NAME),
                h_seq(name(), neg(TOK(TK_PERCENT)))),
         rule(proc_component_ref));
  EVAL(SG_PROC_POINTER_OBJECT, p(ts));
}

//! R1040: proc-target (10.2.2.2)
Stmt_Tree proc_target(TT_Stream &ts) {
  RULE(SG_PROC_TARGET);
  constexpr auto p =
    alts(rule_tag,
         rule(expr),
         name(), // procedure-name
         rule(proc_component_ref));
  EVAL(SG_PROC_TARGET, p(ts));
}

//! R1512: procedure-declaration-stmt (15.4.3.6)
Stmt_Tree procedure_declaration_stmt(TT_Stream &ts) {
  RULE(SG_PROCEDURE_DECLARATION_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PROCEDURE),
        h_parens(opt(rule(proc_interface))),
        opt(h_seq(star(h_seq(TOK(TK_COMMA),
                             rule(proc_attr_spec))),
                  TOK(TK_DBL_COLON))),
        list(TAG(SG_PROC_DECL_LIST),
             rule(proc_decl)),
        eol()
        );
  EVAL(SG_PROCEDURE_DECLARATION_STMT, p(ts));
}

//! R1520: procedure-designator (15.5.1)
Stmt_Tree procedure_designator(TT_Stream &ts) {
  RULE(SG_PROCEDURE_DESIGNATOR);

  /* This is unfortunately tricky, and this implementation will only really work
     for call-stmt.  The problem is that it is impossible to distinguish between
     a paren delimited actual-arg-spec-list at the end of a call-stmt rule, from
     a section-subset-list in a part-ref.  So, we try chopping off the last
     paren-delimited term, and see if we can still match.  However, it isn't a
     full data-ref, but more like: (<part-ref> '%')* <name>, which will math all
     three definitions of procedure-designator, but is unable to distinguish
     between them. */

  /* We start by creating a TT_Stream::Capture that should cover the
     procedure-designator by: */

  //     - Capture where we start
  auto designator = ts.capture_begin();

  //     - Move to the last token
  ts.consume_until_eol();

  //     - Assuming that the token is ')', move back to preceding matching '('
  bool move_okay = ts.move_to_open_paren();

  //     - If we found one, curr is on '(', move back one so peek() is '('
  if (move_okay)
    ts.put_back();

  //     - Mark this as the end of the (possible) designator sequence
  ts.capture_end(designator);

  //     - Create a stream that covers that token range
  TT_Stream designator_ts(ts.capture_to_range(designator));

  //     - Make a parser that can match our designator rule
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(star(h_seq(rule(part_ref),
                             TOK(TK_PERCENT))))),
        name(),
        eol()  // This is the EOL for the sub-stream
        );
  EVAL(SG_PROCEDURE_DESIGNATOR, p(designator_ts));
}

//! R1506: procedure-stmt (15.4.3.2)
Stmt_Tree procedure_stmt(TT_Stream &ts) {
  RULE(SG_PROCEDURE_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(TOK(KW_MODULE)),
        TOK(KW_PROCEDURE),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_SPECIFIC_PROCEDURE_LIST),
             tag_if(TAG(SG_SPECIFIC_PROCEDURE), name())),
        eol());
  EVAL(SG_PROCEDURE_STMT, p(ts));
}

//! R1401: program-stmt (14.1)
Stmt_Tree program_stmt(TT_Stream &ts) {
  RULE(SG_PROGRAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PROGRAM),
        name(),  // program-name
        eol());
  EVAL(SG_PROGRAM_STMT, p(ts));
}

//! R855: protected-stmt (8.6.13)
Stmt_Tree protected_stmt(TT_Stream &ts) {
  RULE(SG_PROTECTED_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_PROTECTED),
        opt(TOK(TK_DBL_COLON)),
        h_list(name()),
        eol());
  EVAL(SG_PROTECTED_STMT, p(ts));
}

/* I:Q */

/* I:R */

//! TRUNCATED R1210: read-stmt (12.6.1)
Stmt_Tree read_stmt(TT_Stream &ts) {
  RULE(SG_READ_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_READ),
        h_alts(
            // This first alternative fakes the io-control-spec-list
            h_seq(tag_if(TAG(SG_IO_CONTROL_SPEC_LIST),
                         rule(consume_parens)),
                  opt(list(TAG(SG_INPUT_ITEM_LIST),
                           rule(input_item)))),
            h_seq(rule(format),
                  opt(h_seq(TOK(TK_COMMA),
                            list(TAG(SG_INPUT_ITEM_LIST),
                                 rule(input_item)))))),
        eol());
  EVAL(SG_READ_STMT, p(ts));
}

//! R1031: rel-op (6.2.4)
Stmt_Tree rel_op(TT_Stream &ts) {
  RULE(SG_REL_OP);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_REL_LT),  // the lexer merges .LT. and <
         TOK(TK_REL_LE),  // similar for the rest of these
         TOK(TK_REL_GT),
         TOK(TK_REL_GE));
  EVAL(SG_REL_OP, p(ts));
}

//! R1411: rename (14.2.2)
Stmt_Tree rename(TT_Stream &ts) {
  RULE(SG_RENAME);
  constexpr auto p =
    alts(rule_tag,
         h_seq(name(), TOK(TK_ARROW), name()),
         h_seq(TOK(KW_OPERATOR), h_parens(TOK(TK_DEF_OP)),
               TOK(TK_ARROW),
               TOK(KW_OPERATOR), h_parens(TOK(TK_DEF_OP)) ));
  EVAL(SG_RENAME, p(ts));
}

//! R1542: return-stmt (15.6.2.7)
Stmt_Tree return_stmt(TT_Stream &ts) {
  RULE(SG_RETURN_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_RETURN), opt(rule(int_expr)), eol());
  EVAL(SG_RETURN_STMT, p(ts));
}

//! TRUNCATED R1226: rewind-stmt (12.8.1)
Stmt_Tree rewind_stmt(TT_Stream &ts) {
  RULE(SG_REWIND_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_REWIND),
        h_alts(rule(int_expr),
               rule(consume_parens)),
        eol());
  EVAL(SG_REWIND_STMT, p(ts));
}

/* I:S */

//! R856: save-stmt (8.6.14)
Stmt_Tree save_stmt(TT_Stream &ts) {
  RULE(SG_SAVE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SAVE),
        opt(h_seq(opt(TOK(TK_DBL_COLON)),
                  list(TAG(SG_SAVED_ENTITY_LIST),
                       rule(saved_entity)))),
        eol()
        );
  EVAL(SG_SAVE_STMT, p(ts));
}

//! R857: saved-entity (8.6.14)
Stmt_Tree saved_entity(TT_Stream &ts) {
  RULE(SG_SAVED_ENTITY);
  constexpr auto p =
    alts(rule_tag,
         name(), // object-name
         // proc-pointer-name,
         h_seq(TOK(TK_SLASHF), name(), TOK(TK_SLASHF)) /* /common-block-name/ */
         );
  EVAL(SG_SAVED_ENTITY, p(ts));
}

//! R920: section-subscript (9.5.3.1)
Stmt_Tree section_subscript(TT_Stream &ts) {
  RULE(SG_SECTION_SUBSCRIPT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(opt(rule(int_expr)),
               TOK(TK_COLON),
               opt(rule(int_expr)),
               opt(h_seq(TOK(TK_COLON), rule(int_expr)))),
         rule(int_expr));
  EVAL(SG_SECTION_SUBSCRIPT, p(ts));
}

//! R1141: select-case-stmt (11.1.9.1)
Stmt_Tree select_case_stmt(TT_Stream &ts) {
  RULE(SG_SELECT_CASE_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))), // case-construct-name
        TOK(KW_SELECT), TOK(KW_CASE),
        h_parens(tag_if(TAG(SG_CASE_EXPR), rule(expr))),
        eol()
        );
  EVAL(SG_SELECT_CASE_STMT, p(ts));
}

//! R1149: select-rank-stmt (11.1.10.1)
Stmt_Tree select_rank_stmt(TT_Stream &ts) {
  RULE(SG_SELECT_RANK_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))), // select-construct-name
        TOK(KW_SELECT), TOK(KW_RANK),
        h_parens(h_seq(opt(h_seq(name(), TOK(TK_ARROW))), // associate-name
                       rule(selector))),
        eol());
  EVAL(SG_SELECT_RANK_STMT, p(ts));
}

//! R1150: select-rank-case-stmt (11.1.10.1)
Stmt_Tree select_rank_case_stmt(TT_Stream &ts) {
  RULE(SG_SELECT_RANK_CASE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_RANK),
        h_alts(TOK(KW_DEFAULT),
               h_parens(TOK(TK_ASTERISK)),
               h_parens(rule(int_constant_expr))),
        eol());
  EVAL(SG_SELECT_RANK_CASE_STMT, p(ts));
}

//! R1152: select-type-stmt (11.1.11.1)
Stmt_Tree select_type_stmt(TT_Stream &ts) {
  RULE(SG_SELECT_TYPE_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))), // select-construct-name
        TOK(KW_SELECT), TOK(KW_TYPE),
        h_parens(h_seq(opt(h_seq(name(), TOK(TK_ARROW))), // associate-name
                       rule(selector))),
        eol());
  EVAL(SG_SELECT_TYPE_STMT, p(ts));
}

//! R1105: selector (11.1.3.1)
Stmt_Tree selector(TT_Stream &ts) {
  RULE(SG_SELECTOR);
  constexpr auto p =
    alts(rule_tag,
         rule(expr),      // do the long one first
         rule(variable));
  EVAL(SG_SELECTOR, p(ts));
}

//! R731: sequence-stmt (7.6.2.3)
Stmt_Tree sequence_stmt(TT_Stream &ts) {
  RULE(SG_SEQUENCE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SEQUENCE),
        eol());
  EVAL(SG_SEQUENCE_STMT, p(ts));
}

//! R712: sign (7.4.3.1)
Stmt_Tree sign(TT_Stream &ts) {
  RULE(SG_SIGN);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_PLUS),
         TOK(TK_MINUS));
  EVAL(SG_SIGN, p(ts));
}

//! R1160: stop-stmt (11.4)
Stmt_Tree stop_stmt(TT_Stream &ts) {
  RULE(SG_STOP_STMT);
  constexpr auto p =
    seq(rule_tag,
         TOK(KW_STOP),
         opt(rule(default_char_expr)),
         opt(h_seq(TOK(TK_COMMA),
                   TOK(KW_QUIET),
                   TOK(TK_EQUAL),
                   rule(logical_expr))),
         eol());
  EVAL(SG_STOP_STMT, p(ts));
}

//! R913: structure-component (9.4.2)
/*! structure-component is a limited form of data-ref, where there must be at
  least one % and the last part-ref cannot have a section-subscript-list */
Stmt_Tree structure_component(TT_Stream &ts) {
  RULE(SG_STRUCTURE_COMPONENT);
  constexpr auto p =
    seq(rule_tag,
        rule(part_ref), TOK(TK_PERCENT),
        star(h_seq(rule(part_ref), TOK(TK_PERCENT))),
        name(),
        opt(rule(image_selector)));
  EVAL(SG_STRUCTURE_COMPONENT, p(ts));
}

//! R756: structure-constructor (7.5.10)
Stmt_Tree structure_constructor(TT_Stream &ts) {
  RULE(SG_STRUCTURE_CONSTRUCTOR);
  constexpr auto p =
    seq(rule_tag,
        rule(derived_type_spec),
        h_parens(opt(list(TAG(SG_COMPONENT_SPEC_LIST),
                          rule(component_spec)))));
  EVAL(SG_STRUCTURE_CONSTRUCTOR, p(ts));
}

//! R1416: submodule-stmt (14.2.3)
Stmt_Tree submodule_stmt(TT_Stream &ts) {
  RULE(SG_SUBMODULE_STMT);
  constexpr auto p =
    seq(rule_tag, TOK(KW_SUBMODULE), TOK(TK_PARENL),
        name(), opt(h_seq(TOK(TK_COLON), name())),
        TOK(TK_PARENR),
        eol());
  EVAL(SG_SUBMODULE_STMT, p(ts));
}

//! R1535: subroutine-stmt (15.6.2.3)
Stmt_Tree subroutine_stmt(TT_Stream &ts) {
  RULE(SG_SUBROUTINE_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(rule(prefix)),
        TOK(KW_SUBROUTINE),
        name(),
        opt(h_seq(h_parens(opt(list(TAG(SG_DUMMY_ARG_LIST),
                                    rule(dummy_arg))))
                  )
            ),
        opt(rule(proc_language_binding_spec)),
        eol()
        );
  EVAL(SG_SUBROUTINE_STMT, p(ts));
}

//! R908: substring (9.4.1)
Stmt_Tree substring(TT_Stream &ts) {
  RULE(SG_SUBSTRING);
  constexpr auto p =
    seq(rule_tag,
        rule(parent_string), h_parens(rule(substring_range)));
  EVAL(SG_SUBSTRING, p(ts));
}

//! R910: substring-range (9.4.1)
Stmt_Tree substring_range(TT_Stream &ts) {
  RULE(SG_SUBSTRING_RANGE);
  constexpr auto p =
    seq(rule_tag,
        opt(rule(int_expr)),
        TOK(TK_COLON),
        opt(rule(int_expr)));
  EVAL(SG_SUBSTRING_RANGE, p(ts));
}

//! R1532: suffix (15.6.2.2)
Stmt_Tree suffix(TT_Stream &ts) {
  RULE(SG_SUFFIX);
#define RESULT h_seq(TOK(KW_RESULT), h_parens(name()))  
  constexpr auto p =
    alts(rule_tag,
         h_seq(rule(proc_language_binding_spec), opt(RESULT)),
         h_seq(RESULT, opt(rule(proc_language_binding_spec)))
         );
  EVAL(SG_SUFFIX, p(ts));
#undef RESULT
}

//! R1164: sync-all-stmt (11.6.3)
Stmt_Tree sync_all_stmt(TT_Stream &ts) {
  RULE(SG_SYNC_ALL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SYNC),
        TOK(KW_ALL),
        opt(h_parens(opt(h_list(rule(sync_stat))))),
        eol());
  EVAL(SG_SYNC_ALL_STMT, p(ts));
}

//! R1164: sync-images-stmt (11.6.3)
Stmt_Tree sync_images_stmt(TT_Stream &ts) {
  RULE(SG_SYNC_ALL_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SYNC),
        TOK(KW_IMAGES),
        h_parens(h_alts(rule(expr),  // image-set
                        TOK(TK_ASTERISK)),
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(rule(sync_stat))))
                 ),
        eol()
        );
  EVAL(SG_SYNC_IMAGES_STMT, p(ts));
}

//! R1168: sync-memory-stmt (11.6.5)
Stmt_Tree sync_memory_stmt(TT_Stream &ts) {
  RULE(SG_SYNC_MEMORY_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SYNC),
        TOK(KW_MEMORY),
        opt(h_parens(opt(h_list(rule(sync_stat))))),
        eol());
  EVAL(SG_SYNC_ALL_STMT, p(ts));
}

//! R1165: sync-stat (11.6.3)
Stmt_Tree sync_stat(TT_Stream &ts) {
  RULE(SG_SYNC_STAT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_STAT), TOK(TK_EQUAL), rule(variable)),
         h_seq(TOK(KW_ERRMSG), TOK(TK_EQUAL), rule(variable))
         );
  EVAL(SG_SYNC_STAT, p(ts));
}

//! R1169: sync-team-stmt (11.6.6)
Stmt_Tree sync_team_stmt(TT_Stream &ts) {
  RULE(SG_SYNC_TEAM_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_SYNC),
        TOK(KW_TEAM),
        h_parens(rule(expr),  // team-value
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(rule(sync_stat))))
                 ),
        eol()
        );
  EVAL(SG_SYNC_TEAM_STMT, p(ts));
}


/* I:T */

//! R860: target-decl (8.6.15)
Stmt_Tree target_decl(TT_Stream &ts) {
  RULE(SG_TARGET_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(),  // object-name
        opt(h_parens(rule(array_spec))),
        opt(h_brackets(rule(coarray_spec))));
  EVAL(SG_TARGET_DECL, p(ts));
}

//! R859: target-stmt (8.6.15)
Stmt_Tree target_stmt(TT_Stream &ts) {
  RULE(SG_TARGET_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_TARGET),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_TARGET_DECL_LIST), rule(target_decl)),
        eol());
  EVAL(SG_TARGET_STMT, p(ts));
}

//! R728: type-attr-spec (7.5.2)
Stmt_Tree type_attr_spec(TT_Stream &ts) {
  RULE(SG_TYPE_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         TOK(KW_ABSTRACT),
         rule(access_spec),
         rule(bind_c),
         h_seq(TOK(KW_EXTENDS), h_parens(name())) // parent-type-name
         );
  EVAL(SG_TYPE_ATTR_SPEC, p(ts));
}

//! R751: type-bound-generic-stmt (7.5.5)
Stmt_Tree type_bound_generic_stmt(TT_Stream &ts) {
  RULE(SG_TYPE_BOUND_GENERIC_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_GENERIC),
        opt(h_seq(TOK(TK_COMMA), rule(access_spec))),
        TOK(TK_DBL_COLON),
        rule(generic_spec),
        TOK(TK_ARROW),
        list(TAG(SG_BINDING_NAME_LIST),
             name()),
        eol());
  EVAL(SG_TYPE_BOUND_GENERIC_STMT, p(ts));
}

//! R748: type-bound-proc-binding (7.5.5)
Stmt_Tree type_bound_proc_binding(TT_Stream &ts) {
  RULE(SG_TYPE_BOUND_PROC_BINDING);
  constexpr auto p =
    alts(rule_tag,
         rule(type_bound_procedure_stmt),
         rule(type_bound_generic_stmt),
         rule(final_procedure_stmt));
  EVAL(SG_TYPE_BOUND_PROC_BINDING, p(ts));
}

//! R750: type-bound-proc-decl (7.5.5)
Stmt_Tree type_bound_proc_decl(TT_Stream &ts) {
  RULE(SG_TYPE_BOUND_PROC_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(), // binding-name
        opt(h_seq(TOK(TK_ARROW),
                  name())) // procedure-name
        );
  EVAL(SG_TYPE_BOUND_PROC_DECL, p(ts));
}

//! R749: type-bound-procedure-stmt (7.5.5)
Stmt_Tree type_bound_procedure_stmt(TT_Stream &ts) {
  RULE(SG_TYPE_BOUND_PROCEDURE_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_PROCEDURE),
               opt(h_seq(
                       opt(h_seq(TOK(TK_COMMA),
                                 list(TAG(SG_BINDING_ATTR_LIST),
                                      rule(binding_attr)))),
                       TOK(TK_DBL_COLON))),
               list(TAG(SG_TYPE_BOUND_PROC_DECL_LIST),
                    rule(type_bound_proc_decl)),
               eol()),
         h_seq(TOK(KW_PROCEDURE),
               h_parens(name()), // interface-name
               TOK(TK_COMMA),
               list(TAG(SG_BINDING_ATTR_LIST),
                    rule(binding_attr)),
               TOK(TK_DBL_COLON),
               list(TAG(SG_BINDING_NAME_LIST),
                    name()),
               eol())
         );
  EVAL(SG_TYPE_BOUND_PROCEDURE_STMT, p(ts));
}

//! R801: type-declaration-stmt (8.2)
Stmt_Tree type_declaration_stmt(TT_Stream &ts) {
  RULE(SG_TYPE_DECLARATION_STMT);
  constexpr auto p =
    alts(rule_tag,
         /* handle R722 + C725 and C726 */
         h_seq(TOK(KW_CHARACTER), TOK(TK_ASTERISK),
               rule(char_length), opt(TOK(TK_COMMA)),
               list(TAG(SG_ENTITY_DECL_LIST), rule(entity_decl)), eol()),
         /* normal R801 */
         h_seq(rule(declaration_type_spec),
               opt(h_seq(star(h_seq(TOK(TK_COMMA), rule(attr_spec))),
                         TOK(TK_DBL_COLON))),
               list(TAG(SG_ENTITY_DECL_LIST), rule(entity_decl)), eol())
         );
  EVAL(SG_TYPE_DECLARATION_STMT, p(ts));
}

//! R1154: type-guard-stmt (11.1.11.1)
Stmt_Tree type_guard_stmt(TT_Stream &ts) {
  RULE(SG_TYPE_GUARD_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_TYPE), TOK(KW_IS), h_parens(rule(type_spec)),
               opt(name()), eol()),  // select-construct-name
         h_seq(TOK(KW_CLASS), TOK(KW_IS), h_parens(rule(derived_type_spec)),
               opt(name()), eol()),  // select-construct-name
         h_seq(TOK(KW_CLASS), TOK(KW_DEFAULT),
               opt(name()), eol())   // select-construct-name
         );
  EVAL(SG_TYPE_GUARD_STMT, p(ts));
}

//! R734: type-param-attr-spec (7.5.3.1)
Stmt_Tree type_param_attr_spec(TT_Stream &ts) {
  RULE(SG_TYPE_PARAM_ATTR_SPEC);
  constexpr auto p =
    alts(rule_tag,
         TOK(KW_KIND),
         TOK(KW_LEN));
  EVAL(SG_TYPE_PARAM_ATTR_SPEC, p(ts));
}

//! R733: type-param-decl (7.5.3.1)
Stmt_Tree type_param_decl(TT_Stream &ts) {
  RULE(SG_TYPE_PARAM_DECL);
  constexpr auto p =
    seq(rule_tag,
        name(), // type-param-name
        opt(h_seq(TOK(TK_EQUAL),
                  rule(int_expr)))
        );
  EVAL(SG_TYPE_PARAM_DECL, p(ts));
}

//! R732: type-param-def-stmt (7.5.3.1)
Stmt_Tree type_param_def_stmt(TT_Stream &ts) {
  RULE(SG_TYPE_PARAM_DEF_STMT);
  constexpr auto p =
    seq(rule_tag,
        rule(integer_type_spec),
        TOK(TK_COMMA),
        rule(type_param_attr_spec),
        TOK(TK_DBL_COLON),
        list(TAG(SG_TYPE_PARAM_DECL_LIST), rule(type_param_decl)),
        eol());
  EVAL(SG_TYPE_PARAM_DEF_STMT, p(ts));
}
    
//! R755: type-param-spec (7.5.9)
Stmt_Tree type_param_spec(TT_Stream &ts) {
  RULE(SG_TYPE_PARAM_SPEC);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_EQUAL))),
        rule(type_param_value)
        );
  EVAL(SG_TYPE_PARAM_SPEC, p(ts));
}

//! R701: type-param-value (7.2)
Stmt_Tree type_param_value(TT_Stream &ts) {
  RULE(SG_TYPE_PARAM_VALUE);
  constexpr auto p =
    alts(rule_tag,
         TOK(TK_ASTERISK),
         TOK(TK_COLON),
         rule(int_expr)
         );
  EVAL(SG_TYPE_PARAM_VALUE, p(ts));
}

//! R702: type-spec (7.3.2.1)
Stmt_Tree type_spec(TT_Stream &ts) {
  RULE(SG_TYPE_SPEC);
  constexpr auto p =
    alts(rule_tag,
         rule(intrinsic_type_spec),
         rule(derived_type_spec));
  EVAL(SG_TYPE_SPEC, p(ts));
}

/* I:U */

//! R1181: unlock-stmt (11.6.10)
Stmt_Tree unlock_stmt(TT_Stream &ts) {
  RULE(SG_UNLOCK_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_UNLOCK),
        h_parens(rule(variable), // lock-variable
                 opt(h_seq(TOK(TK_COMMA),
                           h_list(rule(sync_stat))))),
        eol());
  EVAL(SG_UNLOCK_STMT, p(ts));
}

//! R935: upper-bound-expr (9.7.1.1)
Stmt_Tree upper_bound_expr(TT_Stream &ts) {
  RULE(SG_UPPER_BOUND_EXPR);
  constexpr auto p =
    tag_if(rule_tag, rule(expr));
  EVAL(SG_UPPER_BOUND_EXPR, p(ts));
}

//! R1409: use-stmt (14.2.2)
Stmt_Tree use_stmt(TT_Stream &ts) {
  RULE(SG_USE_STMT);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_USE),
               opt(h_seq(opt(h_seq(TOK(TK_COMMA), rule(module_nature))),
                         TOK(TK_DBL_COLON))),
               name(),
               opt(h_seq(TOK(TK_COMMA), list(TAG(SG_RENAME_LIST),
                                             rule(rename)))),
               eol()),
         h_seq(TOK(KW_USE),
               opt(h_seq(opt(h_seq(TOK(TK_COMMA), rule(module_nature))),
                         TOK(TK_DBL_COLON))),
               name(),
               TOK(TK_COMMA), TOK(KW_ONLY), TOK(TK_COLON),
               opt(list(TAG(SG_ONLY_LIST), rule(only))),
               eol())
         );
  EVAL(SG_USE_STMT, p(ts));
}

/* I:V */

//! R861: value-stmt (8.6.16)
Stmt_Tree value_stmt(TT_Stream &ts) {
  RULE(SG_VALUE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_VALUE),
        opt(TOK(TK_DBL_COLON)),
        h_list(name()), // dummy-arg-name-list
        eol());
  EVAL(SG_VALUE_STMT, p(ts));
}

//! R902: variable (9.2)
Stmt_Tree variable(TT_Stream &ts) {
  RULE(SG_VARIABLE);
  constexpr auto p =
    alts(rule_tag,
         // func-ref needs to be first to accept "name()"
         rule(function_reference),
         rule(designator)
         );
  EVAL(SG_VARIABLE, p(ts));
}


//! R862: volatile-stmt (8.6.17)
Stmt_Tree volatile_stmt(TT_Stream &ts) {
  RULE(SG_VOLATILE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_VOLATILE),
        opt(TOK(TK_DBL_COLON)),
        list(TAG(SG_OBJECT_NAME_LIST),
             name()),
        eol());
  EVAL(SG_VOLATILE_STMT, p(ts));
}

/* I:W */

//! R1223: wait-spec (12.7.2)
Stmt_Tree wait_spec(TT_Stream &ts) {
  RULE(SG_WAIT_SPEC);
  constexpr auto p =
    alts(rule_tag,
         h_seq(TOK(KW_UNIT), TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(KW_END), TOK(TK_EQUAL), rule(label)),
         h_seq(TOK(KW_EOR), TOK(TK_EQUAL), rule(label)),
         h_seq(TOK(KW_ERR), TOK(TK_EQUAL), rule(label)),
         h_seq(TOK(KW_ID), TOK(TK_EQUAL), rule(expr)),
         h_seq(TOK(KW_IOMSG), TOK(TK_EQUAL), rule(variable)),
         h_seq(TOK(KW_IOSTAT), TOK(TK_EQUAL), rule(variable)),
         // do the least-specific entry last: the optional unit=expr
         rule(expr));

  EVAL(SG_WAIT_SPEC, p(ts));
}

//! R1222: wait-stmt (12.7.2)
Stmt_Tree wait_stmt(TT_Stream &ts) {
  RULE(SG_WAIT_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_WAIT),
        h_parens(h_list(rule(wait_spec))),
        eol());
  EVAL(SG_WAIT_STMT, p(ts));
}

//! R1043: where-construct-stmt (10.2.3.1)
Stmt_Tree where_construct_stmt(TT_Stream &ts) {
  RULE(SG_WHERE_CONSTRUCT_STMT);
  constexpr auto p =
    seq(rule_tag,
        opt(h_seq(name(), TOK(TK_COLON))),
        TOK(KW_WHERE),
        h_parens(rule(logical_expr)),
        eol());
  EVAL(SG_WHERE_CONSTRUCT_STMT, p(ts));
}

//! R1041: where-stmt (10.2.3.1)
Stmt_Tree where_stmt(TT_Stream &ts) {
  RULE(SG_WHERE_STMT);
  constexpr auto p =
    seq(rule_tag,
        /* should be WHERE (mask-expr) where-assignment-stmt, but this is
           syntactically equivalent */
        TOK(KW_WHERE), h_parens(rule(logical_expr)),
        rule(assignment_stmt), eol());
  EVAL(SG_WHERE_STMT, p(ts));
}

//! R1211: write-stmt (12.6.1)
Stmt_Tree write_stmt(TT_Stream &ts) {
  RULE(SG_WRITE_STMT);
  constexpr auto p =
    seq(rule_tag,
        TOK(KW_WRITE),
            // This fakes the io-control-spec-list
        tag_if(TAG(SG_IO_CONTROL_SPEC_LIST),
               rule(consume_parens)),
        opt(list(TAG(SG_OUTPUT_ITEM_LIST),
                 rule(output_item))),
        eol());
  EVAL(SG_WRITE_STMT, p(ts));
}

/* I:X */
/* I:Y */
/* I:Z */

// clang-format on

Stmt_Tree parse_stmt_dispatch(int stmt_tag, TT_Stream &ts) {
  switch (stmt_tag) {

  case TAG(SG_ACCESS_STMT):
    return access_stmt(ts);
    break;
  case TAG(SG_ACTION_STMT):
    return action_stmt(ts);
    break;
  case TAG(SG_ALLOCATABLE_STMT):
    return allocatable_stmt(ts);
    break;
  case TAG(SG_ALLOCATE_STMT):
    return allocate_stmt(ts);
    break;
  case TAG(SG_ASSIGNMENT_STMT):
    return assignment_stmt(ts);
    break;
  case TAG(SG_ASSOCIATE_STMT):
    return associate_stmt(ts);
    break;
  case TAG(SG_ASYNCHRONOUS_STMT):
    return asynchronous_stmt(ts); 
    break;
  case TAG(SG_ARITHMETIC_IF_STMT):
    return arithmetic_if_stmt(ts);
    break;
  case TAG(SG_BACKSPACE_STMT):
    return backspace_stmt(ts);
    break;
  case TAG(SG_BIND_STMT):
    return bind_stmt(ts); 
    break;
  case TAG(SG_BINDING_PRIVATE_STMT):
    return binding_private_stmt(ts);
    break;
  case TAG(SG_BLOCK_STMT):
    return block_stmt(ts);
    break;
  case TAG(SG_CALL_STMT):
    return call_stmt(ts);
    break;
  case TAG(SG_CASE_STMT):
    return case_stmt(ts);
    break;
  case TAG(SG_CLOSE_STMT):
    return close_stmt(ts);
    break;
  case TAG(SG_CODIMENSION_STMT):
    return codimension_stmt(ts); 
    break;
  case TAG(SG_COMMON_STMT):
    return common_stmt(ts);
    break;
  case TAG(SG_COMPONENT_DEF_STMT):
    return component_def_stmt(ts);
    break;
  case TAG(SG_COMPUTED_GOTO_STMT):
    return computed_goto_stmt(ts);
    break;
  case TAG(SG_CONTAINS_STMT):
    return contains_stmt(ts);
    break;
  case TAG(SG_CONTINUE_STMT):
    return continue_stmt(ts);
    break;
  case TAG(SG_CYCLE_STMT):
    return cycle_stmt(ts);
    break;
  case TAG(SG_DATA_COMPONENT_DEF_STMT):
    return data_component_def_stmt(ts);
    break;
  case TAG(SG_DATA_STMT):
    return data_stmt(ts);
    break;
  case TAG(SG_DEALLOCATE_STMT):
    return deallocate_stmt(ts);
    break;
  case TAG(SG_DERIVED_TYPE_STMT):
    return derived_type_stmt(ts);
    break;
  case TAG(SG_DIMENSION_STMT):
    return dimension_stmt(ts);
    break;
  case TAG(SG_DO_STMT):
    return do_stmt(ts);
    break;
  case TAG(SG_ELSE_IF_STMT):
    return else_if_stmt(ts);
    break;
  case TAG(SG_ELSE_STMT):
    return else_stmt(ts);
    break;
  case TAG(SG_ELSEWHERE_STMT):
    return elsewhere_stmt(ts);
    break;
  case TAG(SG_END_ASSOCIATE_STMT):
    return end_associate_stmt(ts);
    break;
  case TAG(SG_END_BLOCK_STMT):
    return end_block_stmt(ts);
    break;
  case TAG(SG_END_DO_STMT):
    return end_do_stmt(ts);
    break;
  case TAG(SG_END_ENUM_STMT):
    return end_enum_stmt(ts);
    break;
  case TAG(SG_END_FORALL_STMT):
    return end_forall_stmt(ts);
    break;
  case TAG(SG_END_FUNCTION_STMT):
    return end_function_stmt(ts);
    break;
  case TAG(SG_END_IF_STMT):
    return end_if_stmt(ts);
    break;
  case TAG(SG_END_INTERFACE_STMT):
    return end_interface_stmt(ts);
    break;
  case TAG(SG_END_MODULE_STMT):
    return end_module_stmt(ts);
    break;
  case TAG(SG_END_MP_SUBPROGRAM_STMT):
    return end_mp_subprogram_stmt(ts);
    break;
  case TAG(SG_END_PROGRAM_STMT):
    return end_program_stmt(ts);
    break;
  case TAG(SG_END_SELECT_STMT):
    return end_select_stmt(ts);
    break;
  case TAG(SG_END_SELECT_RANK_STMT):
    return end_select_rank_stmt(ts);
    break;
  case TAG(SG_END_SELECT_TYPE_STMT):
    return end_select_type_stmt(ts);
    break;
  case TAG(SG_END_SUBMODULE_STMT):
    return end_submodule_stmt(ts);
    break;
  case TAG(SG_END_SUBROUTINE_STMT):
    return end_subroutine_stmt(ts);
    break;
  case TAG(SG_END_TYPE_STMT):
    return end_type_stmt(ts);
    break;
  case TAG(SG_END_WHERE_STMT):
    return end_where_stmt(ts);
    break;
  case TAG(SG_ENDFILE_STMT):
    return endfile_stmt(ts);
    break;
  case TAG(SG_ENUM_DEF_STMT):
    return enum_def_stmt(ts);
    break;
  case TAG(SG_ENUMERATOR_DEF_STMT):
    return enumerator_def_stmt(ts);
    break;
  case TAG(SG_EQUIVALENCE_STMT):
    return equivalence_stmt(ts);
    break;
  case TAG(SG_ENTRY_STMT):
    return entry_stmt(ts);
    break;
  case TAG(SG_ERROR_STOP_STMT):
    return error_stop_stmt(ts);
    break;
  case TAG(SG_EVENT_POST_STMT):
    return event_post_stmt(ts); 
    break;
  case TAG(SG_EVENT_WAIT_STMT):
    return event_wait_stmt(ts); 
    break;
  case TAG(SG_EXIT_STMT):
    return exit_stmt(ts);
    break;
  case TAG(SG_EXTERNAL_STMT):
    return external_stmt(ts);
    break;
  case TAG(SG_FAIL_IMAGE_STMT):
    return fail_image_stmt(ts); 
    break;
  case TAG(SG_FLUSH_STMT):
    return flush_stmt(ts);
    break;
  case TAG(SG_FORALL_ASSIGNMENT_STMT):
    return forall_assignment_stmt(ts);
    break;
  case TAG(SG_FORALL_CONSTRUCT_STMT):
    return forall_construct_stmt(ts);
    break;
  case TAG(SG_FORALL_STMT):
    return forall_stmt(ts);
    break;
  case TAG(SG_FORM_TEAM_STMT):
    return form_team_stmt(ts); 
    break;
  case TAG(SG_FORMAT_STMT):
    return format_stmt(ts);
    break;
  case TAG(SG_FINAL_PROCEDURE_STMT):
    return final_procedure_stmt(ts);
    break;
  case TAG(SG_FUNCTION_STMT):
    return function_stmt(ts);
    break;
  case TAG(SG_GENERIC_STMT):
    return generic_stmt(ts);
    break;
  case TAG(SG_GOTO_STMT):
    return goto_stmt(ts);
    break;
  case TAG(SG_IF_STMT):
    return if_stmt(ts);
    break;
  case TAG(SG_IF_THEN_STMT):
    return if_then_stmt(ts);
    break;
  case TAG(SG_IMPLICIT_STMT):
    return implicit_stmt(ts);
    break;
  case TAG(SG_IMPORT_STMT):
    return import_stmt(ts);
    break;
  case TAG(SG_INQUIRE_STMT):
    return inquire_stmt(ts);
    break;
  case TAG(SG_INTENT_STMT):
    return intent_stmt(ts);
    break;
  case TAG(SG_INTERFACE_STMT):
    return interface_stmt(ts);
    break;
  case TAG(SG_INTRINSIC_STMT):
    return intrinsic_stmt(ts);
    break;
  case TAG(SG_LABEL_DO_STMT):
    return label_do_stmt(ts);
    break;
  case TAG(SG_LOCK_STMT):
    return lock_stmt(ts); 
    break;
  case TAG(SG_LOOP_CONTROL):
    return loop_control(ts);
    break;
  case TAG(SG_MASKED_ELSEWHERE_STMT):
    return masked_elsewhere_stmt(ts);
    break;
  case TAG(SG_MACRO_STMT):
    return macro_stmt(ts);
    break;
  case TAG(SG_MODULE_STMT):
    return module_stmt(ts);
    break;
  case TAG(SG_MP_SUBPROGRAM_STMT):
    return mp_subprogram_stmt(ts);
    break;
  case TAG(SG_NAMELIST_STMT):
    return namelist_stmt(ts);
    break;
  case TAG(SG_NONLABEL_DO_STMT):
    return nonlabel_do_stmt(ts);
    break;
  case TAG(SG_NULLIFY_STMT):
    return nullify_stmt(ts);
    break;
  case TAG(SG_OPEN_STMT):
    return open_stmt(ts);
    break;
  case TAG(SG_OPTIONAL_STMT):
    return optional_stmt(ts);
    break;
  case TAG(SG_OTHER_SPECIFICATION_STMT):
    return other_specification_stmt(ts);
    break;
  case TAG(SG_PARAMETER_STMT):
    return parameter_stmt(ts);
    break;
  case TAG(SG_POINTER_ASSIGNMENT_STMT):
    return pointer_assignment_stmt(ts);
    break;
  case TAG(SG_POINTER_STMT):
    return pointer_stmt(ts);
    break;
  case TAG(SG_PRINT_STMT):
    return print_stmt(ts);
    break;
  case TAG(SG_PRIVATE_COMPONENTS_STMT):
    return private_components_stmt(ts);
    break;
  case TAG(SG_PROC_COMPONENT_DEF_STMT):
    return proc_component_def_stmt(ts);
    break;
  case TAG(SG_PROCEDURE_DECLARATION_STMT):
    return procedure_declaration_stmt(ts);
    break;
  case TAG(SG_PROCEDURE_STMT):
    return procedure_stmt(ts);
    break;
  case TAG(SG_PROGRAM_STMT):
    return program_stmt(ts);
    break;
  case TAG(SG_PROTECTED_STMT):
    return protected_stmt(ts);
    break;
  case TAG(SG_READ_STMT):
    return read_stmt(ts);
    break;
  case TAG(SG_RETURN_STMT):
    return return_stmt(ts);
    break;
  case TAG(SG_REWIND_STMT):
    return rewind_stmt(ts);
    break;
  case TAG(SG_SAVE_STMT):
    return save_stmt(ts);
    break;
  case TAG(SG_SELECT_CASE_STMT):
    return select_case_stmt(ts);
    break;
  case TAG(SG_SELECT_RANK_CASE_STMT):
    return select_rank_case_stmt(ts);
    break;
  case TAG(SG_SELECT_RANK_STMT):
    return select_rank_stmt(ts);
    break;
  case TAG(SG_SELECT_TYPE_STMT):
    return select_type_stmt(ts);
    break;
  case TAG(SG_SEQUENCE_STMT):
    return sequence_stmt(ts);
    break;
  case TAG(SG_STOP_STMT):
    return stop_stmt(ts);
    break;
  case TAG(SG_SUBMODULE_STMT):
    return submodule_stmt(ts);
    break;
  case TAG(SG_SUBROUTINE_STMT):
    return subroutine_stmt(ts);
    break;
  case TAG(SG_SYNC_ALL_STMT):
    return sync_all_stmt(ts); 
    break;
  case TAG(SG_SYNC_IMAGES_STMT):
    return sync_images_stmt(ts); 
    break;
  case TAG(SG_SYNC_MEMORY_STMT):
    return sync_memory_stmt(ts); 
    break;
  case TAG(SG_SYNC_TEAM_STMT):
    return sync_team_stmt(ts);
    break;
  case TAG(SG_TARGET_STMT):
    return target_stmt(ts); 
    break;
  case TAG(SG_TYPE_BOUND_GENERIC_STMT):
    return type_bound_generic_stmt(ts);
    break;
  case TAG(SG_TYPE_BOUND_PROCEDURE_STMT):
    return type_bound_procedure_stmt(ts);
    break;
  case TAG(SG_TYPE_DECLARATION_STMT):
    return type_declaration_stmt(ts);
    break;
  case TAG(SG_TYPE_GUARD_STMT):
    return type_guard_stmt(ts);
    break;
  case TAG(SG_TYPE_PARAM_DEF_STMT):
    return type_param_def_stmt(ts);
    break;
  case TAG(SG_UNLOCK_STMT):
    return unlock_stmt(ts); 
    break;
  case TAG(SG_USE_STMT):
    return use_stmt(ts);
    break;
  case TAG(SG_VALUE_STMT):
    return value_stmt(ts);
    break;
  case TAG(SG_VOLATILE_STMT):
    return volatile_stmt(ts);
    break;
  case TAG(SG_WAIT_STMT):
    return wait_stmt(ts); 
    break;
  case TAG(SG_WHERE_CONSTRUCT_STMT):
    return where_construct_stmt(ts);
    break;
  case TAG(SG_WHERE_STMT):
    return where_stmt(ts);
    break;
  case TAG(SG_WRITE_STMT):
    return write_stmt(ts);
    break;
  default:
    FLPR::Syntax_Tags::print(
        std::cerr << "parse_stmt_dispatch: no handler for ", stmt_tag)
        << std::endl;
  }
  return Stmt_Tree();
}

bool is_action_stmt(int const syntag) {
  bool res{false};
  switch (syntag) {
  case TAG(SG_ALLOCATE_STMT):
  case TAG(SG_ASSIGNMENT_STMT):
  case TAG(SG_BACKSPACE_STMT):
  case TAG(SG_CALL_STMT):
  case TAG(SG_CLOSE_STMT):
  case TAG(SG_CONTINUE_STMT):
  case TAG(SG_CYCLE_STMT):
  case TAG(SG_DEALLOCATE_STMT):
  case TAG(SG_ENDFILE_STMT):
  case TAG(SG_ERROR_STOP_STMT):
  case TAG(SG_EVENT_POST_STMT):
  case TAG(SG_EVENT_WAIT_STMT):
  case TAG(SG_EXIT_STMT):
  case TAG(SG_FAIL_IMAGE_STMT):
  case TAG(SG_FLUSH_STMT):
  case TAG(SG_FORM_TEAM_STMT):
  case TAG(SG_GOTO_STMT):
  case TAG(SG_IF_STMT):
  case TAG(SG_INQUIRE_STMT):
  case TAG(SG_LOCK_STMT):
  case TAG(SG_NULLIFY_STMT):
  case TAG(SG_OPEN_STMT):
  case TAG(SG_POINTER_ASSIGNMENT_STMT):
  case TAG(SG_PRINT_STMT):
  case TAG(SG_READ_STMT):
  case TAG(SG_RETURN_STMT):
  case TAG(SG_REWIND_STMT):
  case TAG(SG_STOP_STMT):
  case TAG(SG_SYNC_ALL_STMT):
  case TAG(SG_SYNC_IMAGES_STMT):
  case TAG(SG_SYNC_MEMORY_STMT):
  case TAG(SG_SYNC_TEAM_STMT):
  case TAG(SG_UNLOCK_STMT):
  case TAG(SG_WAIT_STMT):
  case TAG(SG_WHERE_STMT):
  case TAG(SG_WRITE_STMT):
  case TAG(SG_COMPUTED_GOTO_STMT):
  case TAG(SG_ARITHMETIC_IF_STMT):
  case TAG(SG_FORALL_STMT):
  case TAG(SG_MACRO_STMT):
    res = true;
    break;
  default:
    res = false;
  }
  return res;
}

#undef FAIL
#undef INIT_FAIL
#undef RULE
#undef TAG
#undef TOK
} // namespace Stmt
} // namespace FLPR
#undef TRACE_SG
