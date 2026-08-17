#include "gui/core/database_session.h"

#include <utility>

#include "gui/core/query_runner.h"
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

}  // namespace sqlite_manager_gui
