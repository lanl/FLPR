/*
   Copyright (c) 2019-2020, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file File_Info.hh
*/

#ifndef FLPR_FILE_INFO_HH
#define FLPR_FILE_INFO_HH 1

#include <ostream>
#include <string>

//! Namespace for FLPR: The Fortran Language Program Remodeling system
namespace FLPR {
//! The type of a Logical_File
enum class File_Type {
  UNKNOWN,
  FIXEDFMT, //!< .f or .F file
  FREEFMT   //!< .f90 file
};

//! Basic information about an actual file
struct File_Info {

  File_Info(std::string const &filename,
            File_Type file_type = File_Type::UNKNOWN);

  //! The name of the file that was scanned
  std::string filename;
  //! The language level in this file.
  File_Type file_type;
  //! If fixed-format, the last column
  int last_fixed_column = 0;
};

//! Guess the type of a file from its extension
File_Type file_type_from_extension(std::string const &filename);

std::ostream &operator<<(std::ostream &os, File_Type ft);

} // namespace FLPR
#endif
