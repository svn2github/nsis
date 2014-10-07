/*
  Copyright (c) 2002 Robert Rainwater
  Contributors: Justin Frankel, Fritz Elfert, Amir Szekely, Sunil Kamath, Joost Verburg

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Unicode support by Jim Park -- 08/10/2007
*/
#ifndef MAKENSIS_H
#define MAKENSIS_H

#define _WIN32_IE 0x0400
#include "../../Source/Platform.h"
#include <windows.h>
#include <commctrl.h>
#include "utils.h"
#define _RICHEDIT_VER 0x0200
#include <richedit.h>
#undef _RICHEDIT_VER

// Defines
#define NSIS_URL     "http://nsis.sourceforge.net/"
#define NSIS_FOR     "http://forums.winamp.com/forumdisplay.php?forumid=65"
#define NSIS_UPDATE  "http://nsis.sourceforge.net/update.php?version="
#define NSIS_DL_URL  "http://nsis.sourceforge.net/download/"
#define USAGE        _T("Usage:\r\n\r\n - File | Load Script...\r\n - Drag the .nsi file into this window\r\n - Right click the .nsi file and choose \"Compile NSIS Script\"")
#define COPYRIGHT    _T("Copyright (C) 2002 Robert Rainwater")
#define CONTRIB      _T("Fritz Elfert, Justin Frankel, Amir Szekely, Sunil Kamath, Joost Verburg, Anders Kjersem")
#define DOCPATH      "http://nsis.sourceforge.net/Docs/"
#define LOCALDOCS    _T("\\NSIS.chm")
#define ERRBOXTITLE  0 //_T("Error")
#define NSISERROR    _T("Unable to intialize MakeNSIS.  Please verify that makensis.exe is in the same directory as makensisw.exe.")
#define DLGERROR     _T("Unable to intialize MakeNSISW.")
#define SYMBOLSERROR _T("Symbol cannot contain whitespace characters")
#define MULTIDROPERROR _T("Dropping more than one script at a time is not supported")
#define NSISUPDATEPROMPT _T("Running NSIS Update will close MakeNSISW.\nContinue?")
#define REGSEC       HKEY_CURRENT_USER
#define REGSECDEF    HKEY_LOCAL_MACHINE
#define REGKEY       _T("Software\\NSIS")
#define REGLOC       _T("MakeNSISWPlacement")
#define REGVERBOSITY _T("MakeNSISWVerbosity")
#define REGCOMPRESSOR _T("MakeNSISWCompressor")
#define REGSYMSUBKEY _T("Symbols")
#define REGMRUSUBKEY _T("MRU")
#define EXENAME      _T("makensis.exe")
#define MAX_STRING   256
#define TIMEOUT      100
#define MINWIDTH     350
#define MINHEIGHT    180
#define COMPRESSOR_MESSAGE _T("\n\nThe %s compressor created the smallest installer (%d bytes).")
#define RESTORED_COMPRESSOR_MESSAGE _T("\n\nThe %s compressor created the smallest installer (%d bytes).")
#define EXE_HEADER_COMPRESSOR_STAT _T("EXE header size:")
#define TOTAL_SIZE_COMPRESSOR_STAT _T("Total size:")
#define SYMBOL_SET_NAME_MAXLEN 40
#define LOAD_SYMBOL_SET_DLG_NAME _T("Load Symbol Definitions Set")
#define SAVE_SYMBOL_SET_DLG_NAME _T("Save Symbol Definitions Set")
#define LOAD_BUTTON_TEXT _T("Load")
#define SAVE_BUTTON_TEXT _T("Save")
#define LOAD_SYMBOL_SET_MESSAGE _T("Please select a name for the Symbol Definitions Set to load.")
#define SAVE_SYMBOL_SET_MESSAGE _T("Please enter or select a name for the Symbol Definitions Set to save.")

#define WM_MAKENSIS_PROCESSCOMPLETE (WM_USER+1001)
#define WM_MAKENSIS_LOADSYMBOLSET (WM_USER+1002)
#define WM_MAKENSIS_SAVESYMBOLSET (WM_USER+1003)

namespace MakensisAPI {
  extern const TCHAR* SigintEventNameFmt;
  extern const TCHAR* SigintEventNameLegacy;

  enum notify_e {
    NOTIFY_SCRIPT,
    NOTIFY_WARNING,
    NOTIFY_ERROR,
    NOTIFY_OUTPUT
  };
  enum sndmsg_e {
    QUERYHOST = WM_APP
  };
  enum QUERYHOST_e {
    QH_OUTPUTCHARSET = 1
  };
}

typedef enum {
  COMPRESSOR_NONE_SELECTED = -1,
  COMPRESSOR_SCRIPT = 0,
  COMPRESSOR_ZLIB,
  COMPRESSOR_ZLIB_SOLID,
  COMPRESSOR_BZIP2,
  COMPRESSOR_BZIP2_SOLID,
  COMPRESSOR_LZMA,
  COMPRESSOR_LZMA_SOLID,
  COMPRESSOR_BEST,
} NCOMPRESSOR;

#ifdef MAKENSISW_CPP
const TCHAR *compressor_names[] = {_T(""),
                            _T("zlib"),
                            _T("/SOLID zlib"),
                            _T("bzip2"),
                            _T("/SOLID bzip2"),
                            _T("lzma"),
                            _T("/SOLID lzma"),
                            _T("Best")};
const TCHAR *compressor_display_names[] = {_T("Defined in Script/Compiler Default"),
                            _T("ZLIB"),
                            _T("ZLIB (solid)"),
                            _T("BZIP2"),
                            _T("BZIP2 (solid)"),
                            _T("LZMA"),
                            _T("LZMA (solid)"),
                            _T("Best Compressor")};
WORD compressor_commands[] = {IDM_COMPRESSOR_SCRIPT,
                              IDM_ZLIB,
                              IDM_ZLIB_SOLID,
                              IDM_BZIP2,
                              IDM_BZIP2_SOLID,
                              IDM_LZMA,
                              IDM_LZMA_SOLID,
                              IDM_BEST};
#endif

#ifdef TOOLBAR_CPP
int compressor_bitmaps[] = {IDB_COMPRESSOR_SCRIPT,
                            IDB_COMPRESSOR_ZLIB,
                            IDB_COMPRESSOR_ZLIB,
                            IDB_COMPRESSOR_BZIP2,
                            IDB_COMPRESSOR_BZIP2,
                            IDB_COMPRESSOR_LZMA,
                            IDB_COMPRESSOR_LZMA,
                            IDB_COMPRESSOR_BEST};
int compressor_strings[] = {IDS_SCRIPT,
                            IDS_ZLIB,
                            IDS_ZLIB_SOLID,
                            IDS_BZIP2,
                            IDS_BZIP2_SOLID,
                            IDS_LZMA,
                            IDS_LZMA_SOLID,
                            IDS_BEST};
#endif

// Extern Variables

extern const TCHAR* NSISW_VERSION;

DWORD WINAPI   MakeNSISProc(LPVOID TreadParam);
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogResize(HWND hWnd, LPARAM /* unused*/);
INT_PTR CALLBACK AboutProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SymbolSetProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CompressorProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void           SetScript(const TCHAR *script, bool clearArgs = true);
void           CompileNSISScript();
TCHAR*         BuildSymbols();
void           SetCompressor(NCOMPRESSOR);
void           RestoreSymbols();
void           SaveSymbols();
void           DeleteSymbolSet(TCHAR *);
TCHAR**        LoadSymbolSet(TCHAR *);
void           SaveSymbolSet(TCHAR *, TCHAR **);
void           RestoreMRUList();
void           SaveMRUList();

typedef struct NSISScriptData {
  TCHAR *script;
  HGLOBAL script_cmd_args;
  TCHAR *compile_command;
  TCHAR *output_exe;
  TCHAR *input_script;
  TCHAR *branding;
  char  *brandingv;
  TCHAR **symbols;
  int retcode;
  bool userSelectCompressor;
  unsigned char verbosity;
  DWORD logLength;
  DWORD warnings;
  HINSTANCE hInstance;
  HWND hwnd;
  HMENU menu;
  HMENU fileSubmenu;
  HMENU editSubmenu;
  HMENU toolsSubmenu;
  HANDLE thread;
  HANDLE sigint_event;
  HANDLE sigint_event_legacy;
  HWND focused_hwnd;
  CHARRANGE textrange;
  NCOMPRESSOR default_compressor;
  NCOMPRESSOR compressor;
  LPCTSTR compressor_name;
  TCHAR compressor_stats[512];
  LPCTSTR best_compressor_name;
  // Added by Darren Owen (DrO) on 1/10/2003
  int recompile_test;
} NSCRIPTDATA;

extern NSCRIPTDATA g_sdata;

typedef struct ResizeData {
  RECT resizeRect;
  RECT griprect;
  int dx;
  int dy;
} NRESIZEDATA;

typedef struct FindReplaceDialog {
  FINDREPLACE fr;
  UINT uFindReplaceMsg;
  HWND hwndFind;
} NFINDREPLACE;

typedef struct ToolTipStruct {
  HWND tip;
  HWND tip_p;
  HHOOK hook;
} NTOOLTIP;

#endif
