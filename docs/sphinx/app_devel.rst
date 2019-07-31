.. _app_devel:

============================
FLPR Application Development
============================

This (very incomplete) section discusses designing and developing an
application using FLPR.

--------------------------
Types of FLPR Applications
--------------------------

We tend to categorize FLPR-based applications into two modes of
operation: one-shot vs. continuous transformations.  By "one-shot"
transformation, we mean that the application is used to transform the
source code once, and the resultant code is used by developers from
that point forward (i.e. committed to a repository).  An example of a
one-shot code transformations are code modernization tasks, such as
transforming ``real*8`` to ``real(kind=k)``. By "continuous"
transformation, we mean that an application is called as a souce
preprocessor before each and every compilation pass. Continuous
transformation may be used to translate application-specific
extensions to Fortran, or to inject compiler-specific optimization
directives.  FLPR is intended to make writing one-shot utilities
quick and easy, while giving you the power to build production-level
tools to include in your system.

--------------------------------
Fundamental FLPR Data Structures
--------------------------------

``File_Line``
  This subdivides a line of program text into fields and
  provides a categorization for it.  Examples of fields include ``left_txt``
  (control columns for fixed form), ``left_space`` (whitespace between
  ``left_txt`` and ``main_txt``) and ``main_txt`` (the body of the
  Fortran line).  Categorization flags include ``comment``, ``blank``,
  ``label``, etc.

``Logical_Line``
  This structure combines one or more (continued) ``File_Line`` into a
  a sequence of one or more (compound) Fortran statements.

``LL_Stmts``
  This provides a sequential list of statements found throughout a
  list of ``Logical_Line``.

``Stmt_Tree``
  A concrete syntax tree (CST) that organizes the tokens in one
  ``LL_Stmt``.

``Prgm_Tree``
  A CST organizing sequences of ``Stmt_Tree`` into program structures.

``Procedure``
  A high-level view of a subtree of ``Prgm_Tree`` representing a
  procedure block.
  
