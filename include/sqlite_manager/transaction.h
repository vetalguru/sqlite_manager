#ifndef SQLITE_MANAGER_TRANSACTION_H
#define SQLITE_MANAGER_TRANSACTION_H

#include "sqlite_manager/result.h"

namespace sqlite_manager {

class Connection;

// RAII transaction guard.
//
// Usage:
//   auto txn = Transaction::Begin(conn);
//   if (!txn) { /* handle txn.error() */ }
//   ... execute statements ...
//   Status s = txn.value().Commit();   // explicit commit
//   // if Commit() is never called, the destructor rolls back
//
// Rationale: rollback-by-default means an early return or an error
// path cannot accidentally leave a half-finished transaction committed.
//
// Lifetime: must not outlive the Connection.
// Nesting: not supported in v1 (SQLite would need SAVEPOINTs).
class Transaction final {
public:
    // Starts a transaction (BEGIN). Fails if the connection is closed
    // or a transaction is already active on it.
    static Result<Transaction> Begin(Connection& conn);

    Transaction() = default;   // inactive state
    ~Transaction();            // rolls back if still active

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;

    // True until Commit() or Rollback() has been called.
    bool IsActive() const { return conn_ != nullptr; }

    // Commits the transaction. After success the guard becomes inactive.
    // Fails with kMisuse if the transaction is no longer active.
    Status Commit();

    // Rolls back explicitly (the destructor does this implicitly).
    // Safe to call only while active; kMisuse otherwise.
    Status Rollback();

private:
    explicit Transaction(Connection* conn) : conn_(conn) {}

    Connection* conn_ = nullptr;   // not owned; nullptr = inactive
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_TRANSACTION_H