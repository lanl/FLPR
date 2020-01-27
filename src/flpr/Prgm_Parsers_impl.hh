/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Prgm_Parsers_impl.hh

  TEMPLATE CLASS IMPLEMENTATION

  Note that this gets included into Prgm_Parsers.hh after the class definition.

  These are the rules for the "program" or "vertical" grammar that organizes
  statments into program blocks.
*/

//! Shorthand for defining a Parser rule
#define PPARSER(P)                                                             \
  template <typename Node_Data>                                                \
  auto Parsers<Node_Data>::P(State &state)->PP_Result

//! Shorthand for accessing a Syntax_Tag
#define TAG(X) Syntax_Tags::X

//! Shorthand for accessing a Stmt
#define STMT(X) stmt(FLPR::Stmt::X)

#if FLPR_TRACE_PG
#define RULE(T)                                                                \
  constexpr auto rule_tag{Syntax_Tags::T};                                     \
  Syntax_Tags::print(std::cerr << "PGTRACE >  ", Syntax_Tags::T) << '\n'

#define EVAL(T, E)                                                             \
  PP_Result res_ = E;                                                          \
  if (!res_.match)                                                             \
    Syntax_Tags::print(std::cerr << "PGTRACE <! ", Syntax_Tags::T) << '\n';    \
  else                                                                         \
    std::cerr << "PGTRACE <= " << res_.parse_tree << '\n';                     \
  return res_;
#else
#define RULE(T)                                                                \
  constexpr auto rule_tag { Syntax_Tags::T }
#define EVAL(T, E) return E
#endif

// clang-format off

/* I:A */

//! R1102: associate-construct (11.1.3.1)
PPARSER(associate_construct) {
  RULE(PG_ASSOCIATE_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(associate_stmt),
           block,
           STMT(end_associate_stmt));
  EVAL(PG_ASSOCIATE_CONSTRUCT, p(state));
}

/* I:B */

//! R1101: block (11.1.1)
PPARSER(block) {
  RULE(PG_BLOCK);
  constexpr auto p =
    tag_if(rule_tag,
           star(execution_part_construct));
  EVAL(PG_BLOCK, p(state));
}

//! R1107: block-construct (11.1.4)
PPARSER(block_construct) {
  RULE(PG_BLOCK_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
        STMT(block_stmt),
        opt(block_specification_part),
        block,
        STMT(end_block_stmt));
  EVAL(PG_BLOCK_CONSTRUCT, p(state));
}

//! R1109: block-specification-part (11.1.4)
PPARSER(block_specification_part) {
  RULE(PG_BLOCK_SPECIFICATION_PART);
  constexpr auto p =
    seq(rule_tag,
        star(STMT(use_stmt)),
        star(STMT(import_stmt)),
        h_opt_seq(star(declaration_construct),
                  specification_construct)
        );
  EVAL(PG_BLOCK_SPECIFICATION_PART, p(state));
}

/* I:C */

//! R1140: case-construct (11.1.9.1)
PPARSER(case_construct) {
  RULE(PG_CASE_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(select_case_stmt),
           star(h_seq_if(STMT(case_stmt), block)),
           STMT(end_select_stmt));
  EVAL(PG_CASE_CONSTRUCT, p(state));
}

//! R735: component-part (7.5.4.1)
PPARSER(component_part) {
  RULE(PG_COMPONENT_PART);
  constexpr auto p =
    tag_if(rule_tag,
           star(STMT(component_def_stmt)));
  EVAL(PG_COMPONENT_PART, p(state));
}

/* I:D */

//! R507: declaration-construct (5.1)
PPARSER(declaration_construct) {
  RULE(PG_DECLARATION_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         specification_construct,
         STMT(data_stmt),
         STMT(format_stmt),
         STMT(entry_stmt));
  EVAL(PG_DECLARATION_CONSTRUCT, p(state));
}

//! R726: derived-type-def (7.5.2.1)
PPARSER(derived_type_def) {
  RULE(PG_DERIVED_TYPE_DEF);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(derived_type_stmt),
           opt(STMT(private_or_sequence)),
           component_part,
           opt(type_bound_procedure_part),
           STMT(end_type_stmt));
  EVAL(PG_DERIVED_TYPE_DEF, p(state));
}

//! R1119: do-construct (11.1.7.2)
PPARSER(do_construct) {
  RULE(PG_DO_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           do_stmt(),
           block,
           end_do());
  EVAL(PG_DO_CONSTRUCT, p(state));
}

/* I:E */

//! R759: enum-def (7.6)
PPARSER(enum_def) {
  RULE(PG_ENUM_DEF);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(enum_def_stmt),
           plus(TAG(HOIST), STMT(enumerator_def_stmt)),
           STMT(end_enum_stmt));
  EVAL(PG_ENUM_DEF, p(state));
}

//! R514: executable-construct (5.1)
PPARSER(executable_construct) {
  RULE(PG_EXECUTABLE_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         STMT(action_stmt),
         associate_construct,
         block_construct,
         case_construct,
         // FIX change_team_construct,
         // FIX critical_construct,
         do_construct,
         if_construct,
         select_rank_construct,
         select_type_construct,
         where_construct,
         forall_construct
         );
  /* If the current statement has a label that matches a label-do-stmt label
     that we are looking for, we force a non-match so that the end-do-stmt rule
     can look at it */
  if(!state.do_label_stack.empty() && state.ss->has_label()) {
    if(state.ss->label() == state.do_label_stack.top()) {
      return PP_Result{};
    }
  }
  EVAL(PG_EXECUTABLE_CONSTRUCT, p(state));
}

//! R509: execution-part (5.1)
PPARSER(execution_part) {
  RULE(PG_EXECUTION_PART);
  constexpr auto p =
    seq_if(rule_tag,
           executable_construct,
           star(execution_part_construct));
  EVAL(PG_EXECUTION_PART, p(state));
}

//! R510: execution-part-construct (5.1)
PPARSER(execution_part_construct) {
  RULE(PG_EXECUTION_PART_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         executable_construct,
         STMT(format_stmt),
         STMT(data_stmt),
         STMT(entry_stmt));
  EVAL(PG_EXECUTION_PART_CONSTRUCT, p(state));
}

//! R503: external-subprogram (5.1)
PPARSER(external_subprogram) {
  RULE(PG_EXTERNAL_SUBPROGRAM);
  constexpr auto p = alts(rule_tag,
                          function_subprogram,
                          subroutine_subprogram);
  EVAL(PG_EXTERNAL_SUBPROGRAM, p(state));
}

/* I:F */

//! R1052: forall-body-construct (10.2.4.1)
PPARSER(forall_body_construct) {
  RULE(PG_FORALL_BODY_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         STMT(forall_assignment_stmt),
         STMT(where_stmt),
         where_construct,
         forall_construct,
         STMT(forall_stmt));
  EVAL(PG_FORALL_BODY_CONSTRUCT, p(state));
}

//! R1050: forall-construct (10.2.4.1)
PPARSER(forall_construct) {
  RULE(PG_FORALL_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(forall_construct_stmt),
           star(forall_body_construct),
           STMT(end_forall_stmt));
  EVAL(PG_FORALL_CONSTRUCT, p(state));
}

//! R1529: function-subprogram (15.6.2.2)
PPARSER(function_subprogram) {
  RULE(PG_FUNCTION_SUBPROGRAM);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(function_stmt),
           opt(specification_part),
           opt(execution_part),
           opt(internal_subprogram_part),
           STMT(end_function_stmt));
  EVAL(PG_FUNCTION_SUBPROGRAM, p(state));
}

/* I:G */
/* I:H */
/* I:I */

//! R1134: if-construct (11.1.8.1)
PPARSER(if_construct) {
  RULE(PG_IF_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(if_then_stmt),
           block,
           star(h_seq_if(STMT(else_if_stmt), block)),
           opt(h_seq_if(STMT(else_stmt), block)),
           STMT(end_if_stmt)
           );
  EVAL(PG_IF_CONSTRUCT, p(state));
}

//! R505: implicit-part (5.1)
/* Note: formally, this is supposed to be <tt>implicit-part-stmt*
   implicit-stmt<\tt>, but this requires left-recursion, which is difficult in a
   straight RDP.  This modified rule will over-consume FORMAT statements that
   would otherwise be attibuted to the \c declaration-construct. */
PPARSER(implicit_part) {
  RULE(PG_IMPLICIT_PART);
  constexpr auto p = tag_if(rule_tag,
                            star(implicit_part_stmt));

  EVAL(PG_IMPLICIT_PART, p(state));
}

//! R506: implicit-part-stmt (5.1)
PPARSER(implicit_part_stmt) {
  RULE(PG_IMPLICIT_PART_STMT);
  constexpr auto p = alts(rule_tag,
                          STMT(implicit_stmt),
                          STMT(parameter_stmt),
                          STMT(format_stmt),
                          STMT(entry_stmt)
                          );

  EVAL(PG_IMPLICIT_PART_STMT, p(state));
}

//! R1501: interface-block (15.4.3.2)
PPARSER(interface_block) {
  RULE(PG_INTERFACE_BLOCK);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(interface_stmt),
           star(interface_specification),
           STMT(end_interface_stmt));
  EVAL(PG_INTERFACE_BLOCK, p(state));
}

//! R1505: interface-body (15.4.3.2)
PPARSER(interface_body) {
  RULE(PG_INTERFACE_BODY);
  constexpr auto p =
    alts(rule_tag,
         h_seq_if(STMT(function_stmt),
                  opt(specification_part),
                  STMT(end_function_stmt)),
         h_seq_if(STMT(subroutine_stmt),
                  opt(specification_part),
                  STMT(end_subroutine_stmt))
         );
  EVAL(PG_INTERFACE_BODY, p(state));
}

//! R1502: interface-specification (15.4.3.2)
PPARSER(interface_specification) {
  RULE(PG_INTERFACE_SPECIFICATION);
  constexpr auto p =
    alts(rule_tag,
         interface_body,
         STMT(procedure_stmt));
  EVAL(PG_INTERFACE_SPECIFICATION, p(state));
}

//! R512: internal-subprogram (5.1)
PPARSER(internal_subprogram) {
  RULE(PG_INTERNAL_SUBPROGRAM);
  constexpr auto p = alts(rule_tag,
                          function_subprogram,
                          subroutine_subprogram);
  EVAL(PG_INTERNAL_SUBPROGRAM, p(state));
}

//! R511: internal-subprogram-part (5.1)
PPARSER(internal_subprogram_part) {
  RULE(PG_INTERNAL_SUBPROGRAM_PART);
  constexpr auto p = seq_if(rule_tag,
                            STMT(contains_stmt),
                            star(internal_subprogram)
                            );
  EVAL(PG_INTERNAL_SUBPROGRAM_PART, p(state));
}

/* I:J */
/* I:K */
/* I:L */
/* I:M */

//! R1401: main-program (14.1)
PPARSER(main_program) {
  RULE(PG_MAIN_PROGRAM);
  constexpr auto p =
    seq_if(rule_tag,
           opt(STMT(program_stmt)),
           opt(specification_part),
           opt(execution_part),
           opt(internal_subprogram_part),
           STMT(end_program_stmt)
           );
  EVAL(PG_MAIN_PROGRAM, p(state));
}

//! R1404: module (14.2.1)
PPARSER(module) {
  RULE(PG_MODULE);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(module_stmt),
           opt(specification_part),
           opt(module_subprogram_part),
           STMT(end_module_stmt));
  EVAL(PG_MODULE, p(state));
}

//! R1408: module-subprogram (14.2.1)
PPARSER(module_subprogram) {
  RULE(PG_MODULE_SUBPROGRAM);
  constexpr auto p =
    alts(rule_tag,
         function_subprogram,
         subroutine_subprogram,
         separate_module_subprogram);
  EVAL(PG_MODULE_SUBPROGRAM, p(state));
}

//! R1407: module-subprogram-part (14.2.1)
PPARSER(module_subprogram_part) {
  RULE(PG_MODULE_SUBPROGRAM_PART);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(contains_stmt),
           star(module_subprogram));
  EVAL(PG_MODULE_SUBPROGRAM_PART, p(state));
}


/* I:N */
/* I:O */


/* I:P */

//! R501: program (5.1)
PPARSER(program) {
  RULE(PG_PROGRAM);
  constexpr auto p =
    seq(rule_tag,
        plus(TAG(HOIST), program_unit),
        end_stream());
  EVAL(PG_PROGRAM, p(state));
}

//! R502: program-unit (5.1)
PPARSER(program_unit) {
  RULE(PG_PROGRAM_UNIT);
  constexpr auto p =
    alts(rule_tag,
         external_subprogram,
         module,
         // FIX submodule,
         // FIX block_data
         main_program  // must be last
         );
  EVAL(PG_PROGRAM_UNIT, p(state));
}

/* I:Q */
/* I:R */
/* I:S */

//! R1538: separate-module-subprogram (15.6.2.5)
PPARSER(separate_module_subprogram) {
  RULE(PG_SEPARATE_MODULE_SUBPROGRAM);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(mp_subprogram_stmt),
           opt(specification_part),
           opt(execution_part),
           opt(internal_subprogram_part),
           STMT(end_mp_subprogram_stmt));
  EVAL(PG_SEPARATE_MODULE_SUBPROGRAM, p(state));
}

//! R1148: select-rank-construct (11.1.10.1)
PPARSER(select_rank_construct) {
  RULE(PG_SELECT_RANK_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(select_rank_stmt),
           star(seq_if(TAG(HOIST),
                       STMT(select_rank_case_stmt),
                       block)),
           STMT(end_select_rank_stmt));
  EVAL(PG_SELECT_RANK_CONSTRUCT, p(state));
}

//! R1152: select-type-construct (11.1.11.1)
PPARSER(select_type_construct) {
  RULE(PG_SELECT_TYPE_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(select_type_stmt),
           star(seq_if(TAG(HOIST),
                       STMT(type_guard_stmt),
                       block)),
           STMT(end_select_type_stmt));
  EVAL(PG_SELECT_TYPE_CONSTRUCT, p(state));
}

//! R508: specification-construct (5.1)
PPARSER(specification_construct) {
  RULE(PG_SPECIFICATION_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         derived_type_def,
         enum_def,
         STMT(generic_stmt),
         interface_block,
         STMT(parameter_stmt),
         STMT(procedure_declaration_stmt),
         STMT(other_specification_stmt),
         STMT(type_declaration_stmt));
  EVAL(PG_SPECIFICATION_CONSTRUCT, p(state));
}

//! R504: specification-part (5.1)
PPARSER(specification_part) {
  RULE(PG_SPECIFICATION_PART);
  constexpr auto p =
    seq(rule_tag,
        star(STMT(use_stmt)),
        star(STMT(import_stmt)),
        opt(implicit_part),
        star(declaration_construct)
        );
  EVAL(PG_SPECIFICATION_PART, p(state));
}

//! R1534: subroutine-subprogram (15.6.2.3)
PPARSER(subroutine_subprogram) {
  RULE(PG_SUBROUTINE_SUBPROGRAM);
  constexpr auto p = seq_if(rule_tag,
                            STMT(subroutine_stmt),
                            opt(specification_part),
                            opt(execution_part),
                            opt(internal_subprogram_part),
                            STMT(end_subroutine_stmt));
  EVAL(PG_SUBROUTINE_SUBPROGRAM, p(state));
}

/* I:T */

//! R746: type-bound-procedure-part (7.5.5)
PPARSER(type_bound_procedure_part) {
  RULE(PG_TYPE_BOUND_PROCEDURE_PART);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(contains_stmt),
           opt(STMT(binding_private_stmt)),
           star(STMT(type_bound_proc_binding)));
  EVAL(PG_TYPE_BOUND_PROCEDURE_PART, p(state));
}


/* I:U */
/* I:V */
/* I:W */

//! R1044: where-body-construct (10.2.3.1)
PPARSER(where_body_construct) {
  RULE(PG_WHERE_BODY_CONSTRUCT);
  constexpr auto p =
    alts(rule_tag,
         STMT(assignment_stmt),  // should be where-assignment-stmt, but same
         STMT(where_stmt),
         where_construct);
  EVAL(PG_WHERE_BODY_CONSTRUCT, p(state));
}

//! R1042: where-construct (10.2.3.1)
PPARSER(where_construct) {
  RULE(PG_WHERE_CONSTRUCT);
  constexpr auto p =
    seq_if(rule_tag,
           STMT(where_construct_stmt),
           star(where_body_construct),
           star(h_seq_if(STMT(masked_elsewhere_stmt),
                         star(where_body_construct))),
           opt(h_seq_if(STMT(elsewhere_stmt),
                        star(where_body_construct))),
           STMT(end_where_stmt));
  EVAL(PG_WHERE_CONSTRUCT, p(state));
}

/* I:X */
/* I:Y */
/* I:Z */
// clang-format on

#undef PPARSER
#undef RULE
#undef EVAL
#undef TAG
#undef STMT
