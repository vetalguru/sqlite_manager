#include "gui/gtk/new_row_dialog.h"

#include <glibmm/markup.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>

#include <string>
#include <utility>

namespace sqlite_manager_gui::gtk {

NewRowDialog::NewRowDialog(Gtk::Window& parent, const std::string& table,
                           const std::vector<ColumnInfo>& columns,
                           InsertFn on_insert)
    : on_insert_(std::move(on_insert)) {
    set_transient_for(parent);
    set_modal(true);
    set_title("New row in " + table);
    set_default_size(440, -1);

    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(6);
    grid->set_column_spacing(8);
    grid->set_margin(12);

    int row = 0;
    for (const auto& column : columns) {
        const bool required = column.not_null && column.default_value.empty();

        auto* label = Gtk::make_managed<Gtk::Label>();
        Glib::ustring markup = Glib::Markup::escape_text(column.name);
        if (!column.declared_type.empty()) {
            markup += "  <span alpha='55%' size='small'>" +
                      Glib::Markup::escape_text(column.declared_type) +
                      "</span>";
        }
        if (required) markup += " <span foreground='#cc3333'>*</span>";
        label->set_markup(markup);
        label->set_halign(Gtk::Align::START);

        auto* entry = Gtk::make_managed<Gtk::Entry>();
        entry->set_hexpand(true);
        entry->set_placeholder_text(required ? "required"
                                             : "empty → default / NULL");

        grid->attach(*label, 0, row);
        grid->attach(*entry, 1, row);
        fields_.emplace_back(column.name, entry);
        ++row;
    }

    auto* scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroller->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroller->set_child(*grid);
    scroller->set_vexpand(true);
    // Size to the form's natural height (all fields visible) and only
    // scroll once there are many columns.
    scroller->set_propagate_natural_height(true);
    scroller->set_max_content_height(500);

    error_ = Gtk::make_managed<Gtk::Label>();
    error_->set_halign(Gtk::Align::START);
    error_->set_margin(6);
    error_->set_wrap(true);
    error_->set_visible(false);

    auto* cancel = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel->signal_clicked().connect([this]() { close(); });
    auto* insert = Gtk::make_managed<Gtk::Button>("Insert");
    insert->add_css_class("suggested-action");
    insert->signal_clicked().connect(
        sigc::mem_fun(*this, &NewRowDialog::OnInsert));

    auto* buttons = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    buttons->set_spacing(6);
    buttons->set_margin(12);
    buttons->set_halign(Gtk::Align::END);
    buttons->append(*cancel);
    buttons->append(*insert);

    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->append(*scroller);
    box->append(*error_);
    box->append(*buttons);
    set_child(*box);
}

void NewRowDialog::OnInsert() {
    std::vector<std::pair<std::string, sqlite_manager::Cell>> values;
    for (const auto& [name, entry] : fields_) {
        const std::string text = entry->get_text().raw();
        if (!text.empty()) {
            values.emplace_back(
                name,
                sqlite_manager::Cell{sqlite_manager::ValueType::kText, text});
        }
    }

    std::optional<Glib::ustring> error = on_insert_(values);
    if (error) {
        error_->set_markup("<span foreground='#cc3333'>" +
                           Glib::Markup::escape_text(*error) + "</span>");
        error_->set_visible(true);
    } else {
        close();
    }
}

}  // namespace sqlite_manager_gui::gtk
