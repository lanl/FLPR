# Copyright (c) 2019, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# Taken from:
# https://blog.kitware.com/cmake-and-the-default-build-type/

# Set a default build type if none was specified
set(default_build_type "Debug")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()
