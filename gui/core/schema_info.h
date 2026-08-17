#ifndef SQLITE_MANAGER_GUI_CORE_SCHEMA_INFO_H
#define SQLITE_MANAGER_GUI_CORE_SCHEMA_INFO_H

#include <string>
#include <vector>

namespace sqlite_manager_gui {

// The kind of a schema object, as reported by sqlite_master.type.
enum class ObjectKind { kTable, kView, kIndex, kTrigger };

// One column of a table or view, as reported by pragma_table_info.
struct ColumnInfo {
    std::string name;
    std::string declared_type;  // type as written in the schema (may be empty)
    bool not_null = false;
    bool primary_key = false;
    std::string default_value;  // empty when there is no DEFAULT
};

// A schema object listed in sqlite_master.
struct ObjectInfo {
    ObjectKind kind = ObjectKind::kTable;
    std::string name;
    std::string sql;  // the CREATE statement (empty for internal objects)
};

// Full description of one table or view.
struct TableInfo {
    std::string name;
    bool is_view = false;
    std::vector<ColumnInfo> columns;
};

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_SCHEMA_INFO_H
