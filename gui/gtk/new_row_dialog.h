#ifndef SQLITE_MANAGER_GUI_GTK_NEW_ROW_DIALOG_H
#define SQLITE_MANAGER_GUI_GTK_NEW_ROW_DIALOG_H

#include <glibmm/ustring.h>
#include <gtkmm/window.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gui/core/schema_info.h"
#include "sqlite_manager/query_result.h"  // Cell

namespace Gtk {
class Entry;
class Label;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Modal form for inserting one row into a table: an entry per column, with
// NOT-NULL columns (that lack a default) marked required. Leaving a field
// empty omits that column, so its default or NULL applies. The insert is
// delegated to a callback so this widget stays free of database logic.
class NewRowDialog : public Gtk::Window {
public:
    // Attempts the insert with the collected (column, value) pairs. Returns
    // an empty optional on success (the dialog closes) or an error message
    // to show (the dialog stays open).
    using InsertFn = std::function<std::optional<Glib::ustring>(
        const std::vector<std::pair<std::string, sqlite_manager::Cell>>&)>;

    NewRowDialog(Gtk::Window& parent, const std::string& table,
                 const std::vector<ColumnInfo>& columns, InsertFn on_insert);

private:
    void OnInsert();

    InsertFn on_insert_;
    std::vector<std::pair<std::string, Gtk::Entry*>> fields_;
    Gtk::Label* error_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_NEW_ROW_DIALOG_H
