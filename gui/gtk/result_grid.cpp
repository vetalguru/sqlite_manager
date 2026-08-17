#include "gui/gtk/result_grid.h"

#include <giomm/liststore.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <gtkmm/columnview.h>
#include <gtkmm/columnviewcolumn.h>
#include <gtkmm/label.h>
#include <gtkmm/listitem.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace sqlite_manager_gui::gtk {

namespace {

// One result row, wrapped as a GObject so it can live in a Gio::ListStore.
class RowObject : public Glib::Object {
public:
    static Glib::RefPtr<RowObject> create(std::vector<std::string> cells) {
        return Glib::make_refptr_for_instance<RowObject>(
            new RowObject(std::move(cells)));
    }

    const std::vector<std::string>& cells() const { return cells_; }

protected:
    explicit RowObject(std::vector<std::string> cells)
        : Glib::ObjectBase(typeid(RowObject)), cells_(std::move(cells)) {}

private:
    std::vector<std::string> cells_;
};

// Display text of a cell: its value, or empty for SQL NULL.
std::string CellText(const sqlite_manager::Cell& cell) {
    return cell.type == sqlite_manager::ValueType::kNull ? std::string()
                                                         : cell.text;
}

}  // namespace

ResultGrid::ResultGrid() {
    set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    view_ = Gtk::make_managed<Gtk::ColumnView>();
    set_child(*view_);
}

void ResultGrid::RemoveColumns() {
    auto columns = view_->get_columns();
    for (guint i = columns->get_n_items(); i > 0; --i) {
        auto column = std::dynamic_pointer_cast<Gtk::ColumnViewColumn>(
            columns->get_object(i - 1));
        if (column) view_->remove_column(column);
    }
}

void ResultGrid::SetResult(const sqlite_manager::QueryResult& result) {
    RemoveColumns();

    auto store = Gio::ListStore<RowObject>::create();
    for (const auto& row : result.rows) {
        std::vector<std::string> cells;
        cells.reserve(row.size());
        for (const auto& cell : row) cells.push_back(CellText(cell));
        store->append(RowObject::create(std::move(cells)));
    }
    view_->set_model(Gtk::SingleSelection::create(store));

    for (std::size_t c = 0; c < result.columns.size(); ++c) {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect(
            [](const Glib::RefPtr<Gtk::ListItem>& item) {
                auto* label = Gtk::make_managed<Gtk::Label>();
                label->set_halign(Gtk::Align::START);
                label->set_margin(4);
                item->set_child(*label);
            });
        factory->signal_bind().connect(
            [c](const Glib::RefPtr<Gtk::ListItem>& item) {
                auto row =
                    std::dynamic_pointer_cast<RowObject>(item->get_item());
                auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
                if (row && label && c < row->cells().size()) {
                    label->set_text(row->cells()[c]);
                }
            });

        auto column = Gtk::ColumnViewColumn::create(result.columns[c], factory);
        column->set_resizable(true);
        view_->append_column(column);
    }
}

void ResultGrid::Clear() {
    RemoveColumns();
    view_->set_model(
        Gtk::SingleSelection::create(Gio::ListStore<RowObject>::create()));
}

}  // namespace sqlite_manager_gui::gtk
