#ifndef SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
#define SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H

#include <optional>
#include <string>

#include "gui/core/database_session.h"
#include "gui/core/schema_info.h"
#include "gui/wtl/resource.h"
#include "gui/wtl/text.h"
#include "gui/wtl/wtl.h"

namespace sqlite_manager_gui::wtl {

// The application's main window: a File menu, a status bar, and a splitter
// with the schema objects on the left and (added next) result tabs on the
// right. It owns the open DatabaseSession and drives the views from the
// toolkit-free core.
class CMainFrame : public CFrameWindowImpl<CMainFrame> {
public:
    DECLARE_FRAME_WND_CLASS(L"SqliteManagerWtlFrame", IDR_MAINFRAME)

    // clang-format off
    BEGIN_MSG_MAP(CMainFrame)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
        COMMAND_ID_HANDLER(ID_FILE_EXIT, OnFileExit)
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
                         WS_EX_CLIENTEDGE);
        m_objects.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
        m_objects.InsertColumn(0, L"Name", LVCFMT_LEFT, 160);
        m_objects.InsertColumn(1, L"Type", LVCFMT_LEFT, 70);

        // Placeholder for the result tabs added in the next change.
        m_results.Create(m_splitter, rcDefault, nullptr,
                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                         WS_EX_CLIENTEDGE);

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

private:
    void OpenDatabase(LPCWSTR wide_path) {
        auto opened = DatabaseSession::Open(Narrow(wide_path));
        if (!opened.ok()) {
            MessageBox(Widen(opened.error().message).c_str(),
                       L"Cannot open database", MB_ICONERROR | MB_OK);
            return;
        }
        session_.emplace(std::move(opened).value());
        PopulateObjects();
    }

    void PopulateObjects() {
        m_objects.DeleteAllItems();
        if (!session_) return;
        auto objects = session_->ListObjects();
        if (!objects.ok()) {
            SetStatus(Widen("Cannot read schema: " + objects.error().message)
                          .c_str());
            return;
        }
        int row = 0;
        for (const ObjectInfo& object : objects.value()) {
            m_objects.InsertItem(row, Widen(object.name).c_str());
            m_objects.SetItemText(row, 1, KindLabel(object.kind));
            ++row;
        }
        SetStatus(
            Widen(std::to_string(objects.value().size()) + " schema objects")
                .c_str());
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
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
