#ifndef SQLITE_MANAGER_GUI_WTL_RESOURCE_H
#define SQLITE_MANAGER_GUI_WTL_RESOURCE_H

// Resource and command identifiers for the WTL GUI. Shared by the resource
// script (sqlite_manager.rc) and the C++ message maps.
#define IDR_MAINFRAME 128

#define ID_FILE_OPEN 40001
#define ID_FILE_EXIT 40002

#define ID_TXN_BEGIN 40010
#define ID_TXN_COMMIT 40011
#define ID_TXN_ROLLBACK 40012

// Child control identifiers (for WM_NOTIFY routing).
#define IDC_OBJECTS 1000
#define IDC_RESULTS 1001

#endif  // SQLITE_MANAGER_GUI_WTL_RESOURCE_H
