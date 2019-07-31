/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
  \file File_Info.cc
*/

#include "flpr/File_Info.hh"
#include <iostream>

namespace FLPR {

File_Info::File_Info(std::string const &filename, File_Type file_type_in)
    : filename{filename}, file_type{file_type_in} {
  if (File_Type::UNKNOWN == file_type) {
    file_type = file_type_from_extension(filename);
  }
}

std::ostream &operator<<(std::ostream &os, File_Type ft) {
  switch (ft) {
  case File_Type::UNKNOWN:
    os << "unknown format";
    break;
  case File_Type::FIXEDFMT:
    os << "fixed-format";
    break;
  case File_Type::FREEFMT:
    os << "free-format";
    break;
  }
  return os;
}

/*!  Guess the type of a file from its extension */
FLPR::File_Type file_type_from_extension(std::string const &filename) {
  using FLPR::File_Type;
  File_Type file_type{File_Type::UNKNOWN};

  const size_t ptpos = filename.find_last_of('.');
  if (std::string::npos == ptpos) {
    std::cerr << "FLPR::file_type_from_extension: Error: unable to find "
              << "filename extension for \"" << filename << "\"\n";
  } else {
    const std::string extension = filename.substr(ptpos + 1);
    if (extension == "f" || extension == "F" || extension == "F90") {
      file_type = File_Type::FIXEDFMT;
    } else if (extension == "f90") {
      file_type = File_Type::FREEFMT;
    } else {
      std::cerr << "FLPR::file_type_from_extension: Error: unrecognized "
                << "filename extension \"" << extension << "\" for \""
                << filename << "\"\n";
    }
  }
  return file_type;
}
} // namespace FLPR
