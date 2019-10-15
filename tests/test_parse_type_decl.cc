/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*
  Test the type_declaration_stmt parser
*/

#include "flpr/parse_stmt.hh"
#include "parse_helpers.hh"

using namespace FLPR;
using FLPR::Stmt::Stmt_Tree;

#define TDTEST(S) TSS(type_declaration_stmt, S)

bool test_simple() {
  TDTEST("integer i");
  TDTEST("real r");
  TDTEST("complex a,b");
  TDTEST("double precision a,b");
  TDTEST("doubleprecision a,b");

  {
    LL_Helper l({"double precision::a"});
    TT_Stream ts = l.stream1();
    Stmt_Tree st = FLPR::Stmt::type_declaration_stmt(ts);
    TEST_TREE(st, type_declaration_stmt, l);
    auto c = st.ccursor();
    c.down(4);
    TEST_TAG(c->syntag, KW_DOUBLEPRECISION);
    TEST_INT(c.num_branches(), 2);
    TEST_TOK_EQ(Syntax_Tags::BAD, ts.peek(), l);
  }

  {
    LL_Helper l({"doubleprecision::a"});
    TT_Stream ts = l.stream1();
    Stmt_Tree st = FLPR::Stmt::type_declaration_stmt(ts);
    TEST_TREE(st, type_declaration_stmt, l);
    auto c = st.ccursor();
    c.down(4);
    TEST_TAG(c->syntag, KW_DOUBLEPRECISION);
    TEST_INT(c.num_branches(), 0);
    TEST_TOK_EQ(Syntax_Tags::BAD, ts.peek(), l);
  }

  return true;
}

bool test_character() {
  TDTEST("character c");

  // modern length selectors
  TDTEST("character(len=*) c");
  TDTEST("character(len=:) :: c");
  TDTEST("character(len=3+2) c");
  TDTEST("character(*) c");
  TDTEST("character(:) c");
  TDTEST("character(32) c");

  // ancient length selectors
  TDTEST("character*(*) c");
  TDTEST("character*(:) c");
  TDTEST("character*(32) c");
  TDTEST("character*32 c");
  // These next two test R722 + C725 and C726
  TDTEST("character*20, intent(in) :: foo");
  TDTEST("character*20, a, b, c");
  // Test extension that allows names in char-length
  TDTEST("character :: s1*28, l*d_len");
  TDTEST("character*e_len :: readvar");

  // entity-decl selectors
  TDTEST("character :: ohmy(20), star*30");

  return true;
}

bool test_intent() {
  TDTEST("real,intent(in) :: i");
  TDTEST("real,intent(inout) :: i");
  TDTEST("real,intent(in out) :: i");
  TDTEST("real,intent(out) :: i");
  return true;
}

bool test_kind() {
  TDTEST("real*8 i");
  TDTEST("real*8 :: i");
  TDTEST("real(4) i");
  TDTEST("real(real32) i");
  TDTEST("real(kind=kind(0.d0)) i");
  TDTEST("real(kind(0.d0)) i");
  // Do integer as well, as it is in a different production
  TDTEST("integer*8 i");
  TDTEST("integer*8 :: i");
  TDTEST("integer(4) i");
  TDTEST("integer(kind=kind(0.d0)) i");
  TDTEST("integer(kind(0.d0)) i");
  return true;
}

bool test_intrinsic_type_spec() {
  TDTEST("type(integer) :: b");
  TDTEST("type(real) :: b");
  TDTEST("type(character) :: b");
  TDTEST("type(integer(4)) :: b");
  TDTEST("type(real(kind64)) :: b");
  return true;
}

bool test_derived_type_spec() {
  TDTEST("class(tname) b");
  TDTEST("type(tname) :: b");
  TDTEST("type(tname(tpv)) :: b");
  TDTEST("class(tname(key=tpv)) b");
  TDTEST("type(tname(key=:,k2=*,k3=name)) b");
  // Throw these in for fun
  TDTEST("type(*) whatev");
  TDTEST("class(*) :: whatev");
  TDTEST("type(tname) :: v(0:n)");
  return true;
}

bool test_codimension() {
  TDTEST("integer, codimension[:] :: i");
  TDTEST("integer, codimension[:,:] :: i");
  TDTEST("integer, codimension[:,:,:,:,:] :: i");
  TDTEST("integer i[:,:]");
  return true;
}

bool test_dimension() {
  TDTEST("integer, dimension(:) :: i");
  TDTEST("integer, dimension(:,:) :: i");
  TDTEST("integer, dimension(:,:,:,:,:) :: i");
  TDTEST("integer i(:,:)");
  TDTEST("integer i(2)");
  TDTEST("integer i(n,m)");

  TDTEST("integer(INT64) :: temp(size(a%b))");

  return true;
}

bool test_mixed_attr() {
  TDTEST("integer, parameter :: N = 1000000");
  TDTEST("integer, private, parameter :: N = 1000000, q=3");
  TDTEST("real(real64), save, dimension(2) :: accum = (/ 0.d0, 0.d0 /)");
  TDTEST("real(real64), save, dimension(2) :: accum = [0.d0, 0.d0]");
  TDTEST("real(real64), save, target, dimension(2) :: accum = [0.d0, 0.d0]");
  TDTEST("complex, volatile, public, allocatable :: foo");
  TDTEST("real(c_double), pointer, dimension(:,:) :: foo");
  TDTEST("real, external :: f");
  return true;
}

bool test_array_spec() {
  // assume-implied-spec
  TDTEST("integer foo(*)");
  TDTEST("integer foo(5:*)");
  TDTEST("integer foo(dim1,*)");
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(test_simple);
  TEST(test_character);
  TEST(test_intent);
  TEST(test_kind);
  TEST(test_intrinsic_type_spec);
  TEST(test_derived_type_spec);
  TEST(test_codimension);
  TEST(test_dimension);
  TEST(test_mixed_attr);
  TEST(test_array_spec);
  TEST_MAIN_REPORT;
}
