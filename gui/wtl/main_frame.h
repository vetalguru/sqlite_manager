#ifndef SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
#define SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H

#include "gui/wtl/resource.h"
#include "gui/wtl/wtl.h"

namespace sqlite_manager_gui::wtl {

// The application's main window: a menu and a status bar now; a schema pane
// and result tabs are layered on in later changes. File > Open shows a file
// dialog and reports the chosen database in the status bar.
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
        if (dialog.DoModal(*this) == IDOK) {
            SetStatus(dialog.m_szFileName);
        }
        return 0;
    }

    LRESULT OnFileExit(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                       BOOL& /*handled*/) {
        PostMessage(WM_CLOSE);
        return 0;
    }

private:
    void SetStatus(LPCWSTR text) {
        if (m_hWndStatusBar != nullptr) ::SetWindowText(m_hWndStatusBar, text);
    }
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_MAIN_FRAME_H
