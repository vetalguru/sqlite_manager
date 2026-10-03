#ifndef SQLITE_MANAGER_GUI_WTL_RESOURCE_H
#define SQLITE_MANAGER_GUI_WTL_RESOURCE_H

// Resource and command identifiers for the WTL GUI. Shared by the resource
// script (sqlite_manager.rc) and the C++ message maps.
#define IDR_MAINFRAME 128

#define ID_FILE_OPEN 40001
#define ID_FILE_EXIT 40002
#define ID_FILE_EXPORT 40003

#define ID_QUERY_RUN 40030

#define ID_TXN_BEGIN 40010
#define ID_TXN_COMMIT 40011
#define ID_TXN_ROLLBACK 40012

#define ID_EDIT_ADD_ROW 40020
#define ID_EDIT_DELETE_ROW 40021
#define ID_EDIT_ADD_COLUMN 40022
#define ID_EDIT_DROP_COLUMN 40023

// Child control identifiers (for WM_NOTIFY routing).
#define IDC_OBJECTS 1000
#define IDC_RESULTS 1001

// Edit-cell dialog.
#define IDD_EDIT_CELL 200
#define IDC_EDIT_VALUE 2001

// Add/Drop column dialogs.
#define IDD_ADD_COLUMN 201
#define IDD_DROP_COLUMN 202
#define IDC_COL_NAME 2010
#define IDC_COL_TYPE 2011
#define IDC_COL_LIST 2012

// Run-SQL dialog.
#define IDD_RUN_SQL 203
#define IDC_SQL_TEXT 2020

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#endif  // SQLITE_MANAGER_GUI_WTL_RESOURCE_H
