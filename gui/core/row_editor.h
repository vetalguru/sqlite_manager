#ifndef SQLITE_MANAGER_GUI_CORE_ROW_EDITOR_H
#define SQLITE_MANAGER_GUI_CORE_ROW_EDITOR_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "sqlite_manager/query_result.h"  // Cell
#include "sqlite_manager/result.h"

namespace sqlite_manager {
class Connection;
}  // namespace sqlite_manager

namespace sqlite_manager_gui {

// Parameterized single-row edits addressed by rowid. Table and column
// names are quoted; values are bound, never concatenated. Intended for
// editing a real table's rows - the ones "SELECT rowid, * FROM t"
// exposes; views and WITHOUT ROWID tables are not editable this way.
//
// A non-NULL value binds as text and is stored under the column's
// affinity (so text "42" lands in an INTEGER column as an integer); a
// kNull cell stores SQL NULL.

// UPDATE <table> SET <column> = <value> WHERE rowid = <rowid>.
sqlite_manager::Status UpdateCell(sqlite_manager::Connection& conn,
                                  const std::string& table, std::int64_t rowid,
                                  const std::string& column,
                                  const sqlite_manager::Cell& value);

// DELETE FROM <table> WHERE rowid = <rowid>.
sqlite_manager::Status DeleteRow(sqlite_manager::Connection& conn,
                                 const std::string& table, std::int64_t rowid);

// INSERT INTO <table> (<columns>) VALUES (<values>); returns the new
// rowid. With no values, inserts a DEFAULT VALUES row.
sqlite_manager::Result<std::int64_t> InsertRow(
    sqlite_manager::Connection& conn, const std::string& table,
    const std::vector<std::pair<std::string, sqlite_manager::Cell>>& values);

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_ROW_EDITOR_H
