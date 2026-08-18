#include "gui/gtk/main_window.h"

#include <giomm/asyncresult.h>
#include <giomm/file.h>
#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/refptr.h>
#include <gtkmm/alertdialog.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>
#include <gtkmm/paned.h>
#include <gtkmm/separator.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "gui/gtk/column_dialog.h"
#include "gui/gtk/new_row_dialog.h"
#include "gui/gtk/result_grid.h"
#include "gui/gtk/result_tab.h"
#include "gui/gtk/schema_sidebar.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result_writer.h"
#include "sqlite_manager/sql_util.h"

namespace sqlite_manager_gui::gtk {

namespace {

// Private identity for the single ad-hoc query tab. It cannot collide with
// a schema object name (those never contain a control character).
const char* const kQueryTabKey = "\x01query";

std::string RowCount(std::size_t rows) {
    return std::to_string(rows) + (rows == 1 ? " row" : " rows");
}

}  // namespace

MainWindow::MainWindow() {
    set_title("SQLite Manager");
    set_default_size(960, 640);

    auto* header = Gtk::make_managed<Gtk::HeaderBar>();
    auto* open_button = Gtk::make_managed<Gtk::Button>("Open Database…");
    open_button->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnOpenClicked));
    header->pack_start(*open_button);
    export_button_ = Gtk::make_managed<Gtk::Button>("Export…");
    export_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnExport));
    header->pack_end(*export_button_);
    set_titlebar(*header);

    sidebar_ = Gtk::make_managed<SchemaSidebar>();
    sidebar_->signal_object_selected().connect(
        sigc::mem_fun(*this, &MainWindow::OnObjectSelected));

    // Query panel: an SQL entry with a Run button, a notebook of result
    // tabs, and a status line.
    sql_entry_ = Gtk::make_managed<Gtk::Entry>();
    sql_entry_->set_placeholder_text("Enter SQL, press Enter to run");
    sql_entry_->set_hexpand(true);
    sql_entry_->signal_activate().connect(
        sigc::mem_fun(*this, &MainWindow::OnRunSql));

    auto* run_button = Gtk::make_managed<Gtk::Button>("Run");
    run_button->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnRunSql));

    auto* query_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    query_row->set_spacing(6);
    query_row->set_margin(6);
    query_row->append(*sql_entry_);
    query_row->append(*run_button);

    notebook_ = Gtk::make_managed<Gtk::Notebook>();
    notebook_->set_vexpand(true);
    notebook_->set_scrollable(true);
    notebook_->signal_switch_page().connect(
        [this](Gtk::Widget*, guint) { OnTabSwitched(); });

    // Row and transaction actions.
    add_button_ = Gtk::make_managed<Gtk::Button>("Add Row");
    add_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnAddRow));
    delete_button_ = Gtk::make_managed<Gtk::Button>("Delete Row");
    delete_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnDeleteRow));
    add_column_button_ = Gtk::make_managed<Gtk::Button>("Add Column");
    add_column_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnAddColumn));
    drop_column_button_ = Gtk::make_managed<Gtk::Button>("Drop Column");
    drop_column_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnDropColumn));
    begin_button_ = Gtk::make_managed<Gtk::Button>("Begin");
    begin_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnBeginTransaction));
    commit_button_ = Gtk::make_managed<Gtk::Button>("Commit");
    commit_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnCommitTransaction));
    rollback_button_ = Gtk::make_managed<Gtk::Button>("Rollback");
    rollback_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::OnRollbackTransaction));

    auto* toolbar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    toolbar->set_spacing(6);
    toolbar->set_margin(6);
    toolbar->append(*add_button_);
    toolbar->append(*delete_button_);
    toolbar->append(
        *Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    toolbar->append(*add_column_button_);
    toolbar->append(*drop_column_button_);
    toolbar->append(
        *Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    toolbar->append(*begin_button_);
    toolbar->append(*commit_button_);
    toolbar->append(*rollback_button_);

    status_ = Gtk::make_managed<Gtk::Label>("Open a database to begin.");
    status_->set_halign(Gtk::Align::START);
    status_->set_margin(6);

    auto* right = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    right->append(*query_row);
    right->append(*toolbar);
    right->append(*notebook_);
    right->append(*status_);

    auto* paned = Gtk::make_managed<Gtk::Paned>(Gtk::Orientation::HORIZONTAL);
    paned->set_start_child(*sidebar_);
    paned->set_end_child(*right);
    paned->set_resize_start_child(false);
    paned->set_shrink_start_child(false);
    paned->set_position(260);
    set_child(*paned);

    UpdateActions();
}

void MainWindow::OnOpenClicked() {
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Open SQLite Database");
    dialog->open(*this,
                 [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
                     try {
                         auto file = dialog->open_finish(result);
                         if (file) OpenDatabase(file->get_path());
                     } catch (const Glib::Error&) {
                         // Dialog dismissed or failed; nothing to open.
                     }
                 });
}

void MainWindow::OpenDatabase(const std::string& path) {
    ConfirmPending([this, path]() { OpenDatabaseNow(path); });
}

void MainWindow::OpenDatabaseNow(const std::string& path) {
    auto opened = DatabaseSession::Open(path);
    if (!opened.ok()) {
        ReportError("Cannot open database:\n" + opened.error().message);
        return;
    }
    txn_.reset();  // abandon any transaction on the previous connection
    dirty_ = false;
    while (notebook_->get_n_pages() > 0) notebook_->remove_page(0);
    session_.emplace(std::move(opened).value());
    set_title("SQLite Manager — " + Glib::path_get_basename(path));
    status_->set_text("Select a table or view, or enter SQL.");
    RefreshSchema();
    UpdateActions();
}

void MainWindow::RefreshSchema() {
    if (!session_) return;
    auto objects = session_->ListObjects();
    if (!objects.ok()) {
        ReportError("Cannot read schema:\n" + objects.error().message);
        return;
    }
    sidebar_->Show(objects.value());
}

void MainWindow::OnObjectSelected(const ObjectInfo& object) {
    if (!session_) return;
    // Opening a table/view adds or focuses its own tab; nothing is lost, so
    // this needs no confirmation even with a dirty transaction open.
    ShowObject(object);
}

void MainWindow::ShowObject(const ObjectInfo& object) {
    if (!session_) return;
    if (object.kind != ObjectKind::kTable && object.kind != ObjectKind::kView) {
        status_->set_text("Select a table or view to see its rows.");
        return;
    }
    if (auto* existing = find_tab(object.name)) {
        notebook_->set_current_page(notebook_->page_num(*existing));
        return;
    }
    auto* tab = add_tab(object.name, object.name);
    if (object.kind == ObjectKind::kTable) {
        LoadTableInto(tab, object.name);
    } else {
        LoadViewInto(tab, object.name);
    }
}

ResultTab* MainWindow::active_tab() const {
    if (!notebook_) return nullptr;
    const int page = notebook_->get_current_page();
    if (page < 0) return nullptr;
    return dynamic_cast<ResultTab*>(notebook_->get_nth_page(page));
}

ResultTab* MainWindow::find_tab(const std::string& key) const {
    if (key.empty() || !notebook_) return nullptr;
    const int pages = notebook_->get_n_pages();
    for (int i = 0; i < pages; ++i) {
        auto* tab = dynamic_cast<ResultTab*>(notebook_->get_nth_page(i));
        if (tab && tab->key() == key) return tab;
    }
    return nullptr;
}

ResultTab* MainWindow::add_tab(const std::string& key,
                               const Glib::ustring& title) {
    auto* tab = Gtk::make_managed<ResultTab>(key);
    tab->grid().set_edit_handler([this, tab](std::int64_t rowid,
                                             const std::string& column,
                                             const std::string& text) {
        return OnCellEdited(tab->table(), rowid, column, text);
    });

    // A tab label with a close button.
    auto* label_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    label_box->set_spacing(4);
    label_box->append(*Gtk::make_managed<Gtk::Label>(title));
    auto* close = Gtk::make_managed<Gtk::Button>();
    close->set_icon_name("window-close-symbolic");
    close->set_has_frame(false);
    close->set_tooltip_text("Close tab");
    close->signal_clicked().connect([this, tab]() {
        notebook_->remove_page(*tab);
        UpdateActions();
    });
    label_box->append(*close);

    const int page = notebook_->append_page(*tab, *label_box);
    notebook_->set_current_page(page);
    return tab;
}

void MainWindow::OnTabSwitched() {
    if (auto* tab = active_tab(); tab && !tab->table().empty()) {
        sql_entry_->set_text("SELECT * FROM " +
                             sqlite_manager::QuoteIdentifier(tab->table()));
    }
    UpdateActions();
}

void MainWindow::LoadTableInto(ResultTab* tab, const std::string& table) {
    if (!session_ || !tab) return;
    const std::string quoted = sqlite_manager::QuoteIdentifier(table);
    // rowid addresses the row for edits; hide it from the visible columns.
    auto result = session_->RunQuery("SELECT rowid, * FROM " + quoted);
    if (!result.ok()) {
        // No rowid (e.g. a WITHOUT ROWID table): fall back to read-only.
        LoadViewInto(tab, table);
        return;
    }

    const auto& full = result.value();
    sqlite_manager::QueryResult display;
    std::vector<std::int64_t> rowids;
    for (std::size_t i = 1; i < full.columns.size(); ++i) {
        display.columns.push_back(full.columns[i]);
    }
    for (const auto& row : full.rows) {
        rowids.push_back(row.empty() ? 0
                                     : static_cast<std::int64_t>(std::strtoll(
                                           row[0].text.c_str(), nullptr, 10)));
        display.rows.emplace_back(row.begin() + 1, row.end());
    }

    tab->set_table(table);
    tab->set_result(display);
    sql_entry_->set_text("SELECT * FROM " + quoted);
    tab->grid().SetEditableResult(display, std::move(rowids));
    const std::string hint =
        txn_ ? "double-click a cell to edit" : "press Begin to edit";
    status_->set_text(RowCount(display.rows.size()) + " · " + hint);
    UpdateActions();
}

void MainWindow::LoadViewInto(ResultTab* tab, const std::string& name) {
    if (!session_ || !tab) return;
    const std::string quoted = sqlite_manager::QuoteIdentifier(name);
    auto result = session_->RunQuery("SELECT * FROM " + quoted);
    if (!result.ok()) {
        status_->set_text("Error: " + result.error().message);
        return;
    }
    tab->set_table("");  // read-only
    tab->set_result(result.value());
    sql_entry_->set_text("SELECT * FROM " + quoted);
    tab->grid().SetResult(result.value());
    status_->set_text(RowCount(result.value().rows.size()));
    UpdateActions();
}

void MainWindow::ReloadTab(ResultTab* tab) {
    if (tab && !tab->table().empty()) LoadTableInto(tab, tab->table());
}

void MainWindow::OnRunSql() { RunSqlText(sql_entry_->get_text().raw()); }

void MainWindow::RunSqlText(const std::string& sql) {
    if (!session_) return;
    auto result = session_->RunQuery(sql);
    if (!result.ok()) {
        status_->set_text("Error: " + result.error().message);
        return;
    }
    ResultTab* tab = find_tab(kQueryTabKey);
    if (tab == nullptr) {
        tab = add_tab(kQueryTabKey, "Query");
    } else {
        notebook_->set_current_page(notebook_->page_num(*tab));
    }
    tab->set_table("");  // an arbitrary query result is not editable
    tab->set_result(result.value());
    tab->grid().SetResult(result.value());
    status_->set_text(RowCount(result.value().rows.size()));
    UpdateActions();
}

bool MainWindow::OnCellEdited(const std::string& table, std::int64_t rowid,
                              const std::string& column,
                              const std::string& new_text) {
    if (!session_ || table.empty()) return false;
    if (!txn_) {
        status_->set_text("Press Begin to edit inside a transaction.");
        return false;  // reverts the cell to its old value
    }
    const sqlite_manager::Cell value{sqlite_manager::ValueType::kText,
                                     new_text};
    auto status = session_->UpdateCell(table, rowid, column, value);
    if (!status.ok()) {
        status_->set_text("Update failed: " + status.error().message);
        return false;
    }
    dirty_ = true;
    status_->set_text("Updated.");
    return true;
}

void MainWindow::OnAddRow() {
    auto* tab = active_tab();
    if (!session_ || tab == nullptr || tab->table().empty()) return;
    const std::string table = tab->table();
    auto info = session_->DescribeTable(table);
    if (!info.ok()) {
        ReportError("Cannot read columns:\n" + info.error().message);
        return;
    }

    // The dialog collects values; this callback performs the insert and
    // reports success/failure back so the dialog can stay open on error.
    auto* dialog = new NewRowDialog(
        *this, table, info.value().columns,
        [this, tab,
         table](const std::vector<std::pair<std::string, sqlite_manager::Cell>>&
                    values) -> std::optional<Glib::ustring> {
            auto inserted = session_->InsertRow(table, values);
            if (!inserted.ok()) {
                return Glib::ustring(inserted.error().message);
            }
            dirty_ = true;
            ReloadTab(tab);
            status_->set_text("Row added.");
            return std::nullopt;
        });
    dialog->signal_hide().connect([dialog]() { delete dialog; });
    dialog->present();
}

void MainWindow::OnDeleteRow() {
    auto* tab = active_tab();
    if (!session_ || tab == nullptr || tab->table().empty()) return;
    auto rowid = tab->grid().selected_rowid();
    if (!rowid) {
        status_->set_text("Select a row to delete.");
        return;
    }
    auto status = session_->DeleteRow(tab->table(), *rowid);
    if (!status.ok()) {
        status_->set_text("Delete failed: " + status.error().message);
        return;
    }
    dirty_ = true;
    ReloadTab(tab);
    status_->set_text("Row deleted.");
}

void MainWindow::OnAddColumn() {
    auto* tab = active_tab();
    if (!session_ || tab == nullptr || tab->table().empty()) return;
    const std::string table = tab->table();
    auto* dialog = new AddColumnDialog(
        *this, table,
        [this, tab, table](const std::string& name, const std::string& type)
            -> std::optional<Glib::ustring> {
            auto status = session_->AddColumn(table, name, type);
            if (!status.ok()) return Glib::ustring(status.error().message);
            dirty_ = true;
            ReloadTab(tab);
            RefreshSchema();
            status_->set_text("Column \"" + name + "\" added.");
            return std::nullopt;
        });
    dialog->signal_hide().connect([dialog]() { delete dialog; });
    dialog->present();
}

void MainWindow::OnDropColumn() {
    auto* tab = active_tab();
    if (!session_ || tab == nullptr || tab->table().empty()) return;
    const std::string table = tab->table();
    auto info = session_->DescribeTable(table);
    if (!info.ok()) {
        ReportError("Cannot read columns:\n" + info.error().message);
        return;
    }
    std::vector<std::string> names;
    for (const auto& column : info.value().columns) {
        names.push_back(column.name);
    }
    if (names.empty()) {
        status_->set_text("This table has no columns to drop.");
        return;
    }

    auto* dialog = new DropColumnDialog(
        *this, table, names,
        [this, tab,
         table](const std::string& name) -> std::optional<Glib::ustring> {
            auto status = session_->DropColumn(table, name);
            if (!status.ok()) return Glib::ustring(status.error().message);
            dirty_ = true;
            ReloadTab(tab);
            RefreshSchema();
            status_->set_text("Column \"" + name + "\" dropped.");
            return std::nullopt;
        });
    dialog->signal_hide().connect([dialog]() { delete dialog; });
    dialog->present();
}

void MainWindow::OnBeginTransaction() {
    if (!session_ || txn_) return;
    auto txn = sqlite_manager::Transaction::Begin(session_->connection());
    if (!txn.ok()) {
        status_->set_text("Begin failed: " + txn.error().message);
        return;
    }
    txn_.emplace(std::move(txn).value());
    dirty_ = false;
    status_->set_text("Transaction started — edits are now enabled.");
    UpdateActions();
}

void MainWindow::OnCommitTransaction() {
    if (!txn_) return;
    auto status = txn_->Commit();
    txn_.reset();
    dirty_ = false;
    ReloadTab(active_tab());
    if (status.ok()) {
        status_->set_text("Committed.");
    } else {
        status_->set_text("Commit failed: " + status.error().message);
    }
    UpdateActions();
}

void MainWindow::OnRollbackTransaction() {
    if (!txn_) return;
    auto status = txn_->Rollback();
    txn_.reset();
    dirty_ = false;
    ReloadTab(active_tab());
    if (status.ok()) {
        status_->set_text("Rolled back.");
    } else {
        status_->set_text("Rollback failed: " + status.error().message);
    }
    UpdateActions();
}

void MainWindow::OnExport() {
    auto* tab = active_tab();
    if (tab == nullptr || tab->result().columns.empty()) {
        status_->set_text("Nothing to export.");
        return;
    }
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Export Result");
    dialog->set_initial_name("result.csv");
    dialog->save(*this,
                 [this, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
                     try {
                         auto file = dialog->save_finish(result);
                         if (file) ExportTo(file->get_path());
                     } catch (const Glib::Error&) {
                         // Dialog dismissed or failed; nothing to write.
                     }
                 });
}

void MainWindow::ExportTo(const std::string& path) {
    auto* tab = active_tab();
    if (tab == nullptr) return;
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        status_->set_text("Cannot write: " + path);
        return;
    }
    // Choose the format by extension; CSV is the default.
    const bool json =
        path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
    if (json) {
        sqlite_manager::JsonWriter().Write(tab->result(), out);
    } else {
        sqlite_manager::CsvWriter().Write(tab->result(), out);
    }
    status_->set_text("Exported to " + Glib::path_get_basename(path));
}

void MainWindow::UpdateActions() {
    const bool has_session = session_.has_value();
    auto* tab = active_tab();
    const bool editable =
        has_session && tab != nullptr && !tab->table().empty();
    const bool in_txn = txn_.has_value();
    // Mutations are only allowed inside an explicit transaction, so every
    // change can be reviewed and committed or rolled back as a unit.
    const bool can_edit = editable && in_txn;
    add_button_->set_sensitive(can_edit);
    delete_button_->set_sensitive(can_edit);
    add_column_button_->set_sensitive(can_edit);
    drop_column_button_->set_sensitive(can_edit);
    begin_button_->set_sensitive(editable && !in_txn);
    commit_button_->set_sensitive(in_txn);
    rollback_button_->set_sensitive(in_txn);
    export_button_->set_sensitive(tab != nullptr &&
                                  !tab->result().columns.empty());
}

void MainWindow::ReportError(const Glib::ustring& message) {
    auto dialog = Gtk::AlertDialog::create();
    dialog->set_message(message);
    dialog->show(*this);
}

void MainWindow::ConfirmPending(std::function<void()> on_proceed,
                                std::function<void()> on_cancel) {
    if (!txn_ || !dirty_) {
        on_proceed();
        return;
    }

    auto* tab = active_tab();
    const std::string where = (tab != nullptr && !tab->table().empty())
                                  ? "\"" + tab->table() + "\""
                                  : "The database";
    auto dialog = Gtk::AlertDialog::create();
    dialog->set_modal(true);
    dialog->set_message("Save changes before continuing?");
    dialog->set_detail(where +
                       " has uncommitted changes in the open transaction.");
    dialog->set_buttons({"Cancel", "Discard", "Save"});
    dialog->set_cancel_button(0);
    dialog->set_default_button(2);
    dialog->choose(*this, [this, dialog, on_proceed = std::move(on_proceed),
                           on_cancel = std::move(on_cancel)](
                              const Glib::RefPtr<Gio::AsyncResult>& result) {
        int button = 0;
        try {
            button = dialog->choose_finish(result);
        } catch (const Glib::Error&) {
            button = 0;  // dismissed (Escape) counts as Cancel
        }
        if (button == 0) {  // Cancel: stay put
            if (on_cancel) on_cancel();
            return;
        }
        if (button == 2) {  // Save
            auto status = txn_->Commit();
            if (!status.ok()) {
                status_->set_text("Commit failed: " + status.error().message);
                if (on_cancel) on_cancel();
                return;
            }
        } else {  // Discard
            txn_->Rollback();
        }
        txn_.reset();
        dirty_ = false;
        UpdateActions();
        on_proceed();
    });
}

bool MainWindow::on_close_request() {
    if (!txn_ || !dirty_) return false;  // nothing pending; allow the close
    // Block this close and ask; on save or discard, re-issue the close.
    ConfirmPending([this]() { close(); });
    return true;
}

}  // namespace sqlite_manager_gui::gtk
