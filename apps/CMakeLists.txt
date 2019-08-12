# Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# FLPR/apps/CMakeLists.txt


add_library(flprapp
  flpr_format_base.cc
  module_base.cc
  )

target_compile_features(flprapp PUBLIC cxx_std_17)
set_target_properties(flprapp PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(flprapp flpr)
target_include_directories(flprapp
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

# Install the application library header files
set(APP_HEADERS
  "flpr_format_base.hh"
  "module_base.hh"
  "Indent_Table.hh"
  "Timer.hh"
  )

# Add any demo applications to this list
set(APPS_EXE
  "caliper"
  "flpr-format"
  "parse_files"
  "module"
  "ext_demo"
  "flpr_show_cst")

# Installation Info
include(GNUInstallDirs)


# Generate an executable target for each entry in APPS_EXE
foreach(e IN LISTS APPS_EXE)
  add_executable("${e}" "${e}.cc")
  target_link_libraries(${e} flpr flprapp)
  target_compile_features(${e} PUBLIC cxx_std_17)
  set_target_properties(${e} PROPERTIES CXX_EXTENSIONS OFF)
  install(TARGETS "${e}" DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach(e)



install(TARGETS flprapp
  EXPORT   FLPRAppTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )


install(FILES ${APP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flpr)


# Write out config-file package information

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/FLPRAppConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion )

export(EXPORT FLPRAppTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/FLPRAppTargets.cmake
  )

configure_package_config_file(FLPRAppConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/FLPRAppConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)

install(EXPORT FLPRAppTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
  )


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FLPRAppConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/FLPRAppConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake )
