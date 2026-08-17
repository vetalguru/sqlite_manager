#include "gui/core/database_session.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gui/core/csv_importer.h"
#include "gui/core/query_runner.h"
#include "gui/core/row_editor.h"
#include "gui/core/schema_reader.h"

namespace sqlite_manager_gui {

using sqlite_manager::Connection;
using sqlite_manager::QueryResult;
using sqlite_manager::Result;
using sqlite_manager::Status;

Result<DatabaseSession> DatabaseSession::Open(const std::string& path,
                                              OpenMode mode) {
    Connection conn;
    if (auto s = conn.Open(path, mode); !s.ok()) return s.error();
    return DatabaseSession(std::move(conn), path);
}

Result<std::vector<ObjectInfo>> DatabaseSession::ListObjects() {
    return ReadObjects(conn_);
}

Result<TableInfo> DatabaseSession::DescribeTable(const std::string& name) {
    return ReadTable(conn_, name);
}

Result<QueryResult> DatabaseSession::RunQuery(const std::string& sql) {
    return RunSql(conn_, sql);
}

Status DatabaseSession::Execute(const std::string& sql) {
    return conn_.Execute(sql);
}

Status DatabaseSession::UpdateCell(const std::string& table, std::int64_t rowid,
                                   const std::string& column,
                                   const sqlite_manager::Cell& value) {
    return sqlite_manager_gui::UpdateCell(conn_, table, rowid, column, value);
}

Status DatabaseSession::DeleteRow(const std::string& table,
                                  std::int64_t rowid) {
    return sqlite_manager_gui::DeleteRow(conn_, table, rowid);
}

Result<std::int64_t> DatabaseSession::InsertRow(
    const std::string& table,
    const std::vector<std::pair<std::string, sqlite_manager::Cell>>& values) {
    return sqlite_manager_gui::InsertRow(conn_, table, values);
}

Result<int> DatabaseSession::ImportCsv(const std::string& table,
                                       const std::string& text,
                                       bool has_header) {
    return sqlite_manager_gui::ImportCsv(conn_, table, text, has_header);
}

}  // namespace sqlite_manager_gui
