#ifndef SQLITE_MANAGER_GUI_WTL_RESULT_MODEL_H
#define SQLITE_MANAGER_GUI_WTL_RESULT_MODEL_H

#include <cstddef>
#include <string>
#include <vector>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_gui::wtl {

// Toolkit-free presentation of a QueryResult for a report-style list view
// (the Windows list control shows columns of rows). It has no Win32
// dependency, so it builds and is unit-tested on every platform, and the
// WTL views layer the actual control on top of it.

// The display text for one cell: SQL NULL shows as an empty string; every
// other value shows the text form the query produced.
std::string CellText(const sqlite_manager::Cell& cell);

// Column header titles, left to right.
std::vector<std::string> ColumnTitles(
    const sqlite_manager::QueryResult& result);

// Row `row` as display strings, one per column. The result always has
// ColumnTitles().size() entries - a missing or short row is padded with
// empty strings - so a caller can index it by column without bounds checks.
std::vector<std::string> RowText(const sqlite_manager::QueryResult& result,
                                 std::size_t row);

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_RESULT_MODEL_H
