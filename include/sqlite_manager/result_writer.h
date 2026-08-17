#ifndef SQLITE_MANAGER_RESULT_WRITER_H
#define SQLITE_MANAGER_RESULT_WRITER_H

#include <iosfwd>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager {

// Serializes a QueryResult to a stream. Implementations render the same
// model in different formats without any knowledge of how the query was
// executed - shared by the CLI's output modes and the GUI's export.
// (Strategy: a new format is a new implementation, no caller changes.)
class ResultWriter {
public:
    virtual ~ResultWriter() = default;
    virtual void Write(const QueryResult& result, std::ostream& out) const = 0;
};

// Comma-separated values (RFC 4180): a header row, then one row per
// record. A field is quoted when it contains a comma, double quote, CR
// or LF, and embedded quotes are doubled. SQL NULL renders as an empty
// field.
class CsvWriter final : public ResultWriter {
public:
    void Write(const QueryResult& result, std::ostream& out) const override;
};

// JSON: an array of one object per row, keyed by column name. Integers
// and reals are emitted as JSON numbers, text and blobs as strings
// (escaped per RFC 8259), and SQL NULL as null.
class JsonWriter final : public ResultWriter {
public:
    void Write(const QueryResult& result, std::ostream& out) const override;
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_RESULT_WRITER_H
