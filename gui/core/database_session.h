#ifndef SQLITE_MANAGER_GUI_CORE_DATABASE_SESSION_H
#define SQLITE_MANAGER_GUI_CORE_DATABASE_SESSION_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gui/core/schema_editor.h"
#include "gui/core/schema_info.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result.h"

namespace sqlite_manager_gui {

// GTK-free facade over a single open database. Every GUI use-case goes
// through here; it owns the Connection and never references GTK, so the
// whole layer is unit-testable against an in-memory database.
//
// (GRASP Controller/Facade; DIP: the UI depends on this, not on SQLite.)
class DatabaseSession final {
public:
    using OpenMode = sqlite_manager::Connection::OpenMode;

    static sqlite_manager::Result<DatabaseSession> Open(
        const std::string& path, OpenMode mode = OpenMode::kReadWriteCreate);

    DatabaseSession(DatabaseSession&&) noexcept = default;
    DatabaseSession& operator=(DatabaseSession&&) noexcept = default;
    DatabaseSession(const DatabaseSession&) = delete;
    DatabaseSession& operator=(const DatabaseSession&) = delete;

    bool IsOpen() const { return conn_.IsOpen(); }
    const std::string& path() const { return path_; }

    // --- Schema ---
    sqlite_manager::Result<std::vector<ObjectInfo>> ListObjects();
    sqlite_manager::Result<TableInfo> DescribeTable(const std::string& name);

    // --- Query / execute ---
    sqlite_manager::Result<sqlite_manager::QueryResult> RunQuery(
        const std::string& sql);
    sqlite_manager::Status Execute(const std::string& sql);

    // --- Single-row editing (by rowid) ---
    // For editing a real table's rows - the ones "SELECT rowid, * FROM t"
    // exposes. Table and column names are quoted and values are bound.
    sqlite_manager::Status UpdateCell(const std::string& table,
                                      std::int64_t rowid,
                                      const std::string& column,
                                      const sqlite_manager::Cell& value);
    sqlite_manager::Status DeleteRow(const std::string& table,
                                     std::int64_t rowid);
    sqlite_manager::Result<std::int64_t> InsertRow(
        const std::string& table,
        const std::vector<std::pair<std::string, sqlite_manager::Cell>>&
            values);

    // Imports CSV `text` into `table` (see gui/core/csv_importer.h);
    // returns the number of rows imported.
    sqlite_manager::Result<int> ImportCsv(const std::string& table,
                                          const std::string& text,
                                          bool has_header);

    // --- Schema editing (DDL, see gui/core/schema_editor.h) ---
    sqlite_manager::Status AddColumn(const std::string& table,
                                     const std::string& name,
                                     const std::string& type);
    sqlite_manager::Status DropColumn(const std::string& table,
                                      const std::string& name);
    sqlite_manager::Status CreateTable(const std::string& name,
                                       const std::vector<ColumnDef>& columns);
    sqlite_manager::Status DropTable(const std::string& name);

    // Escape hatch for advanced flows (transactions, backups, custom SQL).
    sqlite_manager::Connection& connection() { return conn_; }

private:
    DatabaseSession(sqlite_manager::Connection conn, std::string path)
        : conn_(std::move(conn)), path_(std::move(path)) {}

    sqlite_manager::Connection conn_;
    std::string path_;
};

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_DATABASE_SESSION_H
