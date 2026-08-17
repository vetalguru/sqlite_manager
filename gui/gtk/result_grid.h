#ifndef SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H
#define SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H

#include <gtkmm/scrolledwindow.h>

#include "sqlite_manager/query_result.h"

namespace Gtk {
class ColumnView;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Displays a QueryResult in a scrollable grid (Gtk::ColumnView). A thin
// view over the core's model: it renders columns and cell text, with no
// knowledge of how the query ran. Columns are rebuilt on each result.
class ResultGrid : public Gtk::ScrolledWindow {
public:
    ResultGrid();

    // Replaces the grid's contents with `result`.
    void SetResult(const sqlite_manager::QueryResult& result);

    // Empties the grid (no columns, no rows).
    void Clear();

private:
    void RemoveColumns();

    Gtk::ColumnView* view_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_RESULT_GRID_H
