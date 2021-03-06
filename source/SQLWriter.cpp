﻿//*********************************************************************************************************************************
//
// PROJECT:							General Class Library
// FILE:								SQLWriter
// SUBSYSTEM:						Database library
// LANGUAGE:						C++
// TARGET OS:						None - Standard C++
// NAMESPACE:						GCL::sqlWriter
// AUTHOR:							Gavin Blakeman.
// LICENSE:             GPLv2
//
//                      Copyright 2013-2020 Gavin Blakeman.
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
// OVERVIEW:            The file provides a class for generating/composing/writing SQL queries using a simplified approach that
//                      does not require knowledge of SQL.
//                      The class does not communicate with the database server directly, but does provide functions to create the
//                      SQL command strings to perform the database access.
//                      Typical select query would be written as follows:
//
// CLASSES INCLUDED:    CSQLWriter
//
// HISTORY:             2020-04-25 GGB - Added offset functionality.
//                      2019-12-08 GGB - Added UPSERT functionality for MYSQL.
//                      2015-09-22 GGB - AIRDAS 2015.09 release
//                      2013-01-26 GGB - Development of class for application AIRDAS
//
//*********************************************************************************************************************************

#include "include/SQLWriter.h"

  // Standard C++ library files

#include <iostream>
#include <fstream>
#include <sstream>
#include <typeinfo>
#include <utility>

  // Miscellaneous library header files

#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "boost/locale.hpp"
#include <SCL>

  // GCL library header Files

#include "include/common.h"
#include "include/error.h"
#include "include/GCLError.h"

namespace GCL
{

  std::string const TABLE("TABLE");
  std::string const COLUMN("COLUMN");
  std::string const END("END");

  //******************************************************************************************************************************
  //
  // CSQLWriter
  //
  //******************************************************************************************************************************

  /// @brief      Function to capture the count expression
  /// @param[in]  countExpression: The count expression to capture.
  /// @returns    (*this)
  /// @throws     None
  /// @version    2017-08-12/GGB - Function created.

  sqlWriter &sqlWriter::count(std::string const &countExpression)
  {
    countValue = countExpression;

    return (*this);
  }

  /// @brief      Creates the test for the specified delete query.
  /// @returns    a string representation of the delete query.
  /// @version    2018-05-12/GGB - Function created.

  std::string sqlWriter::createDeleteQuery() const
  {
    std::string returnValue = "DELETE FROM ";

    returnValue += getColumnMap(deleteTable);

    returnValue += createWhereClause();

    return returnValue;
  }

  /// @brief Creates the string for an insert query.
  /// @returns The Insert Query as a string.
  /// @throws None.
  /// @version 2015-03-31/GGB - Function created.

  std::string sqlWriter::createInsertQuery() const
  {
    std::string returnValue = "INSERT INTO " + insertTable + "(";
    bool firstRow = true;
    bool firstValue = true;

    // Output the column names.

    for (auto &element : selectFields)
    {
      std::string columnName;

      if (firstValue)
      {
        firstValue = false;
      }
      else
      {
        returnValue += ", ";
      };
      columnName = element;

      returnValue += columnName;
    };

    returnValue += ") VALUES ";

    firstValue = true;

    for (auto &outerElement : valueFields)
    {
      if (firstRow)
      {
        firstRow = false;
      }
      else
      {
        returnValue += ", ";
      }

      firstValue = true;
      returnValue += "(";

      for (auto & innerElement : outerElement)
      {
        if (firstValue)
        {
          firstValue = false;
        }
        else
        {
          returnValue += ", ";
        };

        if (innerElement.type() == typeid(std::string))
        {
          returnValue += "'" + innerElement.to_string() + "'";
        }
        else if (innerElement.type() == typeid(bindValue))
        {
          std::string temp = innerElement.to_string();

          if (temp.front() == ':')
          {
            returnValue += temp;
          }
          else if (temp.front() == '?')
          {
            returnValue += temp;
          }
          else
          {
            returnValue += ":" + temp;
          }
        }
        else
        {
          returnValue += innerElement.to_string();
        };
      };

      returnValue += ")";
    };

    return returnValue;
  }

  /// @brief Creates the limit clause
  /// @throws None.
  /// @version 2020-04-25/GGB - Function created.

  std::string sqlWriter::createLimitClause() const
  {
    std::string returnValue;

    switch (dialect)
    {
      case MYSQL:
      {
        // Offset does not have to be present. However if offset is present, then limit must also be present.

        if (offsetValue)
        {
          returnValue = "LIMIT " + std::to_string(*offsetValue) + ", "
                        + std::to_string(limitValue ? *limitValue : std::numeric_limits<std::uint64_t>::max()) + " ";
        }
        else
        {
          if (limitValue)
          {
            returnValue = "LIMIT " + std::to_string(*limitValue) + " ";
          };
        };
        break;
      };
      case POSTGRE:
      {
        break;
      }
      default:
      {
        RUNTIME_ERROR(boost::locale::translate("Unknown dialect"), E_SQLWRITER_UNKNOWNDIALECT, LIBRARYNAME);
      }
    }

    return returnValue;
  }

  /// @brief      Creates the set clause.
  /// @throws     GCL::CRuntimeAssert
  /// @version    2017-08-21/GGB - Function created.

  std::string sqlWriter::createSetClause() const
  {
    RUNTIME_ASSERT(!setFields.empty(), boost::locale::translate("No Set fields defined for update query."));

    std::string returnValue = "SET ";
    bool firstValue = true;

    for (auto element : setFields)
    {
      if (firstValue)
      {
        firstValue = false;
      }
      else
      {
        returnValue += ", ";
      };
      returnValue += element.first + " = ";

      if (element.second.type() == typeid(std::string))
      {
        returnValue += "'" + element.second.to_string() + "'";
      }
      else if (element.second.type() == typeid(bindValue))
      {
        std::string temp = element.second.to_string();

        if (temp.front() == ':')
        {
          returnValue += temp;
        }
        else if (temp.front() == '?')
        {
          returnValue += temp;
        }
        else
        {
          returnValue += ":" + temp;
        }
      }
      else
      {
        returnValue += element.second.to_string();
      };
    };

    return returnValue;

  }

  /// @brief Converts the query to a string. Specifically for an update query.
  /// @throws
  /// @version 2017-08-21/GGB - Function created.

  std::string sqlWriter::createUpdateQuery() const
  {
    std::string returnValue = "UPDATE " + updateTable + " ";

    returnValue += createSetClause();

    returnValue += createWhereClause();

    return returnValue;
  }

  /// @brief Converts the upsert query to a string.
  /// @throws GCL::CRuntimeError
  /// @version 2019-12-08/GGB - Function created.

  std::string sqlWriter::createUpsertQuery() const
  {
    // Notes:
    //  1. The where() clauses should be populated. These need to be converted to insert clauses for the insert function.
    //  2. The set clause needs to be included in the insert clauses.

    RUNTIME_ASSERT(dialect == MYSQL, boost::locale::translate("Upsert only implemented for MYSQL."));

    std::string returnValue;
    std::string fieldNames;
    std::string fieldValues;
    bool firstValue = true;

    switch(dialect)
    {
      case MYSQL:
      {
        // Create the field and value clauses for the insert into.

        for (auto const &element : whereFields)
        {
          if (firstValue)
          {
            firstValue = false;
          }
          else
          {
            fieldNames += ", ";
            fieldValues += ", ";
          };

          fieldNames += getColumnMap(std::get<0>(element));
          if (std::get<2>(element).type() == typeid(std::string))
          {
            fieldValues += "'" + std::get<2>(element).to_string() + "'";
          }
          else
          {
            fieldValues += std::get<2>(element).to_string();
          };
        };

        // Add the set clause.

        for (auto const &element : setFields)
        {
          if (firstValue)
          {
            firstValue = false;
          }
          else
          {
            fieldNames += ", ";
            fieldValues += ", ";
          };

          fieldNames += getColumnMap(element.first);
          if (element.second.type() == typeid(std::string))
          {
            fieldValues += "'" + element.second.to_string() + "'";
          }
          else
          {
            fieldValues += element.second.to_string();
          };
        };

        // Before we can create the insert query, we need to

        returnValue = "INSERT INTO " + insertTable + "(";
        returnValue += fieldNames + ") VALUES (";
        returnValue += fieldValues + ") ";
        returnValue += "ON DUPLICATE KEY UPDATE ";

        firstValue = true;
        fieldValues.clear();

        for (auto const &element : setFields)
        {
          if (firstValue)
          {
            firstValue = false;
          }
          else
          {
            fieldValues += ", ";
          };
          fieldValues+= getColumnMap(element.first) + " = ";

          if (element.second.type() == typeid(std::string))
          {
            fieldValues += "'" + element.second.to_string() + "'";
          }
          else
          {
            fieldValues += element.second.to_string();
          };
        };

        returnValue += fieldValues;

        break;
      }
      default:
      {
        CODE_ERROR;
      }
    };

    return returnValue;
  }

  /// @brief      Converts the where clause to a string for creating the SQL string.
  /// @returns    The where clause.
  /// @throws
  /// @version    2015-05-24/GGB - Function created.

  std::string sqlWriter::createWhereClause() const
  {
    std::string returnValue = " WHERE ";
    bool first = true;
    tripleStorage::const_iterator iterator;

    for (iterator = whereFields.begin(); iterator != whereFields.end(); iterator++)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        returnValue += " AND ";
      };

      returnValue += "(";
      returnValue += getColumnMap(std::get<0>(*iterator));
      returnValue += " ";
      returnValue += std::get<1>(*iterator);

      if (std::get<2>(*iterator).type() == typeid(std::string))
      {
        returnValue += " '";
        returnValue += std::get<2>(*iterator).to_string();
        returnValue += "')";
      }
      else
      {
        returnValue += " ";
        returnValue += std::get<2>(*iterator).to_string();
        returnValue += ")";
      }
    };

    return returnValue;
  }

  /// @brief Adds the distinct keyword to a select query.
  /// @returns (*this)
  /// @throws None.
  /// @version 2017-08-19/GGB - Function created.

  sqlWriter &sqlWriter::distinct()
  {
    distinct_ = true;
    return (*this);
  }

  /// @brief      Adds the table name to the from clause. @c from("tbl_name", "n")
  /// @param[in]  fromString: The table name to add to the from clause.
  /// @param[in]  alias: The alias to use for the table name.
  /// @returns    (*this)
  /// @throws     None.
  /// @version    2017-08-20/GGB - Function created.

  sqlWriter &sqlWriter::from(std::string const &fromString, std::string const &alias)
  {
    fromFields.emplace_back(fromString, alias);

    return (*this);
  }

  /// @brief        Searches the databaseMap and determines the mapped column names.
  /// @param[in/out] columnName: The columnName to map.
  /// @returns      true - If the columnName was mapped succesfully.
  /// @returns      false - The columnName is not recognised in the databaseMap.
  /// @throws       None.
  /// @version
  /// @todo         Implement this function. (Bug# 0000193)

  std::string sqlWriter::getColumnMap(std::string const &columnName) const
  {
    std::string returnValue = columnName;

    return returnValue;
  }

  /// @brief      Adds a max() function to the query.
  /// @param[in]  column: The name of the column to take the max of.
  /// @param[in]  as: The column name to assign to the max() function.
  /// @returns    (*this)
  /// @throws     None.
  /// @version    2017-08-20/GGB - Function created.

  sqlWriter &sqlWriter::max(std::string const &column, std::string const &as)
  {
    maxFields.emplace_back(column, as);
    return (*this);
  }

  /// @brief      Adds a min() function to the query.
  /// @param[in]  column: The name of the column to take the max of.
  /// @param[in]  as: The column name to assign to the min() function.
  /// @returns    (*this)
  /// @throws     None.
  /// @version    2017-08-20/GGB - Function created.

  sqlWriter &sqlWriter::min(std::string const &column, std::string const &as)
  {
    minFields.emplace_back(column, as);
    return (*this);
  }

  /// @brief      Function called to add columnData into a table.
  /// @param[in]  tableName: The table name to associate with the column.
  /// @param[in]  columnName: The column name to create.
  /// @returns    true - column added.
  /// @returns    false - column no added.
  /// @throws     None.
  /// @version    2013-01-26/GGB - Function created.

  bool sqlWriter::createColumn(std::string const &tableName, std::string const &columnName)
  {
    TDatabaseMap::iterator iter;

    if ( (iter = databaseMap.find(tableName)) == databaseMap.end() )
    {
      return false;
    }
    else if ( (*iter).second.columnData.find(columnName) != (*iter).second.columnData.end() )
    {
      return false;
    }
    else
    {
      SColumnData columnData;
      columnData.columnName.first = columnName;

      (*iter).second.columnData[columnName] = columnData;
      return true;
    };
  }

  /// @brief Function to add a table to the database map.
  /// @param[in] tableName: The table name.
  /// @returns true =
  /// @note The table is not yet mapped at this point.
  /// @version 2013-01-26/GGB - Function created.

  bool sqlWriter::createTable(std::string const &tableName)
  {
    // Check if the table already exists in the database.

    if ( (databaseMap.find(tableName) != databaseMap.end()) )
    {
      return false;
    }
    else
    {
      STableData newTable;
      newTable.tableName.first = tableName;

      databaseMap[tableName] = newTable;
      return true;
    };
  }

  /// @brief Sets the Mapping of a specified column.
  /// @param[in] tableName: The table name having the column.
  /// @param[in] columnName: The column name to map.
  /// @param[in] columnMap: The columnName to use when referring to the columnName.
  /// @throws None.
  /// @version 2013-01-26/GGB - Function created.

  void sqlWriter::setColumnMap(std::string const &tableName, std::string const &columnName, std::string const &columnMap)
  {
    if ( databaseMap.find(tableName) != databaseMap.end() )
    {
      if ( databaseMap[tableName].columnData.find(columnName) != databaseMap[tableName].columnData.end() )
      {
        databaseMap[tableName].columnData[columnName].columnName.first = columnName;
        databaseMap[tableName].columnData[columnName].columnName.second = columnMap;
      };
    };
  }

  /// @brief Sets the Mapping of the table.
  /// @param[in] tableName: The name of the table.
  /// @param[in] tableMap: The string to map to the tableName.
  /// @version 2013-01-26/GGB - Function created.

  void sqlWriter::setTableMap(std::string const &tableName, std::string const &tableMap)
  {
    if ( databaseMap.find(tableName) != databaseMap.end() )
    {
      databaseMap[tableName].tableName.first = tableName;
      databaseMap[tableName].tableName.second = tableMap;
    };
  }

  /// @brief Output the "FROM" clause as a string.
  /// @returns A string representation of the "FROM" clause.
  /// @version 2016-05-08/GGB: Added support for table alisases and table maps.
  /// @version 2015-04-12/GGB: Function created.

  std::string sqlWriter::createFromClause() const
  {
    std::string returnValue = " FROM ";
    std::string tableName;
    std::string tableAlias;
    bool first = true;
    std::vector<std::string>::const_iterator iterator;

    first = true;
    for (auto element : fromFields)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        returnValue += ", ";
      };
      tableName = element.first;

      returnValue += tableName;

      if (element.second.length() != 0)
      {
        returnValue += " AS " + tableAlias;
      };
    }

    return returnValue;
  }

  /// @brief Creates the "JOIN" clause
  /// @returns A string containing the join clause (begins with the JOIN keyword.
  /// @version 2017-07-29/GGB - Function created.

  std::string sqlWriter::createJoinClause() const
  {
    std::string returnValue;

    for (auto element : joinFields)
    {
      switch (std::get<2>(element))
      {
        case JOIN_LEFT:
        {
          returnValue += + " LEFT JOIN ";
          break;
        };
        case JOIN_RIGHT:
        {
          returnValue += + " RIGHT JOIN ";
          break;
        };
        case JOIN_INNER:
        {
          returnValue += " INNER JOIN ";
          break;
        }
        case JOIN_FULL:
        {
          returnValue += "FULL JOIN ";
          break;
        }
        default:
        {
          CODE_ERROR;
          break;
        }

      };
      returnValue += std::get<3>(element) + " ON ";
      returnValue += std::get<0>(element) + "." + std::get<1>(element) + "=" + std::get<3>(element) + "." + std::get<4>(element);
    }

    return returnValue;
  }

  /// @brief Creates the "ORDER BY" clause.
  /// @returns A string representation of the "Order By" clause.
  /// @throws None.
  /// @version 2015-04-12/GGB - Function created.

  std::string sqlWriter::createOrderByClause() const
  {
    orderByStorage_t::const_iterator iterator;
    bool first = true;

    std::string returnValue = " ORDER BY ";

    for (iterator = orderByFields.begin(); iterator != orderByFields.end(); iterator++)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        returnValue += ", ";
      };
      returnValue += getColumnMap((*iterator).first);
      returnValue += " ";

      if ((*iterator).second == ASC)
      {
        returnValue += "ASC ";
      }
      else if ((*iterator).second == DESC)
      {
        returnValue += "DESC ";
      };
    };

    return returnValue;
  }

  /// @brief Function to create the select clause.
  /// @returns A string representation of the select clause.
  /// @note This function also performs the mapping to the correct table.columnNames. Additionally, if only the columnName is
  ///       given the function will also search the correct tableName or tableAlias and add that to the term.
  /// @version 2017-08-20/GGB - Added support for min() and max()
  /// @version 2017-08-19/GGB - Added support for DISTINCT
  /// @version 2017-08-12/GGB - Added code to support COUNT() clauses.
  /// @version 2015-04-12/GGB - Function created.

  std::string sqlWriter::createSelectClause() const
  {
    std::string returnValue = "SELECT ";
    bool first = true;
    std::vector<std::string>::const_iterator iterator;
    std::string columnName;

    if ( (dialect == MICROSOFT) && (limitValue))
    {
      returnValue += "TOP " + std::to_string(*limitValue) + " ";
    };

    if (distinct_)
    {
      returnValue += "DISTINCT ";
    };

    for (iterator = selectFields.begin(); iterator != selectFields.end(); iterator++)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        returnValue += ", ";
      };
      columnName = (*iterator);

      returnValue += columnName;
    };

    if (countValue)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        returnValue += ", ";
      };

      if (*countValue == "*")
      {
        returnValue += "COUNT(*) ";
      }
      else
      {
        returnValue += "COUNT(" + *countValue + ") ";
      };
    }

    if (!maxFields.empty())
    {
      for (auto element : maxFields)
      {
        if (first)
        {
          first = false;
        }
        else
        {
          returnValue += ", ";
        };

        returnValue += "MAX(" + element.first + ")";
        if (!element.second.empty())
        {
          returnValue += " AS " + element.second;
        };
      }
    };

    if (!minFields.empty())
    {
      for (auto element : minFields)
      {
        if (first)
        {
          first = false;
        }
        else
        {
          returnValue += ", ";
        };

        returnValue += "MIN(" + element.first + ")";
        if (!element.second.empty())
        {
          returnValue += " AS " + element.second;
        };
      }
    };

    return returnValue;
  }

  /// @brief      Produces the string for a SELECT query.
  /// @returns    A string containing the select clause.
  /// @throws     None.
  /// @version    2020-04-25/GGB - Added support for the LIMIT and OFFSET clauses.
  /// @version    2017-08-12/GGB - Added check for countValue in selectClause if statement.
  /// @version    2015-03-30/GGB - Function created.

  std::string sqlWriter::createSelectQuery() const
  {
    std::string returnValue;

    if ( !selectFields.empty() || countValue ||
         !maxFields.empty() || !minFields.empty() )
    {
      returnValue += createSelectClause();
    }
    else
    {
      RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: No Select fields in select clause."), E_SQLWRITER_NOSELECTFIELDS,
                    LIBRARYNAME);
    };

    if (!fromFields.empty())
    {
      returnValue += createFromClause();
    }
    else
    {
      RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: No from fields in select clause."), E_SQLWRITER_NOFROMFIELD,
                    LIBRARYNAME);
    };

    if (!joinFields.empty())
    {
      returnValue += createJoinClause();
    }

    if (!whereFields.empty())
    {
      returnValue += createWhereClause();
    };

    if (!orderByFields.empty())
    {
      returnValue += createOrderByClause();
    };

    returnValue += createLimitClause();

    return returnValue;
  }

  /// @brief Constructor for the from clause.
  /// @param[in] fields: The fields to add to the from clause.
  /// @throws None.
  /// @version 2017-08-20/GGB - Added support for aliases.
  /// @version 2015-03-30/GGB - Function created.

  sqlWriter &sqlWriter::from(std::initializer_list<std::string> fields)
  {
    for (auto elem : fields)
    {
      fromFields.emplace_back(elem, "");
    }

    return (*this);
  }

  /// @brief Set the query type to a 'DELETE' query.
  /// @param[in] tableName: The name of the table to execute the delete query on.
  /// @returns *this
  /// @version 2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version 2018-05-12/GGB - Added parameter for the deletion table name.
  /// @version 2015-03-30/GGB - Function created.

  sqlWriter &sqlWriter::deleteFrom(std::string const &tableName)
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_delete;

    deleteTable = tableName;

    return *this;
  }

  /// @brief Set the query type to a 'INSERT' query.
  /// @param[in] tableName: The table name to insert into.
  /// @returns *this
  /// @version 2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version 2018-08-19/GGB - Function created.

  sqlWriter &sqlWriter::insertInto(std::string tableName)
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_insert;

    insertTable = tableName;

    return *this;
  }

  /// @brief Set the query type to a 'INSERT' query.
  /// @param[in] tableName: The table name to insert into.
  /// @param[in] fields: The fields and field values.
  /// @returns *this
  /// @version 2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version 2017-07-26/GGB - Added support for fields.
  /// @version 2015-03-30/GGB - Function created.

  sqlWriter &sqlWriter::insertInto(std::string tableName, std::initializer_list<std::string> fields)
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_insert;

    insertTable = tableName;

    for (auto elem : fields)
    {
      selectFields.push_back(elem);
    };

    return *this;
  }

  /// @brief      Adds join statements to the query.
  /// @param[in]  fields: The join statements to add.
  /// @returns    (*this)
  /// @throws     None.
  /// @version    2016-05-08/GGB - Function created.

  sqlWriter &sqlWriter::join(std::initializer_list<parameterJoin> fields)
  {
    for (auto elem: fields)
    {
      joinFields.push_back(elem);
    }

    return *this;
  }

  /// @brief  Sets the limit value. This will be interpreted based on the SQL dialect in use.
  /// @param[in] limit: The maximum number of records to return.
  /// @version 2020-04-25/GGB - Changed parameter from std::size_t to std::uint64_t.
  /// @version 2019-12-14/GGB - Changed parameter from long to std::size_t
  /// @version 2015-04-12/GGB - Function created.

  sqlWriter &sqlWriter::limit(std::uint64_t limit)
  {
    limitValue = limit;
    return (*this);
  }

  /// @brief  Sets the offset value. This will be interpreted based on the SQL dialect in use.
  /// @param[in] offset: The offset of the first record to return.
  /// @version 2020-04-25/GGB - Function created.

  sqlWriter &sqlWriter::offset(std::uint64_t offset)
  {
    offsetValue = offset;
    return (*this);
  }


  /// @brief Copy the orderBy pairs to the list.
  /// @param[in] fields: The orderBy Pairs to copy
  /// @returns *this
  /// @throws None.
  /// @version 2019-10-24/GGB - Changed the orderBy type and storage.
  /// @version 2015-04-12/GGB - Function created.

  sqlWriter &sqlWriter::orderBy(std::initializer_list<std::pair<std::string, EOrderBy>> fields)
  {
    for (auto elem : fields)
    {
      orderByFields.push_back(elem);
    };

    return *this;
  }

  /// @brief      Function to load a map file and store all the aliases.
  /// @param[in]  ifn: The file to read the database mapping from.
  /// @throws     GCL::CRuntimeError
  /// @version    2013-01-26/GGB - Function created.

  void sqlWriter::readMapFile(boost::filesystem::path const &ifn)
  {
    std::ifstream ifs;
    std::string textLine;
    std::string szCommand;
    int lineNumber = 1;
    std::string currentTable;
    size_t spacePosn, token1S, token1E, token2S, token2E, equalPosn;
    std::string szToken1, szToken2;

    ifs.open(ifn.string());

    if (!ifs)
    {
      RUNTIME_ERROR("Could not open SQL map file:" + ifn.native());
    }
    else
    {
      while (ifs.good())
      {
        std::getline(ifs, textLine);
        if ( (textLine.size() > 1) && (textLine[0] != ';') )
        {
          spacePosn = textLine.find_first_of(' ', 0);
          szCommand = textLine.substr(0, spacePosn);
          boost::trim(szCommand);
          equalPosn = textLine.find_first_of('=', 0);
          token1S = textLine.find_first_of('[', 0);
          token1E = textLine.find_first_of(']', 0);
          token2S = textLine.find_first_of('[', equalPosn);
          token2E = textLine.find_first_of(']', equalPosn);

          if (token1S < token1E)
          {
            szToken1 = textLine.substr(token1S + 1, token1E - token1S - 1);
          }
          else if (szCommand != END)
          {
            RUNTIME_ERROR("Error in SQL map file:" + ifn.native() + "Syntax error on line: " +
                          std::to_string(lineNumber) + " - Needs at least one token.");
          };

          if (token2S < token2E)
          {
            szToken2 = textLine.substr(token2S + 1, token2E - token2S - 1);
          }
          else
          {
            szToken2.clear();
          };

          if (szCommand == COLUMN)
          {
            if (currentTable.empty())
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Syntax command on line: " << lineNumber << " - COLUMN directive found, but no TABLE directive in force." << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Syntax Error."), E_SQLWRITER_SYNTAXERROR, LIBRARYNAME);
            }
            else if (szToken1.empty())
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Syntax command on line: " << lineNumber << " - COLUMN directive found, column name." << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Syntax Error."), E_SQLWRITER_SYNTAXERROR, LIBRARYNAME);
            }
            else if ( (*databaseMap.find(currentTable)).second.columnData.find(szToken1) ==
                      (*databaseMap.find(currentTable)).second.columnData.end())
            {
              RUNTIME_ERROR("Error in SQL map file: " + ifn.native() +
                            "Syntax command on line: " + std::to_string(lineNumber) + " - Invalid column name.");
            }
            else
            {
              if (!szToken2.empty())
              {
                setColumnMap(currentTable, szToken1, szToken2);
              };
            };
          }
          else if (szCommand == TABLE)
          {
            if (!currentTable.empty())
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Syntax command on line: " << lineNumber << " - TABLE directive found, but a TABLE directive is already specified." << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Syntax Error."), E_SQLWRITER_SYNTAXERROR, LIBRARYNAME);
            }
            else if (szToken1.empty() )
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Syntax command on line: " << lineNumber << " - TABLE directive found, but no table name." << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Syntax Error."), E_SQLWRITER_SYNTAXERROR, LIBRARYNAME);
            }
            else if (databaseMap.find(szToken1) == databaseMap.end())
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Error on line: " << lineNumber << " - Invalid Table name." << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Invalid Table Name."), E_SQLWRITER_INVALIDTABLENAME,
                            LIBRARYNAME);
            }
            else
            {
              currentTable = szToken1;
              if (!szToken2.empty())
              {
                setTableMap(currentTable, szToken2);  // Add the alias into the record.
              };
            };
          }
          else if (szCommand == END)
          {
            if ( (token1S == textLine.npos) &&
                 (token1E == textLine.npos) &&
                 (token2S == textLine.npos) &&
                 (token2E == textLine.npos) &&
                 (equalPosn == textLine.npos) )
            {
              currentTable.clear();
            }
            else
            {
              std::clog << "Error in SQL map file: " << ifn << std::endl;
              std::clog << "Syntax command on line: " << lineNumber << std::endl;
              RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Syntax Error."), E_SQLWRITER_SYNTAXERROR, LIBRARYNAME);
            };
          }
          else
          {
            std::clog << "Error in SQL map file: " << ifn << std::endl;
            std::clog << "Invalid command on line: " << lineNumber << std::endl;
            RUNTIME_ERROR(boost::locale::translate("MAPPED SQL WRITER: Invalid Command."), E_SQLWRITER_INVALIDCOMMAND, LIBRARYNAME);
          };
        };

        lineNumber++;
      };
      ifs.close();
    };
  }

  /// @brief Resets all the fields for the query.
  /// @throws None.
  /// @version 2020-04-25/GGB - Added offsetValue to support offsets.
  /// @version 2019-12-08/GGB - Update queryType = qt_none when the query is reset.
  /// @version 2017-08-19/GGB - Added support for distinct.
  /// @version 2017-08-12/GGB - Added support for count expressions.
  /// @version 2015-05-19/GGB - Function created.

  void sqlWriter::resetQuery()
  {
    selectFields.clear();
    fromFields.clear();
    whereFields.clear();
    insertTable.clear();
    valueFields.clear();
    orderByFields.clear();
    joinFields.clear();
    limitValue.reset();
    offsetValue.reset();
    countValue.reset();
    distinct_ = false;
    maxFields.clear();
    minFields.clear();
    updateTable.clear();
    setFields.clear();
    queryType = qt_none;
  }

  /// @brief Resets the where clause of a query.
  /// @throws None.
  /// @version 2017-08-19/GGB - Function created.

  void sqlWriter::resetWhere()
  {
    whereFields.clear();
  }

  /// @brief      Set the query type to a 'SELECT' query.
  /// @returns    A reference to (*this)
  /// @version    2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version    2017-08-20/GGB - Function created.

  sqlWriter &sqlWriter::select()
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_select;

    return (*this);
  }

  /// @brief      Specify the selection fields for the select query.
  /// @param[in]  fields: The fields to add to the select clause.
  /// @returns    A reference to (*this)
  /// @version    2020-10-02/GGB - Call to select() to reset the query and set the query type.
  /// @version    2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version    2014-12-19/GGB - Function created.

  sqlWriter &sqlWriter::select(std::initializer_list<std::string> fields)
  {
    select();

    for (auto elem : fields)
    {
      selectFields.push_back(elem);
    };

    return *this;
  }

  /// @brief      Specify table name and fields for the select query.
  /// @param[in]  tableName: The name of the table to append to each of the field names.
  /// @param[in]  fields: The list of field names.
  /// @returns    A reference to (*this)
  /// @note       This can be called recursivly with different table and field specifiers.
  /// @version    2020-10-02/GGB - Function created.

  sqlWriter &sqlWriter::select(std::string const &tableName, std::initializer_list<std::string> fields)
  {
    select();

    for (auto elem : fields)
    {
      selectFields.push_back(tableName + "." + elem);
    };

    return *this;
  }

  /// @brief Processes a single set clause.
  /// @param[in] columnName: The columnName to set
  /// @param[in] value: The value to set.
  /// @returns (*this)
  /// @throws None.
  /// @version 2017-08-21/GGB - Function created.

  sqlWriter &sqlWriter::set(std::string const &columnName, parameter const &value)
  {
    setFields.emplace_back(columnName, value);

    return (*this);
  }

  /// @brief Processes a list of set clauses
  /// @param[in] fields: The initialiser list of set clauses.
  /// @returns (*this)
  /// @throws
  /// @version 2020-03-24/GGB - Function created.

  sqlWriter &sqlWriter::set(std::initializer_list<parameterPair> fields)
  {
    for (auto elem : fields)
    {
      setFields.push_back(elem);
    };

    return *this;
  }

  /// @brief Converts the query into an SQL query string.
  /// @returns The SQL query as a string.
  /// @throws None.
  /// @version 2019-12-08/GGB - Added UPSERT query.
  /// @version 2017-08-12/GGB - Function created.

  std::string sqlWriter::string() const
  {
    std::string returnValue;

    switch (queryType)
    {
      case qt_select:
      {
        returnValue = createSelectQuery();
        break;
      };
      case qt_insert:
      {
        returnValue = createInsertQuery();
        break;
      };
      case qt_update:
      {
        returnValue = createUpdateQuery();
        break;
      }
      case qt_delete:
      {
        returnValue = createDeleteQuery();
        break;
      }
      case qt_upsert:
      {
        returnValue = createUpsertQuery();
        break;
      }
      default:
      {
        CODE_ERROR;
        break;
      };
    }

    return returnValue;
  }

  /// @brief
  /// @todo Implement the getTableMap function. (Bug# 0000194)

  std::string sqlWriter::getTableMap(std::string const &search) const
  {
    std::string returnValue = search;

    return returnValue;
  }

  /// @brief Sets the table name for an update query.
  /// @param[in] tableName: The tableName to update.
  /// @returns (*this)
  /// @throws None.
  /// @version 2019-12-08/GGB - If the query is restarted without resetQuery() being called, then resetQuery() will be called.
  /// @version 2017-08-21/GGB - Function created.

  sqlWriter &sqlWriter::update(std::string const &tableName)
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_update;

    updateTable = tableName;

    return (*this);
  }

  /// @brief Sets the table name for an upsert query.
  /// @param[in] tableName: The tableName to upsert.
  /// @returns (*this)
  /// @throws None.
  /// @details @code upsert("tbl_scheduleModification").set("Value", true).where({"ModificationType", "=", 1} @endcode
  ///          translates to
  ///          @code INSERT INTO tbl_scheduleModification(ModificationType, Value) VALUES (1, true) ON DUPLICATE UPDATE
  ///                Value = true @endcode
  /// @version 2019-12-08/GGB - Function created.

  sqlWriter &sqlWriter::upsert(std::string const &tableName)
  {
    if (queryType != qt_none)
    {
      resetQuery();
    };

    queryType = qt_upsert;

    insertTable = tableName;

    return (*this);
  }

  /// @brief        Stores the value fields for the query.
  /// @param[in]    fields: The parameter values to include in the query.
  /// @returns      (*this)
  /// @version      2017-07-26/GGB - Changed code to use parameter rather than parameter pair.
  /// @version      2015-03-31/GGB - Function created.

  sqlWriter &sqlWriter::values(std::initializer_list<parameterStorage> fields)
  {
    for (const auto &f : fields)
    {
      valueFields.emplace_back(f);
    };

    return *this;
  }

  /// @brief Verifies if a string constitutes a valid SQL operator.
  /// @param[in] oper: The string to test.
  /// @returns true - The string is a valid operator.
  /// @returns false - The string is not a valid operator.
  /// @note Valid operators are: "=", "<>", "!=", ">", "<", ">=", "<=", "BETWEEN", "LIKE", "IN"
  /// @throws None.
  /// @version

  bool sqlWriter::verifyOperator(std::string const &oper) const
  {
    bool returnValue = false;

    returnValue = ( (oper == "=") || (oper == "<>") || (oper == "!=") || (oper == ">") || (oper == "<") ||
                    (oper == ">=") || (oper == "<=") || (oper == "BETWEEN") || (oper == "LIKE") || (oper == "IN"));

    return returnValue;
  }

  /// @brief        Stores the where fields in the list.
  /// @param[in]    fields: The initialiser list of fields to store.
  /// @returns      (*this)
  /// @throws       None.
  /// @version      2015-03-30/GGB - Function created.

  sqlWriter &sqlWriter::where(std::initializer_list<parameterTriple> fields)
  {
    for (auto elem : fields)
    {
      whereFields.push_back(elem);
    };

    return (*this);
  }

  /// @brief      to_string function for a bind value.
  /// @param[in]  bv: The value to convert to a std::string.
  /// @version    2020-09-24/GGB - Function created.

  std::string to_string(GCL::sqlWriter::bindValue const &bv)
  {
    return bv.to_string();
  }

}  // namespace GCL

