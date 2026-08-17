#ifndef SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H
#define SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H

#include <gtkmm/scrolledwindow.h>
#include <sigc++/connection.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "sqlite_manager/query_result.h"

namespace Gtk {
class ColumnView;
class EditableLabel;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Displays a QueryResult in a scrollable grid (Gtk::ColumnView), either
// read-only or with editable cells. A thin view over the core model: it
// renders cells and reports edits, but performs no database work itself.
class ResultGrid : public Gtk::ScrolledWindow {
public:
    // Invoked when a cell is edited. Returns true to accept the new text
    // (the cell keeps it), false to revert the cell to its old value.
    using EditHandler =
        std::function<bool(std::int64_t rowid, const std::string& column,
                           const std::string& new_text)>;

    ResultGrid();

    // Read-only display of `result`.
    void SetResult(const sqlite_manager::QueryResult& result);

    // Editable display: `result` holds the visible columns and rows, and
    // `rowids` the rowid of each row (parallel to result.rows). Editing a
    // cell invokes the handler set with set_edit_handler().
    void SetEditableResult(const sqlite_manager::QueryResult& result,
                           std::vector<std::int64_t> rowids);

    void set_edit_handler(EditHandler handler) {
        edit_handler_ = std::move(handler);
    }

    // Empties the grid (no columns, no rows).
    void Clear();

private:
    void Populate(const sqlite_manager::QueryResult& result, bool editable,
                  const std::vector<std::int64_t>& rowids);
    void RemoveColumns();
    bool AcceptEdit(std::int64_t rowid, std::size_t column,
                    const std::string& text);

    Gtk::ColumnView* view_ = nullptr;
    EditHandler edit_handler_;
    std::vector<std::string> columns_;
    std::map<Gtk::EditableLabel*, sigc::connection> edit_conns_;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H
