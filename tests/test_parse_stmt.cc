/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Tests for full statement parsers.
*/

#include "flpr/parse_stmt.hh"
#include "parse_helpers.hh"

using namespace FLPR;
using FLPR::Stmt::Stmt_Tree;

bool allocatable_stmt() {
  TSS(allocatable_stmt, "allocatable a");
  TSS(allocatable_stmt, "allocatable a(1)");
  TSS(allocatable_stmt, "allocatable a(:,:)");
  TSS(allocatable_stmt, "allocatable a(:,:)[:], b(:,:)[:]");
  TSS(allocatable_stmt, "allocatable a[:]");
  TSS(allocatable_stmt, "allocatable :: a");
  TSS(allocatable_stmt, "allocatable :: a(1)");
  TSS(allocatable_stmt, "allocatable :: a(:,:)");
  TSS(allocatable_stmt, "allocatable :: a(:,:)[:], b(:,:)[:]");
  TSS(allocatable_stmt, "allocatable :: a[:]");
  return true;
}

bool allocate_stmt() {
  TSS(allocate_stmt, "allocate (a)");
  TSS(allocate_stmt, "allocate (a(4))");
  TSS(allocate_stmt, "allocate (a,b,c,d(5))");
  TSS(allocate_stmt, "allocate(a, stat=s)");
  TSS(allocate_stmt, "allocate(a(2), mold=b(2,:), "
                     "source=q, stat=s, ERRMSG=es)");
  TSS(allocate_stmt, "allocate(foo[*])");
  TSS(allocate_stmt, "allocate(foo[1:2,3,*])");
  TSS(allocate_stmt, "allocate(foo[1:2,3,1:*])");
  TSS(allocate_stmt, "allocate(foo(3)[1:2,3,*])");
  TSS(allocate_stmt, "allocate(foo(3)[1:2,3,*], bar(6))");
  TSS(allocate_stmt, "allocate(this%v(newSize))");
  return true;
}

bool arithmetic_if_stmt() {
  TSS(arithmetic_if_stmt, "if(i-j) 5000, 6000, 8000 ");
  return true;
}

bool assignment_stmt() {
  TSS(assignment_stmt, "i = 1");
  TSS(assignment_stmt, "i = (1 + sum(a(3,:)) - f(2))");
  TSS(assignment_stmt, "c%re = q");
  TSS(assignment_stmt, "a(:) = 0.d0");
  TSS(assignment_stmt, "a(:3) = 0.d0");
  TSS(assignment_stmt, "a(1:3) = 0.d0");
  TSS(assignment_stmt, "a(1:,2) = 0.d0");
  TSS(assignment_stmt, "r%q%a(1:,2)[1] = 0.d0");
  TSS(assignment_stmt, "itype(m)(8:8)=char(0)");
  return true;
}

bool associate_stmt() {
  TSS(associate_stmt,
      "associate(dfrdp => "
      "cell_data%scale_r(mat_loop)*(teos_r(nm,npup,nt)-teos_r(nm,npdown,nt))/ "
      "(cell_data%scale_p(mat_loop)*(p(npup)-p(npdown))), fr   => "
      "cell_data%scale_r(mat_loop)*teos_r(nm,nump,nt))");
  return true;
}

bool asynchronous_stmt() {
  TSS(asynchronous_stmt, "asynchronous a");
  TSS(asynchronous_stmt, "asynchronous :: a");
  TSS(asynchronous_stmt, "asynchronous a,b,c");
  TSS(asynchronous_stmt, "asynchronous :: a,b,c");
  return true;
}

bool bind_stmt() {
  TSS(bind_stmt, "bind(c) entity_name");
  TSS(bind_stmt, "bind(c) entity_name,/cblockname/");
  TSS(bind_stmt, "bind(c) :: entity_name");
  TSS(bind_stmt, "bind(c) :: entity_name,/cblockname/");
  TSS(bind_stmt, "bind(c,name='foo') :: entity_name");
  TSS(bind_stmt, "bind(c,name='foo') :: /cblockname/");
  return true;
}

bool block_stmt() {
  TSS(block_stmt, "block");
  TSS(block_stmt, "name: block");
  return true;
}

bool call_stmt() {
  TSS(call_stmt, "call foo");
  TSS(call_stmt, "call foo()");
  TSS(call_stmt, "call foo(bar,2+2,r)");
  TSS(call_stmt, "call s%foo(bar,2+2,r)");
  TSS(call_stmt, "call rs(s = foo)");
  return true;
}

bool case_stmt() {
  TSS(case_stmt, "case (1)");
  TSS(case_stmt, "case (1:)");
  TSS(case_stmt, "case (:2)");
  TSS(case_stmt, "case (1:2)");
  TSS(case_stmt, "CASE ('(')");
  return true;
}

bool codimension_stmt() {
  TSS(codimension_stmt, "codimension a[*]");
  TSS(codimension_stmt, "codimension :: a[:]");
  TSS(codimension_stmt, "codimension :: a[:], b[1:*]");
  TSS(codimension_stmt, "codimension :: a[:], b[1:*], c[3,1:2,1:*]");
  return true;
}
  
bool common_stmt() {
  TSS(common_stmt, "common a");
  TSS(common_stmt, "common a,b");
  TSS(common_stmt, "common /z/ a");
  TSS(common_stmt, "common /z/ a,b");
  TSS(common_stmt, "common /z/ a,b(20)");
  TSS(common_stmt, "common /z/ a,b(:,:) /y/ q");
  TSS(common_stmt, "common /z/ a,b(20) /y/q//r");
  TSS(common_stmt, "common /z/ a,b(20), /y/q, //r");
  TSS(common_stmt, "common // a");
  TSS(common_stmt, "common / / a");
  return true;
}

bool cycle_stmt() {
  TSS(cycle_stmt, "cycle");
  TSS(cycle_stmt, "cycle loop_name");
  return true;
}

bool component_def_stmt() {
  TSS(component_def_stmt, "type(tname) :: v(0:n)");
  return true;
}

bool data_stmt() {
  TSS(data_stmt, "data chart/'t'/");
  TSS(data_stmt, "data chart/'t'/,crun/'.run'/,pathd/'test/'/");
  TSS(data_stmt, "data sfac/.2d0/,sadd/100.d0/");
  return true;
}

bool deallocate_stmt() {
  TSS(deallocate_stmt, "deallocate(foo)");
  TSS(deallocate_stmt, "deallocate(DMesh%SnxS, STAT=status)")
  return true;
}

bool derived_type_stmt() {
  TSS(derived_type_stmt, "type foo");
  TSS(derived_type_stmt, "type::foo");
  TSS(derived_type_stmt, "type, abstract::foo");
  TSS(derived_type_stmt, "type, bind(c)::foo");
  TSS(derived_type_stmt, "type, extends(ptype)::foo");
  TSS(derived_type_stmt,
      "type, private, bind(c), abstract, extends(ptype)::foo");
  TSS(derived_type_stmt, "type, public ::foo(bar,p)");
  return true;
};

bool do_stmt() {
  // nonlabel-do-stmt
  TSS(do_stmt, "do");
  TSS(do_stmt, "dname: do");
  TSS(do_stmt, "do i=1,2");
  TSS(do_stmt, "do, i=1,a(j)");
  TSS(do_stmt, "do i=1,n,2");
  TSS(do_stmt, "do, i=1,n+1,2");
  TSS(do_stmt, "dname: do, i=1,n+1,2");
  FSS(do_stmt, "do while done");
  TSS(do_stmt, "do while(.not. done)");
  TSS(do_stmt, "do: do, while(do)");
  // label-do-stmt
  TSS(do_stmt, "do 10");
  TSS(do_stmt, "while:do 10");
  TSS(do_stmt, "do 10 i=1,10");
  TSS(do_stmt, "do:do 10 i=1,10");
  return true;
}

bool else_stmt() {
  TSS(else_stmt, "else");
  TSS(else_stmt, "else cname");
  return true;
}

bool else_if_stmt() {
  TSS(else_if_stmt, "else if(foo) then");
  TSS(else_if_stmt, "elseif(foo) then");
  TSS(else_if_stmt, "else if(foo) then cname");
  TSS(else_if_stmt, "elseif(foo) then cname");
  return true;
}

bool elsewhere_stmt() {
  TSS(elsewhere_stmt, "elsewhere");
  TSS(elsewhere_stmt, "else where");
  TSS(elsewhere_stmt, "elsewhere lbl");
  TSS(elsewhere_stmt, "else where lbl");
  return true;
}

bool end_block_stmt() {
  TSS(end_block_stmt, "end block");
  TSS(end_block_stmt, "end block name");
  TSS(end_block_stmt, "endblock");
  TSS(end_block_stmt, "endblock name");
  return true;
}

bool end_do() {
  TSS(end_do, "end do");
  TSS(end_do, "end do foo");
  TSS(end_do, "enddo");
  TSS(end_do, "enddo foo");
  TSS(end_do, "100 continue");
  return true;
}

bool end_function_stmt() {
  TSS(end_function_stmt, "end");
  TSS(end_function_stmt, "end function");
  TSS(end_function_stmt, "end function foo");
  return true;
}

bool end_if_stmt() {
  TSS(end_if_stmt, "end if");
  TSS(end_if_stmt, "endif");
  TSS(end_if_stmt, "end if cname");
  TSS(end_if_stmt, "endif cname");
  return true;
}

bool end_subroutine_stmt() {
  TSS(end_subroutine_stmt, "end");
  TSS(end_subroutine_stmt, "end subroutine");
  TSS(end_subroutine_stmt, "end subroutine foo");
  return true;
}

bool end_where_stmt() {
  TSS(end_where_stmt, "end where");
  TSS(end_where_stmt, "end where lbl");
  TSS(end_where_stmt, "endwhere");
  TSS(end_where_stmt, "endwhere lbl");
  return true;
}

bool entry_stmt() {
  TSS(entry_stmt, "entry foo");
  TSS(entry_stmt, "entry foo()");
  TSS(entry_stmt, "entry foo(a)");
  TSS(entry_stmt, "entry foo(a,*,b)");
  TSS(entry_stmt, "entry foo() RESULT(a)");
  TSS(entry_stmt, "entry foo(a) BIND(C)");
  TSS(entry_stmt, "entry foo(a,*,b) bind(c) result(q)");
  return true;
}

bool exit_stmt() {
  TSS(exit_stmt, "exit");
  TSS(exit_stmt, "exit construct_name");
  return true;
}

bool external_stmt() {
  TSS(external_stmt, "external a");
  TSS(external_stmt, "external ::a");
  TSS(external_stmt, "external a,b,c");
  TSS(external_stmt, "external :: a,b,c")
  return true;
}

bool fail_image_stmt() {
  TSS(fail_image_stmt, "FAIL image");
  return true;
}

bool forall_construct_stmt() {
  TSS(forall_construct_stmt, "forall(i=1:strln)");
  FSS(forall_stmt, "forall(i=1:strln)");
  return true;
}

bool format_stmt() {
  TSS(format_stmt, "100 format(boring)");
  TSS(format_stmt, "100 format(it-doesnt [] matter())");
  return true;
}

bool function_stmt() {
  TSS(function_stmt, "recursive pure function foo(a,b,c) bind(c, name=foo)");
  TSS(function_stmt,
      "recursive pure function foo(a,b,c) bind(c, name=foo) result(q)");
  TSS(function_stmt,
      "recursive pure function foo(a,b,c) result(q) bind(c, name=foo)");
  TSS(function_stmt, "recursive pure function foo(a,b,c) result(q)");
  TSS(function_stmt, "integer(kindof(foo)) function foo(a,b,c)");
  TSS(function_stmt, "function foo result(bar)");
  return true;
}

bool generic_stmt() {
  TSS(generic_stmt, "generic::foo=>bar1");
  TSS(generic_stmt, "generic::foo=>bar1,bar2");
  TSS(generic_stmt, "generic,private::foo=>bar1");
  TSS(generic_stmt, "generic,public::foo=>bar1,bar2");
  TSS(generic_stmt, "generic,public::write(unformatted)=>bar1,bar2");
  TSS(generic_stmt, "generic,public::assignment(=)=>bar1");
  return true;
}

bool goto_stmt() {
  TSS(goto_stmt, "go to 1");
  TSS(goto_stmt, "go to 12");
  TSS(goto_stmt, "go to 123");
  TSS(goto_stmt, "go to 1234");
  TSS(goto_stmt, "go to 12345");
  TSS(goto_stmt, "goto 1");
  TSS(goto_stmt, "goto 12");
  TSS(goto_stmt, "goto 123");
  TSS(goto_stmt, "goto 1234");
  TSS(goto_stmt, "goto 12345");
  return true;
}

bool if_stmt() {
  TSS(if_stmt, "if(a==3) a=3");
  TSS(if_stmt, "IF(.TRUE.) RETURN");
  TSS(if_stmt, "if(cond) goto 10"); // test token unsmash
  FSS(if_stmt, "IF (.TRUE.) IF(.FALSE.) THEN");
  return true;
}

bool if_then_stmt() {
  TSS(if_then_stmt, "if(a==3) then");
  TSS(if_then_stmt, "cname: if(.true.) then");
  return true;
}

bool implicit_stmt() {
  TSS(implicit_stmt, "implicit none");
  TSS(implicit_stmt, "implicit none()");
  TSS(implicit_stmt, "implicit none(external)");
  TSS(implicit_stmt, "implicit none(type)");
  TSS(implicit_stmt, "implicit none(type,external)");
  TSS(implicit_stmt, "implicit integer(a)");
  TSS(implicit_stmt, "implicit real(a-z)");
  TSS(implicit_stmt, "implicit real(kind=8)(a-z)");
  TSS(implicit_stmt, "implicit real(8)(a-z)");
  TSS(implicit_stmt, "implicit type(*)(a-z)");
  return true;
}

bool inquire_stmt() {
  TSS(inquire_stmt, "inquire(file=array(1:ub), exist=iexist)");
  TSS(inquire_stmt, "inquire(iolength=RECLEN) recltest");
  return true;
}

bool intent_stmt() {
  TSS(intent_stmt, "intent(in) a, b");
  TSS(intent_stmt, "intent(out) :: a, b");
  TSS(intent_stmt, "intent(inout) :: a");
  return true;
}

bool interface_stmt() {
  TSS(interface_stmt, "interface operator(-)");
  return true;
}

bool masked_elsewhere_stmt() {
  TSS(masked_elsewhere_stmt, "else where (a < 3)");
  TSS(masked_elsewhere_stmt, "elsewhere (a < 3)");
  TSS(masked_elsewhere_stmt, "else where (a < 3) lbl");
  TSS(masked_elsewhere_stmt, "elsewhere (a < 3) lbl");
  return true;
}

bool namelist_stmt() {
  TSS(namelist_stmt, "namelist /a/ b");
  TSS(namelist_stmt, "namelist /a/ b,c,d");
  TSS(namelist_stmt, "namelist /a/ b,c,d, /x/ y");
  TSS(namelist_stmt, "namelist /a/ b,c,d, /x/ y,z");
  TSS(namelist_stmt, "namelist /a/ b,c,d, /x/ y,z /m/n");
  return true;
}

bool parameter_stmt() {
  TSS(parameter_stmt, "100 parameter(a=1)");
  TSS(parameter_stmt, "100 parameter(a=1, b=4+2/2**8)");
  return true;
}

bool pointer_assignment_stmt() {
  TSS(pointer_assignment_stmt, "a=>b");
  TSS(pointer_assignment_stmt, "a=>b(1:5)");
  TSS(pointer_assignment_stmt, "m%a=>b");
  return true;
}

bool pointer_stmt() {
  TSS(pointer_stmt, "pointer a");
  TSS(pointer_stmt, "pointer a,b");
  TSS(pointer_stmt, "pointer :: a");
  TSS(pointer_stmt, "pointer :: a,b");
  TSS(pointer_stmt, "pointer a(:),b");
  TSS(pointer_stmt, "pointer::a(:,:),b(:)");
  return true;
}

bool print_stmt() {
  TSS(print_stmt, "print *, foo");
  TSS(print_stmt, "print 100, foo");
  TSS(print_stmt, "print (1xa), foo");
  TSS(print_stmt, "print (1xa), foo+2");
  TSS(print_stmt, "print *, 'hello world'");
  TSS(print_stmt, "print *, 'hello world', (a(i),b(i), i=1,n)");
  return true;
}

bool protected_stmt() {
  TSS(protected_stmt, "protected a");
  TSS(protected_stmt, "protected :: a");
  TSS(protected_stmt, "protected a,b,c");
  TSS(protected_stmt, "protected :: a,b,c");
  return true;
}

bool read_stmt() {
  TSS(read_stmt, "read (6, *) size");
  TSS(read_stmt, "read 10, a, b");
  TSS(read_stmt, "read *, a(lbound(a,1):ubound(a,1))");
  TSS(read_stmt, "read (6, fmt=\"(\" // CHAR_FMT // \")\") X, y, z");
  TSS(read_stmt, "read (iostat=ios, unit=6, FMT='(10f8.2)') a,b");
  return true;
}

bool return_stmt() {
  TSS(return_stmt, "return");
  TSS(return_stmt, "return a+3");
  return true;
}

bool select_case_stmt() {
  TSS(select_case_stmt, "CHECK_PARENS: SELECT CASE (LINE (I:I))");
  return true;
}

bool select_rank_stmt() {
  TSS(select_rank_stmt, "CHECK_PARENS: SELECT rank(x)");
  TSS(select_rank_stmt, "SELECT rank(x)");
  return true;
}

bool select_rank_case_stmt() {
  TSS(select_rank_case_stmt, "rank default");
  TSS(select_rank_case_stmt, "rank (*)");
  TSS(select_rank_case_stmt, "rank (2)");
  TSS(select_rank_case_stmt, "rank (2+1)");
  return true;
}


bool subroutine_stmt() {
  TSS(subroutine_stmt, "subroutine foo");
  TSS(subroutine_stmt,
      "recursive pure subroutine foo(a,b,c) bind(c, name=foo)");
  return true;
}

bool type_bound_generic_stmt() {
  TSS(type_bound_generic_stmt, "generic :: foo=>bar");
  TSS(type_bound_generic_stmt, "generic,public :: foo=>bar");
  TSS(type_bound_generic_stmt, "generic,private :: foo=>bar,dip");
  TSS(type_bound_generic_stmt, "generic,public :: operator(.foo.)=>bar");
  TSS(type_bound_generic_stmt, "generic:: assignment(=)=>bar");
  TSS(type_bound_generic_stmt, "generic:: read(formatted)=>bar");
  return true;
}

bool type_bound_proc_binding() {
  TSS(type_bound_proc_binding, "procedure, private::save_to");
  return true;
}

bool use_stmt() {
  TSS(use_stmt, "use foo");
  TSS(use_stmt, "use :: foo");
  TSS(use_stmt, "use, intrinsic:: foo");
  TSS(use_stmt, "use foo, a=>b");
  TSS(use_stmt, "use foo, a=>b, operator(.my.)=>operator(.notmy.)");
  TSS(use_stmt, "use foo, only:");
  TSS(use_stmt, "use foo, only:f1,f2");
  TSS(use_stmt, "use foo, only:assignment(=)");
  TSS(use_stmt, "use, intrinsic:: foo, only:a=>b");
  return true;
}

bool value_stmt() {
  TSS(value_stmt, "value a");
  TSS(value_stmt, "value :: a");
  TSS(value_stmt, "value a, b,c");
  TSS(value_stmt, "value ::a,b,c");
  return true;
}

bool where_construct_stmt() {
  TSS(where_construct_stmt, "where(.true.)");
  TSS(where_construct_stmt, "lbl: where(.true.)");
  FSS(where_construct_stmt, "where(.true.) a=3");
  return true;
}

bool where_stmt() {
  TSS(where_stmt, "where(a>3) a= 3");
  return true;
}

bool write_stmt() {
  TSS(write_stmt, "write (6, 10) A, S, J");
  TSS(write_stmt, "write (lp, fmt='(10f8.2)') (LOG(a(i)), i=1,n+9, k), g");
  TSS(write_stmt, "write(*,100) e/(x+f)");
  return true;
}

// For one-keyword statements
bool easy() {
  TSS(contains_stmt, "contains");
  TSS(continue_stmt, "continue");
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(easy);
  TEST(allocatable_stmt);
  TEST(allocate_stmt);
  TEST(arithmetic_if_stmt);
  TEST(asynchronous_stmt);
  TEST(assignment_stmt);
  TEST(bind_stmt);
  TEST(block_stmt);
  TEST(call_stmt);
  TEST(case_stmt);
  TEST(codimension_stmt);
  TEST(common_stmt);
  TEST(component_def_stmt);
  TEST(cycle_stmt);
  TEST(data_stmt);
  TEST(deallocate_stmt);
  TEST(derived_type_stmt);
  TEST(do_stmt);
  TEST(else_stmt);
  TEST(else_if_stmt);
  TEST(elsewhere_stmt);
  TEST(end_block_stmt);
  TEST(end_do);
  TEST(end_function_stmt);
  TEST(end_if_stmt);
  TEST(end_subroutine_stmt);
  TEST(end_where_stmt);
  TEST(entry_stmt);
  TEST(exit_stmt);
  TEST(external_stmt);
  TEST(fail_image_stmt);
  TEST(forall_construct_stmt);
  TEST(format_stmt);
  TEST(function_stmt);
  TEST(generic_stmt);
  TEST(goto_stmt);
  TEST(if_stmt);
  TEST(if_then_stmt);
  TEST(implicit_stmt);
  TEST(intent_stmt);
  TEST(interface_stmt);
  TEST(inquire_stmt);
  TEST(masked_elsewhere_stmt);
  TEST(namelist_stmt);
  TEST(parameter_stmt);
  TEST(pointer_assignment_stmt);
  TEST(print_stmt);
  TEST(protected_stmt);
  TEST(read_stmt);
  TEST(return_stmt);
  TEST(select_case_stmt);
  TEST(select_rank_stmt);
  TEST(select_rank_case_stmt);
  TEST(subroutine_stmt);
  // test_type_declaration_stmt is handled in test_parse_type_decl.cc
  TEST(type_bound_generic_stmt);
  TEST(type_bound_proc_binding);
  TEST(use_stmt);
  TEST(value_stmt);
  TEST(where_construct_stmt);
  TEST(where_stmt);
  TEST(write_stmt);
  TEST(associate_stmt);
  TEST_MAIN_REPORT;
}
