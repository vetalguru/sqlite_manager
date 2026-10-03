#ifndef SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
#define SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "gui/core/database_session.h"
#include "gui/core/schema_info.h"
#include "gui/wtl/edit_cell_dialog.h"
#include "gui/wtl/resource.h"
#include "gui/wtl/result_model.h"
#include "gui/wtl/text.h"
#include "gui/wtl/wtl.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/sql_util.h"
#include "sqlite_manager/transaction.h"

namespace sqlite_manager_gui::wtl {

// The application's main window: a File menu, a status bar, and a splitter
// with the schema objects on the left and the rows of the selected table or
// view on the right. It owns the open DatabaseSession and drives the views
// from the toolkit-free core.
class CMainFrame : public CFrameWindowImpl<CMainFrame> {
public:
    DECLARE_FRAME_WND_CLASS(L"SqliteManagerWtlFrame", IDR_MAINFRAME)

    // clang-format off
    BEGIN_MSG_MAP(CMainFrame)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
        COMMAND_ID_HANDLER(ID_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_TXN_BEGIN, OnTxnBegin)
        COMMAND_ID_HANDLER(ID_TXN_COMMIT, OnTxnCommit)
        COMMAND_ID_HANDLER(ID_TXN_ROLLBACK, OnTxnRollback)
        COMMAND_ID_HANDLER(ID_EDIT_ADD_ROW, OnAddRow)
        COMMAND_ID_HANDLER(ID_EDIT_DELETE_ROW, OnDeleteRow)
        NOTIFY_HANDLER(IDC_OBJECTS, LVN_ITEMACTIVATE, OnObjectActivate)
        NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, OnResultDblClick)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()
    // clang-format on

    LRESULT OnCreate(LPCREATESTRUCT /*create*/) {
        CreateSimpleStatusBar();

        m_splitter.Create(
            m_hWnd, rcDefault, nullptr,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        m_hWndClient = m_splitter;

        m_objects.Create(m_splitter, rcDefault, nullptr,
                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
                             LVS_SHOWSELALWAYS,
                         WS_EX_CLIENTEDGE, IDC_OBJECTS);
        m_objects.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
        m_objects.InsertColumn(0, L"Name", LVCFMT_LEFT, 160);
        m_objects.InsertColumn(1, L"Type", LVCFMT_LEFT, 70);

        m_results.Create(m_splitter, rcDefault, nullptr,
                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                         WS_EX_CLIENTEDGE, IDC_RESULTS);
        m_results.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT |
                                           LVS_EX_GRIDLINES);

        m_splitter.SetSplitterPanes(m_objects, m_results);
        m_splitter.SetSplitterPosPct(28);

        SetStatus(L"Open a database to begin.");
        return 0;
    }

    void OnDestroy() { ::PostQuitMessage(0); }

    LRESULT OnFileOpen(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                       BOOL& /*handled*/) {
        CFileDialog dialog(
            /*bOpenFileDialog=*/TRUE, nullptr, nullptr,
            OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
            L"SQLite databases\0*.db;*.sqlite;*.sqlite3\0All files\0*.*\0",
            *this);
        dialog.m_ofn.lpstrTitle = L"Open SQLite Database";
        if (dialog.DoModal(*this) == IDOK) OpenDatabase(dialog.m_szFileName);
        return 0;
    }

    LRESULT OnFileExit(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                       BOOL& /*handled*/) {
        PostMessage(WM_CLOSE);
        return 0;
    }

    // Double-click or Enter on a schema object loads its rows.
    LRESULT OnObjectActivate(int /*id*/, LPNMHDR header, BOOL& /*handled*/) {
        const LPNMITEMACTIVATE activate =
            reinterpret_cast<LPNMITEMACTIVATE>(header);
        const int index = activate->iItem;
        if (index >= 0 && static_cast<size_t>(index) < objects_.size()) {
            LoadObject(objects_[static_cast<size_t>(index)]);
        }
        return 0;
    }

    LRESULT OnTxnBegin(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                       BOOL& /*handled*/) {
        if (!session_ || txn_) return 0;
        auto begun = sqlite_manager::Transaction::Begin(session_->connection());
        if (!begun.ok()) {
            SetStatus(Widen("Begin failed: " + begun.error().message).c_str());
            return 0;
        }
        txn_.emplace(std::move(begun).value());
        dirty_ = false;
        SetStatus(L"Transaction started - edits are now enabled.");
        return 0;
    }

    LRESULT OnTxnCommit(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                        BOOL& /*handled*/) {
        if (!txn_) return 0;
        const sqlite_manager::Status status = txn_->Commit();
        txn_.reset();
        dirty_ = false;
        ReloadCurrent();
        SetStatus(
            status.ok()
                ? L"Committed."
                : Widen("Commit failed: " + status.error().message).c_str());
        return 0;
    }

    LRESULT OnTxnRollback(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                          BOOL& /*handled*/) {
        if (!txn_) return 0;
        const sqlite_manager::Status status = txn_->Rollback();
        txn_.reset();
        dirty_ = false;
        ReloadCurrent();
        SetStatus(
            status.ok()
                ? L"Rolled back."
                : Widen("Rollback failed: " + status.error().message).c_str());
        return 0;
    }

    // Double-click a result cell to edit it (editable table, in a
    // transaction).
    LRESULT OnResultDblClick(int /*id*/, LPNMHDR header, BOOL& /*handled*/) {
        const LPNMITEMACTIVATE activate =
            reinterpret_cast<LPNMITEMACTIVATE>(header);
        EditCell(activate->iItem, activate->iSubItem);
        return 0;
    }

    LRESULT OnAddRow(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        if (!EnsureEditable()) return 0;
        auto inserted = session_->InsertRow(current_->name, {});
        if (!inserted.ok()) {
            SetStatus(
                Widen("Insert failed: " + inserted.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadCurrent();
        SetStatus(L"Row added - double-click a cell to edit it.");
        return 0;
    }

    LRESULT OnDeleteRow(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                        BOOL& /*handled*/) {
        if (!EnsureEditable()) return 0;
        const int selected = m_results.GetSelectedIndex();
        if (selected < 0 || static_cast<size_t>(selected) >= rowids_.size()) {
            SetStatus(L"Select a row to delete.");
            return 0;
        }
        const sqlite_manager::Status status = session_->DeleteRow(
            current_->name, rowids_[static_cast<size_t>(selected)]);
        if (!status.ok()) {
            SetStatus(
                Widen("Delete failed: " + status.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadCurrent();
        SetStatus(L"Row deleted.");
        return 0;
    }

private:
    // True when the current object is an editable table inside a
    // transaction; otherwise reports why and returns false.
    bool EnsureEditable() {
        if (!session_ || !current_ || !editable_) {
            SetStatus(L"Select a table to edit.");
            return false;
        }
        if (!txn_) {
            SetStatus(L"Press Begin to edit inside a transaction.");
            return false;
        }
        return true;
    }

    void OpenDatabase(LPCWSTR wide_path) {
        auto opened = DatabaseSession::Open(Narrow(wide_path));
        if (!opened.ok()) {
            MessageBox(Widen(opened.error().message).c_str(),
                       L"Cannot open database", MB_ICONERROR | MB_OK);
            return;
        }
        session_.emplace(std::move(opened).value());
        ClearResults();
        PopulateObjects();
    }

    void PopulateObjects() {
        m_objects.DeleteAllItems();
        objects_.clear();
        if (!session_) return;
        auto objects = session_->ListObjects();
        if (!objects.ok()) {
            SetStatus(Widen("Cannot read schema: " + objects.error().message)
                          .c_str());
            return;
        }
        objects_ = std::move(objects).value();
        int row = 0;
        for (const ObjectInfo& object : objects_) {
            m_objects.InsertItem(row, Widen(object.name).c_str());
            m_objects.SetItemText(row, 1, KindLabel(object.kind));
            ++row;
        }
        SetStatus(
            Widen(std::to_string(objects_.size()) + " schema objects").c_str());
    }

    void LoadObject(const ObjectInfo& object) {
        if (!session_) return;
        if (object.kind == ObjectKind::kTable) {
            current_ = object;
            LoadEditable(object.name);
        } else if (object.kind == ObjectKind::kView) {
            current_ = object;
            LoadReadonly(object.name);
        } else {
            current_.reset();
            editable_ = false;
            ClearResults();
            SetStatus(L"Select a table or view to see its rows.");
        }
    }

    // A table loaded with its rowid so cells can be addressed for editing;
    // the rowid column is kept out of the display. Falls back to read-only
    // if the table has no rowid (e.g. WITHOUT ROWID).
    void LoadEditable(const std::string& name) {
        const std::string quoted = sqlite_manager::QuoteIdentifier(name);
        auto result = session_->RunQuery("SELECT rowid, * FROM " + quoted);
        if (!result.ok()) {
            LoadReadonly(name);
            return;
        }
        const sqlite_manager::QueryResult& full = result.value();
        sqlite_manager::QueryResult display;
        for (size_t i = 1; i < full.columns.size(); ++i) {
            display.columns.push_back(full.columns[i]);
        }
        rowids_.clear();
        for (const std::vector<sqlite_manager::Cell>& row : full.rows) {
            rowids_.push_back(row.empty()
                                  ? 0
                                  : static_cast<std::int64_t>(std::strtoll(
                                        row[0].text.c_str(), nullptr, 10)));
            display.rows.emplace_back(row.begin() + 1, row.end());
        }
        editable_ = true;
        current_columns_ = display.columns;
        current_result_ = display;
        FillResults(display);
        ReportRowCount(name, display.rows.size());
    }

    void LoadReadonly(const std::string& name) {
        const std::string quoted = sqlite_manager::QuoteIdentifier(name);
        auto result = session_->RunQuery("SELECT * FROM " + quoted);
        if (!result.ok()) {
            editable_ = false;
            ClearResults();
            SetStatus(Widen("Error: " + result.error().message).c_str());
            return;
        }
        editable_ = false;
        rowids_.clear();
        current_columns_ = result.value().columns;
        current_result_ = result.value();
        FillResults(result.value());
        ReportRowCount(name, result.value().rows.size());
    }

    void ReportRowCount(const std::string& name, size_t rows) {
        SetStatus(Widen(name + " - " + std::to_string(rows) +
                        (rows == 1 ? " row" : " rows"))
                      .c_str());
    }

    // Edit one cell (double-clicked) of the current editable table.
    void EditCell(int item, int sub) {
        if (!session_ || !current_ || !editable_) return;
        if (!txn_) {
            SetStatus(L"Press Begin to edit inside a transaction.");
            return;
        }
        if (item < 0 || static_cast<size_t>(item) >= rowids_.size()) return;
        if (sub < 0 || static_cast<size_t>(sub) >= current_columns_.size()) {
            return;
        }
        const std::vector<sqlite_manager::Cell>& row =
            current_result_.rows[static_cast<size_t>(item)];
        const std::wstring value =
            Widen(static_cast<size_t>(sub) < row.size()
                      ? CellText(row[static_cast<size_t>(sub)])
                      : std::string());
        CEditCellDialog dialog(
            Widen(current_columns_[static_cast<size_t>(sub)]), value);
        if (dialog.DoModal(*this) != IDOK) return;
        const sqlite_manager::Cell cell{sqlite_manager::ValueType::kText,
                                        Narrow(dialog.Value())};
        const sqlite_manager::Status status = session_->UpdateCell(
            current_->name, rowids_[static_cast<size_t>(item)],
            current_columns_[static_cast<size_t>(sub)], cell);
        if (!status.ok()) {
            SetStatus(
                Widen("Update failed: " + status.error().message).c_str());
            return;
        }
        dirty_ = true;
        ReloadCurrent();
        SetStatus(L"Updated.");
    }

    void FillResults(const sqlite_manager::QueryResult& result) {
        ClearResults();
        const std::vector<std::string> titles = ColumnTitles(result);
        for (size_t c = 0; c < titles.size(); ++c) {
            m_results.InsertColumn(static_cast<int>(c),
                                   Widen(titles[c]).c_str(), LVCFMT_LEFT, 140);
        }
        for (size_t r = 0; r < result.rows.size(); ++r) {
            const std::vector<std::string> cells = RowText(result, r);
            m_results.InsertItem(static_cast<int>(r),
                                 cells.empty() ? L"" : Widen(cells[0]).c_str());
            for (size_t c = 1; c < cells.size(); ++c) {
                m_results.SetItemText(static_cast<int>(r), static_cast<int>(c),
                                      Widen(cells[c]).c_str());
            }
        }
    }

    void ClearResults() {
        m_results.DeleteAllItems();
        CHeaderCtrl header = m_results.GetHeader();
        for (int i = header.GetItemCount() - 1; i >= 0; --i) {
            m_results.DeleteColumn(i);
        }
    }

    // Re-run the current object's query (e.g. after commit/rollback).
    void ReloadCurrent() {
        if (current_) LoadObject(*current_);
    }

    static LPCWSTR KindLabel(ObjectKind kind) {
        switch (kind) {
            case ObjectKind::kTable:
                return L"table";
            case ObjectKind::kView:
                return L"view";
            case ObjectKind::kIndex:
                return L"index";
            case ObjectKind::kTrigger:
                return L"trigger";
        }
        return L"";
    }

    void SetStatus(LPCWSTR text) {
        if (m_hWndStatusBar != nullptr) ::SetWindowText(m_hWndStatusBar, text);
    }

    CSplitterWindow m_splitter;
    CListViewCtrl m_objects;
    CListViewCtrl m_results;
    std::optional<DatabaseSession> session_;
    std::optional<sqlite_manager::Transaction> txn_;
    std::vector<ObjectInfo> objects_;    // parallel to the left list rows
    std::optional<ObjectInfo> current_;  // the object shown on the right
    sqlite_manager::QueryResult current_result_;  // rows shown on the right
    std::vector<std::int64_t> rowids_;            // rowid per displayed row
    std::vector<std::string> current_columns_;    // displayed column names
    bool editable_ = false;  // the current object is an editable table
    bool dirty_ = false;     // open transaction has edits
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
