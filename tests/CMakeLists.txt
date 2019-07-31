# Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# FLPR/src/tests/CMakeLists.txt

# Make a common test helper library 'flpr_test'
add_library(flpr_test test_utils.cc LL_Helper.cc)
target_compile_features(flpr_test PUBLIC cxx_std_17)
set_target_properties(flpr_test PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(flpr_test flpr)
target_include_directories(flpr_test
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/>
  )

# Add any tests to this list. These are roughly ordered in terms of their
# place in the hierarchy of data structures.
set(TEST_EXE
  "test_safe_list"
  "test_tree"
  "test_file_line"
  "test_line_accum"
  "test_logical_line"
  "test_logical_file"
  "test_tt_stream"
  "test_stmt_cover"
  "test_parse_stmt"
  "test_parse_substmt"
  "test_parse_type_decl"
  "test_parse_prgm"
  )

# Create tests from each entry in TEST_EXE
foreach(e IN LISTS TEST_EXE)
  add_executable("${e}" "${e}.cc")
  target_link_libraries(${e} flpr_test flpr)
  target_include_directories(${e}
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../>
    )
  add_test(NAME "${e}" COMMAND "${e}")
endforeach(e)

# Add in a new test target called "check" that rebuilds test files first
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${TEST_EXE})
