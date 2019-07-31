# Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# FLPR/CMakeLists.txt

cmake_minimum_required (VERSION 3.12)
project (FLPR VERSION 0.1.0 LANGUAGES CXX)

# Allow CMake to find some project include files
list (INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

include(BuildType)
include(CompilerFlags)

option(BUILD_SHARED_LIBS "build shared libraries (default=static)" false)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(docs)
add_subdirectory(apps)
