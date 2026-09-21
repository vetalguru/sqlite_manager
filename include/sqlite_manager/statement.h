#ifndef SQLITE_MANAGER_STATEMENT_H
#define SQLITE_MANAGER_STATEMENT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "sqlite_manager/result.h"

struct sqlite3_stmt;

namespace sqlite_manager {

class Connection;

// The SQLite storage class of a result value (see sqlite3_column_type).
enum class ValueType { kInteger, kFloat, kText, kBlob, kNull };

// RAII wrapper over a prepared SQL statement.
//
// Typical usage:
//   auto stmt = Statement::Prepare(conn, "SELECT name FROM t WHERE id = ?");
//   if (!stmt) { /* handle stmt.error() */ }
//   stmt.value().BindInt64(1, 42);
//   while (true) {
//       auto step = stmt.value().Step();
//       if (!step) { /* error */ }
//       if (step.value() == Statement::StepResult::kDone) break;
//       auto name = stmt.value().ColumnText(0);
//   }
//
// Lifetime: must not outlive the Connection it was prepared on.
// Threading: same rule as Connection - one thread at a time.
class Statement final {
public:
    enum class StepResult { kRow, kDone };

    // Compiles a single SQL statement. Only whitespace, comments and
    // empty statements (";") may follow it; any further statement is an
    // error (kMisuse): use PrepareNext() to walk a batch.
    static Result<Statement> Prepare(Connection& conn, const std::string& sql);

    // Compiles the next statement of a batch, starting at byte offset
    // `pos` in `sql`, and advances `pos` past it. Leading whitespace,
    // comments and empty statements are skipped. When nothing but those
    // remains, succeeds with an empty Statement (IsValid() is false) and
    // leaves `pos` at the end. Prepare the next statement only after
    // running the previous one: it may depend on the schema changes made.
    //
    //   std::size_t pos = 0;
    //   while (true) {
    //       auto stmt = Statement::PrepareNext(conn, sql, pos);
    //       if (!stmt) { /* error */ }
    //       if (!stmt.value().IsValid()) break;  // batch finished
    //       /* bind, step, read */
    //   }
    static Result<Statement> PrepareNext(Connection& conn,
                                         const std::string& sql,
                                         std::size_t& pos);

    Statement() = default;  // empty (not prepared) state
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    bool IsValid() const { return stmt_ != nullptr; }

    // --- Binding (1-based index, as in SQLite) ---
    Status BindInt64(int index, std::int64_t value);
    Status BindDouble(int index, double value);
    Status BindText(int index, const std::string& value);
    Status BindBlob(int index, const void* data, int size);
    Status BindNull(int index);

    // --- Binding by name ---
    // Pass the full parameter name including its leading sigil, as in
    // SQLite: ":name", "@name" or "$name". Fails with kRange if no such
    // parameter exists in the statement.
    Status BindInt64(const std::string& name, std::int64_t value);
    Status BindDouble(const std::string& name, double value);
    Status BindText(const std::string& name, const std::string& value);
    Status BindBlob(const std::string& name, const void* data, int size);
    Status BindNull(const std::string& name);

    // --- Execution ---
    // Advances one row. kRow: a row is ready to read. kDone: finished.
    Result<StepResult> Step();

    // Rewinds the statement for re-execution (bindings are kept).
    Status Reset();

    // --- Reading the current row (0-based index, as in SQLite) ---
    // Contract: call only after Step() returned kRow, with a valid index.
    // Out-of-contract calls return 0/empty/NULL per SQLite semantics.
    int ColumnCount() const;
    // Name of a result column (its AS alias, or the source expression).
    std::string ColumnName(int index) const;
    // Storage class of the current row's value in this column.
    ValueType ColumnType(int index) const;
    std::int64_t ColumnInt64(int index) const;
    double ColumnDouble(int index) const;
    std::string ColumnText(int index) const;
    std::vector<std::uint8_t> ColumnBlob(int index) const;
    bool ColumnIsNull(int index) const;

private:
    explicit Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

    sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_STATEMENT_H