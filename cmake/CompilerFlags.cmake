# Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.
#
# This is open source software; you can redistribute it and/or modify it
# under the terms of the BSD-3 License. If software is modified to produce
# derivative works, such modified software should be clearly marked, so as
# not to confuse it with the version available from LANL. Full text of the
# BSD-3 License can be found in the LICENSE file of the repository.


# Sets up the project defaults for compiler flags

# Setting the CMAKE_*_FLAGS_* is a cmake antipattern.  Should fix that.

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -Wno-unused-parameter -g -O0")
  set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wno-unused-parameter -Wdeprecated -g -O0")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  set(CMAKE_C_FLAGS_DEBUG "-w2 -g -O0")
  set(CMAKE_CXX_FLAGS_DEBUG "-w2 -g -O0")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()




