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


  
