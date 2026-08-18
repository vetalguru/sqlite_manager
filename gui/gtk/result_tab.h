#ifndef SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H
#define SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H

#include <gtkmm/box.h>

#include <string>
#include <utility>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_gui::gtk {

class ResultGrid;

// One notebook page: a result grid plus the context needed to act on it.
// `key` is the page's identity for de-duplication (the schema object name,
// or a private sentinel for the ad-hoc query page). `table` names the table
// backing an editable grid, and is empty for a read-only view or query
// result. `result` is what the grid shows, kept so the page can be exported.
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

private:
    std::string key_;
    std::string table_;
    sqlite_manager::QueryResult result_;
    ResultGrid* grid_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_RESULT_TAB_H
