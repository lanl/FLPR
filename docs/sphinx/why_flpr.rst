.. _why_flpr:

=========
Why FLPR?
=========

A fair question to ask is "why did you develop FLPR when there are
several available open-source Fortran front-ends?"  The main reason is
that FLPR is designed for performing complex transformations on
Fortran source code, whereas compiler front-ends are designed to
produce intermediate representations of the code semantics.  While
both toolsets require an understanding the syntax of a Fortran source
file, each toolset has a very different view of what is *important*
about that file. For FLPR, the most important thing to capture is the
layout of the text in the file, while a compiler front-end wants to
discard that information as quickly as possible.

For simple tasks like converting ``real*8`` to ``real(kind=k)``,
generic text transformation tools like ``sed`` and ``awk`` can easily
make changes across a large number of source files. However, consider
the following one-shot modernization task. Suppose your project uses
the convention that uppercased dummy arguments in subroutines implies
that they are output arguments, and lowercase implies that they are
input:

.. code-block:: fortranfixed

   c     fixed-format file    
         subroutine foo(n,m,S)
         integer n,m,S,locali
         ...

You can use FLPR to transform the type declarations into a modern form:

.. code-block:: fortran

   ! free-format
   subroutine foo(n,m,S)
     integer, intent(in) :: n,m
     integer, intent(out) :: S
     integer :: locali
     ...

Doing so requires syntactic information that would be extremely
complicated to encode in terms of the regular expressions required by
generic text transformation tools.

Another reason for using FLPR over most other tools is that it has an
extensible grammar.  You can easily register new *action-stmt* or new
*other-specification-stmt* parsers (extending block-like structures
is on the TO-DO list).  With this capability, you can implement
continuous transformation tools that translate application-specific
statements into standard Fortran.  Some examples where this might be
useful are:

1. Replacing boilerplate code with simple-to-maintain statements;
2. Testing out potential new standard Fortran language features;
3. or adapting code to particular platform characteristics.

