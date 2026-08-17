#include "gui/gtk/main_window.h"

#include <giomm/asyncresult.h>
#include <giomm/file.h>
#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/refptr.h>
#include <gtkmm/alertdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/filedialog.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>

#include <utility>

#include "gui/gtk/schema_sidebar.h"

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

    auto* placeholder =
        Gtk::make_managed<Gtk::Label>("Open a database to begin.");

    auto* paned = Gtk::make_managed<Gtk::Paned>(Gtk::Orientation::HORIZONTAL);
    paned->set_start_child(*sidebar_);
    paned->set_end_child(*placeholder);
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

void MainWindow::ReportError(const Glib::ustring& message) {
    auto dialog = Gtk::AlertDialog::create();
    dialog->set_message(message);
    dialog->show(*this);
}

}  // namespace sqlite_manager_gui::gtk
