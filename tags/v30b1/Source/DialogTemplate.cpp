/*
 * DialogTemplate.cpp
 * 
 * This file is a part of NSIS.
 * 
 * Copyright (C) 2002 Amir Szekely <kichik@netvision.net.il>
 * 
 * Licensed under the zlib/libpng license (the "License");
 * you may not use this file except in compliance with the License.
 * 
 * Licence details can be found in the file COPYING.
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.
 */

#include "DialogTemplate.h"
#include "winchar.h"
#include <cassert> // for assert(3)
#ifndef _WIN32
#  include "util.h" // for Unicode conversion functions
#  include <stdio.h>
#  include <stdlib.h>
#  include <iconv.h>
#  include <errno.h>
#endif

using namespace std;

//////////////////////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////////////////////

static inline DWORD ConvertEndianness(DWORD d) {
  return FIX_ENDIAN_INT32(d);
}

static inline WORD ConvertEndianness(WORD w) {
  return FIX_ENDIAN_INT16(w);
}

static inline short ConvertEndianness(short s) {
  return ConvertEndianness(WORD(s));
}

#define ALIGN(dwToAlign, dwAlignOn) dwToAlign = (dwToAlign%dwAlignOn == 0) ? dwToAlign : dwToAlign - (dwToAlign%dwAlignOn) + dwAlignOn

// Reads a variant length array from seeker into readInto and advances seeker
void ReadVarLenArr(LPBYTE &seeker, WINWCHAR* &readInto, unsigned int uCodePage) {
  WORD* arr = (WORD*)seeker;
  switch (ConvertEndianness(arr[0])) {
  case 0x0000:
    readInto = 0;
    seeker += sizeof(WORD);
    break;
  case 0xFFFF:
    readInto = MAKEINTRESOURCEWINW(ConvertEndianness(arr[1]));
    seeker += 2*sizeof(WORD);
    break;
  default:
    {
      readInto = WinWStrDupFromWinWStr((WINWCHAR*) arr);
      WINWCHAR *wseeker = (WINWCHAR*) seeker;
      while (*wseeker++);
      seeker = LPBYTE(wseeker);
    }
    break;
  }
}

// A macro that writes a given string (that can be a number too) into the buffer
#define WriteStringOrId(x) \
  if (x) \
    if (IS_INTRESOURCE(x)) { \
      *(WORD*)seeker = 0xFFFF; \
      seeker += sizeof(WORD); \
      *(WORD*)seeker = ConvertEndianness(WORD((ULONG_PTR)(x))); \
      seeker += sizeof(WORD); \
    } \
    else { \
      WinWStrCpy((WINWCHAR *) seeker, x); \
      seeker += WinWStrLen((WINWCHAR *) seeker) * sizeof(WINWCHAR) + sizeof(WINWCHAR); \
    } \
  else \
    seeker += sizeof(WORD);

// A macro that adds the size of x (which can be a string a number, or nothing) to dwSize
#define AddStringOrIdSize(x) dwSize += x ? (IS_INTRESOURCE(x) ? sizeof(DWORD) : (DWORD)((WinWStrLen(x) + 1) * sizeof(WINWCHAR))) : sizeof(WORD)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDialogTemplate::CDialogTemplate(BYTE* pbData, bool build_unicode, unsigned int uCodePage) {
  m_uCodePage = uCodePage;
  m_build_unicode = build_unicode;

  m_dwHelpId = 0;
  m_szClass = 0;
  m_szFont = 0;
  m_sFontSize = 0;
  m_sFontWeight = 0;
  m_bItalic = 0;
  m_bCharset = 0;
  m_szMenu = 0;
  m_szTitle = 0;

  WORD wItems = 0;

  if (*(DWORD*)pbData == EXTENDED_DIALOG) { // Extended dialog template signature
    m_bExtended = true;

    DLGTEMPLATEEX* dTemplateEx = (DLGTEMPLATEEX*)pbData;

    m_dwHelpId = ConvertEndianness(dTemplateEx->helpID);
    m_dwStyle = ConvertEndianness(dTemplateEx->style);
    m_dwExtStyle = ConvertEndianness(dTemplateEx->exStyle);
    m_sX = ConvertEndianness(dTemplateEx->x);
    m_sY = ConvertEndianness(dTemplateEx->y);
    m_sWidth = ConvertEndianness(dTemplateEx->cx);
    m_sHeight = ConvertEndianness(dTemplateEx->cy);

    wItems = ConvertEndianness(dTemplateEx->cDlgItems);
  }
  else {
    m_bExtended = false;

    DLGTEMPLATE* dTemplate = (DLGTEMPLATE*)pbData;

    m_dwStyle = ConvertEndianness(dTemplate->style);
    m_dwExtStyle = ConvertEndianness(dTemplate->dwExtendedStyle);
    m_sX = ConvertEndianness(dTemplate->x);
    m_sY = ConvertEndianness(dTemplate->y);
    m_sWidth = ConvertEndianness(dTemplate->cx);
    m_sHeight = ConvertEndianness(dTemplate->cy);

    wItems = ConvertEndianness(dTemplate->cdit);
  }

  BYTE* seeker = pbData + (m_bExtended ? sizeof(DLGTEMPLATEEX) : sizeof(DLGTEMPLATE));

  // Read menu variant length array
  ReadVarLenArr(seeker, m_szMenu, m_uCodePage);
  // Read class variant length array
  ReadVarLenArr(seeker, m_szClass, m_uCodePage);
  // Read title variant length array
  ReadVarLenArr(seeker, m_szTitle, m_uCodePage);
  // Read font size and variant length array (only if style DS_SETFONT is used!)
  if (m_dwStyle & DS_SETFONT) {
    m_sFontSize = ConvertEndianness(*(short*)seeker);
    seeker += sizeof(short);
    if (m_bExtended) {
      m_sFontWeight = ConvertEndianness(*(short*)seeker);
      seeker += sizeof(short);
      m_bItalic = *(BYTE*)seeker;
      seeker += sizeof(BYTE);
      m_bCharset = *(BYTE*)seeker;
      seeker += sizeof(BYTE);
    }
    ReadVarLenArr(seeker, m_szFont, m_uCodePage);
  }

  // Read items
  for (int i = 0; i < wItems; i++) {
    // DLGITEMTEMPLATE[EX]s must be aligned on DWORD boundary
    if (DWORD(seeker - pbData) % sizeof(DWORD))
      seeker += sizeof(WORD);

    DialogItemTemplate* item = new DialogItemTemplate;
    ZeroMemory(item, sizeof(DialogItemTemplate));

    if (m_bExtended) {
      DLGITEMTEMPLATEEX* rawItem = (DLGITEMTEMPLATEEX*)seeker;

      item->dwHelpId = ConvertEndianness(rawItem->helpID);
      item->dwStyle = ConvertEndianness(rawItem->style);
      item->dwExtStyle = ConvertEndianness(rawItem->exStyle);
      item->sX = ConvertEndianness(rawItem->x);
      item->sY = ConvertEndianness(rawItem->y);
      item->sWidth = ConvertEndianness(rawItem->cx);
      item->sHeight = ConvertEndianness(rawItem->cy);
      item->wId = ConvertEndianness(rawItem->id);

      seeker += sizeof(DLGITEMTEMPLATEEX);
    }
    else {
      DLGITEMTEMPLATE* rawItem = (DLGITEMTEMPLATE*)seeker;

      item->dwHelpId = 0;
      item->dwStyle = ConvertEndianness(rawItem->style);
      item->dwExtStyle = ConvertEndianness(rawItem->dwExtendedStyle);
      item->sX = ConvertEndianness(rawItem->x);
      item->sY = ConvertEndianness(rawItem->y);
      item->sWidth = ConvertEndianness(rawItem->cx);
      item->sHeight = ConvertEndianness(rawItem->cy);
      item->wId = ConvertEndianness(rawItem->id);

      seeker += sizeof(DLGITEMTEMPLATE);
    }

    // Read class variant length array
    ReadVarLenArr(seeker, item->szClass, m_uCodePage);
    // Read title variant length array
    ReadVarLenArr(seeker, item->szTitle, m_uCodePage);

    // Read creation data variant length array
    // First read the size of the array (no null termination)
    item->wCreateDataSize = ConvertEndianness(*(WORD*)seeker);
    seeker += sizeof(WORD);
    // Then read the array it self (if size is not 0)
    if (item->wCreateDataSize) {
      item->wCreateDataSize -= sizeof(WORD); // Size includes size field itself...
      item->szCreationData = new char[item->wCreateDataSize];
      CopyMemory(item->szCreationData, seeker, item->wCreateDataSize);
      seeker += item->wCreateDataSize;
    }

    // Add the item to the vector
    m_vItems.push_back(item);
  }
}

static void free_template_string(WINWCHAR*s)
{
  if (!IS_INTRESOURCE(s)) free(s);
}

CDialogTemplate::~CDialogTemplate() {
  free_template_string(m_szMenu);
  free_template_string(m_szClass);
  free(m_szTitle);
  free(m_szFont);

  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    free_template_string(m_vItems[i]->szClass);
    free_template_string(m_vItems[i]->szTitle);
    if (m_vItems[i]->szCreationData) delete [] m_vItems[i]->szCreationData;
    delete m_vItems[i];
  }
}

//////////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////////

// Returns the width of the dialog
short CDialogTemplate::GetWidth() {
  return m_sWidth;
}

// Returns the height of the dialog
short CDialogTemplate::GetHeight() {
  return m_sHeight;
}

// Returns info about the item with the id wId
DialogItemTemplate* CDialogTemplate::GetItem(WORD wId) {
  for (unsigned int i = 0; i < m_vItems.size(); i++)
    if (m_vItems[i]->wId == wId)
      return m_vItems[i];
  return 0;
}

// Returns info about the item with the indexed i
DialogItemTemplate* CDialogTemplate::GetItemByIdx(DWORD i) {
  if (i >= m_vItems.size()) return 0;
  return m_vItems[i];
}

// Removes an item
// Returns 1 if removed, 0 otherwise
int CDialogTemplate::RemoveItem(WORD wId) {
  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    if (m_vItems[i]->wId == wId) {
      m_vItems.erase(m_vItems.begin() + i);
      return 1;
    }
  }
  return 0;
}

// Sets the font of the dialog
void CDialogTemplate::SetFont(TCHAR* szFaceName, WORD wFontSize) {
  m_dwStyle &= ~DS_SHELLFONT;
  if (!_tcscmp(szFaceName, _T("MS Shell Dlg"))) // TODO: "MS Shell Dlg 2"?
    m_dwStyle |= DS_SHELLFONT;
  m_dwStyle |= DS_SETFONT;
  m_bCharset = DEFAULT_CHARSET;
  free(m_szFont);
  m_szFont = WinWStrDupFromTChar(szFaceName, m_uCodePage);
  m_sFontSize = wFontSize;
}

// Adds an item to the dialog
void CDialogTemplate::AddItem(DialogItemTemplate item) {
  DialogItemTemplate* newItem = new DialogItemTemplate;
  CopyMemory(newItem, &item, sizeof(DialogItemTemplate));

  if (item.szClass && !IS_INTRESOURCE(item.szClass)) {
    newItem->szClass = WinWStrDupFromWinWStr(item.szClass);
  }
  if (item.szTitle && !IS_INTRESOURCE(item.szTitle)) {
    newItem->szTitle = WinWStrDupFromWinWStr(item.szTitle);
  }
  if (item.wCreateDataSize) {
    newItem->szCreationData = new char[item.wCreateDataSize];
    memcpy(newItem->szCreationData, item.szCreationData, item.wCreateDataSize);
  }
  m_vItems.push_back(newItem);
}

// Moves all of the items in the dialog by (x,y)
void CDialogTemplate::MoveAll(short x, short y) {
  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    m_vItems[i]->sX += x;
    m_vItems[i]->sY += y;
  }
}

// Resizes the dialog by (x,y)
void CDialogTemplate::Resize(short x, short y) {
  m_sWidth += x;
  m_sHeight += y;
}

#ifdef _WIN32
// Creates a dummy dialog that is used for converting units
HWND CDialogTemplate::CreateDummyDialog() {
  DWORD dwTemp;
  BYTE* pbDlg = Save(dwTemp);
  HWND hDlg = CreateDialogIndirect(GetModuleHandle(0), (DLGTEMPLATE*)pbDlg, 0, 0);
  FreeSavedTemplate(pbDlg);
  if (!hDlg)
    throw runtime_error("Can't create dialog from template!");

  return hDlg;
}

// Converts pixels to this dialog's units
void CDialogTemplate::PixelsToDlgUnits(short& x, short& y) {
  HWND hDlg = CreateDummyDialog();
  RECT r = {0, 0, 10000, 10000};
  MapDialogRect(hDlg, &r);
  DestroyWindow(hDlg);

  x = short(float(x) / (float(r.right)/10000));
  y = short(float(y) / (float(r.bottom)/10000));
}

void CDialogTemplate::PixelsToDlgUnits(SIZE& siz) {
  short x = (short)siz.cx, y = (short)siz.cy;
  PixelsToDlgUnits(x, y);
  siz.cx = x, siz.cy = y;
}

// Converts pixels to this dialog's units
void CDialogTemplate::DlgUnitsToPixels(short& x, short& y) {
  HWND hDlg = CreateDummyDialog();
  RECT r = {0, 0, 10000, 10000};
  MapDialogRect(hDlg, &r);
  DestroyWindow(hDlg);

  x = short(float(x) * (float(r.right)/10000));
  y = short(float(y) * (float(r.bottom)/10000));
}

// Returns the size of a string in the dialog (in dialog units)
SIZE CDialogTemplate::GetStringSize(WORD id, TCHAR *str) {
  HWND hDlg = CreateDummyDialog();

  LOGFONT f;
  GetObject((HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0), sizeof(LOGFONT), &f);

  HDC memDC = CreateCompatibleDC(GetDC(hDlg));
  HFONT font = CreateFontIndirect(&f);
  SelectObject(memDC, font);

  SIZE size;
  GetTextExtentPoint32(memDC, str, (int) _tcslen(str), &size);

  DestroyWindow(hDlg);
  DeleteObject(font);
  DeleteDC(memDC);

  PixelsToDlgUnits(size);

  return size;
}

// Trims the right margins of a control to fit a given text string size.
void CDialogTemplate::RTrimToString(WORD id, TCHAR *str, int margins) {
  DialogItemTemplate* item = GetItem(id);
  if (!item) return;

  SIZE size = GetStringSize(id, str);

  size.cx += margins;
  size.cy += 2;

  item->sWidth = short(size.cx);
  item->sHeight = short(size.cy);
}

// Trims the left margins of a control to fit a given text string size.
void CDialogTemplate::LTrimToString(WORD id, TCHAR *str, int margins) {
  DialogItemTemplate* item = GetItem(id);
  if (!item) return;

  SIZE size = GetStringSize(id, str);

  size.cx += margins;
  size.cy += 2;

  item->sX += item->sWidth - short(size.cx);
  item->sWidth = short(size.cx);
  item->sHeight = short(size.cy);
}

// Trims the left and right margins of a control to fit a given text string size.
void CDialogTemplate::CTrimToString(WORD id, TCHAR *str, int margins) {
  DialogItemTemplate* item = GetItem(id);
  if (!item) return;

  SIZE size = GetStringSize(id, str);

  size.cx += margins;
  size.cy += 2;

  item->sX += item->sWidth/2 - short(size.cx/2);
  item->sWidth = short(size.cx);
  item->sHeight = short(size.cy);
}
#endif

// Moves every item right and gives it the WS_EX_RIGHT extended style
void CDialogTemplate::ConvertToRTL() {
  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    bool addExStyle = false;
    bool addExLeftScrollbar = true;

    // Button
    if ((ULONG_PTR)(m_vItems[i]->szClass) == 0x80) {
      m_vItems[i]->dwStyle ^= BS_LEFTTEXT;
      m_vItems[i]->dwStyle ^= BS_RIGHT;
      m_vItems[i]->dwStyle ^= BS_LEFT;

      if ((m_vItems[i]->dwStyle & (BS_LEFT|BS_RIGHT)) == (BS_LEFT|BS_RIGHT)) {
        m_vItems[i]->dwStyle ^= BS_LEFT;
        m_vItems[i]->dwStyle ^= BS_RIGHT;
        if (m_vItems[i]->dwStyle & (BS_RADIOBUTTON|BS_CHECKBOX|BS_USERBUTTON)) {
          m_vItems[i]->dwStyle |= BS_RIGHT;
        }
      }
    }
    // Edit
    else if ((ULONG_PTR)(m_vItems[i]->szClass) == 0x81) {
      if ((m_vItems[i]->dwStyle & ES_CENTER) == 0) {
        m_vItems[i]->dwStyle ^= ES_RIGHT;
      }
    }
    // Static
    else if ((ULONG_PTR)(m_vItems[i]->szClass) == 0x82) {
      if ((m_vItems[i]->dwStyle & SS_TYPEMASK) == SS_LEFT || (m_vItems[i]->dwStyle & SS_TYPEMASK) == SS_LEFTNOWORDWRAP)
      {
        m_vItems[i]->dwStyle &= ~SS_TYPEMASK;
        m_vItems[i]->dwStyle |= SS_RIGHT;
      }
      else if ((m_vItems[i]->dwStyle & SS_TYPEMASK) == SS_ICON) {
        m_vItems[i]->dwStyle |= SS_CENTERIMAGE;
      }
    }
    else if (!IS_INTRESOURCE(m_vItems[i]->szClass) && !WinWStrNICmpASCII(m_vItems[i]->szClass, "RichEdit20", 10)) {
      if ((m_vItems[i]->dwStyle & ES_CENTER) == 0) {
        m_vItems[i]->dwStyle ^= ES_RIGHT;
      }
    }
    else if (!IS_INTRESOURCE(m_vItems[i]->szClass) && !WinWStrICmpASCII(m_vItems[i]->szClass, "SysTreeView32")) {
      m_vItems[i]->dwStyle |= TVS_RTLREADING;
      m_vItems[i]->dwExtStyle |= WS_EX_LAYOUTRTL;
      addExStyle = true;
      addExLeftScrollbar = false;
    }
    else if (!IS_INTRESOURCE(m_vItems[i]->szClass) && !WinWStrICmpASCII(m_vItems[i]->szClass, "SysListView32")) {
      m_vItems[i]->dwExtStyle |= WS_EX_LAYOUTRTL;
      addExLeftScrollbar = false;
    }
    else addExStyle = true;

    if (addExStyle)
      m_vItems[i]->dwExtStyle |= WS_EX_RIGHT;
    if (addExLeftScrollbar)
      m_vItems[i]->dwExtStyle |= WS_EX_LEFTSCROLLBAR;

    m_vItems[i]->dwExtStyle |= WS_EX_RTLREADING;

    m_vItems[i]->sX = m_sWidth - m_vItems[i]->sWidth - m_vItems[i]->sX;
  }
  m_dwExtStyle |= WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
}

// Saves the dialog in the form of DLGTEMPLATE[EX]
BYTE* CDialogTemplate::Save(DWORD& dwSize) {
  // We need the size first to know how much memory to allocate
  dwSize = GetSize();
  BYTE* pbDlg = new BYTE[dwSize];
  ZeroMemory(pbDlg, dwSize);
  BYTE* seeker = pbDlg;

  if (m_bExtended) {
    DLGTEMPLATEEX dh = {
      ConvertEndianness(WORD(0x0001)),
      ConvertEndianness(WORD(0xFFFF)),
      ConvertEndianness(m_dwHelpId),
      ConvertEndianness(m_dwExtStyle),
      ConvertEndianness(m_dwStyle),
      ConvertEndianness(WORD(m_vItems.size())),
      ConvertEndianness(m_sX),
      ConvertEndianness(m_sY),
      ConvertEndianness(m_sWidth),
      ConvertEndianness(m_sHeight)
    };

    CopyMemory(seeker, &dh, sizeof(DLGTEMPLATEEX));
    seeker += sizeof(DLGTEMPLATEEX);
  }
  else {
    DLGTEMPLATE dh = {
      ConvertEndianness(m_dwStyle),
      ConvertEndianness(m_dwExtStyle),
      ConvertEndianness(WORD(m_vItems.size())),
      ConvertEndianness(m_sX),
      ConvertEndianness(m_sY),
      ConvertEndianness(m_sWidth),
      ConvertEndianness(m_sHeight)
    };

    CopyMemory(seeker, &dh, sizeof(DLGTEMPLATE));
    seeker += sizeof(DLGTEMPLATE);
  }

  // Write menu variant length array
  WriteStringOrId(m_szMenu);
  // Write class variant length array
  WriteStringOrId(m_szClass);
  // Write title variant length array
  WriteStringOrId(m_szTitle);

  // Write font variant length array, size, and extended info (if needed)
  if (m_dwStyle & DS_SETFONT) {
    *(short*)seeker = ConvertEndianness(m_sFontSize);
    seeker += sizeof(short);
    if (m_bExtended) {
      *(short*)seeker = ConvertEndianness(m_sFontWeight);
      seeker += sizeof(short);
      *(BYTE*)seeker = m_bItalic;
      seeker += sizeof(BYTE);
      *(BYTE*)seeker = m_bCharset;
      seeker += sizeof(BYTE);
    }
    WriteStringOrId(m_szFont);
  }

  // Write all of the items
  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    // DLGITEMTEMPLATE[EX]s must be aligned on DWORD boundary
    if ((seeker - pbDlg) % sizeof(DWORD))
      seeker += sizeof(WORD);

    if (m_bExtended) {
      DLGITEMTEMPLATEEX dih = {
        ConvertEndianness(m_vItems[i]->dwHelpId),
        ConvertEndianness(m_vItems[i]->dwExtStyle),
        ConvertEndianness(m_vItems[i]->dwStyle),
        ConvertEndianness(m_vItems[i]->sX),
        ConvertEndianness(m_vItems[i]->sY),
        ConvertEndianness(m_vItems[i]->sWidth),
        ConvertEndianness(m_vItems[i]->sHeight),
        ConvertEndianness(m_vItems[i]->wId)
      };

      CopyMemory(seeker, &dih, sizeof(DLGITEMTEMPLATEEX));
      seeker += sizeof(DLGITEMTEMPLATEEX);
    }
    else {
      DLGITEMTEMPLATE dih = {
        ConvertEndianness(m_vItems[i]->dwStyle),
        ConvertEndianness(m_vItems[i]->dwExtStyle),
        ConvertEndianness(m_vItems[i]->sX),
        ConvertEndianness(m_vItems[i]->sY),
        ConvertEndianness(m_vItems[i]->sWidth),
        ConvertEndianness(m_vItems[i]->sHeight),
        ConvertEndianness(m_vItems[i]->wId)
      };

      CopyMemory(seeker, &dih, sizeof(DLGITEMTEMPLATE));
      seeker += sizeof(DLGITEMTEMPLATE);
    }

    // Write class variant length array
    const WINWCHAR *szClass = m_vItems[i]->szClass;
#ifdef _UNICODE
    static const WINWCHAR clsRE20W[] = {'R','i','c','h','E','d','i','t','2','0','W',0};
    static const WINWCHAR clsRE20A[] = {'R','i','c','h','E','d','i','t','2','0','A',0};
    if (!IS_INTRESOURCE(szClass)) {
      // transmute RichEdit20A/W control into RichEdit20T that matches the target
      if (m_build_unicode && !WinWStrICmpASCII(szClass, "RichEdit20A")) szClass = clsRE20W;
      if (!m_build_unicode && !WinWStrICmpASCII(szClass, "RichEdit20W")) szClass = clsRE20A;
    }
#endif
    WriteStringOrId(szClass);
    // Write title variant length array
    WriteStringOrId(m_vItems[i]->szTitle);

    // Write creation data variant length array
    // First write its size
    WORD wCreateDataSize = m_vItems[i]->wCreateDataSize;
    if (m_vItems[i]->wCreateDataSize) wCreateDataSize += sizeof(WORD);
    *(WORD*)seeker = ConvertEndianness(wCreateDataSize);
    seeker += sizeof(WORD);
    // If size is nonzero write the data too
    if (m_vItems[i]->wCreateDataSize) {
      CopyMemory(seeker, m_vItems[i]->szCreationData, m_vItems[i]->wCreateDataSize);
      seeker += m_vItems[i]->wCreateDataSize;
    }
  }

  assert((DWORD)(seeker - pbDlg) == dwSize);

  // DONE!
  return pbDlg;
}

// Returns the size that the DLGTEMPLATE[EX] will take when saved
DWORD CDialogTemplate::GetSize() {
  DWORD dwSize = m_bExtended ? sizeof(DLGTEMPLATEEX) : sizeof(DLGTEMPLATE);

  // Menu
  AddStringOrIdSize(m_szMenu);
  // Class
  AddStringOrIdSize(m_szClass);
  // Title
  AddStringOrIdSize(m_szTitle);

  // Font
  if (m_dwStyle & DS_SETFONT) {
    dwSize += sizeof(WORD) + (m_bExtended ? sizeof(short) + 2*sizeof(BYTE) : 0);
    AddStringOrIdSize(m_szFont);
  }

  for (unsigned int i = 0; i < m_vItems.size(); i++) {
    // DLGITEMTEMPLATE[EX]s must be aligned on DWORD boundary
    ALIGN(dwSize, sizeof(DWORD));

    dwSize += m_bExtended ? sizeof(DLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE);

    // Class
    AddStringOrIdSize(m_vItems[i]->szClass);
    // Title
    AddStringOrIdSize(m_vItems[i]->szTitle);

    dwSize += sizeof(WORD) + m_vItems[i]->wCreateDataSize;
  }

  return dwSize;
}
