![flipper-logo](docs/flpr-logo.png)
# FLPR: The Fortran Language Program Remodeling system

The *Fortran Language Program Remodeling* system, or FLPR, is a C++17
library for manipulating Fortran source code. This package contains:
- A "best effort" Fortran 2018 input parser for fixed and free
  form inputs. 
- Data structures for manipulating source code at the program unit,
  statement, and physical line levels.
- Sample applications (in the `apps/` directory) that illustrate usage
  and provide some ideas as to how you could use the library.


## Getting started

Please see the documentation on [Read the
Docs](https://flpr.readthedocs.io), or in the
[getting started](docs/sphinx/getting_started.rst) file.


Any questions? Please file an issue on GitHub, or email <flpr-dev@lanl.gov>.

## Project status

FLPR is undergoing active development, and isn't ready for a formal
release yet.  In general, the default `develop` branch is expected to
be functional at all times, but all other branches are "as-is."  Feel
free to fork FLPR and submit PRs for new features or fixes, but please
file an issue in the tracker before starting, just so we know what
people are thinking about.

## Authors

FLPR was created by Paul Henning (<phenning@lanl.gov>).


## Release

FLPR is released as open source under a BSD-3 License.  For more
details, please see the [LICENSE](LICENSE) and [NOTICE](NOTICE) files.

`LANL C19039`

Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
