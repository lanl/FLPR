# Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# FLPR/docs/CMakeLists.txt

find_package(Doxygen)
set(DOXYGEN_GENERATE_MAN no)
set(DOXYGEN_RECURSIVE NO)
set(DOXYGEN_FULL_PATH_NAMES NO)
doxygen_add_docs(
  doxygen
  ${PROJECT_SOURCE_DIR}/src/flpr
  )
