#ifndef SQLITE_MANAGER_CLI_RESULT_VIEW_H
#define SQLITE_MANAGER_CLI_RESULT_VIEW_H

#include <iosfwd>

namespace sqlite_manager_cli {

struct QueryResult;

// View: renders a QueryResult to a stream. Implementations may format
// the same model as a table, CSV, JSON, etc. without any knowledge of
// how the query was executed.
class ResultView {
public:
    virtual ~ResultView() = default;
    virtual void Render(const QueryResult& result, std::ostream& out) const = 0;
};

// Aligned box table with a header row and a separator rule:
//   | id | name    |
//   |----|---------|
//   | 1  | M855    |
// An empty result set prints the header and rule with no data rows.
class TableView final : public ResultView {
public:
    void Render(const QueryResult& result, std::ostream& out) const override;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_RESULT_VIEW_H
