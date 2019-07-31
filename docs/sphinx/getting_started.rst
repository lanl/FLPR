.. _getting_started:

===============
Getting Started
===============

This page provides information on how to get FLPR installed, and gives
some pointers on exploring sample applications.

.. contents::


---------------
Installing FLPR
---------------

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

FLPR has a few external tool requirements.  Their minimum versions are:

* CMake_ v3.12
* C++17 support.  FLPR is regularly built with gcc8, gcc9, and clang8
* flex_ v2.6.4.  Note that any compilation warnings/errors about C++17 not
  accepting the ``register`` keyword are due to using an old version
  of ``flex``. 
* gperf_ v3.1 (required only if changing one particular source file)

.. _CMake: https://cmake.org/download/
.. _flex: https://github.com/westes/flex/
.. _gperf: https://www.gnu.org/software/gperf/

^^^^^^^^^^^
Obtain FLPR
^^^^^^^^^^^

The main home for FLPR is on GitHub_, and it can cloned with:

.. code-block:: bash

  $ git clone https://github.com/LANL/FLPR.git

.. _GitHub: https://github.com/lanl/FLPR
  
^^^^^^^^^
Configure
^^^^^^^^^

FLPR uses CMake to manage the configuration process. Make sure that
you have a ``flex`` and modern compiler available in the path and the
configuration is as simple as:

.. code-block:: bash

  $ mkdir build && cd build
  $ cmake /path/to/flpr


There are two types of cmake build types supported: ``Debug`` or
``Release``. These build configurations change the compiler flags
being used.  Development work should always be done in ``Debug`` mode
(the default), while deployed builds should be done with
``Release``.  Specify a build type using *one* of these defines after
the cmake command:

.. code-block:: bash
		
   -DCMAKE_BUILD_TYPE=Debug
   -DCMAKE_BUILD_TYPE=Release

^^^^^
Build
^^^^^

Once CMake has completed, FLPR can be built with make:

.. code-block:: bash

  $ make

Note, by default, the FLPR test suite is built, but not executed.  To
run the tests, simply do: 

.. code-block:: bash

  $ make test

While the Sphinx documentation (what you are reading now) is not yet
integrated into CMake, the code documentation is, and can be built with:

.. code-block:: bash

  $ make doxygen

The code documentation is not installed, but is found in the
``docs/html/index.html`` file under the build directory.

^^^^^^^
Install
^^^^^^^

To install FLPR, just run:

.. code-block:: bash

  $ make install

FLPR installs files to the ``lib``, ``include`` and ``bin``
directories of the ``CMAKE_INSTALL_PREFIX``. Additionally, FLPR
installs a CMake configuration file that can help you use FLPR in
other projects. By setting ``FLPR_DIR`` to point to the root of your
FLPR installation, you can call ``find_package(FLPR)`` inside your
CMake project and FLPR will be automatically detected and available
for use.


-------------------
Sample Applications
-------------------

The FLPR library also contains some sample applications, which are
intended to a feeling of what FLPR application development is like.
The samples can be found in the ``apps/`` directory, and they are
built and installed during the normal build process.

caliper.cc
   Demonstration program to insert fictitious performance caliper
   calls in each external subprogram and module subprogram (not
   internal subprograms).  The caliper calls include the subprogram
   name as an actual parameter, and mark the beginning and end of each
   executable section.  The executable section is scanned for
   conditional return statements: if they exist, a labeled continue
   statement is introduced about the end caliper, and the return is
   replaced with a branch to that continue.

ext_demo.cc
   Demonstration of how to extend an *action-stmt* parser.  This
   example introduces a rule that accepts a comma after the
   *io-control-spec-list* of a *write-stmt*, which, while not allowed
   by the standard, is accepted by many Fortran compiler front-ends.

flpr-format.cc
   A weak attempt at an analogue to ``clang-format``.  It can do
   fixed-form to free-form conversion, reindent code using specified
   rules, remove empty compound statements (e.g. convert ``foo; ;
   bar`` to ``foo; bar``), or split compound statements. See
   :ref:`FLPRApps-label` for details on why the structure of this file
   is unusual.
   
module.cc
   Demonstrating how to selectively insert a *use-stmt* into
   subprograms that contain a *call-stmt* to a particular name.  You
   may want functionality like this when moving old code into modules.

parse_files.cc
   Just has FLPR build parse trees for a list of inputs.  You can use
   this to see if FLPR would run into any problems on your code base.


.. _FLPRApps-label:

------------------------------------------
Extending sample applications with FLPRApp
------------------------------------------

The sample application ``apps/flpr-format.cc`` is a thin wrapper
around calls to ``apps/flpr_format_base.hh``.  The idea behind this is
to give you the opportunity to extend the input grammar, without
having to replicate all of the flpr-format functionality.  This
structure will be extended to the ``module`` demonstration as well.

The flpr_format_base files are exported as a CMake package called
FLPRApp, which contains application-related includes and libraries.
To make an extended version of flpr-format, you need to add a
``find_package(FLPRApp)`` to your CMake configuration (in addition to
``find_package(FLPR)``), and then build something that looks like
``apps/flpr-format.cc`` but registers FLPR statement extensions before
looping across the files.


