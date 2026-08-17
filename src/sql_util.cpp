#include "sqlite_manager/sql_util.h"

#include <sqlite3.h>

namespace sqlite_manager {

bool IsCompleteStatement(const std::string& sql) {
    return sqlite3_complete(sql.c_str()) != 0;
}

}  // namespace sqlite_manager
