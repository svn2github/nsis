/* 
  Copyright (c) 2002 Robert Rainwater
  Copyright (c) 2002 Robert Rainwater
  Contributors: Justin Frankel, Fritz Elfert, and Sunil Kamath

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

  Unicode support by Jim Park -- 08/20/2007

*/
#ifndef UTILS_H
#define UTILS_H
#include "resource.h"
#include "toolbar.h"

#define MRU_LIST_SIZE 5
#define MRU_DISPLAY_LENGTH 40

int SetArgv(const TCHAR *cmdLine, TCHAR ***argv);
void SetTitle(HWND hwnd,TCHAR *substr);
void SetBranding(HWND hwnd);
void CopyToClipboard(HWND hwnd);
void ClearLog(HWND hwnd);
void LogMessage(HWND hwnd,const TCHAR *str);
void ErrorMessage(HWND hwnd,const TCHAR *str);
#define DisableItems(hwnd) Items(hwnd, 0)
#define EnableItems(hwnd) Items(hwnd, 1)
void Items(HWND hwnd, int on);
/*void DisableItems(HWND hwnd);
void EnableItems(HWND hwnd);*/
bool OpenRegSettingsKey(HKEY &hKey, bool create = false);
void RestoreWindowPos(HWND hwnd);
void SaveWindowPos(HWND hwnd);
void ResetObjects();
void ResetSymbols();
int InitBranding();
void InitTooltips(HWND h);
void DestroyTooltips();
void AddTip(HWND hWnd,LPTSTR lpszToolTip);
void ShowDocs();
void RestoreCompressor();
void SaveCompressor();
void SetCompressorStats();

BOOL PopMRUFile(TCHAR* fname);
void PushMRUFile(TCHAR* fname);
void BuildMRUMenus();
void LoadMRUFile(int position);
void ClearMRUList();

BOOL FileExists(TCHAR *fname);

HMENU FindSubMenu(HMENU hMenu, UINT uId);
#endif
