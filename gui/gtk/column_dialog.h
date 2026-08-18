#ifndef SQLITE_MANAGER_GUI_GTK_COLUMN_DIALOG_H
#define SQLITE_MANAGER_GUI_GTK_COLUMN_DIALOG_H

#include <glibmm/ustring.h>
#include <gtkmm/window.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Gtk {
class DropDown;
class Entry;
class Label;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Modal form to add a column to a table: a name and an optional declared
// type. The ALTER is delegated to a callback so this widget stays free of
// database logic. The new column is nullable - a SQLite restriction.
class AddColumnDialog : public Gtk::Window {
public:
    // Returns an empty optional on success (the dialog closes) or an error
    // message to show (the dialog stays open).
    using AddFn = std::function<std::optional<Glib::ustring>(
        const std::string& name, const std::string& type)>;

    AddColumnDialog(Gtk::Window& parent, const std::string& table,
                    AddFn on_add);

private:
    void OnAdd();

    AddFn on_add_;
    Gtk::Entry* name_ = nullptr;
    Gtk::Entry* type_ = nullptr;
    Gtk::Label* error_ = nullptr;
};

// Modal form to drop a column from a table: pick one of its columns. The
// drop is delegated to a callback, matching AddColumnDialog.
class DropColumnDialog : public Gtk::Window {
public:
    using DropFn =
        std::function<std::optional<Glib::ustring>(const std::string& name)>;

    DropColumnDialog(Gtk::Window& parent, const std::string& table,
                     const std::vector<std::string>& columns, DropFn on_drop);

private:
    void OnDrop();

    DropFn on_drop_;
    std::vector<std::string> columns_;
    Gtk::DropDown* choice_ = nullptr;
    Gtk::Label* error_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_COLUMN_DIALOG_H
