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

// Framed ASCII table with a header row:
//   +----+---------+
//   | id | name    |
//   +----+---------+
//   | 1  | M855    |
//   +----+---------+
// SQL NULL renders as the text "NULL".
class TableView final : public ResultView {
public:
    void Render(const QueryResult& result, std::ostream& out) const override;
};

// Comma-separated values (RFC 4180): a header row, then one row per
// record. A field is quoted when it contains a comma, double quote, CR
// or LF, and embedded quotes are doubled. SQL NULL renders as an empty
// field.
class CsvView final : public ResultView {
public:
    void Render(const QueryResult& result, std::ostream& out) const override;
};

// JSON: an array of one object per row, keyed by column name. Integers
// and reals are emitted as JSON numbers, text and blobs as strings
// (escaped per RFC 8259), and SQL NULL as null.
class JsonView final : public ResultView {
public:
    void Render(const QueryResult& result, std::ostream& out) const override;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_RESULT_VIEW_H
