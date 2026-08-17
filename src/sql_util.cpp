#include "sqlite_manager/sql_util.h"

#include <sqlite3.h>

namespace sqlite_manager {

bool IsCompleteStatement(const std::string& sql) {
    return sqlite3_complete(sql.c_str()) != 0;
}

std::string QuoteIdentifier(const std::string& name) {
    std::string out = "\"";
    for (const char c : name) {
        if (c == '"') out += '"';  // double an embedded quote
        out += c;
    }
    out += '"';
    return out;
}

}  // namespace sqlite_manager
