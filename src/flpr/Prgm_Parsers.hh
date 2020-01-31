/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file Prgm_Parsers.hh
*/
#ifndef FLPR_PRGM_PARSERS_HH
#define FLPR_PRGM_PARSERS_HH 1

#include "flpr/LL_Stmt.hh"
#include "flpr/Parser_Result.hh"
#include "flpr/Prgm_Tree.hh"
#include "flpr/Tree.hh"
#include "flpr/parse_stmt.hh"
#include <iostream>
#include <vector>

#define FLPR_TRACE_PG 0

namespace FLPR {

//! All of the parser information for program-level constructs
namespace Prgm {

//! The parsers that organize statements into program structures
/*! This is a wrapper for a bunch of static functions, used so that the client
  can extend the \c Node_Data type or provide an alternative \c Stmt_Src */
template <typename Node_Data = Prgm_Node_Data> struct Parsers {
  //! Define the Prgm_Tree
  /*! Node_Data should be (derived from) FLPR::Prgm::PT_Node_Data */
  using Prgm_Tree = FLPR::Tree<Node_Data>;
  //! The result from each parser in Prgm::Parsers
  using PP_Result = FLPR::details_::Parser_Result<Prgm_Tree>;

  class State {
  private:
    SL_Range<LL_Stmt> stmt_range_;

  public:
    explicit State(LL_STMT_SEQ &ll_stmts)
        : stmt_range_(ll_stmts), ss{stmt_range_} {}
    explicit State(SL_Range<LL_Stmt> const &ll_stmt_range)
        : stmt_range_(ll_stmt_range), ss{stmt_range_} {}
    SL_Range_Iterator<LL_Stmt> ss;
    std::vector<int> do_label_stack;
  };

#include "Prgm_Parsers_utils.hh"

  static PP_Result associate_construct(State &state);
  static PP_Result block(State &state);
  static PP_Result block_construct(State &state);
  static PP_Result block_specification_part(State &state);
  static PP_Result case_construct(State &state);
  static PP_Result component_part(State &state);
  static PP_Result declaration_construct(State &state);
  static PP_Result derived_type_def(State &state);
  static PP_Result do_construct(State &state);
  static PP_Result enum_def(State &state);
  static PP_Result executable_construct(State &state);
  static PP_Result execution_part(State &state);
  static PP_Result execution_part_construct(State &state);
  static PP_Result external_subprogram(State &state);
  static PP_Result forall_body_construct(State &ss);
  static PP_Result forall_construct(State &ss);
  static PP_Result function_subprogram(State &state);
  static PP_Result if_construct(State &state);
  static PP_Result implicit_part(State &state);
  static PP_Result implicit_part_stmt(State &state);
  static PP_Result interface_block(State &state);
  static PP_Result interface_body(State &state);
  static PP_Result interface_specification(State &state);
  static PP_Result internal_subprogram(State &state);
  static PP_Result internal_subprogram_part(State &state);
  static PP_Result main_program(State &ss);
  static PP_Result module(State &ss);
  static PP_Result module_subprogram(State &ss);
  static PP_Result module_subprogram_part(State &ss);
  static PP_Result other_specification_stmt(State &state);
  static PP_Result program(State &state);
  static PP_Result program_unit(State &state);
  static PP_Result select_rank_construct(State &state);
  static PP_Result select_type_construct(State &state);
  static PP_Result separate_module_subprogram(State &ss);
  static PP_Result specification_construct(State &state);
  static PP_Result specification_part(State &state);
  static PP_Result subroutine_subprogram(State &state);
  static PP_Result type_bound_procedure_part(State &state);
  static PP_Result where_body_construct(State &state);
  static PP_Result where_construct(State &state);
};

#include "Prgm_Parsers_impl.hh"

//! Declare an explicit instantiation of the default Prgm::Parser
extern template struct Parsers<>;
} // namespace Prgm
} // namespace FLPR

#undef FLPR_TRACE_PG

#endif
