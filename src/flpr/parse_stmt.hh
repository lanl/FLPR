
/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file parse_stmt.hh
*/
#ifndef FLPR_PARSE_STMT_HH
#define FLPR_PARSE_STMT_HH 1

#include "flpr/Stmt_Tree.hh"
#include "flpr/TT_Stream.hh"

namespace FLPR {

//! Namespace containing parsers used to recognize Fortran statements
namespace Stmt {

//! Dispatch to a particular parser give stmt_tag
Stmt_Tree parse_stmt_dispatch(int stmt_tag, TT_Stream &ts);

//! Return true if this syntag is considered an action-stmt
bool is_action_stmt(int const syntag);

/*! \defgroup StmtParsers Parsers for complete Fortran statements
  @{ */
Stmt_Tree access_stmt(TT_Stream &ts);
Stmt_Tree action_stmt(TT_Stream &ts);
Stmt_Tree allocatable_stmt(TT_Stream &ts);
Stmt_Tree allocate_stmt(TT_Stream &ts);
Stmt_Tree arithmetic_if_stmt(TT_Stream &ts);
Stmt_Tree assignment_stmt(TT_Stream &ts);
Stmt_Tree associate_stmt(TT_Stream &ts);
Stmt_Tree asynchronous_stmt(TT_Stream &ts);
Stmt_Tree backspace_stmt(TT_Stream &ts);
Stmt_Tree bind_stmt(TT_Stream &ts);
Stmt_Tree binding_private_stmt(TT_Stream &ts);
Stmt_Tree block_stmt(TT_Stream &ts);
Stmt_Tree call_stmt(TT_Stream &ts);
Stmt_Tree case_stmt(TT_Stream &ts);
Stmt_Tree close_stmt(TT_Stream &ts);
Stmt_Tree codimension_stmt(TT_Stream &ts);
Stmt_Tree common_stmt(TT_Stream &ts);
Stmt_Tree component_def_stmt(TT_Stream &ts);
Stmt_Tree computed_goto_stmt(TT_Stream &ts);
Stmt_Tree contains_stmt(TT_Stream &ts);
Stmt_Tree continue_stmt(TT_Stream &ts);
Stmt_Tree cycle_stmt(TT_Stream &ts);
Stmt_Tree data_component_def_stmt(TT_Stream &ts);
Stmt_Tree data_stmt(TT_Stream &ts);
Stmt_Tree deallocate_stmt(TT_Stream &ts);
Stmt_Tree derived_type_stmt(TT_Stream &ts);
Stmt_Tree dimension_stmt(TT_Stream &ts);
Stmt_Tree do_stmt(TT_Stream &ts);
Stmt_Tree else_if_stmt(TT_Stream &ts);
Stmt_Tree else_stmt(TT_Stream &ts);
Stmt_Tree elsewhere_stmt(TT_Stream &ts);
Stmt_Tree end_associate_stmt(TT_Stream &ts);
Stmt_Tree end_block_stmt(TT_Stream &ts);
Stmt_Tree end_do(TT_Stream &ts);
Stmt_Tree end_do_stmt(TT_Stream &ts);
Stmt_Tree end_enum_stmt(TT_Stream &ts);
Stmt_Tree end_forall_stmt(TT_Stream &ts);
Stmt_Tree end_function_stmt(TT_Stream &ts);
Stmt_Tree end_if_stmt(TT_Stream &ts);
Stmt_Tree end_interface_stmt(TT_Stream &ts);
Stmt_Tree end_module_stmt(TT_Stream &ts);
Stmt_Tree end_mp_subprogram_stmt(TT_Stream &ts);
Stmt_Tree end_program_stmt(TT_Stream &ts);
Stmt_Tree end_select_stmt(TT_Stream &ts);
Stmt_Tree end_select_rank_stmt(TT_Stream &ts);
Stmt_Tree end_select_type_stmt(TT_Stream &ts);
Stmt_Tree end_submodule_stmt(TT_Stream &ts);
Stmt_Tree end_subroutine_stmt(TT_Stream &ts);
Stmt_Tree end_type_stmt(TT_Stream &ts);
Stmt_Tree end_where_stmt(TT_Stream &ts);
Stmt_Tree endfile_stmt(TT_Stream &ts);
Stmt_Tree entry_stmt(TT_Stream &ts);
Stmt_Tree enum_def_stmt(TT_Stream &ts);
Stmt_Tree enumerator_def_stmt(TT_Stream &ts);
Stmt_Tree equivalence_stmt(TT_Stream &ts);
Stmt_Tree error_stop_stmt(TT_Stream &ts);
Stmt_Tree event_post_stmt(TT_Stream &ts);
Stmt_Tree event_wait_stmt(TT_Stream &ts);
Stmt_Tree exit_stmt(TT_Stream &ts);
Stmt_Tree external_stmt(TT_Stream &ts);
Stmt_Tree fail_image_stmt(TT_Stream &ts);
Stmt_Tree final_procedure_stmt(TT_Stream &ts);
Stmt_Tree flush_stmt(TT_Stream &ts);
Stmt_Tree forall_assignment_stmt(TT_Stream &ts);
Stmt_Tree forall_construct_stmt(TT_Stream &ts);
Stmt_Tree forall_stmt(TT_Stream &ts);
Stmt_Tree form_team_stmt(TT_Stream &ts);
Stmt_Tree format_stmt(TT_Stream &ts);
Stmt_Tree function_stmt(TT_Stream &ts);
Stmt_Tree generic_stmt(TT_Stream &ts);
Stmt_Tree goto_stmt(TT_Stream &ts);
Stmt_Tree if_stmt(TT_Stream &ts);
Stmt_Tree if_then_stmt(TT_Stream &ts);
Stmt_Tree implicit_stmt(TT_Stream &ts);
Stmt_Tree import_stmt(TT_Stream &ts);
Stmt_Tree inquire_stmt(TT_Stream &ts);
Stmt_Tree intent_stmt(TT_Stream &ts);
Stmt_Tree interface_stmt(TT_Stream &ts);
Stmt_Tree intrinsic_stmt(TT_Stream &ts);
Stmt_Tree label_do_stmt(TT_Stream &ts);
Stmt_Tree lock_stmt(TT_Stream &ts);
Stmt_Tree macro_stmt(TT_Stream &ts);
Stmt_Tree masked_elsewhere_stmt(TT_Stream &ts);
Stmt_Tree module_stmt(TT_Stream &ts);
Stmt_Tree mp_subprogram_stmt(TT_Stream &ts);
Stmt_Tree namelist_stmt(TT_Stream &ts);
Stmt_Tree nonlabel_do_stmt(TT_Stream &ts);
Stmt_Tree nullify_stmt(TT_Stream &ts);
Stmt_Tree open_stmt(TT_Stream &ts);
Stmt_Tree optional_stmt(TT_Stream &ts);
Stmt_Tree other_specification_stmt(TT_Stream &ts);
Stmt_Tree parameter_stmt(TT_Stream &ts);
Stmt_Tree pointer_assignment_stmt(TT_Stream &ts);
Stmt_Tree pointer_stmt(TT_Stream &ts);
Stmt_Tree print_stmt(TT_Stream &ts);
Stmt_Tree private_components_stmt(TT_Stream &ts);
Stmt_Tree private_or_sequence(TT_Stream &ts);
Stmt_Tree proc_component_def_stmt(TT_Stream &ts);
Stmt_Tree procedure_declaration_stmt(TT_Stream &ts);
Stmt_Tree procedure_stmt(TT_Stream &ts);
Stmt_Tree program_stmt(TT_Stream &ts);
Stmt_Tree protected_stmt(TT_Stream &ts);
Stmt_Tree read_stmt(TT_Stream &ts);
Stmt_Tree return_stmt(TT_Stream &ts);
Stmt_Tree rewind_stmt(TT_Stream &ts);
Stmt_Tree save_stmt(TT_Stream &ts);
Stmt_Tree select_case_stmt(TT_Stream &ts);
Stmt_Tree select_rank_stmt(TT_Stream &ts);
Stmt_Tree select_rank_case_stmt(TT_Stream &ts);
Stmt_Tree select_type_stmt(TT_Stream &ts);
Stmt_Tree sequence_stmt(TT_Stream &ts);
Stmt_Tree stop_stmt(TT_Stream &ts);
Stmt_Tree submodule_stmt(TT_Stream &ts);
Stmt_Tree subroutine_stmt(TT_Stream &ts);
Stmt_Tree sync_all_stmt(TT_Stream &ts);
Stmt_Tree sync_images_stmt(TT_Stream &ts);
Stmt_Tree sync_memory_stmt(TT_Stream &ts);
Stmt_Tree sync_team_stmt(TT_Stream &ts);
Stmt_Tree target_stmt(TT_Stream &ts);
Stmt_Tree type_bound_generic_stmt(TT_Stream &ts);
Stmt_Tree type_bound_procedure_stmt(TT_Stream &ts);
Stmt_Tree type_declaration_stmt(TT_Stream &ts);
Stmt_Tree type_guard_stmt(TT_Stream &ts);
Stmt_Tree type_param_def_stmt(TT_Stream &ts);
Stmt_Tree unlock_stmt(TT_Stream &ts);
Stmt_Tree use_stmt(TT_Stream &ts);
Stmt_Tree value_stmt(TT_Stream &ts);
Stmt_Tree volatile_stmt(TT_Stream &ts);
Stmt_Tree wait_stmt(TT_Stream &ts);
Stmt_Tree where_construct_stmt(TT_Stream &ts);
Stmt_Tree where_stmt(TT_Stream &ts);
Stmt_Tree write_stmt(TT_Stream &ts);
/*! @} */

/*! \defgroup StmtParsers Parsers for portions of Fortran statements
  @{ */
Stmt_Tree actual_arg_spec(TT_Stream &ts);
Stmt_Tree allocatable_decl(TT_Stream &ts);
Stmt_Tree allocate_coarray_spec(TT_Stream &ts);
Stmt_Tree allocate_coshape_spec(TT_Stream &ts);
Stmt_Tree allocation(TT_Stream &ts);
Stmt_Tree array_element(TT_Stream &ts);
Stmt_Tree array_spec(TT_Stream &ts);
Stmt_Tree association(TT_Stream &ts);
Stmt_Tree assumed_implied_spec(TT_Stream &ts);
Stmt_Tree assumed_rank_spec(TT_Stream &ts);
Stmt_Tree assumed_shape_spec(TT_Stream &ts);
Stmt_Tree assumed_size_spec(TT_Stream &ts);
Stmt_Tree case_value_range(TT_Stream &ts);
Stmt_Tree char_selector(TT_Stream &ts);
Stmt_Tree coarray_spec(TT_Stream &ts);
Stmt_Tree component_initialization(TT_Stream &ts);
Stmt_Tree concurrent_control(TT_Stream &ts);
Stmt_Tree concurrent_header(TT_Stream &ts);
Stmt_Tree concurrent_locality(TT_Stream &ts);
Stmt_Tree data_ref(TT_Stream &ts);
Stmt_Tree data_stmt_object(TT_Stream &ts);
Stmt_Tree data_stmt_set(TT_Stream &ts);
Stmt_Tree data_stmt_value(TT_Stream &ts);
Stmt_Tree declaration_type_spec(TT_Stream &ts);
Stmt_Tree declaration_type_spec_no_kind(TT_Stream &ts);
Stmt_Tree deferred_shape_spec(TT_Stream &ts);
Stmt_Tree defined_io_generic_spec(TT_Stream &ts);
Stmt_Tree derived_type_spec(TT_Stream &ts);
Stmt_Tree designator(TT_Stream &ts);
Stmt_Tree dummy_arg(TT_Stream &ts);
Stmt_Tree explicit_coshape_spec(TT_Stream &ts);
Stmt_Tree explicit_shape_spec(TT_Stream &ts);
Stmt_Tree expr(TT_Stream &ts);
Stmt_Tree extended_intrinsic_op(TT_Stream &ts);
Stmt_Tree function_reference(TT_Stream &ts);
Stmt_Tree generic_spec(TT_Stream &ts);
Stmt_Tree image_selector(TT_Stream &ts);
Stmt_Tree image_selector_spec(TT_Stream &ts);
Stmt_Tree implicit_none_spec(TT_Stream &ts);
Stmt_Tree implied_shape_or_assumed_size_spec(TT_Stream &ts);
Stmt_Tree implied_shape_spec(TT_Stream &ts);
Stmt_Tree initialization(TT_Stream &ts);
Stmt_Tree input_item(TT_Stream &ts);
Stmt_Tree int_constant_expr(TT_Stream &ts);
Stmt_Tree int_expr(TT_Stream &ts);
Stmt_Tree integer_type_spec(TT_Stream &ts);
Stmt_Tree intent_spec(TT_Stream &ts);
Stmt_Tree intrinsic_operator(TT_Stream &ts);
Stmt_Tree intrinsic_type_spec(TT_Stream &ts);
Stmt_Tree intrinsic_type_spec_no_kind(TT_Stream &ts);
Stmt_Tree io_implied_do(TT_Stream &ts);
Stmt_Tree io_implied_do_control(TT_Stream &ts);
Stmt_Tree io_implied_do_object(TT_Stream &ts);
Stmt_Tree kind_selector(TT_Stream &ts);
Stmt_Tree label(TT_Stream &ts);
Stmt_Tree language_binding_spec(TT_Stream &ts);
Stmt_Tree length_selector(TT_Stream &ts);
Stmt_Tree letter_spec(TT_Stream &ts);
Stmt_Tree locality_spec(TT_Stream &ts);
Stmt_Tree logical_expr(TT_Stream &ts);
Stmt_Tree logical_literal_constant(TT_Stream &ts);
Stmt_Tree loop_control(TT_Stream &ts);
Stmt_Tree lower_bound_expr(TT_Stream &ts);
Stmt_Tree mult_op(TT_Stream &ts);
Stmt_Tree output_item(TT_Stream &ts);
Stmt_Tree parent_string(TT_Stream &ts);
Stmt_Tree part_ref(TT_Stream &ts);
Stmt_Tree pointer_object(TT_Stream &ts);
Stmt_Tree prefix(TT_Stream &ts);
Stmt_Tree prefix_spec(TT_Stream &ts);
Stmt_Tree proc_component_attr_spec(TT_Stream &ts);
Stmt_Tree proc_component_ref(TT_Stream &ts);
Stmt_Tree proc_decl(TT_Stream &ts);
Stmt_Tree proc_interface(TT_Stream &ts);
Stmt_Tree proc_language_binding_spec(TT_Stream &ts);
Stmt_Tree proc_pointer_init(TT_Stream &ts);
Stmt_Tree proc_pointer_object(TT_Stream &ts);
Stmt_Tree proc_target(TT_Stream &ts);
Stmt_Tree procedure_designator(TT_Stream &ts);
Stmt_Tree rel_op(TT_Stream &ts);
Stmt_Tree rename(TT_Stream &ts);
Stmt_Tree saved_entity(TT_Stream &ts);
Stmt_Tree section_subscript(TT_Stream &ts);
Stmt_Tree selector(TT_Stream &ts);
Stmt_Tree sign(TT_Stream &ts);
Stmt_Tree structure_component(TT_Stream &ts);
Stmt_Tree structure_constructor(TT_Stream &ts);
Stmt_Tree substring(TT_Stream &ts);
Stmt_Tree substring_range(TT_Stream &ts);
Stmt_Tree suffix(TT_Stream &ts);
Stmt_Tree sync_stat(TT_Stream &ts);
Stmt_Tree type_attr_spec(TT_Stream &ts);
Stmt_Tree type_bound_proc_binding(TT_Stream &ts);
Stmt_Tree type_param_spec(TT_Stream &ts);
Stmt_Tree type_param_value(TT_Stream &ts);
Stmt_Tree type_spec(TT_Stream &ts);
Stmt_Tree upper_bound_expr(TT_Stream &ts);
Stmt_Tree variable(TT_Stream &ts);
Stmt_Tree wait_spec(TT_Stream &ts);

Stmt_Tree consume_parens(TT_Stream &ts);

/*! @} */

} // namespace Stmt
} // namespace FLPR

#endif
