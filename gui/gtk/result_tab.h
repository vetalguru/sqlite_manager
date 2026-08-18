#ifndef SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H
#define SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H

#include <gtkmm/box.h>

#include <functional>
#include <string>
#include <utility>

#include "sqlite_manager/query_result.h"

namespace Gtk {
class Button;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

class ResultGrid;

// Handlers for a tab's own toolbar; each acts on this specific tab. Set by
// the owner (MainWindow), which holds the session and transaction.
struct TabActions {
    std::function<void()> add_row;
    std::function<void()> delete_row;
    std::function<void()> add_column;
    std::function<void()> drop_column;
    std::function<void()> export_result;
};

// One notebook page: a per-table toolbar (row/column edits + export) over a
// result grid, plus the context needed to act on it. Because the edit
// controls belong to a particular table, they live here rather than in a
// shared toolbar. `key` is the page's identity for de-duplication (the
// schema object name, or a private sentinel for the ad-hoc query page).
// `table` names the table backing an editable grid, empty for a read-only
// view or query result. `result` is what the grid shows, kept for export.
class ResultTab : public Gtk::Box {
public:
    explicit ResultTab(std::string key);

    const std::string& key() const { return key_; }
    ResultGrid& grid() { return *grid_; }

    const std::string& table() const { return table_; }
    void set_table(std::string table) { table_ = std::move(table); }

    const sqlite_manager::QueryResult& result() const { return result_; }
    void set_result(sqlite_manager::QueryResult result) {
        result_ = std::move(result);
    }

    // Wires the toolbar buttons to their handlers (call once).
    void set_actions(TabActions actions);
    // Enables the row/column edit buttons (an editable table, in a
    // transaction).
    void set_edit_enabled(bool enabled);
    // Enables the Export button (the tab holds a result to write).
    void set_export_enabled(bool enabled);

private:
    std::string key_;
    std::string table_;
    sqlite_manager::QueryResult result_;
    ResultGrid* grid_ = nullptr;
    Gtk::Button* add_row_ = nullptr;
    Gtk::Button* delete_row_ = nullptr;
    Gtk::Button* add_column_ = nullptr;
    Gtk::Button* drop_column_ = nullptr;
    Gtk::Button* export_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H
