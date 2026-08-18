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
#include "gui/gtk/schema_sidebar.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result_writer.h"
#include "sqlite_manager/sql_util.h"

namespace sqlite_manager_gui::gtk {

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

    // Query panel: an SQL entry with a Run button, the result grid, and a
    // status line.
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

    grid_ = Gtk::make_managed<ResultGrid>();
    grid_->set_vexpand(true);
    grid_->set_edit_handler([this](std::int64_t rowid,
                                   const std::string& column,
                                   const std::string& new_text) {
        return OnCellEdited(rowid, column, new_text);
    });

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
    right->append(*grid_);
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
    auto opened = DatabaseSession::Open(path);
    if (!opened.ok()) {
        ReportError("Cannot open database:\n" + opened.error().message);
        return;
    }
    txn_.reset();  // abandon any transaction on the previous connection
    edit_table_.clear();
    last_result_ = {};
    session_.emplace(std::move(opened).value());
    set_title("SQLite Manager — " + Glib::path_get_basename(path));
    grid_->Clear();
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
    if (object.kind == ObjectKind::kTable) {
        LoadTable(object.name);
    } else if (object.kind == ObjectKind::kView) {
        const std::string sql =
            "SELECT * FROM " + sqlite_manager::QuoteIdentifier(object.name);
        sql_entry_->set_text(sql);
        RunSqlText(sql);  // views are read-only
    } else {
        edit_table_.clear();
        last_result_ = {};
        grid_->Clear();
        status_->set_text("Select a table or view to see its rows.");
        UpdateActions();
    }
}

void MainWindow::LoadTable(const std::string& table) {
    if (!session_) return;
    const std::string quoted = sqlite_manager::QuoteIdentifier(table);
    // rowid addresses the row for edits; hide it from the visible columns.
    auto result = session_->RunQuery("SELECT rowid, * FROM " + quoted);
    if (!result.ok()) {
        // No rowid (e.g. a WITHOUT ROWID table): fall back to read-only.
        RunSqlText("SELECT * FROM " + quoted);
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

    edit_table_ = table;
    last_result_ = display;
    sql_entry_->set_text("SELECT * FROM " + quoted);
    grid_->SetEditableResult(display, std::move(rowids));
    const auto rows = display.rows.size();
    status_->set_text(std::to_string(rows) + (rows == 1 ? " row" : " rows") +
                      " · double-click a cell to edit");
    UpdateActions();
}

void MainWindow::ReloadTable() {
    if (!edit_table_.empty()) LoadTable(edit_table_);
}

void MainWindow::OnRunSql() { RunSqlText(sql_entry_->get_text().raw()); }

void MainWindow::RunSqlText(const std::string& sql) {
    if (!session_) return;
    edit_table_.clear();  // an arbitrary query result is not editable
    auto result = session_->RunQuery(sql);
    if (!result.ok()) {
        grid_->Clear();
        status_->set_text("Error: " + result.error().message);
        return;
    }
    last_result_ = result.value();
    grid_->SetResult(result.value());
    const auto rows = result.value().rows.size();
    status_->set_text(std::to_string(rows) + (rows == 1 ? " row" : " rows"));
    UpdateActions();
}

bool MainWindow::OnCellEdited(std::int64_t rowid, const std::string& column,
                              const std::string& new_text) {
    if (!session_ || edit_table_.empty()) return false;
    const sqlite_manager::Cell value{sqlite_manager::ValueType::kText,
                                     new_text};
    auto status = session_->UpdateCell(edit_table_, rowid, column, value);
    if (!status.ok()) {
        status_->set_text("Update failed: " + status.error().message);
        return false;
    }
    status_->set_text("Updated.");
    return true;
}

void MainWindow::OnAddRow() {
    if (!session_ || edit_table_.empty()) return;
    auto info = session_->DescribeTable(edit_table_);
    if (!info.ok()) {
        ReportError("Cannot read columns:\n" + info.error().message);
        return;
    }

    // The dialog collects values; this callback performs the insert and
    // reports success/failure back so the dialog can stay open on error.
    auto* dialog = new NewRowDialog(
        *this, edit_table_, info.value().columns,
        [this](const std::vector<std::pair<std::string, sqlite_manager::Cell>>&
                   values) -> std::optional<Glib::ustring> {
            auto inserted = session_->InsertRow(edit_table_, values);
            if (!inserted.ok()) {
                return Glib::ustring(inserted.error().message);
            }
            ReloadTable();
            status_->set_text("Row added.");
            return std::nullopt;
        });
    dialog->signal_hide().connect([dialog]() { delete dialog; });
    dialog->present();
}

void MainWindow::OnDeleteRow() {
    if (!session_ || edit_table_.empty()) return;
    auto rowid = grid_->selected_rowid();
    if (!rowid) {
        status_->set_text("Select a row to delete.");
        return;
    }
    auto status = session_->DeleteRow(edit_table_, *rowid);
    if (!status.ok()) {
        status_->set_text("Delete failed: " + status.error().message);
        return;
    }
    ReloadTable();
    status_->set_text("Row deleted.");
}

void MainWindow::OnAddColumn() {
    if (!session_ || edit_table_.empty()) return;
    auto* dialog = new AddColumnDialog(
        *this, edit_table_,
        [this](const std::string& name,
               const std::string& type) -> std::optional<Glib::ustring> {
            auto status = session_->AddColumn(edit_table_, name, type);
            if (!status.ok()) return Glib::ustring(status.error().message);
            ReloadTable();
            RefreshSchema();
            status_->set_text("Column \"" + name + "\" added.");
            return std::nullopt;
        });
    dialog->signal_hide().connect([dialog]() { delete dialog; });
    dialog->present();
}

void MainWindow::OnDropColumn() {
    if (!session_ || edit_table_.empty()) return;
    auto info = session_->DescribeTable(edit_table_);
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
        *this, edit_table_, names,
        [this](const std::string& name) -> std::optional<Glib::ustring> {
            auto status = session_->DropColumn(edit_table_, name);
            if (!status.ok()) return Glib::ustring(status.error().message);
            ReloadTable();
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
    status_->set_text("Transaction started.");
    UpdateActions();
}

void MainWindow::OnCommitTransaction() {
    if (!txn_) return;
    auto status = txn_->Commit();
    txn_.reset();
    ReloadTable();
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
    ReloadTable();
    if (status.ok()) {
        status_->set_text("Rolled back.");
    } else {
        status_->set_text("Rollback failed: " + status.error().message);
    }
    UpdateActions();
}

void MainWindow::OnExport() {
    if (last_result_.columns.empty()) {
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
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        status_->set_text("Cannot write: " + path);
        return;
    }
    // Choose the format by extension; CSV is the default.
    const bool json =
        path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
    if (json) {
        sqlite_manager::JsonWriter().Write(last_result_, out);
    } else {
        sqlite_manager::CsvWriter().Write(last_result_, out);
    }
    status_->set_text("Exported to " + Glib::path_get_basename(path));
}

void MainWindow::UpdateActions() {
    const bool has_session = session_.has_value();
    const bool editable = has_session && !edit_table_.empty();
    const bool in_txn = txn_.has_value();
    add_button_->set_sensitive(editable);
    delete_button_->set_sensitive(editable);
    add_column_button_->set_sensitive(editable);
    drop_column_button_->set_sensitive(editable);
    begin_button_->set_sensitive(has_session && !in_txn);
    commit_button_->set_sensitive(in_txn);
    rollback_button_->set_sensitive(in_txn);
    export_button_->set_sensitive(!last_result_.columns.empty());
}

void MainWindow::ReportError(const Glib::ustring& message) {
    auto dialog = Gtk::AlertDialog::create();
    dialog->set_message(message);
    dialog->show(*this);
}

}  // namespace sqlite_manager_gui::gtk
