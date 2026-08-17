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

#include <string>
#include <utility>

#include "gui/gtk/result_grid.h"
#include "gui/gtk/schema_sidebar.h"
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

    status_ = Gtk::make_managed<Gtk::Label>("Open a database to begin.");
    status_->set_halign(Gtk::Align::START);
    status_->set_margin(6);

    auto* right = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    right->append(*query_row);
    right->append(*grid_);
    right->append(*status_);

    auto* paned = Gtk::make_managed<Gtk::Paned>(Gtk::Orientation::HORIZONTAL);
    paned->set_start_child(*sidebar_);
    paned->set_end_child(*right);
    paned->set_resize_start_child(false);
    paned->set_shrink_start_child(false);
    paned->set_position(260);
    set_child(*paned);
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
    session_.emplace(std::move(opened).value());
    set_title("SQLite Manager — " + Glib::path_get_basename(path));
    grid_->Clear();
    status_->set_text("Select a table or view, or enter SQL.");
    RefreshSchema();
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
    if (object.kind == ObjectKind::kTable || object.kind == ObjectKind::kView) {
        const std::string sql =
            "SELECT * FROM " + sqlite_manager::QuoteIdentifier(object.name);
        sql_entry_->set_text(sql);
        RunSqlText(sql);
    } else {
        grid_->Clear();
        status_->set_text("Select a table or view to see its rows.");
    }
}

void MainWindow::OnRunSql() { RunSqlText(sql_entry_->get_text().raw()); }

void MainWindow::RunSqlText(const std::string& sql) {
    if (!session_) return;
    auto result = session_->RunQuery(sql);
    if (!result.ok()) {
        grid_->Clear();
        status_->set_text("Error: " + result.error().message);
        return;
    }
    grid_->SetResult(result.value());
    const auto rows = result.value().rows.size();
    status_->set_text(std::to_string(rows) + (rows == 1 ? " row" : " rows"));
}

void MainWindow::ReportError(const Glib::ustring& message) {
    auto dialog = Gtk::AlertDialog::create();
    dialog->set_message(message);
    dialog->show(*this);
}

}  // namespace sqlite_manager_gui::gtk
