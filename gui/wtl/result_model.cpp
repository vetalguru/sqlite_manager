#include "gui/wtl/result_model.h"

#include <cstddef>

namespace sqlite_manager_gui::wtl {

std::string CellText(const sqlite_manager::Cell& cell) {
    if (cell.type == sqlite_manager::ValueType::kNull) return std::string();
    return cell.text;
}

std::vector<std::string> ColumnTitles(
    const sqlite_manager::QueryResult& result) {
    return result.columns;
}

std::vector<std::string> RowText(const sqlite_manager::QueryResult& result,
                                 std::size_t row) {
    std::vector<std::string> cells(result.columns.size());
    if (row >= result.rows.size()) return cells;
    const std::vector<sqlite_manager::Cell>& source = result.rows[row];
    for (std::size_t i = 0; i < cells.size() && i < source.size(); ++i) {
        cells[i] = CellText(source[i]);
    }
    return cells;
}

}  // namespace sqlite_manager_gui::wtl
