// Minimal WTL application: a frame window and a message loop. The real
// views (schema pane, result tabs, editing dialogs) are layered on in later
// changes; this is enough for the Windows CI job to compile and link the
// GUI. WTL extends ATL, which ships with MSVC, so this file builds only on
// Windows (guarded by SQLITE_MANAGER_BUILD_WTL_GUI).

// Opt into Common Controls v6 so the window uses the modern, themed controls.
#pragma comment(linker,                                                        \
                "\"/manifestdependency:type='win32' "                          \
                "name='Microsoft.Windows.Common-Controls' version='6.0.0.0' "  \
                "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' " \
                "language='*'\"")

// WTL/ATL headers must be included in this exact order (atlbase before
// atlapp; _Module declared between atlapp and the window headers), so keep
// clang-format from sorting them.
// clang-format off
#include <atlbase.h>
#include <atlapp.h>

CAppModule _Module;

#include <atlwin.h>
#include <atlframe.h>
#include <atlcrack.h>
#include <commctrl.h>
// clang-format on

namespace {

// The top-level window. For now an empty client area; subsequent changes add
// the menu, status bar, splitter, schema pane and result tabs.
class CMainFrame : public CFrameWindowImpl<CMainFrame> {
public:
    DECLARE_FRAME_WND_CLASS(nullptr, 0)

    BEGIN_MSG_MAP(CMainFrame)
    MSG_WM_DESTROY(OnDestroy)
    CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    void OnDestroy() { ::PostQuitMessage(0); }
};

int RunMessageLoop(int cmd_show) {
    CMessageLoop loop;
    _Module.AddMessageLoop(&loop);

    CMainFrame frame;
    if (frame.CreateEx() == nullptr) return 0;
    frame.SetWindowText(L"SQLite Manager");
    frame.ShowWindow(cmd_show);
    frame.UpdateWindow();

    const int result = loop.Run();
    _Module.RemoveMessageLoop();
    return result;
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show) {
    const INITCOMMONCONTROLSEX icc{sizeof(INITCOMMONCONTROLSEX),
                                   ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES |
                                       ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES |
                                       ICC_STANDARD_CLASSES};
    ::InitCommonControlsEx(&icc);

    const HRESULT hr = _Module.Init(nullptr, instance);
    ATLASSERT(SUCCEEDED(hr));
    static_cast<void>(hr);

    const int result = RunMessageLoop(cmd_show);
    _Module.Term();
    return result;
}
