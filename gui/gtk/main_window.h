#ifndef SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
#define SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H

#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>

#include <cstdint>
#include <optional>
#include <string>

#include "gui/core/database_session.h"
#include "gui/core/schema_info.h"
#include "sqlite_manager/transaction.h"

namespace Gtk {
class Button;
class Entry;
class Label;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

class SchemaSidebar;
class ResultGrid;

// The application's main window: a header bar with an "Open" action, a
// schema sidebar, and a query panel (SQL entry + result grid). It owns
// the open DatabaseSession and wires the views to the core.
class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:
    void OnOpenClicked();
    void OpenDatabase(const std::string& path);
    void RefreshSchema();
    void OnObjectSelected(const ObjectInfo& object);
    void OnRunSql();
    void RunSqlText(const std::string& sql);
    void LoadTable(const std::string& table);
    void ReloadTable();
    bool OnCellEdited(std::int64_t rowid, const std::string& column,
                      const std::string& new_text);
    void OnAddRow();
    void OnDeleteRow();
    void OnBeginTransaction();
    void OnCommitTransaction();
    void OnRollbackTransaction();
    void UpdateActions();
    void ReportError(const Glib::ustring& message);

    std::optional<DatabaseSession> session_;
    std::optional<sqlite_manager::Transaction> txn_;
    SchemaSidebar* sidebar_ = nullptr;  // owned by the widget tree
    ResultGrid* grid_ = nullptr;
    Gtk::Entry* sql_entry_ = nullptr;
    Gtk::Label* status_ = nullptr;
    Gtk::Button* add_button_ = nullptr;
    Gtk::Button* delete_button_ = nullptr;
    Gtk::Button* begin_button_ = nullptr;
    Gtk::Button* commit_button_ = nullptr;
    Gtk::Button* rollback_button_ = nullptr;
    std::string edit_table_;  // table backing an editable grid; empty if none
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
