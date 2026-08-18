#include "gui/gtk/column_dialog.h"

#include <glibmm/markup.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/stringlist.h>

#include <string>
#include <utility>
#include <vector>

namespace sqlite_manager_gui::gtk {

namespace {

// An error line hidden until there is a message (same shape as NewRowDialog).
Gtk::Label* MakeErrorLabel() {
    auto* error = Gtk::make_managed<Gtk::Label>();
    error->set_halign(Gtk::Align::START);
    error->set_margin(6);
    error->set_wrap(true);
    error->set_visible(false);
    return error;
}

void ShowError(Gtk::Label* error, const Glib::ustring& message) {
    error->set_markup("<span foreground='#cc3333'>" +
                      Glib::Markup::escape_text(message) + "</span>");
    error->set_visible(true);
}

// A dimmed, small hint line.
Gtk::Label* MakeNote(const Glib::ustring& text) {
    auto* note = Gtk::make_managed<Gtk::Label>();
    note->set_markup("<span alpha='55%' size='small'>" +
                     Glib::Markup::escape_text(text) + "</span>");
    note->set_halign(Gtk::Align::START);
    note->set_margin(12);
    note->set_wrap(true);
    return note;
}

}  // namespace

AddColumnDialog::AddColumnDialog(Gtk::Window& parent, const std::string& table,
                                 AddFn on_add)
    : on_add_(std::move(on_add)) {
    set_transient_for(parent);
    set_modal(true);
    set_title("Add column to " + table);
    set_default_size(440, -1);

    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(6);
    grid->set_column_spacing(8);
    grid->set_margin(12);

    auto* name_label = Gtk::make_managed<Gtk::Label>("Name");
    name_label->set_halign(Gtk::Align::START);
    name_ = Gtk::make_managed<Gtk::Entry>();
    name_->set_hexpand(true);
    name_->set_placeholder_text("column name");

    auto* type_label = Gtk::make_managed<Gtk::Label>("Type");
    type_label->set_halign(Gtk::Align::START);
    type_ = Gtk::make_managed<Gtk::Entry>();
    type_->set_hexpand(true);
    type_->set_placeholder_text("e.g. TEXT (optional)");

    grid->attach(*name_label, 0, 0);
    grid->attach(*name_, 1, 0);
    grid->attach(*type_label, 0, 1);
    grid->attach(*type_, 1, 1);

    error_ = MakeErrorLabel();

    auto* cancel = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel->signal_clicked().connect([this]() { close(); });
    auto* add = Gtk::make_managed<Gtk::Button>("Add");
    add->add_css_class("suggested-action");
    add->signal_clicked().connect(
        sigc::mem_fun(*this, &AddColumnDialog::OnAdd));

    auto* buttons = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    buttons->set_spacing(6);
    buttons->set_margin(12);
    buttons->set_halign(Gtk::Align::END);
    buttons->append(*cancel);
    buttons->append(*add);

    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->append(*grid);
    box->append(
        *MakeNote("The new column is nullable: SQLite cannot add a NOT NULL or "
                  "PRIMARY KEY column to an existing table."));
    box->append(*error_);
    box->append(*buttons);
    set_child(*box);
}

void AddColumnDialog::OnAdd() {
    const std::string name = name_->get_text().raw();
    if (name.empty()) {
        ShowError(error_, "A column name is required.");
        return;
    }
    std::optional<Glib::ustring> error = on_add_(name, type_->get_text().raw());
    if (error) {
        ShowError(error_, *error);
    } else {
        close();
    }
}

DropColumnDialog::DropColumnDialog(Gtk::Window& parent,
                                   const std::string& table,
                                   const std::vector<std::string>& columns,
                                   DropFn on_drop)
    : on_drop_(std::move(on_drop)), columns_(columns) {
    set_transient_for(parent);
    set_modal(true);
    set_title("Drop column from " + table);
    set_default_size(440, -1);

    std::vector<Glib::ustring> names;
    names.reserve(columns_.size());
    for (const std::string& name : columns_) names.emplace_back(name);
    choice_ = Gtk::make_managed<Gtk::DropDown>();
    choice_->set_model(Gtk::StringList::create(names));
    choice_->set_hexpand(true);

    auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    row->set_spacing(8);
    row->set_margin(12);
    row->append(*Gtk::make_managed<Gtk::Label>("Column"));
    row->append(*choice_);

    error_ = MakeErrorLabel();

    auto* cancel = Gtk::make_managed<Gtk::Button>("Cancel");
    cancel->signal_clicked().connect([this]() { close(); });
    auto* drop = Gtk::make_managed<Gtk::Button>("Drop");
    drop->add_css_class("destructive-action");
    drop->signal_clicked().connect(
        sigc::mem_fun(*this, &DropColumnDialog::OnDrop));

    auto* buttons = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    buttons->set_spacing(6);
    buttons->set_margin(12);
    buttons->set_halign(Gtk::Align::END);
    buttons->append(*cancel);
    buttons->append(*drop);

    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->append(*row);
    box->append(*MakeNote(
        "Dropping a column deletes its data and fails if the column is part "
        "of a primary key, unique constraint or index."));
    box->append(*error_);
    box->append(*buttons);
    set_child(*box);
}

void DropColumnDialog::OnDrop() {
    const guint index = choice_->get_selected();
    if (index >= columns_.size()) {
        ShowError(error_, "Select a column to drop.");
        return;
    }
    std::optional<Glib::ustring> error = on_drop_(columns_[index]);
    if (error) {
        ShowError(error_, *error);
    } else {
        close();
    }
}

}  // namespace sqlite_manager_gui::gtk
