﻿//**********************************************************************************************************************************
//
// PROJECT:             General Class Library
// FILE:                dataParser/dataDelimited.h
// SUBSYSTEM:           Data File Parser
// LANGUAGE:						C++
// TARGET OS:						None.
// NAMESPACE:						GCL
// AUTHOR:							Gavin Blakeman.
// LICENSE:             GPLv2
//
//                      Copyright 2020 Gavin Blakeman.
//                      This file is part of the General Class Library (GCL)
//
//                      GCL is free software: you can redistribute it and/or modify it under the terms of the GNU General
//                      Public License as published by the Free Software Foundation, either version 2 of the License, or
//                      (at your option) any later version.
//
//                      GCL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
//                      implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
//                      for more details.
//
//                      You should have received a copy of the GNU General Public License along with GCL.  If not,
//                      see <http://www.gnu.org/licenses/>.
//
// OVERVIEW:            Implementation of a configuration file reader.
//
// CLASSES INCLUDED:
//
// HISTORY:             2020-10-13 GGB - File Created
//
//**********************************************************************************************************************************

#ifndef DATADELIMITED_H
#define DATADELIMITED_H

  // GCL header files

#include "include/dataParser/dataParserCore.h"

namespace GCL
{
  class CDelimitedParser : CDataParser
  {
  private:
    bool includesHeader_;
    std::string delimiter_;

    std::string currentString;
    std::string::size_type currentPosn;

  protected:
    bool processHeader();
    bool processData();

  public:
    CDelimitedParser();

    CDelimitedParser &delimiterCharacter(std::string const &) noexcept;
    CDelimitedParser &includesHeader(bool) noexcept;

    bool parseData(std::string const &);

    void clear() noexcept;
  };
}

#endif // DATADELIMITED_H
