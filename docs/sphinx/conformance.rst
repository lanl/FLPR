.. _conformance:

=====================
Standards Conformance
=====================

This document discusses the language that FLPR recognizes, and where
there are deficiencies.

---------------
Standards Basis
---------------

This implementation is based on the 28-DEC-2017 Draft N2146 of the
ISO/IEC JTC1/SC22/WG5 Fortran Language Standard, available from the 
`Fortran Working Group <https://wg5-fortran.org/documents.html>`_.  As
such, it targets the language of the Fortran 2018 standard.

-------------------
Explicit Deviations
-------------------

There are several situations where we have elected not to conform with
the Fortran specification:

1. The FLPR parser *is sensitive* to whitespace in the input text in
   *both* free and fixed source form. This is a deviation from Section
   6.3.3.1.2. If you have very traditional fixed form source code
   which does not use spaces to delimit names and keywords, you will need
   to preprocess that source code with another tool before processing it
   with FLPR.
2. The FLPR parser does not recognize any nondefault character set.


-------------------
Retro-Extensions
-------------------

Some features that have been deleted or made obsolescent in the
current standard have been implemented to assist modernization
efforts:

- FORTRAN 77 *kind-selector* (e.g. ``real*8``)
- *computed-goto-stmt*
- *arithmetic-if-stmt*
- *forall-stmt*
- *label-do-stmt*
- *non-block-do*

Note that Hollerith contants are *not* implemented. 

   
--------------------------------
Incomplete Sub-statement Parsing
--------------------------------

There are situations where FLPR does not build a completely elaborated
concrete syntax tree for a statement. For example, the syntax rules:

* *expr*
* *io-control-spec-list*
* *position-spec-list*

are examples of incomplete parsers in FLPR (there are many).  The
expected behavior is that FLPR will indicate a range of tokens that
would be matched by these rules, but present them as a simple sequence
rather than as a tree.

FLPR is implemented in a top-down fashion, so the user should expect
that FLPR can parse the general structure of a *program*, but the
details of an individual statement may not be fully elaborated.



------------------
Known Deficiencies
------------------

There are some parsers that simply have not been implemented yet.  The
current list includes:

* *asynchronous-stmt*
* *bind-stmt*
* *block-data*
* *change-team-construct*
* *codimension-stmt*
* *critical-construct*
* *event-post-stmt*
* *event-wait-stmt*
* *fail-image-stmt*
* *form-team-stmt*
* *lock-stmt*
* *submodule*
* *sync-all-stmt*
* *sync-images-stmt*
* *sync-memory-stmt*
* *sync-team-stmt*
* *unlock-stmt*
* *wait-stmt*  

These will be implemented soon, but if one in particular is holding
you up, please file an issue on GitHub.  Contributions are always
welcome!



-------------------
Potential "gotchas"
-------------------
- FLPR ignores Fortran ``include`` directives and all preprocessor
  symbols, so it may get confused if you depend on macro expansion or
  file inclusion to produce valid Fortran.  Note that there is an
  internal FLPR preprocessor, but it only works when directed to.
- FLPR parsers produce *concrete* syntax trees, which include all of
  the punctuation and layout information.  These are challenging to
  extract information from.  See the function ``has_call_named()`` in
  ``apps/module.cc`` for an example of how to traverse the concrete
  syntax tree.  Going forward, more abstract representations will be
  introduced. 
- When looking for *action-stmt* in the input to transform, remember to
  look "inside" *if-stmt*, which contain an *action-stmt*!



