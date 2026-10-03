#ifndef SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
#define SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gui/core/database_session.h"
#include "gui/core/schema_info.h"
#include "gui/wtl/column_dialogs.h"
#include "gui/wtl/edit_cell_dialog.h"
#include "gui/wtl/resource.h"
#include "gui/wtl/result_model.h"
#include "gui/wtl/result_pane.h"
#include "gui/wtl/run_sql_dialog.h"
#include "gui/wtl/text.h"
#include "gui/wtl/wtl.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result_writer.h"
#include "sqlite_manager/sql_util.h"
#include "sqlite_manager/transaction.h"

namespace sqlite_manager_gui::wtl {

// The application's main window: a menu, a status bar, and a splitter with
// the schema objects on the left and a notebook of result tabs on the right.
// Tables and views each open in their own tab (a CResultPane); the shared
// connection, transaction and commands act on the active tab.
class CMainFrame : public CFrameWindowImpl<CMainFrame> {
public:
    DECLARE_FRAME_WND_CLASS(L"SqliteManagerWtlFrame", IDR_MAINFRAME)

    // clang-format off
    BEGIN_MSG_MAP(CMainFrame)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_ID_HANDLER(ID_FILE_OPEN_DB, OnFileOpen)
        COMMAND_ID_HANDLER(ID_FILE_EXPORT, OnFileExport)
        COMMAND_ID_HANDLER(ID_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_QUERY_RUN, OnRunSql)
        COMMAND_ID_HANDLER(ID_TXN_BEGIN, OnTxnBegin)
        COMMAND_ID_HANDLER(ID_TXN_COMMIT, OnTxnCommit)
        COMMAND_ID_HANDLER(ID_TXN_ROLLBACK, OnTxnRollback)
        COMMAND_ID_HANDLER(ID_EDIT_ADD_ROW, OnAddRow)
        COMMAND_ID_HANDLER(ID_EDIT_DELETE_ROW, OnDeleteRow)
        COMMAND_ID_HANDLER(ID_EDIT_ADD_COLUMN, OnAddColumn)
        COMMAND_ID_HANDLER(ID_EDIT_DROP_COLUMN, OnDropColumn)
        NOTIFY_HANDLER(IDC_OBJECTS, LVN_ITEMACTIVATE, OnObjectActivate)
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

        m_view.Create(
            m_splitter, rcDefault, nullptr,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

        m_splitter.SetSplitterPanes(m_objects, m_view);
        m_splitter.SetSplitterPosPct(28);

        SetStatus(L"Open a database to begin.");
        return 0;
    }

    void OnClose() {
        if (ConfirmPending()) SetMsgHandled(FALSE);  // let the default close
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

    LRESULT OnFileExport(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                         BOOL& /*handled*/) {
        CResultPane* pane = ActivePane();
        if (pane == nullptr || pane->result().columns.empty()) {
            SetStatus(L"Nothing to export.");
            return 0;
        }
        CFileDialog dialog(/*bOpenFileDialog=*/FALSE, L"csv", L"result.csv",
                           OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                           L"CSV\0*.csv\0JSON\0*.json\0All files\0*.*\0",
                           *this);
        dialog.m_ofn.lpstrTitle = L"Export Result";
        if (dialog.DoModal(*this) == IDOK) {
            ExportResult(pane->result(), dialog.m_szFileName);
        }
        return 0;
    }

    LRESULT OnRunSql(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        if (!session_) {
            SetStatus(L"Open a database first.");
            return 0;
        }
        CRunSqlDialog dialog;
        if (dialog.DoModal(*this) != IDOK) return 0;
        const std::string sql = Narrow(dialog.sql);
        if (sql.empty()) return 0;
        auto result = session_->RunQuery(sql);
        if (!result.ok()) {
            SetStatus(Widen("Error: " + result.error().message).c_str());
            return 0;
        }
        CResultPane& pane = OpenOrFocus(QueryKey(), L"Query");
        pane.Show(result.value(), {}, false, std::string());
        ReportRowCount("Query", pane.result().rows.size());
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
        ReloadActive();
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
        ReloadActive();
        SetStatus(
            status.ok()
                ? L"Rolled back."
                : Widen("Rollback failed: " + status.error().message).c_str());
        return 0;
    }

    LRESULT OnAddRow(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        CResultPane* pane = ActivePane();
        if (!EnsureEditable(pane)) return 0;
        auto inserted = session_->InsertRow(pane->table(), {});
        if (!inserted.ok()) {
            SetStatus(
                Widen("Insert failed: " + inserted.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadPane(*pane);
        SetStatus(L"Row added - double-click a cell to edit it.");
        return 0;
    }

    LRESULT OnDeleteRow(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                        BOOL& /*handled*/) {
        CResultPane* pane = ActivePane();
        if (!EnsureEditable(pane)) return 0;
        const int selected = pane->SelectedRow();
        if (selected < 0 ||
            static_cast<size_t>(selected) >= pane->rowids().size()) {
            SetStatus(L"Select a row to delete.");
            return 0;
        }
        const sqlite_manager::Status status = session_->DeleteRow(
            pane->table(), pane->rowids()[static_cast<size_t>(selected)]);
        if (!status.ok()) {
            SetStatus(
                Widen("Delete failed: " + status.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadPane(*pane);
        SetStatus(L"Row deleted.");
        return 0;
    }

    LRESULT OnAddColumn(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                        BOOL& /*handled*/) {
        CResultPane* pane = ActivePane();
        if (!EnsureEditable(pane)) return 0;
        CAddColumnDialog dialog;
        if (dialog.DoModal(*this) != IDOK) return 0;
        const std::string name = Narrow(dialog.name);
        if (name.empty()) {
            SetStatus(L"A column name is required.");
            return 0;
        }
        const sqlite_manager::Status status =
            session_->AddColumn(pane->table(), name, Narrow(dialog.type));
        if (!status.ok()) {
            SetStatus(
                Widen("Add column failed: " + status.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadPane(*pane);
        SetStatus(Widen("Column \"" + name + "\" added.").c_str());
        return 0;
    }

    LRESULT OnDropColumn(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                         BOOL& /*handled*/) {
        CResultPane* pane = ActivePane();
        if (!EnsureEditable(pane)) return 0;
        const std::vector<std::string>& columns = pane->result().columns;
        if (columns.empty()) {
            SetStatus(L"This table has no columns to drop.");
            return 0;
        }
        std::vector<std::wstring> wide_columns;
        wide_columns.reserve(columns.size());
        for (const std::string& column : columns) {
            wide_columns.push_back(Widen(column));
        }
        CDropColumnDialog dialog(std::move(wide_columns));
        if (dialog.DoModal(*this) != IDOK || dialog.selected.empty()) return 0;
        const std::string name = Narrow(dialog.selected);
        const sqlite_manager::Status status =
            session_->DropColumn(pane->table(), name);
        if (!status.ok()) {
            SetStatus(
                Widen("Drop column failed: " + status.error().message).c_str());
            return 0;
        }
        dirty_ = true;
        ReloadPane(*pane);
        SetStatus(Widen("Column \"" + name + "\" dropped.").c_str());
        return 0;
    }

    // Double-click or Enter on a schema object opens or focuses its tab.
    LRESULT OnObjectActivate(int /*id*/, LPNMHDR header, BOOL& /*handled*/) {
        const LPNMITEMACTIVATE activate =
            reinterpret_cast<LPNMITEMACTIVATE>(header);
        const int index = activate->iItem;
        if (index >= 0 && static_cast<size_t>(index) < objects_.size()) {
            LoadObject(objects_[static_cast<size_t>(index)]);
        }
        return 0;
    }

    // Edit one cell of a pane's table (invoked by the pane on double-click).
    void EditCell(CResultPane& pane, int item, int sub) {
        if (!session_ || !pane.editable()) return;
        if (!txn_) {
            SetStatus(L"Press Begin to edit inside a transaction.");
            return;
        }
        if (item < 0 || static_cast<size_t>(item) >= pane.rowids().size()) {
            return;
        }
        const std::vector<std::string>& columns = pane.result().columns;
        if (sub < 0 || static_cast<size_t>(sub) >= columns.size()) return;
        const std::vector<sqlite_manager::Cell>& row =
            pane.result().rows[static_cast<size_t>(item)];
        const std::wstring value =
            Widen(static_cast<size_t>(sub) < row.size()
                      ? CellText(row[static_cast<size_t>(sub)])
                      : std::string());
        CEditCellDialog dialog(Widen(columns[static_cast<size_t>(sub)]), value);
        if (dialog.DoModal(*this) != IDOK) return;
        const sqlite_manager::Cell cell{sqlite_manager::ValueType::kText,
                                        Narrow(dialog.Value())};
        const sqlite_manager::Status status = session_->UpdateCell(
            pane.table(), pane.rowids()[static_cast<size_t>(item)],
            columns[static_cast<size_t>(sub)], cell);
        if (!status.ok()) {
            SetStatus(
                Widen("Update failed: " + status.error().message).c_str());
            return;
        }
        dirty_ = true;
        ReloadPane(pane);
        SetStatus(L"Updated.");
    }

private:
    static const char* QueryKey() { return "\x01query"; }

    bool EnsureEditable(CResultPane* pane) {
        if (!session_ || pane == nullptr || !pane->editable()) {
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
        if (!ConfirmPending()) return;  // keep the current unsaved work
        auto opened = DatabaseSession::Open(Narrow(wide_path));
        if (!opened.ok()) {
            MessageBox(Widen(opened.error().message).c_str(),
                       L"Cannot open database", MB_ICONERROR | MB_OK);
            return;
        }
        txn_.reset();
        dirty_ = false;
        m_view.RemoveAllPages();
        for (std::unique_ptr<CResultPane>& pane : panes_) {
            if (pane->m_hWnd != nullptr) pane->DestroyWindow();
        }
        panes_.clear();
        session_.emplace(std::move(opened).value());
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
        if (object.kind != ObjectKind::kTable &&
            object.kind != ObjectKind::kView) {
            SetStatus(L"Select a table or view to see its rows.");
            return;
        }
        CResultPane& pane = OpenOrFocus(object.name, Widen(object.name));
        if (object.kind == ObjectKind::kTable) {
            LoadEditableInto(pane, object.name);
        } else {
            LoadReadonlyInto(pane, object.name);
        }
    }

    // Finds the pane for `key`, activating it, or creates a new tab.
    CResultPane& OpenOrFocus(const std::string& key,
                             const std::wstring& title) {
        for (const std::unique_ptr<CResultPane>& pane : panes_) {
            if (pane->key() == key) {
                const int index = PageIndexOf(*pane);
                if (index >= 0) m_view.SetActivePage(index);
                return *pane;
            }
        }
        auto pane = std::make_unique<CResultPane>(key);
        pane->Create(m_view, rcDefault, nullptr,
                     WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        pane->set_edit_handler([this](CResultPane& p, int item, int sub) {
            EditCell(p, item, sub);
        });
        m_view.AddPage(pane->m_hWnd, title.c_str(), -1, pane.get());
        CResultPane& ref = *pane;
        panes_.push_back(std::move(pane));
        return ref;
    }

    int PageIndexOf(const CResultPane& pane) {
        for (int i = 0; i < m_view.GetPageCount(); ++i) {
            if (m_view.GetPageHWND(i) == pane.m_hWnd) return i;
        }
        return -1;
    }

    CResultPane* ActivePane() {
        const int page = m_view.GetActivePage();
        if (page < 0) return nullptr;
        return reinterpret_cast<CResultPane*>(m_view.GetPageData(page));
    }

    // A table loaded with its rowid so cells can be addressed for editing;
    // the rowid column is kept out of the display. Falls back to read-only
    // if the table has no rowid.
    void LoadEditableInto(CResultPane& pane, const std::string& name) {
        const std::string quoted = sqlite_manager::QuoteIdentifier(name);
        auto result = session_->RunQuery("SELECT rowid, * FROM " + quoted);
        if (!result.ok()) {
            LoadReadonlyInto(pane, name);
            return;
        }
        const sqlite_manager::QueryResult& full = result.value();
        sqlite_manager::QueryResult display;
        for (size_t i = 1; i < full.columns.size(); ++i) {
            display.columns.push_back(full.columns[i]);
        }
        std::vector<std::int64_t> rowids;
        for (const std::vector<sqlite_manager::Cell>& r : full.rows) {
            rowids.push_back(r.empty() ? 0
                                       : static_cast<std::int64_t>(std::strtoll(
                                             r[0].text.c_str(), nullptr, 10)));
            display.rows.emplace_back(r.begin() + 1, r.end());
        }
        pane.Show(std::move(display), std::move(rowids), true, name);
        ReportRowCount(name, pane.result().rows.size());
    }

    void LoadReadonlyInto(CResultPane& pane, const std::string& name) {
        const std::string quoted = sqlite_manager::QuoteIdentifier(name);
        auto result = session_->RunQuery("SELECT * FROM " + quoted);
        if (!result.ok()) {
            SetStatus(Widen("Error: " + result.error().message).c_str());
            return;
        }
        pane.Show(result.value(), {}, false, std::string());
        ReportRowCount(name, pane.result().rows.size());
    }

    void ReloadPane(CResultPane& pane) {
        if (!pane.table().empty()) LoadEditableInto(pane, pane.table());
    }

    void ReloadActive() {
        if (CResultPane* pane = ActivePane()) ReloadPane(*pane);
    }

    void ReportRowCount(const std::string& name, size_t rows) {
        SetStatus(Widen(name + " - " + std::to_string(rows) +
                        (rows == 1 ? " row" : " rows"))
                      .c_str());
    }

    void ExportResult(const sqlite_manager::QueryResult& result,
                      LPCWSTR wide_path) {
        std::ofstream out;
        out.open(wide_path, std::ios::binary);
        if (!out) {
            SetStatus(L"Cannot write the file.");
            return;
        }
        const std::wstring path(wide_path);
        bool json = false;
        if (path.size() >= 5) {
            std::wstring ext = path.substr(path.size() - 5);
            for (wchar_t& ch : ext)
                ch = static_cast<wchar_t>(std::towlower(ch));
            json = (ext == L".json");
        }
        if (json) {
            sqlite_manager::JsonWriter().Write(result, out);
        } else {
            sqlite_manager::CsvWriter().Write(result, out);
        }
        SetStatus(L"Exported.");
    }

    // When a dirty transaction is open, asks whether to save (commit),
    // discard (rollback), or cancel. Returns true if it is OK to proceed.
    bool ConfirmPending() {
        if (!txn_ || !dirty_) return true;
        const int choice =
            MessageBox(L"Save changes before continuing?", L"SQLite Manager",
                       MB_YESNOCANCEL | MB_ICONQUESTION);
        if (choice == IDCANCEL) return false;
        if (choice == IDYES) {
            const sqlite_manager::Status status = txn_->Commit();
            if (!status.ok()) {
                SetStatus(
                    Widen("Commit failed: " + status.error().message).c_str());
                return false;
            }
        } else {
            static_cast<void>(txn_->Rollback());  // discarding on purpose
        }
        txn_.reset();
        dirty_ = false;
        return true;
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
    CTabView m_view;
    std::optional<DatabaseSession> session_;
    std::optional<sqlite_manager::Transaction> txn_;
    std::vector<ObjectInfo> objects_;  // parallel to the left list rows
    std::vector<std::unique_ptr<CResultPane>> panes_;  // one per result tab
    bool dirty_ = false;  // open transaction has edits
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
