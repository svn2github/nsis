/*
 * util.h
 * 
 * This file is a part of NSIS.
 * 
 * Copyright (C) 1999-2014 Nullsoft and Contributors
 * 
 * Licensed under the zlib/libpng license (the "License");
 * you may not use this file except in compliance with the License.
 * 
 * Licence details can be found in the file COPYING.
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.
 *
 * Unicode support by Jim Park -- 07/23/2007
 */

#ifndef ___NSIS_UTIL_H___
#define ___NSIS_UTIL_H___

#include "../Platform.h"
#include "config.h"
#include <shlobj.h>

extern TCHAR ps_tmpbuf[NSIS_MAX_STRLEN*2];
TCHAR * NSISCALL GetNSISString(TCHAR *outbuf, int strtab);
#define GetNSISStringTT(strtab) GetNSISString(0, (strtab))
#define GetNSISStringNP(strtab) ((const TCHAR *)g_blocks[NB_STRINGS].offset+(strtab))

// A negative string table index means it's a language string table, so we
// use the LANG_STR_TAB() macro to decode it.
#define GetNSISTab(strtab) (strtab < 0 ? LANG_STR_TAB(strtab) : strtab)

void NSISCALL myRegGetStr(HKEY root, const TCHAR *sub, const TCHAR *name, TCHAR *out, int x64);
#define myatoi(s) ( (int)strtoiptr(s) )
INT_PTR NSISCALL strtoiptr(const TCHAR *s);
#define myitoa iptrtostr
void NSISCALL iptrtostr(TCHAR *s, INT_PTR d);
TCHAR * NSISCALL mystrcpy(TCHAR *out, const TCHAR *in);
int NSISCALL mystrlen(const TCHAR *in);
TCHAR * NSISCALL mystrcat(TCHAR *out, const TCHAR *concat);
TCHAR * NSISCALL mystrstr(TCHAR *a, TCHAR *b);
WIN32_FIND_DATA * NSISCALL file_exists(TCHAR *buf);
TCHAR * NSISCALL my_GetTempFileName(TCHAR *buf, const TCHAR *dir);
BOOL NSISCALL myReadFile(HANDLE h, LPVOID buf, DWORD cb);

//BOOL NSISCALL my_SetWindowText(HWND hWnd, const TCHAR *val);
#define my_SetWindowText SetWindowText
BOOL NSISCALL my_SetDialogItemText(HWND dlg, UINT idx, const TCHAR *val);
//#define my_SetDialogItemText SetDlgItemText
//int NSISCALL my_GetWindowText(HWND hWnd, TCHAR *val, int size);
#define my_GetWindowText GetWindowText
int NSISCALL my_GetDialogItemText(UINT idx, TCHAR *val);
//#define my_GetDialogItemText GetDlgItemText

#ifdef NSIS_CONFIG_LOG
extern TCHAR log_text[2048]; // BUGBUG: Should this be 2*NSIS_MAX_STRLEN? ...and a little larger when NSIS_CONFIG_LOG_TIMESTAMP is defined!
void NSISCALL log_write(int close);
const TCHAR * _RegKeyHandleToName(HKEY hKey);
void _LogData2Hex(TCHAR *buf, size_t cchbuf, BYTE *data, size_t cbdata);
void log_printf(TCHAR *format, ...);
#define log_printf2(x1,x2) log_printf(x1,x2);
#define log_printf3(x1,x2,x3) log_printf(x1,x2,x3);
#define log_printf4(x1,x2,x3,x4) log_printf(x1,x2,x3,x4);
#define log_printf5(x1,x2,x3,x4,x5) log_printf(x1,x2,x3,x4,x5);
#define log_printf6(x1,x2,x3,x4,x5,x6) log_printf(x1,x2,x3,x4,x5,x6);
#define log_printf7(x1,x2,x3,x4,x5,x6,x7) log_printf(x1,x2,x3,x4,x5,x6,x7);
#define log_printf8(x1,x2,x3,x4,x5,x6,x7,x8) log_printf(x1,x2,x3,x4,x5,x6,x7,x8);
#define RegKeyHandleToName(x1) _RegKeyHandleToName(x1);
#define LogData2Hex(x1,x2,x3,x4) _LogData2Hex(x1,x2,x3,x4);
extern int log_dolog;
extern TCHAR g_log_file[1024];
#else
#define log_printf(x1)
#define log_printf2(x1,x2)
#define log_printf3(x1,x2,x3)
#define log_printf4(x1,x2,x3,x4)
#define log_printf5(x1,x2,x3,x4,x5)
#define log_printf6(x1,x2,x3,x4,x5,x6)
#define log_printf7(x1,x2,x3,x4,x5,x6,x7)
#define log_printf8(x1,x2,x3,x4,x5,x6,x7,x8)
#define RegKeyHandleToName(x1) NULL
#define LogData2Hex(x1,x2,x3,x4)
#endif

HANDLE NSISCALL myCreateProcess(TCHAR *cmd);
int NSISCALL my_MessageBox(const TCHAR *text, UINT type);

void NSISCALL myDelete(TCHAR *buf, int flags);

HANDLE NSISCALL myOpenFile(const TCHAR *fn, DWORD da, DWORD cd);
int NSISCALL validpathspec(TCHAR *ubuf);
TCHAR * NSISCALL addtrailingslash(TCHAR *str);
//TCHAR NSISCALL lastchar(const TCHAR *str);
#define lastchar(str) *CharPrev(str,str+mystrlen(str))
TCHAR * NSISCALL findchar(TCHAR *str, TCHAR c);
TCHAR * NSISCALL trimslashtoend(TCHAR *buf);
TCHAR * NSISCALL skip_root(TCHAR *path);
int NSISCALL is_valid_instpath(TCHAR *s);
void NSISCALL validate_filename(TCHAR *fn);

/**
 * MoveFileOnReboot tries to move a file by the name of pszExisting to the
 * name pszNew.
 *
 * @param pszExisting The old name of the file.
 * @param pszNew The new name of the file.
 */
void NSISCALL MoveFileOnReboot(LPCTSTR pszExisting, LPCTSTR pszNew);
DWORD NSISCALL remove_ro_attr(LPCTSTR file);

#ifdef _NSIS_NODEFLIB_CRTMEMCPY
#define mini_memcpy memcpy
#else
void NSISCALL mini_memcpy(void *out, const void *in, UINT_PTR cb);
#endif

enum myGetProcAddressFunctions {
#ifndef _WIN64
  MGA_GetDiskFreeSpaceEx,
  MGA_MoveFileEx,
  MGA_GetUserDefaultUILanguage,
  MGA_RegDeleteKeyEx,
  MGA_OpenProcessToken,
  MGA_LookupPrivilegeValue,
  MGA_AdjustTokenPrivileges,
#endif
  MGA_InitiateShutdown,
  MGA_SHAutoComplete, // x64 can link to shlwapi directly but as long as MGA_SHGetFolderPath is used we can stick with myGetProcAddress
  MGA_SHGetFolderPath, // TODO: This can probably call something else directly on x64
};

void * NSISCALL myGetProcAddress(const enum myGetProcAddressFunctions func);
void NSISCALL MessageLoop(UINT uCheckedMsg);

/**
 * This function is useful for Unicode support.  Since the Windows
 * GetProcAddress function always takes a char*, this function wraps
 * the windows call and does the appropriate translation when
 * appropriate.
 *
 * @param dllHandle Handle to the DLL loaded by LoadLibraryEx.
 * @param funcName The name of the function to get the address of.
 * @return The pointer to the function.  Null if failure.
 */
void * NSISCALL NSISGetProcAddress(HANDLE dllHandle, TCHAR* funcName);

// Turn a pair of chars into a word
// Turn four chars into a dword
#ifdef __BIG_ENDIAN__ // Not very likely, but, still...
#ifdef _UNICODE
#ifdef _NSIS_NO_INT64_SHR
#define CMP4CHAR(mem, const4) ((((LPDWORD)(mem))[0] == (const4[3]|const4[2]<<16)) && (((LPDWORD)(mem))[1] == (const4[1]|const4[0]<<16)))
#else
#define CMP4CHAR(mem, const4) (*(PDWORD64)(mem) == (const4[3]|const4[2]<<16|(DWORD64)const4[1]<<32|(DWORD64)const4[0]<<48))
#endif
#define SET2CHAR(mem, const2) (*(LPDWORD)(mem) = (const2[1]|const2[0]<<16))
#else
#define CMP4CHAR(mem, const4) (*(LPDWORD)(mem) == (const4[3]|const4[2]<<8|const4[1]<<16|const4[0]<<24))
#define SET2CHAR(mem, const2) (*(LPWORD)(mem) = (const2[1]|const2[0]<<8))
#endif
#else
#ifdef _UNICODE
#ifdef _NSIS_NO_INT64_SHR
#define CMP4CHAR(mem, const4) ((((LPDWORD)(mem))[0] == (DWORD)(const4[0]|const4[1]<<16)) && (((LPDWORD)(mem))[1] == (DWORD)(const4[2]|const4[3]<<16)))
#else
#define CMP4CHAR(mem, const4) (*(PDWORD64)(mem) == (DWORD64)(const4[0]|const4[1]<<16|(DWORD64)const4[2]<<32|(DWORD64)const4[3]<<48))
#endif
#define SET2CHAR(mem, const2) (*(LPDWORD)(mem) = (const2[0]|const2[1]<<16))
#else
#define CMP4CHAR(mem, const4) (*(LPDWORD)(mem) == (DWORD)(const4[0]|const4[1]<<8|const4[2]<<16|const4[3]<<24))
#define SET2CHAR(mem, const2) (*(LPWORD)(mem) = (const2[0]|const2[1]<<8))
#endif
#endif

#endif//!___NSIS_UTIL_H___
