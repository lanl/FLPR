/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#include "flpr/parse_stmt.hh"
#include "parse_helpers.hh"

using namespace FLPR;
using FLPR::Stmt::Stmt_Tree;

bool actual_arg_spec() {
  TPS(actual_arg_spec, "a+7, p=foo", TK_COMMA);
  return true;
}

bool allocate_coarray_spec() {
  // No brackets
  TSS(allocate_coarray_spec, "*");
  /* The first 1 is a label to feed to the File_Line parser so it doesn't
     eat the first scalar-int */
  TSS(allocate_coarray_spec, "1 1,*");
  TSS(allocate_coarray_spec, "1 1,2,3,*");
  TSS(allocate_coarray_spec, "1 1:1,2,3,*");
  TSS(allocate_coarray_spec, "1 1:1,1:2,1:3,1:*");
  return true;
}

bool array_element() {
  TPS(array_element, "a(1)", BAD);
  TPS(array_element, "a(1,2)", BAD);
  TPS(array_element, "a(b)(1)", TK_PARENL);
  TPS(array_element, "c%a(b)(1)", TK_PARENL);
  TPS(array_element, "c(2)%d%a(b)(1)", TK_PARENL);
  TPS(array_element, "c(3)%a(b)(1)", TK_PARENL);
  FPS(array_element, "a(1:3)", TK_NAME);
  FPS(array_element, "a=", TK_NAME);
  FPS(array_element, "a%b", TK_NAME);
  TPS(array_element, "c(2)[5,2,STAT=var]%d[1]%a(b)[3](1)", TK_PARENL);
  return true;
}

bool coarray_spec() {
  TPS(coarray_spec, ":]", TK_BRACKETR);   // deferred-coshape-spec-list
  TPS(coarray_spec, ":,:]", TK_BRACKETR); // deferred-coshape-spec-list
  TPS(coarray_spec, "*]", TK_BRACKETR);   // explict-coshape-spec

  /* The first 1 is a label to feed to the File_Line parser so it doesn't
     eat the first scalar-int */
  TPS(coarray_spec, "1 3,*]", TK_BRACKETR);     // explict-coshape-spec
  TPS(coarray_spec, "1 3:*]", TK_BRACKETR);     // explict-coshape-spec
  TPS(coarray_spec, "1 1,3:*]", TK_BRACKETR);   // explict-coshape-spec
  TPS(coarray_spec, "1 1:2,3:*]", TK_BRACKETR); // explict-coshape-spec
  return true;
}

bool expr() {
  TPS(expr, "a", BAD);
  TPS(expr, "'a b c',", TK_COMMA);
  TPS(expr, "\"a b c\",", TK_COMMA);
  TPS(expr, "','", BAD);
  TPS(expr, "a==3,", TK_COMMA);
  FPS(expr, "a=3", TK_NAME);
  TPS(expr, "a+b(3,[:])", BAD);
  TPS(expr, "a+b(3,[:]))", TK_PARENR);
  TPS(expr, "a+b(3,[:])]", TK_BRACKETR);
  TPS(expr, "a+b%q(3,[:]),]", TK_COMMA);
  TPS(expr, "a**3+b(3,[:]):]", TK_COLON);
  TSS(expr, "999 1");
  FPS(expr, ":", TK_COLON);
  return true;
}

bool generic_spec() {
  TSS(generic_spec, "foo");             // generic-name
  TSS(generic_spec, "write");           // generic-name
  TSS(generic_spec, "OPERATOR(.def.)"); // defined-operator
  TSS(generic_spec, "assignment(=)");   // assignment-operator
  TSS(generic_spec, "READ(FORMATTED)");
  TSS(generic_spec, "READ(UNFORMATTED)");
  TSS(generic_spec, "WRITE(FORMATTED)");
  TSS(generic_spec, "WRITE(UNFORMATTED)");
  return true;
}

bool image_selector() {
  TPS(image_selector, "[1]", BAD);
  TPS(image_selector, "[a+1]", BAD);
  TPS(image_selector, "[a+1,b-1]", BAD);
  TPS(image_selector, "[a+1,b-1,stat=a]", BAD);
  TPS(image_selector, "[2,3,4,stat=a]", BAD);
  TPS(image_selector, "[2+2,team=3,stat=a]", BAD);
  return true;
}

bool proc_component_ref() {
  TPS(proc_component_ref, "a % b =>", TK_ARROW);
  return true;
}

bool procedure_designator() {
  TSS(procedure_designator, "a");                     // procedure-name
  TSS(procedure_designator, "a%b");                   // proc-component-ref
  TSS(procedure_designator, "a(1)%b");                // data-ref % binding-name
  TSS(procedure_designator, "a(::)%b");               // data-ref % binding-name
  TSS(procedure_designator, "q%a(:)[z, STAT=foo]%b"); // data-ref % binding-name

  TPS(procedure_designator, "a()", TK_PARENL);       // procedure-name
  TPS(procedure_designator, "a%b()", TK_PARENL);     // proc-component-ref
  TPS(procedure_designator, "a(1)%b()", TK_PARENL);  // data-ref % binding-name
  TPS(procedure_designator, "a(::)%b()", TK_PARENL); // data-ref % binding-name
  TPS(procedure_designator, "q%a(:)[z, STAT=foo]%b()",
      TK_PARENL); // data-ref % binding-name

  return true;
}

bool variable() {
  TPS(variable, "a", BAD);
  TPS(variable, "a+", TK_PLUS);
  TPS(variable, "a=", TK_EQUAL);
  TPS(variable, "a%b(:)=", TK_EQUAL);
  TPS(variable, "b[2,3,4,stat=var]=", TK_EQUAL);
  return true;
}

bool wait_spec() {
  // Fake-out the line parser with a label
  TSS(wait_spec, "9999 1");      // file-unit-number
  TSS(wait_spec, "myfile");      // file-unit-number
  TSS(wait_spec, "unit+1");      // file-unit-number
  TSS(wait_spec, "unit=1");      // file-unit-number
  TSS(wait_spec, "unit=unit+1"); // file-unit-number
  TSS(wait_spec, "unit=myfile"); // file-unit-number
  TSS(wait_spec, "end=3");       // label
  TSS(wait_spec, "eor=32");      // label
  TSS(wait_spec, "err=330");     // label
  TSS(wait_spec, "id=a+3");      // scalar-int-expr
  TSS(wait_spec, "iomsg=iomsg(i+1)");
  TSS(wait_spec, "iostat=foo");
  return true;
}

int main() {
  TEST_MAIN_DECL;
  TEST(actual_arg_spec);
  TEST(allocate_coarray_spec);
  TEST(array_element);
  TEST(coarray_spec);
  TEST(expr);
  TEST(image_selector);
  TEST(proc_component_ref);
  TEST(procedure_designator);
  TEST(generic_spec);
  TEST(variable);
  TEST(wait_spec);
  TEST_MAIN_REPORT;
}
