#ifndef SQLITE_MANAGER_GUI_CORE_CSV_IMPORTER_H
#define SQLITE_MANAGER_GUI_CORE_CSV_IMPORTER_H

#include <string>
#include <vector>

#include "sqlite_manager/result.h"

namespace sqlite_manager {
class Connection;
}  // namespace sqlite_manager

namespace sqlite_manager_gui {

// Parses CSV text (RFC 4180) into a grid of string fields. Fields may be
// quoted; within quotes a doubled quote is a literal quote, and commas,
// CR, and LF are ordinary characters. No type interpretation is done.
std::vector<std::vector<std::string>> ParseCsv(const std::string& text);

// Imports CSV `text` into `table`. When has_header is true the first row
// names the target columns; otherwise the columns are taken positionally
// from the table's schema. An empty field imports as SQL NULL. All rows
// are inserted in a single transaction (rolled back on any error), and
// the number of data rows imported is returned.
sqlite_manager::Result<int> ImportCsv(sqlite_manager::Connection& conn,
                                      const std::string& table,
                                      const std::string& text, bool has_header);

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_CSV_IMPORTER_H
