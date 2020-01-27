.. _coding_practices:

=================================
Coding Practices For Contributors
=================================

This page provides information on project expectations for C++ coding
practices.  Contributors should check that these practices are followed
before submitting a pull request.

.. contents::


---------------
C++ Code Layout
---------------

FLPR uses ``clang-format`` to manage C++ code layout.  There is a
``.clang-format`` file provided in the root directory, which is a copy
of the LLVM style layout.  Please run ``clang-format`` before commiting
code: other developers should be able to run ``git pull``, then
``clang-format``, and see only their own changes.


--------------------------------------
C++ Language Standards and Portability
--------------------------------------

It is expected that the FLPR code base can be compiled with a C++ '17
compliant compiler, without compiler-specific extensions.  If
possible, please try both current GCC and LLVM compilers for
portability.

------------------
Code Documentation
------------------

Adding Doxygen documentation blocks for files and functions is
encouraged.  Please use the ``Qt`` style of block comment without
intermediate "*"::
  
  /*!
    \file somename.cc
    ... more doxygen text ...
  */


----------
File Names
----------

The following file name extensions are to be used:

  .hh & .cc
    C++ language header and source files

  .l
    Flex input

  .md
    Markdown-style documentation

  .rst
    Restructured Text-style documentation

File names should not depend on case-sensitivity for uniqueness.

--------------
Include Guards
--------------

The code section of a C++ header file should be wrapped with an include
guard ``#ifdef``.  For the file ``FLPR/src/flpr/Stmt_Tree.hh``, the
guards should look like::
  
  #ifndef FLPR_STMT_TREE_HH
  #define FLPR_STMT_TREE_HH 1

  ... declarations ...

  #endif

 

