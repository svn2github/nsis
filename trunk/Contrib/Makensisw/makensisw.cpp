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

  Unicode support by Jim Park -- 08/18/2007
*/
#define MAKENSISW_CPP

#include "makensisw.h"
#include <windowsx.h>
#include <shlwapi.h>
#include <stdio.h>
#include "resource.h"
#include "toolbar.h"
#include "update.h"

namespace MakensisAPI {
#ifdef _WIN64
  const TCHAR* SigintEventNameFmt = _T("makensis win32 sigint event %Iu");
#else
  const TCHAR* SigintEventNameFmt = _T("makensis win32 sigint event %u");
#endif
  const TCHAR* SigintEventNameLegacy = _T("makensis win32 signint event");
}

NSCRIPTDATA g_sdata;
NRESIZEDATA g_resize;
NFINDREPLACE g_find;
TCHAR g_findbuf[128];
extern NTOOLBAR g_toolbar;
int g_symbol_set_mode;

NSIS_ENTRYPOINT_SIMPLEGUI
int WINAPI _tWinMain(HINSTANCE hInst,HINSTANCE hOldInst,LPTSTR CmdLineParams,int ShowCmd) {
  MSG  msg;
  int status;
  HACCEL haccel;

  memset(&g_sdata,0,sizeof(NSCRIPTDATA));
  memset(&g_resize,0,sizeof(NRESIZEDATA));
  memset(&g_find,0,sizeof(NFINDREPLACE));
  g_sdata.hInstance = hInst;
  g_sdata.symbols = NULL;
  g_sdata.sigint_event_legacy = CreateEvent(NULL, FALSE, FALSE, MakensisAPI::SigintEventNameLegacy);
  g_sdata.verbosity = (unsigned char) ReadRegSettingDW(REGVERBOSITY, 4);
  if (g_sdata.verbosity > 4) g_sdata.verbosity = 4;
  RestoreSymbols();

  HINSTANCE hRichEditDLL = LoadLibrary(_T("RichEd20.dll"));

  if (!InitBranding()) {
    MessageBox(0,NSISERROR,ERRBOXTITLE,MB_ICONEXCLAMATION|MB_OK|MB_TASKMODAL);
    return 1;
  }
  ResetObjects();
  HWND hDialog = CreateDialog(g_sdata.hInstance,MAKEINTRESOURCE(DLG_MAIN),0,DialogProc);
  if (!hDialog) {
    MessageBox(0,DLGERROR,ERRBOXTITLE,MB_ICONEXCLAMATION|MB_OK|MB_TASKMODAL);
    return 1;
  }
  haccel = LoadAccelerators(g_sdata.hInstance, MAKEINTRESOURCE(IDK_ACCEL));
  while ((status=GetMessage(&msg,0,0,0))!=0) {
    if (status==-1) return -1;
    if (!IsDialogMessage(g_find.hwndFind, &msg)) {
      if (!TranslateAccelerator(hDialog,haccel,&msg)) {
        if (!IsDialogMessage(hDialog,&msg)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }
  }
  MemSafeFree(g_sdata.script);
  if (g_sdata.script_cmd_args) GlobalFree(g_sdata.script_cmd_args);
  if (g_sdata.sigint_event) CloseHandle(g_sdata.sigint_event);
  if (g_sdata.sigint_event_legacy) CloseHandle(g_sdata.sigint_event_legacy);
  FreeLibrary(hRichEditDLL);
  FinalizeUpdate();
  return (int) msg.wParam;
}

void SetScript(const TCHAR *script, bool clearArgs /*= true*/)
{
  MemSafeFree(g_sdata.script);

  if (clearArgs)
  {
    if (g_sdata.script_cmd_args)
    {
      GlobalFree(g_sdata.script_cmd_args);
    }

    // Pointing to a single char.  Maybe _T('\0')
    g_sdata.script_cmd_args = GlobalAlloc(GHND, sizeof(TCHAR));
  }

  g_sdata.script = (TCHAR*) MemAlloc((lstrlen(script) + 1)*sizeof(TCHAR));
  lstrcpy(g_sdata.script, script);
}

void AddScriptCmdArgs(const TCHAR *arg)
{
  g_sdata.script_cmd_args = GlobalReAlloc(g_sdata.script_cmd_args,
    GlobalSize(g_sdata.script_cmd_args) + (lstrlen(arg) + 2/* quotes */ + 1 /* space */)*sizeof(TCHAR),
    0);

  TCHAR *args = (TCHAR *) GlobalLock(g_sdata.script_cmd_args);

  lstrcat(args, _T(" \""));
  lstrcat(args, arg);
  lstrcat(args, _T("\""));

  GlobalUnlock(g_sdata.script_cmd_args);
}

void ProcessCommandLine()
{
  TCHAR **argv;
  int i, j;
  int argc = SetArgv((TCHAR *)GetCommandLine(), &argv);
  if (argc > 1) {
    for (i = 1; i < argc; i++)
    {
      if (!StrCmpNI(argv[i], _T("/XSetCompressor "), lstrlen(_T("/XSetCompressor "))))
      {
        TCHAR *p = argv[i] + lstrlen(_T("/XSetCompressor "));
        if(!StrCmpNI(p,_T("/FINAL "), lstrlen(_T("/FINAL "))))
        {
          p += lstrlen(_T("/FINAL "));
        }

        while (*p == _T(' ')) p++;

        for (j = (int) COMPRESSOR_SCRIPT + 1; j < (int) COMPRESSOR_BEST; j++)
        {
          if (!lstrcmpi(p, compressor_names[j]))
          {
            SetCompressor((NCOMPRESSOR) j);
          }
        }
      }
      else if (!lstrcmpi(argv[i], _T("/ChooseCompressor")))
      {
        g_sdata.userSelectCompressor = TRUE;
      }
      else if (argv[i][0] == _T('-') || argv[i][0] == _T('/'))
      {
        AddScriptCmdArgs(argv[i]);
      }
      else
      {
        SetScript(argv[i], false);
        PushMRUFile(g_sdata.script);
        break;
      }
    }
  }
  MemSafeFree(argv);
}

DWORD CALLBACK SaveFileStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
  HANDLE hFile = (HANDLE) ((DWORD_PTR*)dwCookie)[0];
  DWORD cbio;
#ifdef UNICODE
  if (!((DWORD_PTR*)dwCookie)[1])
  {
    if (!WriteUTF16LEBOM(hFile)) return -1;
    ((DWORD_PTR*)dwCookie)[1] = TRUE;
  }
#endif
  BOOL wop = WriteFile(hFile, pbBuff, cb, &cbio, 0);
  return (*pcb = (LONG) cbio, !wop);
}


INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_INITDIALOG:
    {
      g_sdata.hwnd=hwndDlg;
      HICON hIcon = LoadIcon(g_sdata.hInstance,MAKEINTRESOURCE(IDI_ICON));
      SetClassLongPtr(hwndDlg,GCLP_HICON,(LONG_PTR)hIcon);
      // Altered by Darren Owen (DrO) on 29/9/2003
      // Added in receiving of mouse and key events from the richedit control
      SendMessage(GetDlgItem(hwndDlg,IDC_LOGWIN),EM_SETEVENTMASK,(WPARAM)NULL,ENM_SELCHANGE|ENM_MOUSEEVENTS|ENM_KEYEVENTS);
      DragAcceptFiles(g_sdata.hwnd,FALSE);
      g_sdata.menu = GetMenu(g_sdata.hwnd);
      g_sdata.fileSubmenu = FindSubMenu(g_sdata.menu, IDM_FILE);
      g_sdata.editSubmenu = FindSubMenu(g_sdata.menu, IDM_EDIT);
      g_sdata.toolsSubmenu = FindSubMenu(g_sdata.menu, IDM_TOOLS);
      RestoreMRUList();
      CreateToolBar();
      InitTooltips(g_sdata.hwnd);
      SetDlgItemText(g_sdata.hwnd,IDC_VERSION,g_sdata.branding);
      HFONT hFont = CreateFont(14,FW_NORMAL,FIXED_PITCH|FF_DONTCARE,_T("Courier New"));
      SendDlgItemMessage(hwndDlg,IDC_LOGWIN,WM_SETFONT,(WPARAM)hFont,0);
      RestoreWindowPos(g_sdata.hwnd);
      RestoreCompressor();
      SetScript(_T(""));
      g_sdata.compressor = COMPRESSOR_NONE_SELECTED;
      g_sdata.userSelectCompressor = FALSE;

      ProcessCommandLine();

      if(g_sdata.compressor == COMPRESSOR_NONE_SELECTED) {
        SetCompressor(g_sdata.default_compressor);
      }

      if(g_sdata.userSelectCompressor) {
        if (DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_COMPRESSOR),g_sdata.hwnd,(DLGPROC)CompressorProc)) {
          EnableItems(g_sdata.hwnd);
          return TRUE;
        }
      }

      CompileNSISScript();
      return TRUE;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      GetGripperPos(hwndDlg, g_resize.griprect);
      HDC hdc = BeginPaint(hwndDlg, &ps);
      DrawFrameControl(hdc, &g_resize.griprect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
      EndPaint(hwndDlg, &ps);
      return TRUE;
    }
    case WM_DESTROY:
    {
      DragAcceptFiles(g_sdata.hwnd,FALSE);
      SaveSymbols();
      SaveCompressor();
      SaveMRUList();
      SaveWindowPos(g_sdata.hwnd);
      ImageList_Destroy(g_toolbar.imagelist);
      ImageList_Destroy(g_toolbar.imagelistd);
      ImageList_Destroy(g_toolbar.imagelisth);
      DestroyTooltips();
      PostQuitMessage(0);
      return TRUE;
    }
    case WM_CLOSE:
    {
      if (!g_sdata.thread) {
        DestroyWindow(hwndDlg);
      }
      return TRUE;
    }
    case WM_DROPFILES: {
      int num;
      TCHAR szTmp[MAX_PATH];
      num = DragQueryFile((HDROP)wParam,(UINT)-1,NULL,0);
      if (num==1) {
        DragQueryFile((HDROP)wParam,0,szTmp,MAX_PATH);
        if (lstrlen(szTmp)>0) {
          SetScript(szTmp);
          PushMRUFile(g_sdata.script);
          ResetObjects();
          CompileNSISScript();
        }
      } else {
        MessageBox(hwndDlg,MULTIDROPERROR,ERRBOXTITLE,MB_OK|MB_ICONSTOP);
      }
      DragFinish((HDROP)wParam);
      break;
    }
    case WM_GETMINMAXINFO:
    {
      ((MINMAXINFO*)lParam)->ptMinTrackSize.x=MINWIDTH;
      ((MINMAXINFO*)lParam)->ptMinTrackSize.y=MINHEIGHT;
    }
    case WM_ENTERSIZEMOVE:
    {
      GetClientRect(g_sdata.hwnd, &g_resize.resizeRect);
      return TRUE;
    }
    case WM_SIZE:
    {
      if ((wParam == SIZE_MAXHIDE)||(wParam == SIZE_MAXSHOW)) return TRUE;
      RECT rSize;
      if (hwndDlg == g_sdata.hwnd) {
        GetClientRect(g_sdata.hwnd, &rSize);
        if (((rSize.right==0)&&(rSize.bottom==0))||((g_resize.resizeRect.right==0)&&(g_resize.resizeRect.bottom==0)))  return TRUE;
        g_resize.dx = rSize.right - g_resize.resizeRect.right;
        g_resize.dy = rSize.bottom - g_resize.resizeRect.bottom;
        EnumChildWindows(g_sdata.hwnd, DialogResize, (LPARAM)0);
        g_resize.resizeRect = rSize;
      }
      return TRUE;
    }
    case WM_SIZING:
    {
      InvalidateRect(hwndDlg, &g_resize.griprect, TRUE);
      GetGripperPos(hwndDlg, g_resize.griprect);
      return TRUE;
    }
    case WM_NCHITTEST:
    {
      RECT r = g_resize.griprect;
      MapWindowPoints(hwndDlg, 0, (POINT*)&r, 2);
      POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      if (PtInRect(&r, pt))
      {
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, HTBOTTOMRIGHT);
        return TRUE;
      }
      return FALSE;
    }
    case WM_MAKENSIS_PROCESSCOMPLETE:
    {
      if (g_sdata.thread) {
        CloseHandle(g_sdata.thread);
        g_sdata.thread=0;
      }
      if(g_sdata.compressor == COMPRESSOR_BEST) {
        if (g_sdata.retcode==0 && FileExists(g_sdata.output_exe)) {
          TCHAR temp_file_name[1024]; // BUGBUG: Hardcoded buffer size
          wsprintf(temp_file_name,_T("%s_makensisw_temp"),g_sdata.output_exe);
          if(!lstrcmpi(g_sdata.compressor_name,compressor_names[(int)COMPRESSOR_SCRIPT+1])) {
            SetCompressorStats();
            CopyFile(g_sdata.output_exe,temp_file_name,false);
            g_sdata.best_compressor_name = g_sdata.compressor_name;
            g_sdata.compressor_name = compressor_names[(int)COMPRESSOR_SCRIPT+2];
            ResetObjects();

            CompileNSISScript();
            return TRUE;
          }
          else {
            int this_compressor=0;
            int i;
            HANDLE hPrev, hThis;
            DWORD prevSize=0, thisSize=0;


            for(i=(int)COMPRESSOR_SCRIPT+2; i<(int)COMPRESSOR_BEST; i++) {
              if(!lstrcmpi(g_sdata.compressor_name,compressor_names[i])) {
                this_compressor = i;
                break;
              }
            }

            if(FileExists(temp_file_name)) {
              hPrev = CreateFile(temp_file_name,GENERIC_READ, FILE_SHARE_READ,
                                 NULL, OPEN_EXISTING, (DWORD)NULL, NULL);
              if(hPrev != INVALID_HANDLE_VALUE) {
                prevSize = GetFileSize(hPrev, 0);
                CloseHandle(hPrev);

                if(prevSize != INVALID_FILE_SIZE) {
                  hThis = CreateFile(g_sdata.output_exe,GENERIC_READ, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, (DWORD)NULL, NULL);
                  if(hThis != INVALID_HANDLE_VALUE) {
                    thisSize = GetFileSize(hThis, 0);
                    CloseHandle(hThis);

                    if(thisSize != INVALID_FILE_SIZE) {
                      if(prevSize > thisSize) {
                        CopyFile(g_sdata.output_exe,temp_file_name,false);
                        SetCompressorStats();
                        g_sdata.best_compressor_name = g_sdata.compressor_name;
                      }
                    }
                  }
                }
              }
            }

            if(this_compressor == ((int)COMPRESSOR_BEST - 1)) {
              TCHAR buf[1024];

              g_sdata.compressor_name = compressor_names[(int)COMPRESSOR_SCRIPT+1];

              if(!lstrcmpi(g_sdata.best_compressor_name,compressor_names[this_compressor])) {
                wsprintf(buf,COMPRESSOR_MESSAGE,g_sdata.best_compressor_name,thisSize);
                LogMessage(g_sdata.hwnd,buf);
              }
              else {
                CopyFile(temp_file_name,g_sdata.output_exe,false);
                wsprintf(buf,RESTORED_COMPRESSOR_MESSAGE,g_sdata.best_compressor_name,prevSize);
                LogMessage(g_sdata.hwnd,buf);
                LogMessage(g_sdata.hwnd, g_sdata.compressor_stats);
              }
              DeleteFile(temp_file_name);
              g_sdata.compressor_stats[0] = _T('\0');
            }
            else {
              g_sdata.compressor_name = compressor_names[this_compressor+1];
              ResetObjects();

              CompileNSISScript();
              return TRUE;
            }
          }
        }
      }
      EnableItems(g_sdata.hwnd);
      if (!g_sdata.retcode) {
        MessageBeep(MB_ICONASTERISK);
        if (g_sdata.warnings)
          SetTitle(g_sdata.hwnd,_T("Finished with Warnings"));
        else
          SetTitle(g_sdata.hwnd,_T("Finished Sucessfully"));
        // Added by Darren Owen (DrO) on 1/10/2003
        if(g_sdata.recompile_test)
          PostMessage(g_sdata.hwnd, WM_COMMAND, LOWORD(IDC_TEST), 0);
      }
      else {
        MessageBeep(MB_ICONEXCLAMATION);
        SetTitle(g_sdata.hwnd,_T("Compile Error: See Log for Details"));
      }

      // Added by Darren Owen (DrO) on 1/10/2003
      // ensures the recompile and run state is reset after use
      g_sdata.recompile_test = 0;
      DragAcceptFiles(g_sdata.hwnd,TRUE);
      return TRUE;
    }
    case MakensisAPI::QUERYHOST: {
      if (MakensisAPI::QH_OUTPUTCHARSET) {
        const UINT reqcp = 1200; // We want UTF-16LE
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)(1+reqcp));
        return TRUE;
      }
      return FALSE;
    }
    case WM_NOTIFY:
      switch (((NMHDR*)lParam)->code ) {
        case EN_SELCHANGE:
          SendDlgItemMessage(hwndDlg,IDC_LOGWIN, EM_EXGETSEL, 0, (LPARAM) &g_sdata.textrange);
          {
            BOOL enabled = (g_sdata.textrange.cpMax-g_sdata.textrange.cpMin<=0?FALSE:TRUE);
            EnableMenuItem(g_sdata.menu,IDM_COPYSELECTED,(enabled?MF_ENABLED:MF_GRAYED));
            EnableToolBarButton(IDM_COPY,enabled);
          }
        // Altered by Darren Owen (DrO) on 6/10/2003
        // Allows the detection of the right-click menu when running on OSes below Windows 2000
        // and will then simulate the effective WM_CONTEXTMENU message that would be received
        // note: removed the WM_CONTEXTMENU handling to prevent a duplicate menu appearing on
        // Windows 2000 and higher
        case EN_MSGFILTER:
          #define lpnmMsg ((MSGFILTER*)lParam)
          if(WM_RBUTTONUP == lpnmMsg->msg || (WM_KEYUP == lpnmMsg->msg && lpnmMsg->wParam == VK_APPS)){
          POINT pt;
          HWND edit = GetDlgItem(g_sdata.hwnd,IDC_LOGWIN);
          RECT r;
            GetCursorPos(&pt);

            // Added and altered by Darren Owen (DrO) on 29/9/2003
            // Will place the right-click menu in the top left corner of the window
            // if the application key is pressed and the mouse is not in the window
            // from here...
            ScreenToClient(edit, &pt);
            GetClientRect(edit, &r);
            if(!PtInRect(&r, pt))
              pt.x = pt.y = 0;
            MapWindowPoints(edit, HWND_DESKTOP, &pt, 1);
            TrackPopupMenu(g_sdata.editSubmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_sdata.hwnd, 0);
          }
        case TBN_DROPDOWN:
        {
          LPNMTOOLBAR pToolBar = (LPNMTOOLBAR) lParam;
          if(pToolBar->hdr.hwndFrom == g_toolbar.hwnd && pToolBar->iItem == IDM_COMPRESSOR) {
            ShowToolbarDropdownMenu();
            return TBDDRET_DEFAULT;
          }
          else {
            return TBDDRET_NODEFAULT;
          }
        }
      }
      return TRUE;
    case WM_COPYDATA:
    {
      PCOPYDATASTRUCT cds = PCOPYDATASTRUCT(lParam);
      switch (cds->dwData) {
        case MakensisAPI::NOTIFY_SCRIPT:
          MemSafeFree(g_sdata.input_script);
          g_sdata.input_script = (TCHAR*) MemAlloc(cds->cbData * sizeof(TCHAR));
          lstrcpy(g_sdata.input_script, (TCHAR *)cds->lpData);
          break;
        case MakensisAPI::NOTIFY_WARNING:
          g_sdata.warnings++;
          break;
        case MakensisAPI::NOTIFY_ERROR:
          break;
        case MakensisAPI::NOTIFY_OUTPUT:
          MemSafeFree(g_sdata.output_exe);
          g_sdata.output_exe = (TCHAR*) MemAlloc(cds->cbData * sizeof(TCHAR));
          lstrcpy(g_sdata.output_exe, (TCHAR *)cds->lpData);
          break;
      }
      return TRUE;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDM_BROWSESCR: {
          if (g_sdata.input_script) {
            TCHAR str[MAX_PATH],*str2;
            lstrcpy(str,g_sdata.input_script);
            str2=_tcsrchr(str,_T('\\'));
            if(str2!=NULL) *(str2+1)=0;
            ShellExecute(g_sdata.hwnd,_T("open"),str,NULL,NULL,SW_SHOWNORMAL);
          }
          return TRUE;
        }
        case IDM_ABOUT:
        {
          return DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_ABOUT),hwndDlg,(DLGPROC)AboutProc);
        }
        case IDM_NSISHOME:
        {
          ShellExecuteA(g_sdata.hwnd,"open",NSIS_URL,NULL,NULL,SW_SHOWNORMAL);
          return TRUE;
        }
        case IDM_FORUM:
        {
          ShellExecuteA(g_sdata.hwnd,"open",NSIS_FOR,NULL,NULL,SW_SHOWNORMAL);
          return TRUE;
        }
        case IDM_NSISUPDATE:
        {
          Update();
          return TRUE;
        }
        case IDM_SELECTALL:
        {
          SendDlgItemMessage(g_sdata.hwnd, IDC_LOGWIN, EM_SETSEL, 0, -1);
          return TRUE;
        }
        case IDM_DOCS:
        {
          ShowDocs();
          return TRUE;
        }
        case IDM_LOADSCRIPT:
        {
          if (!g_sdata.thread) {
            OPENFILENAME l={sizeof(l),};
            TCHAR buf[MAX_PATH];
            l.hwndOwner = hwndDlg;
            l.lpstrFilter = _T("NSIS Script (*.nsi)\0*.nsi\0All Files (*.*)\0*.*\0");
            l.lpstrFile = buf;
            l.nMaxFile = MAX_STRING-1;
            l.lpstrTitle = _T("Load Script");
            l.lpstrDefExt = _T("log");
            l.lpstrFileTitle = NULL;
            l.lpstrInitialDir = NULL;
            l.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
            buf[0] = _T('\0');
            if (GetOpenFileName(&l)) {
              SetScript(buf);
              PushMRUFile(g_sdata.script);
              ResetObjects();
              CompileNSISScript();
            }
          }
          return TRUE;
        }
        case IDM_MRU_FILE:
        case IDM_MRU_FILE+1:
        case IDM_MRU_FILE+2:
        case IDM_MRU_FILE+3:
        case IDM_MRU_FILE+4:
          LoadMRUFile(LOWORD(wParam)-IDM_MRU_FILE);
          return TRUE;
        case IDM_CLEAR_MRU_LIST:
          ClearMRUList();
          return TRUE;
        case IDM_COMPRESSOR:
        {
          SetCompressor((NCOMPRESSOR)(g_sdata.compressor+1));
          return TRUE;
        }
        case IDM_CLEARLOG:
        {
          if (!g_sdata.thread) {
            ClearLog(g_sdata.hwnd);
          }
          return TRUE;
        }
        case IDM_RECOMPILE:
        {
          CompileNSISScript();
          return TRUE;
        }
        // Added by Darren Owen (DrO) on 1/10/2003
        case IDM_RECOMPILE_TEST:
        {
          g_sdata.recompile_test = 1;
          CompileNSISScript();
          return TRUE;
        }
        case IDM_SETTINGS:
        {
          DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_SETTINGS),g_sdata.hwnd,(DLGPROC)SettingsProc);
          return TRUE;
        }
        case IDM_TEST:
        case IDC_TEST:
        {
          if (g_sdata.output_exe) {
            ShellExecute(g_sdata.hwnd,_T("open"),g_sdata.output_exe,NULL,NULL,SW_SHOWNORMAL);
          }
          return TRUE;
        }
        case IDM_EDITSCRIPT:
        {
          if (g_sdata.input_script) {
            LPCTSTR verb = _T("open"); // BUGBUG: Should not force the open verb?
            HINSTANCE hi = ShellExecute(g_sdata.hwnd,verb,g_sdata.input_script,NULL,NULL,SW_SHOWNORMAL);
            if ((UINT_PTR)hi <= 32) {
              TCHAR path[MAX_PATH];
              if (GetWindowsDirectory(path,sizeof(path))) {
                lstrcat(path,_T("\\notepad.exe"));
                ShellExecute(g_sdata.hwnd,verb,path,g_sdata.input_script,NULL,SW_SHOWNORMAL);
              }
            }
          }
          return TRUE;
        }
        case IDCANCEL:
        case IDM_EXIT:
        {
          if (!g_sdata.thread) {
            DestroyWindow(g_sdata.hwnd);
          }
          return TRUE;
        }
        case IDM_CANCEL:
        {
          SetEvent(g_sdata.sigint_event);
          SetEvent(g_sdata.sigint_event_legacy);
          return TRUE;
        }
        case IDM_COPY:
        {
          CopyToClipboard(g_sdata.hwnd);
          return TRUE;
        }
        case IDM_COPYSELECTED:
        {
          SendDlgItemMessage(g_sdata.hwnd,IDC_LOGWIN, WM_COPY, 0, 0);
          return TRUE;
        }
        case IDM_SAVE:
        {
          OPENFILENAME l={sizeof(l),};
          TCHAR buf[MAX_STRING];
          l.hwndOwner = hwndDlg;
          l.lpstrFilter = _T("Log Files (*.log)\0*.log\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
          l.lpstrFile = buf;
          l.nMaxFile = MAX_STRING-1;
          l.lpstrTitle = _T("Save Output");
          l.lpstrDefExt = _T("log");
          l.lpstrInitialDir = NULL;
          l.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_PATHMUSTEXIST;
          lstrcpy(buf,_T("output"));
          if (GetSaveFileName(&l)) {
            HANDLE hFile = CreateFile(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
            if (INVALID_HANDLE_VALUE != hFile) { // BUGBUG:TODO: Error message for bad hFile or failed EM_STREAMOUT?
              WPARAM opts = sizeof(TCHAR) > 1 ? (SF_TEXT|SF_UNICODE) : (SF_TEXT);
              DWORD_PTR cookie[2] = { (DWORD_PTR)hFile, FALSE };
              EDITSTREAM es = { (DWORD_PTR)&cookie, 0, SaveFileStreamCallback };
              SendMessage(GetDlgItem(g_sdata.hwnd, IDC_LOGWIN), EM_STREAMOUT, opts, (LPARAM)&es);
              CloseHandle(hFile);
            }
          }
          return TRUE;
        }
        case IDM_FIND:
        {
          if (!g_find.uFindReplaceMsg) g_find.uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
          memset(&g_find.fr, 0, sizeof(FINDREPLACE));
          g_find.fr.lStructSize = sizeof(FINDREPLACE);
          g_find.fr.hwndOwner = hwndDlg;
          g_find.fr.Flags = FR_NOUPDOWN;
          g_find.fr.lpstrFindWhat = g_findbuf;
          g_find.fr.wFindWhatLen = COUNTOF(g_findbuf);
          g_find.hwndFind = FindText(&g_find.fr);
          return TRUE;
        }
        default:
          {
            int i;
            DWORD command = LOWORD(wParam);
            for(i=(int)COMPRESSOR_SCRIPT; i<=(int)COMPRESSOR_BEST; i++) {
              if(command == compressor_commands[i]) {
                SetCompressor((NCOMPRESSOR)i);
                return TRUE;
              }
            }
          }
      }
    }
  }
  if (g_find.uFindReplaceMsg && msg == g_find.uFindReplaceMsg) {
    LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;
    if (lpfr->Flags & FR_FINDNEXT) {
      WPARAM flags = FR_DOWN;
      if (lpfr->Flags & FR_MATCHCASE) flags |= FR_MATCHCASE;
      if (lpfr->Flags & FR_WHOLEWORD) flags |= FR_WHOLEWORD;
      FINDTEXTEX ft;
      SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_EXGETSEL, 0, (LPARAM)&ft.chrg);
      ft.chrg.cpMin = (ft.chrg.cpMax == ft.chrg.cpMin) ? 0 : ft.chrg.cpMax;
      ft.chrg.cpMax = (LONG) SendDlgItemMessage(hwndDlg, IDC_LOGWIN, WM_GETTEXTLENGTH, 0, 0);
      ft.lpstrText = lpfr->lpstrFindWhat;
      ft.chrg.cpMin = (LONG) SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_FINDTEXTEX, flags, (LPARAM)&ft);
      if (ft.chrg.cpMin != -1)
        SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
      else
        MessageBeep(MB_ICONASTERISK);
    }
    if (lpfr->Flags & FR_DIALOGTERM) g_find.hwndFind = 0;
    return TRUE;
  }
  return 0;
}

DWORD WINAPI MakeNSISProc(LPVOID TreadParam) {
  TCHAR eventnamebuf[100];
  wsprintf(eventnamebuf, MakensisAPI::SigintEventNameFmt, g_sdata.hwnd);
  if (g_sdata.sigint_event) CloseHandle(g_sdata.sigint_event);
  g_sdata.sigint_event = CreateEvent(NULL, FALSE, FALSE, eventnamebuf);
  if (!g_sdata.sigint_event) {
    ErrorMessage(g_sdata.hwnd, _T("There was an error creating the abort event."));
    PostMessage(g_sdata.hwnd, WM_MAKENSIS_PROCESSCOMPLETE, 0, 0);
    return 1;
  }

  STARTUPINFO si;
  HANDLE newstdout,read_stdout;
  
  if (!InitSpawn(si, read_stdout, newstdout)) {
    ErrorMessage(g_sdata.hwnd, _T("There was an error creating the pipe."));
    PostMessage(g_sdata.hwnd, WM_MAKENSIS_PROCESSCOMPLETE, 0, 0);
    return 1;
  }
  PROCESS_INFORMATION pi;
  if (!CreateProcess(0, g_sdata.compile_command, 0, 0, TRUE, CREATE_NEW_CONSOLE, 0, 0, &si, &pi)) {
    TCHAR buf[MAX_STRING]; // BUGBUG: TODO: Too small?
    wsprintf(buf,_T("Could not execute:\r\n %s."), g_sdata.compile_command);
    ErrorMessage(g_sdata.hwnd, buf);
    FreeSpawn(0, read_stdout, newstdout);
    PostMessage(g_sdata.hwnd, WM_MAKENSIS_PROCESSCOMPLETE, 0, 0);
    return 1;
  }
  CloseHandle(newstdout); // close this handle (duplicated in subprocess) now so we get ERROR_BROKEN_PIPE

  char iob[(1024 & ~1) + sizeof(WCHAR)];
  WCHAR *p = (WCHAR*) iob, wcl = 0;
  DWORD cbiob = sizeof(iob) - sizeof(WCHAR), cb = 0, cbofs = 0, cch, cbio;
  for(;;)
  {
    BOOL rok = ReadFile(read_stdout, iob+cbofs, cbiob-cbofs, &cbio, NULL);
    cb += cbio, cch = cb / sizeof(WCHAR);
    if (!cch)
    {
      if (!rok) break; // TODO: if cb is non-zero we should report a incomplete read error?
      cbofs += cbio; // We only have 1 byte, need to read more to get a complete WCHAR
      continue;
    }
    char oddbyte = (char)(cb % 2), incompsurr;
    cbofs = 0;
    if ((incompsurr = IS_HIGH_SURROGATE(p[cch-1])))
      wcl = p[--cch], cbofs = sizeof(WCHAR); // Store leading surrogate part and complete it later
    if (oddbyte)
      oddbyte = iob[cb-1], ++cbofs;
logappendfinal:
    p[cch] = L'\0';
    LogMessage(g_sdata.hwnd, p);
    p[0] = wcl, iob[cbofs - !!oddbyte] = oddbyte, cb = 0;
    if (!rok) // No more data can be read
    {
      if (cbofs) // Unable to complete the surrogate pair or odd byte
      {
        p[0] = 0xfffd, cch = 1, cbofs = 0; 
        goto logappendfinal;
      }
      break;
    }
  }
  FreeSpawn(&pi, read_stdout, 0);
  g_sdata.retcode = pi.dwProcessId;
  PostMessage(g_sdata.hwnd, WM_MAKENSIS_PROCESSCOMPLETE, 0, 0);
  return 0;
}

BOOL CALLBACK DialogResize(HWND hWnd, LPARAM /* unused */)
{
  RECT r;
  GetWindowRect(hWnd, &r);
  ScreenToClient(g_sdata.hwnd, (LPPOINT)&r);
  ScreenToClient(g_sdata.hwnd, ((LPPOINT)&r)+1);
  if(hWnd != g_toolbar.hwnd) {
    switch (GetDlgCtrlID(hWnd)) {
      case IDC_LOGWIN:
        SetWindowPos(hWnd, 0, r.left, r.top,r.right - r.left + g_resize.dx, r.bottom - r.top + g_resize.dy, SWP_NOZORDER|SWP_NOMOVE);
        break;
      case IDC_TEST:
      case IDCANCEL:
        SetWindowPos(hWnd, 0, r.left + g_resize.dx, r.top + g_resize.dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
        break;
      default:
        SetWindowPos(hWnd, 0, r.left, r.top + g_resize.dy, r.right - r.left + g_resize.dx, r.bottom - r.top, SWP_NOZORDER);
        break;
    }
  }
  else {
      RECT r2;
      GetWindowRect(g_toolbar.hwnd, &r2);
      SetWindowPos(hWnd, 0, 0, 0, r.right - r.left + g_resize.dx, r2.bottom-r2.top, SWP_NOMOVE|SWP_NOZORDER);
  }
  RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
  return TRUE;
}

INT_PTR CALLBACK AboutProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      HFONT fontnorm = CreateFont(13, FW_NORMAL, FIXED_PITCH|FF_DONTCARE, _T("Tahoma")),
            fontbold = CreateFont(13, FW_BOLD, FIXED_PITCH|FF_DONTCARE, _T("Tahoma"));
      if (!fontbold) {
        fontnorm = CreateFont(12, FW_NORMAL, FIXED_PITCH|FF_DONTCARE, _T("MS Shell Dlg"));
        fontbold = CreateFont(12, FW_BOLD, FIXED_PITCH|FF_DONTCARE, _T("MS Shell Dlg"));
      }
      SendDlgItemMessage(hwndDlg, IDC_ABOUTVERSION, WM_SETFONT, (WPARAM)fontbold, FALSE);
      SendDlgItemMessage(hwndDlg, IDC_ABOUTCOPY, WM_SETFONT, (WPARAM)fontnorm, FALSE);
      SendDlgItemMessage(hwndDlg, IDC_ABOUTPORTIONS, WM_SETFONT, (WPARAM)fontnorm, FALSE);
      SendDlgItemMessage(hwndDlg, IDC_NSISVER, WM_SETFONT, (WPARAM)fontnorm, FALSE);
      SendDlgItemMessage(hwndDlg, IDC_OTHERCONTRIB, WM_SETFONT, (WPARAM)fontnorm, FALSE);
      SetDlgItemText(hwndDlg, IDC_NSISVER, g_sdata.branding);
      SetDlgItemText(hwndDlg, IDC_ABOUTVERSION, NSISW_VERSION);
      SetDlgItemText(hwndDlg, IDC_ABOUTCOPY, COPYRIGHT);
      SetDlgItemText(hwndDlg, IDC_OTHERCONTRIB, CONTRIB);
      break;
    }
    case WM_COMMAND:
      if (IDOK != LOWORD(wParam)) break;
      // fall through
    case WM_CLOSE:
      return EndDialog(hwndDlg, TRUE);
    case WM_DESTROY:
      DeleteObject((HGDIOBJ)SendDlgItemMessage(hwndDlg, IDC_ABOUTVERSION, WM_GETFONT, 0, 0));
      DeleteObject((HGDIOBJ)SendDlgItemMessage(hwndDlg, IDC_ABOUTCOPY, WM_GETFONT, 0, 0));
      break;
  }
  return FALSE;
}

void EnableSymbolSetButtons(HWND hwndDlg)
{
  LRESULT n = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETCOUNT, 0, 0);
  EnableWindow(GetDlgItem(hwndDlg, IDC_CLEAR), n > 0);
  EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE), n > 0);
}

void EnableSymbolEditButtons(HWND hwndDlg)
{
  LRESULT n = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETSELCOUNT, 0, 0);
  EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT), n == 1);
  EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), n != 0);
}

void SetSymbols(HWND hwndDlg, TCHAR **symbols)
{
    int i = 0;
    SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_RESETCONTENT , 0, 0);
    if (symbols) {
      while (symbols[i]) {
        SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_ADDSTRING, 0, (LPARAM)symbols[i]);
        i++;
      }
    }
    EnableSymbolSetButtons(hwndDlg);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHT), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), FALSE);
}

TCHAR **GetSymbols(HWND hwndDlg)
{
  LRESULT n = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETCOUNT, 0, 0);
  TCHAR **symbols = NULL;
  if(n > 0) {
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (n+1)*sizeof(TCHAR *));
    symbols = (TCHAR **)GlobalLock(hMem);
    for (LRESULT i = 0; i < n; i++) {
      LRESULT len = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETTEXTLEN, (WPARAM)i, 0);
      symbols[i] = (TCHAR*) MemAllocZI((len+1)*sizeof(TCHAR));
      SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETTEXT, (WPARAM)i, (LPARAM)symbols[i]);
    }
    symbols[n] = NULL;
  }

  return symbols;
}

INT_PTR CALLBACK SettingsProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      int i = 0;

      for(i = (int)COMPRESSOR_SCRIPT; i <= (int)COMPRESSOR_BEST; i++) {
        SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_ADDSTRING, 0, (LPARAM)compressor_display_names[i]);
      }
      SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_SETCURSEL, (WPARAM)g_sdata.default_compressor, (LPARAM)0);

      SetSymbols(hwndDlg, g_sdata.symbols);
      SetFocus(GetDlgItem(hwndDlg, IDC_SYMBOL));
      break;
    }
    case WM_MAKENSIS_LOADSYMBOLSET:
    {
      TCHAR *name = (TCHAR *)wParam;
      TCHAR **symbols = LoadSymbolSet(name);
      HGLOBAL hMem;

      SetSymbols(hwndDlg, symbols);
      if(symbols) {
        hMem = GlobalHandle(symbols);
        GlobalUnlock(hMem);
        GlobalFree(hMem);
      }
      break;
    }
    case WM_MAKENSIS_SAVESYMBOLSET:
    {
      TCHAR *name = (TCHAR *)wParam;
      TCHAR **symbols = GetSymbols(hwndDlg);
      HGLOBAL hMem;

      if(symbols) {
        SaveSymbolSet(name, symbols);
        hMem = GlobalHandle(symbols);
        GlobalUnlock(hMem);
        GlobalFree(hMem);
      }
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDOK:
        {
          ResetObjects();
          ResetSymbols();
          g_sdata.symbols = GetSymbols(hwndDlg);

          INT_PTR n = SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
          if (n >= (INT_PTR)COMPRESSOR_SCRIPT && n <= (INT_PTR)COMPRESSOR_BEST) {
            g_sdata.default_compressor = (NCOMPRESSOR)n;
          }
          else {
            g_sdata.default_compressor = COMPRESSOR_SCRIPT;
          }
          EndDialog(hwndDlg, TRUE);
          SetCompressor(g_sdata.default_compressor);
        }
        break;
        case IDCANCEL:
          EndDialog(hwndDlg, TRUE);
          break;
        case IDC_RIGHT:
        {
          LRESULT n = SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXTLENGTH, 0, 0);
          if(n > 0) {
            TCHAR *buf = (TCHAR*) MemAllocZI((n+1)*sizeof(TCHAR));
            SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXT, n+1, (LPARAM)buf);
            if(_tcsstr(buf,_T(" ")) || _tcsstr(buf,_T("\t"))) {
              MessageBox(hwndDlg,SYMBOLSERROR,ERRBOXTITLE,MB_OK|MB_ICONSTOP);
              MemFree(buf);
              break;
            }

            n = SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_GETTEXTLENGTH, 0, 0);
            if(n > 0) {
              TCHAR *buf2 = (TCHAR*) MemAllocZI((n+1)*sizeof(TCHAR));
              SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_GETTEXT, n+1, (LPARAM)buf2);
              TCHAR *buf3 = (TCHAR*) MemAllocZI((lstrlen(buf)+lstrlen(buf2)+2)*sizeof(TCHAR));
              wsprintf(buf3,_T("%s=%s"),buf,buf2);
              MemFree(buf);
              buf = buf3;
              MemFree(buf2);
            }
            INT_PTR idx = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_ADDSTRING, 0, (LPARAM)buf);
            if (idx >= 0)
            {
              SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_SETSEL, FALSE, -1);
              SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_SETSEL, TRUE, idx);
            }
            EnableSymbolEditButtons(hwndDlg);
            SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_SETTEXT, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_SETTEXT, 0, 0);
            MemFree(buf);
            EnableSymbolSetButtons(hwndDlg);
          }
        }
        break;
        case IDC_LEFT:
        {
          if (SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETSELCOUNT, 0, 0) != 1)
            break;

          int index;
          INT_PTR num = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETSELITEMS, 1, (LPARAM)&index);
          if(num == 1) {
            INT_PTR n = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETTEXTLEN, (WPARAM)index, 0);
            if(n > 0) {
              TCHAR *buf = (TCHAR*) MemAllocZI((n+1)*sizeof(TCHAR));
              SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETTEXT, (WPARAM)index, (LPARAM)buf);
              TCHAR *p = _tcsstr(buf,_T("="));
              if(p) {
                SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_SETTEXT, 0, (LPARAM)(p+1));
                *p=0;
              }
              SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_SETTEXT, 0, (LPARAM)buf);
              MemFree(buf);
              SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_DELETESTRING, (WPARAM)index, 0);
              EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), FALSE);
              EnableSymbolSetButtons(hwndDlg);
            }
          }
        }
        break;
        case IDC_CLEAR:
        {
          SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_RESETCONTENT , 0, 0);
          EnableSymbolSetButtons(hwndDlg);
        }
        break;
        case IDC_LOAD:
        case IDC_SAVE:
        {
          g_symbol_set_mode = IDC_LOAD == LOWORD(wParam) ? 1 : 2;
          DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_SYMBOLSET),hwndDlg,(DLGPROC)SymbolSetProc);
        }
        break;
        case IDC_DEL:
        {
          INT_PTR n = SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETSELCOUNT, 0, 0);
          int *items = (int*) MemAllocZI(n*sizeof(int));
          if (items) {
            SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_GETSELITEMS, (WPARAM)n, (LPARAM)items);
            for(INT_PTR i=n-1;i>=0;i--)
              SendDlgItemMessage(hwndDlg, IDC_SYMBOLS, LB_DELETESTRING, (WPARAM)items[i], 0);
            MemFree(items);
          }
          EnableSymbolEditButtons(hwndDlg);
          EnableSymbolSetButtons(hwndDlg);
        }
        break;
        case IDC_SYMBOL:
          if(HIWORD(wParam) == EN_CHANGE)
          {
            LRESULT n = SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXTLENGTH, 0, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHT), n > 0);
          }
          break;
        case IDC_SYMBOLS:
          if (HIWORD(wParam) == LBN_SELCHANGE)
          {
            EnableSymbolEditButtons(hwndDlg);
          }
          else if (HIWORD(wParam) == LBN_DBLCLK)
          {
            SendDlgItemMessage(hwndDlg, IDC_LEFT, BM_CLICK, 0, 0);
          }
          break;
        }
      break;
    }
  }
  return FALSE;
}

INT_PTR CALLBACK CompressorProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      int i=0;

      for(i=(int)COMPRESSOR_SCRIPT; i<= (int)COMPRESSOR_BEST; i++) {
        SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_ADDSTRING, 0, (LPARAM)compressor_display_names[i]);
      }
      SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_SETCURSEL, (WPARAM)g_sdata.compressor, (LPARAM)0);

      SetFocus(GetDlgItem(hwndDlg, IDC_COMPRESSOR));
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDOK:
        {
          INT_PTR n = SendDlgItemMessage(hwndDlg, IDC_COMPRESSOR, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
          if(n >= (INT_PTR)COMPRESSOR_SCRIPT && n <= (INT_PTR)COMPRESSOR_BEST)
            SetCompressor((NCOMPRESSOR)n);
          else
            SetCompressor(g_sdata.default_compressor);
          EndDialog(hwndDlg, 0);
          break;
        }
        case IDCANCEL:
        {
          EndDialog(hwndDlg, 1);
          LogMessage(g_sdata.hwnd,USAGE);
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

INT_PTR CALLBACK SymbolSetProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      HWND hwndEdit;
      HKEY hKey;

      EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), FALSE);
      if (OpenRegSettingsKey(hKey)) {
        HKEY hSubKey;

        if (RegOpenKeyEx(hKey,REGSYMSUBKEY,0,KEY_READ,&hSubKey) == ERROR_SUCCESS) {
          TCHAR subkey[1024];
          int i=0;

          while (RegEnumKey(hSubKey,i,subkey,sizeof(subkey)) == ERROR_SUCCESS) {
            SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_ADDSTRING, 0, (LPARAM)subkey);
            i++;
          }
          RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
      }

      hwndEdit = FindWindowEx(GetDlgItem(hwndDlg, IDC_NAMES), 0, 0, 0); // Handle of list
      hwndEdit = FindWindowEx(GetDlgItem(hwndDlg, IDC_NAMES), hwndEdit, 0, 0); //Handle of edit box
      SendMessage(hwndEdit, EM_LIMITTEXT, (WPARAM)SYMBOL_SET_NAME_MAXLEN, 0);
      if(g_symbol_set_mode == 1) { //Load
        SetWindowText(hwndDlg, LOAD_SYMBOL_SET_DLG_NAME);
        SetWindowText(GetDlgItem(hwndDlg, IDOK), LOAD_BUTTON_TEXT);
        SendMessage(hwndEdit, EM_SETREADONLY, (WPARAM)TRUE, 0);
      }
      else {
        SetWindowText(hwndDlg, SAVE_SYMBOL_SET_DLG_NAME);
        SetWindowText(GetDlgItem(hwndDlg, IDOK), SAVE_BUTTON_TEXT);
      }
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDOK:
        {
          HWND hwndEdit;
          TCHAR name[SYMBOL_SET_NAME_MAXLEN+1];

          hwndEdit = FindWindowEx(GetDlgItem(hwndDlg, IDC_NAMES), 0, 0, 0); // Handle of list
          hwndEdit = FindWindowEx(GetDlgItem(hwndDlg, IDC_NAMES), hwndEdit, 0, 0); //Handle of edit box
          SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)SYMBOL_SET_NAME_MAXLEN+1, (LPARAM)name);
          if(!*name) {
            if(g_symbol_set_mode == 1) { //Load
              MessageBox(hwndDlg,LOAD_SYMBOL_SET_MESSAGE,LOAD_SYMBOL_SET_DLG_NAME,MB_OK|MB_ICONEXCLAMATION);
            }
            else {
              MessageBox(hwndDlg,SAVE_SYMBOL_SET_MESSAGE,SAVE_SYMBOL_SET_DLG_NAME,MB_OK|MB_ICONEXCLAMATION);
            }
          }
          else {
            HWND hwndParent = GetParent(hwndDlg);
            if(g_symbol_set_mode == 1) { //Load
              SendMessage(hwndParent, WM_MAKENSIS_LOADSYMBOLSET, (WPARAM)name, (LPARAM)NULL);
            }
            else {
              SendMessage(hwndParent, WM_MAKENSIS_SAVESYMBOLSET, (WPARAM)name, (LPARAM)NULL);
            }
            EndDialog(hwndDlg, TRUE);
          }
          break;
        }
        case IDCANCEL:
        {
          EndDialog(hwndDlg, TRUE);
          break;
        }
        case IDC_DEL:
        {
          LONG_PTR n = SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_GETCURSEL, 0, 0);
          if(n != CB_ERR) {
            INT_PTR len = SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_GETLBTEXTLEN, (WPARAM)n, 0);
            TCHAR *buf = (TCHAR*) MemAllocZI((len+1)*sizeof(TCHAR));
            if(SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_GETLBTEXT, (WPARAM)n, (LPARAM)buf) != CB_ERR) {
              SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_DELETESTRING, n, 0);
              DeleteSymbolSet(buf);
            }
            MemFree(buf);
          }
          EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), FALSE);
          break;
        }
        case IDC_NAMES:
        {
          if(HIWORD(wParam) == CBN_SELCHANGE)
          {
            LONG_PTR n = SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_GETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DEL), CB_ERR != n);
          }
          else if(HIWORD(wParam) == CBN_DBLCLK)
          {
            LONG_PTR n = SendDlgItemMessage(hwndDlg, IDC_NAMES, CB_GETCURSEL, 0, 0);
            if (n != CB_ERR) SendDlgItemMessage(hwndDlg, IDOK, BM_CLICK, 0, 0);
          }
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

void SetCompressor(NCOMPRESSOR compressor)
{
  int i;

  if(g_sdata.compressor != compressor) {
    WORD command;
    LPCTSTR compressor_name;

    if(compressor > COMPRESSOR_SCRIPT && compressor < COMPRESSOR_BEST) {
      command = compressor_commands[(int)compressor];
      compressor_name = compressor_names[(int)compressor];
    }
    else if(compressor == COMPRESSOR_BEST) {
      command = compressor_commands[(int)compressor];
      compressor_name = compressor_names[(int)COMPRESSOR_SCRIPT+1];
    }
    else {
      compressor = COMPRESSOR_SCRIPT;
      command = IDM_COMPRESSOR_SCRIPT;
      compressor_name = _T("");
    }
    g_sdata.compressor = compressor;
    g_sdata.compressor_name = compressor_name;
    UpdateToolBarCompressorButton();
    for(i=(int)COMPRESSOR_SCRIPT; i<= (int)COMPRESSOR_BEST; i++) {
      CheckMenuItem(g_sdata.menu, compressor_commands[i], MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(g_sdata.menu, command, MF_BYCOMMAND | MF_CHECKED);
    ResetObjects();
  }
}

