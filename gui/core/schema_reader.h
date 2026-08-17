#ifndef SQLITE_MANAGER_GUI_CORE_SCHEMA_READER_H
#define SQLITE_MANAGER_GUI_CORE_SCHEMA_READER_H

#include <string>
#include <vector>

#include "gui/core/schema_info.h"
#include "sqlite_manager/result.h"

namespace sqlite_manager {
class Connection;
}  // namespace sqlite_manager

namespace sqlite_manager_gui {

// Lists user objects (tables, views, indexes, triggers) from
// sqlite_master, excluding internal sqlite_* objects, ordered by kind
// then name.
sqlite_manager::Result<std::vector<ObjectInfo>> ReadObjects(
    sqlite_manager::Connection& conn);

// Describes one table or view: its columns (name, declared type, NOT
// NULL, primary key, default) via pragma_table_info, plus whether the
// object is a view. Missing objects yield an empty column list.
sqlite_manager::Result<TableInfo> ReadTable(sqlite_manager::Connection& conn,
                                            const std::string& name);

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_SCHEMA_READER_H
