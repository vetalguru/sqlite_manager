#include "gui/gtk/result_tab.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/enums.h>
#include <gtkmm/separator.h>

#include <utility>

#include "gui/gtk/result_grid.h"

namespace sqlite_manager_gui::gtk {

ResultTab::ResultTab(std::string key)
    : Gtk::Box(Gtk::Orientation::VERTICAL), key_(std::move(key)) {
    // Per-table toolbar: row and column edits, plus export of this result.
    auto* toolbar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    toolbar->set_spacing(6);
    toolbar->set_margin(6);
    add_row_ = Gtk::make_managed<Gtk::Button>("Add Row");
    delete_row_ = Gtk::make_managed<Gtk::Button>("Delete Row");
    add_column_ = Gtk::make_managed<Gtk::Button>("Add Column");
    drop_column_ = Gtk::make_managed<Gtk::Button>("Drop Column");
    export_ = Gtk::make_managed<Gtk::Button>("Export…");
    toolbar->append(*add_row_);
    toolbar->append(*delete_row_);
    toolbar->append(
        *Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    toolbar->append(*add_column_);
    toolbar->append(*drop_column_);
    toolbar->append(
        *Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    toolbar->append(*export_);

    grid_ = Gtk::make_managed<ResultGrid>();
    grid_->set_vexpand(true);

    append(*toolbar);
    append(*grid_);

    set_edit_enabled(false);
    set_export_enabled(false);
}

void ResultTab::set_actions(TabActions actions) {
    if (actions.add_row) {
        add_row_->signal_clicked().connect(std::move(actions.add_row));
    }
    if (actions.delete_row) {
        delete_row_->signal_clicked().connect(std::move(actions.delete_row));
    }
    if (actions.add_column) {
        add_column_->signal_clicked().connect(std::move(actions.add_column));
    }
    if (actions.drop_column) {
        drop_column_->signal_clicked().connect(std::move(actions.drop_column));
    }
    if (actions.export_result) {
        export_->signal_clicked().connect(std::move(actions.export_result));
    }
}

void ResultTab::set_edit_enabled(bool enabled) {
    add_row_->set_sensitive(enabled);
    delete_row_->set_sensitive(enabled);
    add_column_->set_sensitive(enabled);
    drop_column_->set_sensitive(enabled);
}

void ResultTab::set_export_enabled(bool enabled) {
    export_->set_sensitive(enabled);
}

}  // namespace sqlite_manager_gui::gtk
