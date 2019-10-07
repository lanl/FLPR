# Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# FLPR/src/CMakeLists.txt

# ---------------------------- COMMON LIBRARY -----------------------------

find_package(FLEX 2.6)

FLEX_TARGET(Fortran_Scanner scan_fort.l
  ${FLPR_BINARY_DIR}/scan_fort.cc
  DEFINES_FILE ${FLPR_BINARY_DIR}/scan_fort.hh)

set(Libflpr_SRCS
  File_Info.cc
  File_Line.cc
  Indent_Table.cc
  LL_Stmt.cc
  LL_Stmt_Src.cc
  LL_TT_Range.cc
  Line_Accum.cc
  Logical_File.cc
  Logical_Line.cc
  Prgm_Tree.cc
  Stmt_Parser_Exts.cc
  Stmt_Tree.cc
  Syntax_Tags.cc
  Token_Text.cc
  TT_Stream.cc
  parse_stmt.cc
  scan_fort.l
  utils.cc
  )

set(flpr_headers
  File_Info.hh
  File_Line.hh
  Indent_Table.hh
  LL_Stmt.hh
  LL_Stmt_Src.hh
  LL_TT_Range.hh
  Line_Accum.hh
  Logical_File.hh
  Logical_Line.hh
  Parsed_File.hh
  Parser_Result.hh
  Prgm_Parsers.hh
  Prgm_Parsers_impl.hh
  Prgm_Parsers_utils.hh
  Prgm_Tree.hh
  Procedure.hh
  Procedure_Visitor.hh
  Range_Partition.hh
  Safe_List.hh
  Stmt_Parser_Exts.hh
  Stmt_Parsers.hh
  Stmt_Tree.hh
  Syntax_Tags.hh
  Syntax_Tags_Defs.hh
  TT_Stream.hh
  Token_Text.hh
  Tree.hh
  flpr.hh
  parse_stmt.hh
  utils.hh
  )


set_source_files_properties(${FLEX_Fortran_Scanner_OUTPUTS}
  PROPERTIES GENERATED TRUE)
set_source_files_properties(Logical_Line.cc
  PROPERTIES OBJECT_DEPENDS ${FLEX_Fortran_Scanner_OUTPUT_HEADER})

# Make sure to have the FLEX outputs listed first, so the built header
# is available for other compilation.
add_library(flpr 
  ${FLEX_Fortran_Scanner_OUTPUTS}
  ${Libflpr_SRCS}
  )
target_compile_features(flpr PUBLIC cxx_std_17)
set_target_properties(flpr PROPERTIES CXX_EXTENSIONS OFF)

# We need the CURRENT_BINARY include so that non-generated source can
# include a FLEX-generated header
target_include_directories(flpr
  PUBLIC
    $<BUILD_INTERFACE:${FLPR_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${FLPR_SOURCE_DIR}>
    $<BUILD_INTERFACE:${FLPR_BINARY_DIR}>
  )

# Installation Info
include(GNUInstallDirs)

install(TARGETS flpr
  EXPORT   FLPRTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

# Install the headers
install(
  FILES ${flpr_headers}
  DESTINATION include/flpr)
  


# Write out config-file package information

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${FLPR_BINARY_DIR}/FLPRConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion )

export(EXPORT FLPRTargets
  FILE ${FLPR_BINARY_DIR}/FLPRTargets.cmake
  )

configure_package_config_file(FLPRConfig.cmake.in
  ${FLPR_BINARY_DIR}/FLPRConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)

install(EXPORT FLPRTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  )

install(FILES ${FLPR_BINARY_DIR}/FLPRConfig.cmake
  ${FLPR_BINARY_DIR}/FLPRConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake )


