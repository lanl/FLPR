================
Welcome to FLPR!
================

.. epigraph::
   **flipper** *noun*

   1. A component of pinball machines, which were a staple of 1970s arcades
      but still have ardent fans.
   2. One who renovates neglected properties for profit.

------------
Introduction
------------

The *Fortran Language Program Remodeling* system, or FLPR, is a C++
library for manipulating Fortran source code. You could use FLPR to
quickly develop small code maintenance utilities, or to build
something as ambitious as a source-to-source translation tool to
implement language extensions for your project.

-------------------------------
Distinguishing features of FLPR
-------------------------------

- Layout and comment management:

  - Gives developers recognizable code when debugging transformed source.
  - Preserves important knowledge stored in comments.

- Proper extensible grammar:

  - Better than regexp at recognizing complex structures.
  - Allows implementation of application-specific language features.

- Compact API allows you to quickly write "disposable" utilities for
  doing specific code transformations.



Any questions?  Please file an issue on GitHub, or email
flpr-dev@lanl.gov


-------------------------------------------------------------------------------

.. toctree::
   :maxdepth: 2
   :caption: Basics 

   getting_started
   

.. toctree::
   :maxdepth: 2
   :caption: Using FLPR

   app_devel
   flpr_devel
   
.. toctree::
   :maxdepth: 2
   :caption: FAQs

   conformance
   why_flpr

   


