#ifndef SQLITE_MANAGER_GUI_CORE_SCHEMA_EDITOR_H
#define SQLITE_MANAGER_GUI_CORE_SCHEMA_EDITOR_H

#include <string>
#include <vector>

#include "sqlite_manager/result.h"

namespace sqlite_manager {
class Connection;
}  // namespace sqlite_manager

namespace sqlite_manager_gui {

// A column definition for CreateTable.
struct ColumnDef {
    std::string name;
    std::string type;  // declared type (may be empty)
    bool not_null = false;
    bool primary_key = false;
};

// Data-definition (DDL) helpers. Identifiers are quoted; the declared type
// is written verbatim (it is a type name, not an identifier), so callers
// should pass a plain type such as "TEXT" or "INTEGER".

// ALTER TABLE <table> ADD COLUMN <name> [<type>]. The new column is
// nullable - SQLite does not allow adding a NOT NULL, PRIMARY KEY, or
// UNIQUE column to an existing table.
sqlite_manager::Status AddColumn(sqlite_manager::Connection& conn,
                                 const std::string& table,
                                 const std::string& name,
                                 const std::string& type);

// ALTER TABLE <table> DROP COLUMN <name>. Fails if the column is part of a
// primary key, a unique constraint, or an index (SQLite restriction).
sqlite_manager::Status DropColumn(sqlite_manager::Connection& conn,
                                  const std::string& table,
                                  const std::string& name);

// CREATE TABLE <name> (<columns>). Requires a name and at least one column.
sqlite_manager::Status CreateTable(sqlite_manager::Connection& conn,
                                   const std::string& name,
                                   const std::vector<ColumnDef>& columns);

// DROP TABLE <name>.
sqlite_manager::Status DropTable(sqlite_manager::Connection& conn,
                                 const std::string& name);

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_SCHEMA_EDITOR_H
