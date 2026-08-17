#ifndef SQLITE_MANAGER_CLI_TABLE_VIEW_H
#define SQLITE_MANAGER_CLI_TABLE_VIEW_H

#include <iosfwd>

#include "sqlite_manager/result_writer.h"

namespace sqlite_manager_cli {

// Framed ASCII table with a header row:
//   +----+---------+
//   | id | name    |
//   +----+---------+
//   | 1  | M855    |
//   +----+---------+
// SQL NULL renders as the text "NULL". This is the terminal's native
// presentation; CSV and JSON live in the library (sqlite_manager) as
// reusable writers shared with other front-ends.
class TableView final : public sqlite_manager::ResultWriter {
public:
    void Write(const sqlite_manager::QueryResult& result,
               std::ostream& out) const override;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_TABLE_VIEW_H
