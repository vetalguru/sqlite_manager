#ifndef SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
#define SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H

#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "gui/core/database_session.h"
#include "gui/core/schema_info.h"
#include "sqlite_manager/transaction.h"

namespace Gtk {
class Button;
class Entry;
class Label;
class Notebook;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

class SchemaSidebar;
class ResultTab;

// The application's main window: a header bar with an "Open" action, a
// schema sidebar, and a query panel (SQL entry + a notebook of result
// tabs). It owns the open DatabaseSession and wires the views to the core.
// Tables and views each open in their own tab; the toolbar and the shared
// transaction act on the active tab.
class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:
    // Blocks the window close while a dirty transaction is open, so the
    // prompt can offer to commit, discard, or stay.
    bool on_close_request() override;

    void OnOpenClicked();
    void OpenDatabase(const std::string& path);
    void OpenDatabaseNow(const std::string& path);
    void RefreshSchema();
    void OnObjectSelected(const ObjectInfo& object);
    void ShowObject(const ObjectInfo& object);

    // Result tabs.
    ResultTab* active_tab() const;
    ResultTab* find_tab(const std::string& key) const;
    ResultTab* add_tab(const std::string& key, const Glib::ustring& title);
    void OnTabSwitched();
    void LoadTableInto(ResultTab* tab, const std::string& table);
    void LoadViewInto(ResultTab* tab, const std::string& name);
    void ReloadTab(ResultTab* tab);

    void OnRunSql();
    void RunSqlText(const std::string& sql);
    bool OnCellEdited(const std::string& table, std::int64_t rowid,
                      const std::string& column, const std::string& new_text);
    void OnAddRow();
    void OnDeleteRow();
    void OnAddColumn();
    void OnDropColumn();
    void OnBeginTransaction();
    void OnCommitTransaction();
    void OnRollbackTransaction();
    void OnExport();
    void ExportTo(const std::string& path);
    void UpdateActions();
    void ReportError(const Glib::ustring& message);
    // Runs `on_proceed` after resolving any pending (dirty) transaction: it
    // asks whether to save (commit), discard (rollback), or cancel. With no
    // pending changes it runs `on_proceed` at once. `on_cancel`, if given,
    // runs when the user cancels or the commit fails.
    void ConfirmPending(std::function<void()> on_proceed,
                        std::function<void()> on_cancel = {});

    std::optional<DatabaseSession> session_;
    std::optional<sqlite_manager::Transaction> txn_;
    SchemaSidebar* sidebar_ = nullptr;  // owned by the widget tree
    Gtk::Notebook* notebook_ = nullptr;
    Gtk::Entry* sql_entry_ = nullptr;
    Gtk::Label* status_ = nullptr;
    Gtk::Button* add_button_ = nullptr;
    Gtk::Button* delete_button_ = nullptr;
    Gtk::Button* add_column_button_ = nullptr;
    Gtk::Button* drop_column_button_ = nullptr;
    Gtk::Button* begin_button_ = nullptr;
    Gtk::Button* commit_button_ = nullptr;
    Gtk::Button* rollback_button_ = nullptr;
    Gtk::Button* export_button_ = nullptr;
    bool dirty_ = false;  // the open transaction has uncommitted edits
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
