/*
  NSIS-DL 1.3 - http downloading DLL for NSIS
  Copyright (C) 2001-2002 Yaroslav Faybishenko & Justin Frankel

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

  Unicode support by Jim Park -- 08/24/2007
*/
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "netinc.h"
#include "util.h"
#include "httpget.h"

#include <nsis/pluginapi.h> // nsis plugin

void *operator new( size_t num_bytes )
{
  return GlobalAlloc(GPTR,num_bytes);
}
void operator delete( void *p ) { if (p) GlobalFree(p); }


HMODULE     hModule;
HWND        g_hwndProgressBar;
HWND        g_hwndStatic;
static int  g_cancelled;
static WNDPROC lpWndProcOld;

static UINT uMsgCreate;

HWND childwnd;
HWND hwndL;
HWND hwndB;

static LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (uMsgCreate && message == uMsgCreate)
  {
    static HWND hwndPrevFocus;
    static BOOL fCancelDisabled;

    if (wParam)
    {
      childwnd = FindWindowEx((HWND) lParam, NULL, _T("#32770"), NULL);
      hwndL = GetDlgItem(childwnd, 1016);
      hwndB = GetDlgItem(childwnd, 1027);
      HWND hwndP = GetDlgItem(childwnd, 1004);
      HWND hwndS = GetDlgItem(childwnd, 1006);
      if (childwnd && hwndP && hwndS)
      {
        // Where to restore focus to before we disable the cancel button
        hwndPrevFocus = GetFocus();
        if (!hwndPrevFocus)
          hwndPrevFocus = hwndP;

        if (IsWindowVisible(hwndL))
          ShowWindow(hwndL, SW_HIDE);
        else
          hwndL = NULL;
        if (IsWindowVisible(hwndB))
          ShowWindow(hwndB, SW_HIDE);
        else
          hwndB = NULL;

        RECT wndRect, ctlRect;

        GetClientRect(childwnd, &wndRect);

        GetWindowRect(hwndS, &ctlRect);

        HWND s = g_hwndStatic = CreateWindow(
          _T("STATIC"),
          _T(""),
          WS_CHILD | WS_CLIPSIBLINGS | SS_CENTER,
          0,
          wndRect.bottom / 2 - (ctlRect.bottom - ctlRect.top) / 2,
          wndRect.right,
          ctlRect.bottom - ctlRect.top,
          childwnd,
          NULL,
          hModule,
          NULL
        );

        DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        dwStyle |= GetWindowLongPtr(hwndP, GWL_STYLE) & PBS_SMOOTH;

        GetWindowRect(hwndP, &ctlRect);

        HWND pb = g_hwndProgressBar = CreateWindow(
          _T("msctls_progress32"),
          _T(""),
          dwStyle,
          0,
          wndRect.bottom / 2 + (ctlRect.bottom - ctlRect.top) / 2,
          wndRect.right,
          ctlRect.bottom - ctlRect.top,
          childwnd,
          NULL,
          hModule,
          NULL
        );

        LRESULT c = SendMessage(hwndP, PBM_SETBARCOLOR, 0, 0);
        SendMessage(hwndP, PBM_SETBARCOLOR, 0, c);
        SendMessage(pb, PBM_SETBARCOLOR, 0, c);

        c = SendMessage(hwndP, PBM_SETBKCOLOR, 0, 0);
        SendMessage(hwndP, PBM_SETBKCOLOR, 0, c);
        SendMessage(pb, PBM_SETBKCOLOR, 0, c);

        // set font
        LRESULT hFont = SendMessage((HWND) lParam, WM_GETFONT, 0, 0);
        SendMessage(pb, WM_SETFONT, hFont, 0);
        SendMessage(s, WM_SETFONT, hFont, 0);

        ShowWindow(pb, SW_SHOWNA);
        ShowWindow(s, SW_SHOWNA);

        fCancelDisabled = EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
        SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDCANCEL), TRUE);
      }
      else
        childwnd = NULL;
    }
    else if (childwnd)
    {
      if (hwndB)
      {
        ShowWindow(hwndB, SW_SHOWNA);
        hwndB = NULL;
      }
      if (hwndL)
      {
        ShowWindow(hwndL, SW_SHOWNA);
        hwndL = NULL;
      }

      // Prevent wierd stuff happening if the cancel button happens to be
      // pressed at the moment we are finishing and restore the previous focus
      // and cancel button states
      SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)hwndPrevFocus, TRUE);
      SendMessage(GetDlgItem(hwnd, IDCANCEL), BM_SETSTATE, FALSE, 0);
      if (fCancelDisabled)
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);

      if (g_hwndStatic)
      {
        DestroyWindow(g_hwndStatic);
        g_hwndStatic = NULL;
      }
      if (g_hwndProgressBar)
      {
        DestroyWindow(g_hwndProgressBar);
        g_hwndProgressBar = NULL;
      }
      childwnd = NULL;
    }
  }
  else if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
  {
    g_cancelled = 1;
  }
  else
  {
    return CallWindowProc(lpWndProcOld, hwnd, message, wParam, lParam);
  }
  return 0;
}

extern "C" BOOL APIENTRY DllMain(HINSTANCE _hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  hModule = _hModule;
  return TRUE;
}

#define INT32_MAX 0x7fffffff

int MulDiv64(int nNumber, __int64 nNumerator, __int64 nDenominator)
{
    // ok, a complete implementation would handle negatives too, 
    // but this method is probably not generally useful.
    while (nNumerator > INT32_MAX || nDenominator > INT32_MAX)
    {
        nNumerator = Int64ShraMod32(nNumerator, 1);
        nDenominator = Int64ShraMod32(nDenominator, 1);
    }
    return MulDiv(nNumber, (int)nNumerator, (int)nDenominator);
}


static __int64 g_file_size;
static DWORD g_dwLastTick = 0;
void progress_callback(const char *msg, __int64 read_bytes)
{
  // flicker reduction by A. Schiffler
  DWORD dwLastTick = g_dwLastTick;
  DWORD dwThisTick = GetTickCount();
  if (childwnd)
  {
    if (dwThisTick - dwLastTick > 500)
    {
      SetWindowTextA(g_hwndStatic, msg);
      dwLastTick = dwThisTick;
    }
    if (g_file_size)
      SendMessage(g_hwndProgressBar, PBM_SETPOS, (WPARAM) MulDiv64(30000, read_bytes, g_file_size), 0);
    g_dwLastTick = dwLastTick;
  }
}

extern char *_strstr(char *i, const char *s);
#define strstr _strstr

extern "C"
{

__declspec(dllexport) void download (HWND   parent,
              int    string_size,
              TCHAR   *variables,
              stack_t **stacktop)
{
  char buf[1024];
  char url[1024];
  TCHAR filenameT[1024];
  static char proxy[1024];
  BOOL bSuccess=FALSE;
  int timeout_ms=30000;
  int getieproxy=1;
  int manualproxy=0;
  int translation_version;

  const char *error=NULL;

  // translation version 2 & 1
  static char szDownloading[1024]; // "Downloading %s"
  static char szConnecting[1024];  // "Connecting ..."
  static char szSecond[1024];      // " (1 second remaining)" for v2
                                   // "second" for v1
  static char szMinute[1024];      // " (1 minute remaining)" for v2
                                   // "minute" for v1
  static char szHour[1024];        // " (1 hour remaining)" for v2
                                   // "hour" for v1
  static char szProgress[1024];    // "%skB (%d%%) of %skB at %u.%01ukB/s" for v2
                                   // "%dkB (%d%%) of %dkB at %d.%01dkB/s" for v1

  // translation version 2 only
  static char szSeconds[1024];     // " (%u seconds remaining)"
  static char szMinutes[1024];     // " (%u minutes remaining)"
  static char szHours[1024];       // " (%u hours remaining)"

  // translation version 1 only
  static char szPlural[1024];      // "s";
  static char szRemaining[1024];   // " (%d %s%s remaining)";

  EXDLL_INIT();

  PopStringA(url);
  if (!lstrcmpiA(url, "/TRANSLATE2")) {
    PopStringA(szDownloading);
    PopStringA(szConnecting);
    PopStringA(szSecond);
    PopStringA(szMinute);
    PopStringA(szHour);
    PopStringA(szSeconds);
    PopStringA(szMinutes);
    PopStringA(szHours);
    PopStringA(szProgress);
    PopStringA(url);
    translation_version=2;
  } else if (!lstrcmpiA(url, "/TRANSLATE")) {
    PopStringA(szDownloading);
    PopStringA(szConnecting);
    PopStringA(szSecond);
    PopStringA(szMinute);
    PopStringA(szHour);
    PopStringA(szPlural);
    PopStringA(szProgress);
    PopStringA(szRemaining);
    PopStringA(url);
    translation_version=1;
  } else {
    lstrcpyA(szDownloading, "Downloading %s");
    lstrcpyA(szConnecting, "Connecting ...");
    lstrcpyA(szSecond, " (1 second remaining)");
    lstrcpyA(szMinute, " (1 minute remaining)");
    lstrcpyA(szHour, " (1 hour remaining)");
    lstrcpyA(szSeconds, " (%u seconds remaining)");
    lstrcpyA(szMinutes, " (%u minutes remaining)");
    lstrcpyA(szHours, " (%u hours remaining)");
    lstrcpyA(szProgress, "%skB (%d%%) of %skB at %u.%01ukB/s");
    translation_version=2;
  }
  lstrcpynA(buf, url, 10);
  if (!lstrcmpiA(buf, "/TIMEOUT=")) {
    timeout_ms=my_atoi(url+9);
    PopStringA(url);
  }
  if (!lstrcmpiA(url, "/PROXY")) {
    getieproxy=0;
    manualproxy=1;
    PopStringA(proxy);
    PopStringA(url);
  }
  if (!lstrcmpiA(url, "/NOIEPROXY")) {
    getieproxy=0;
    PopStringA(url);
  }
  popstring(filenameT);

  static char main_buf[8192];
  char *filenameA;
#ifdef _UNICODE
  filenameA = main_buf;
  wsprintfA(filenameA, "%S", filenameT);
#else
  filenameA = filenameT;
#endif

  HANDLE hFile = CreateFile(filenameT,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    wsprintfA(buf, "Unable to open %s", filenameA);
    error = buf;
  }
  else
  {
    if (parent)
    {
      uMsgCreate = RegisterWindowMessage(_T("nsisdl create"));

      lpWndProcOld = (WNDPROC)SetWindowLongPtr(parent,GWLP_WNDPROC,(LONG_PTR)ParentWndProc);

      SendMessage(parent, uMsgCreate, TRUE, (LPARAM) parent);

      // set initial text
      char *p = filenameA;
      while (*p) p++;
      while (*p !='\\' && p != filenameA) p = CharPrevA(filenameA, p);
      wsprintfA(buf, szDownloading, p != filenameA ? p + 1 : p);
      SetDlgItemTextA(childwnd, 1006, buf);
      SetWindowTextA(g_hwndStatic, szConnecting);
    }
    {
      WSADATA wsaData;
      WSAStartup(MAKEWORD(1, 1), &wsaData);

      JNL_HTTPGet *get = 0;

      char *buf = main_buf, *p = NULL;

      HKEY hKey;
      if (getieproxy && RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",0,KEY_READ,&hKey) == ERROR_SUCCESS)
      {
        DWORD l = 4;
        DWORD t;
        DWORD v;
        if (RegQueryValueExA(hKey,"ProxyEnable",NULL,&t,(unsigned char*)&v,&l) == ERROR_SUCCESS && t == REG_DWORD && v)
        {
          l=8192;
          if (RegQueryValueExA(hKey,"ProxyServer",NULL,&t,(unsigned char *)buf,&l ) == ERROR_SUCCESS && t == REG_SZ)
          {
            p=strstr(buf,"http=");
            if (!p) p=buf;
            else {
              p+=5;
            }
            char *tp=strstr(p,";");
            if (tp) *tp=0;
            char *p2=strstr(p,"=");
            if (p2) p=0; // we found the wrong proxy
          }
        }
        buf[8192-1]=0;
        RegCloseKey(hKey);
      }
      if (manualproxy == 1) {
        p = proxy;
      }

      DWORD start_time=GetTickCount();
      get=new JNL_HTTPGet(JNL_CONNECTION_AUTODNS,16384,(p&&p[0])?p:NULL);
      int         st;
      int         has_printed_headers = 0;
      __int64     cl = 0;
      int         len;
      __int64     sofar = 0;
      DWORD last_recv_time=start_time;

      get->addheader ("User-Agent: NSISDL/1.2 (Mozilla)");
      get->addheader ("Accept: */*");

      get->connect (url);

      while (1) {
        if (g_cancelled)
            error = "cancel";

        if (error)
        {
          if (parent)
          {
            SendMessage(parent, uMsgCreate, FALSE, (LPARAM) parent);
            SetWindowLongPtr(parent, GWLP_WNDPROC, (LONG_PTR)lpWndProcOld);
          }
          break;
        }

        st = get->run ();

        if (st == -1) {
          lstrcpynA(url, get->geterrorstr(), sizeof(url));
          error = url;
        } else if (st == 1) {
          if (sofar < cl || get->get_status () != 2)
            error="download incomplete";
          else
          {
            bSuccess=TRUE;
            error = "success";
          }
        } else {

          if (get->get_status () == 0) {
            // progressFunc ("Connecting ...", 0);
            if (last_recv_time+timeout_ms < GetTickCount())
              error = "Timed out on connecting.";
            else
              Sleep(10); // don't busy-loop while connecting

          } else if (get->get_status () == 1) {

            progress_callback("Reading headers", 0);
            if (last_recv_time+timeout_ms < GetTickCount())
              error = "Timed out on getting headers.";
            else
              Sleep(10); // don't busy-loop while reading headers

          } else if (get->get_status () == 2) {

            if (! has_printed_headers) {
              has_printed_headers = 1;
              last_recv_time=GetTickCount();

              cl = get->content_length ();
              if (cl == 0)
                error = "Server did not specify content length.";
              else if (g_hwndProgressBar) {
                SendMessage(g_hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 30000));
                g_file_size = cl;
              }
            }

            int data_downloaded = 0;
            while ((len = get->bytes_available ()) > 0) {
              data_downloaded++;
              if (len > 8192)
                len = 8192;
              len = get->get_bytes (buf, len);
              if (len > 0) {
                last_recv_time=GetTickCount();
                DWORD dw;
                WriteFile(hFile,buf,len,&dw,NULL);
                sofar += len;
                int time_sofar=(GetTickCount()-start_time)/1000;
                int bps = (int)(sofar/(time_sofar?time_sofar:1));
                int remain = MulDiv64(time_sofar, cl, sofar) - time_sofar;

                if (translation_version == 2) {
                  char *rtext=remain==1?szSecond:szSeconds;;
                  if (remain >= 60)
                  {
                    remain/=60;
                    rtext=remain==1?szMinute:szMinutes;
                    if (remain >= 60)
                    {
                      remain/=60;
                      rtext=remain==1?szHour:szHours;
                    }
                  }

                  char sofar_str[128];
                  char cl_str[128];
                  myitoa64(sofar/1024, sofar_str);
                  myitoa64(cl/1024, cl_str);

                  wsprintfA (buf,
                        szProgress, //%skB (%d%%) of %skB @ %u.%01ukB/s
                        sofar_str,
                        MulDiv64(100, sofar, cl),
                        cl_str,
                        bps/1024,((bps*10)/1024)%10
                        );
                  if (remain) wsprintfA(buf+lstrlenA(buf),rtext,
                        remain
                        );
                } else if (translation_version == 1) {
                  char *rtext=szSecond;
                  if (remain >= 60)
                  {
                    remain/=60;
                    rtext=szMinute;
                    if (remain >= 60)
                    {
                      remain/=60;
                      rtext=szHour;
                    }
                  }

                  wsprintfA (buf,
                        szProgress, //%dkB (%d%%) of %dkB @ %d.%01dkB/s
                        int(sofar/1024),
                        MulDiv64(100, sofar, cl),
                        int(cl/1024),
                        bps/1024,((bps*10)/1024)%10
                        );
                  if (remain) wsprintfA(buf+lstrlenA(buf),szRemaining,
                        remain,
                        rtext,
                        remain==1?"":szPlural
                        );
                }
                progress_callback(buf, sofar);
              } else {
                if (sofar < cl)
                  error = "Server aborted.";
              }
            }
            if (GetTickCount() > last_recv_time+timeout_ms)
            {
              if (sofar != cl)
              {
                error = "Downloading timed out.";
              }
              else
              {
                // workaround for bug #1713562
                //   buggy servers that wait for the client to close the connection.
                //   another solution would be manually stopping when cl == sofar,
                //   but then buggy servers that return wrong content-length will fail.
                bSuccess = TRUE;
                error = "success";
              }
            }
            else if (!data_downloaded)
              Sleep(10);

          } else {
            error = "Bad response status.";
          }
        }

      }

      // Clean up the connection then release winsock
      if (get) delete get;
      WSACleanup();
    }

    CloseHandle(hFile);
  }

  if (g_cancelled || !bSuccess) {
    DeleteFile(filenameT);
  }

  PushStringA(error);
}


__declspec(dllexport) void download_quiet(HWND   parent,
              int    stringsize,
              TCHAR   *variables,
              stack_t **stacktop)
{
  g_hwndProgressBar=0;
  download(NULL,stringsize,variables,stacktop);
}

} //extern "C"
