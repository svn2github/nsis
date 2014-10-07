/*
 * bgbg.c
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
 * Reviewed for Unicode support by Jim Park -- 08/22/2007
 */

#include "../Platform.h"
#include "resource.h"
#include "config.h"
#include "fileform.h"
#include "state.h"
#include "ui.h"
#include "util.h"

#ifdef NSIS_SUPPORT_BGBG

#define c1 header->bg_color1
#define c2 header->bg_color2

LRESULT CALLBACK BG_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_WINDOWPOSCHANGING:
      {
        LPWINDOWPOS wp = (LPWINDOWPOS) lParam;
        wp->flags |= SWP_NOACTIVATE;
        wp->hwndInsertAfter = g_hwnd;
        break;
      }
    case WM_PAINT:
      {
        header *header = g_header;

        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(hwnd,&ps);
        RECT r;
        LOGBRUSH lh;
        int ry;

        lh.lbStyle = BS_SOLID;

        GetClientRect(hwnd,&r);
        // this portion by Drew Davidson, drewdavidson@mindspring.com
        ry=r.bottom;
        r.bottom=0;

        // JF: made slower, reduced to 4 pixels high, because I like how it looks better/
        while (r.top < ry)
        {
          int rv,gv,bv;
          HBRUSH brush;
          rv = (GetRValue(c2) * r.top + GetRValue(c1) * (ry-r.top)) / ry;
          gv = (GetGValue(c2) * r.top + GetGValue(c1) * (ry-r.top)) / ry;
          bv = (GetBValue(c2) * r.top + GetBValue(c1) * (ry-r.top)) / ry;
          lh.lbColor = RGB(rv,gv,bv);
          brush = CreateBrushIndirect(&lh);
          // note that we don't need to do "SelectObject(hdc, brush)"
          // because FillRect lets us specify the brush as a parameter.
          r.bottom+=4;
          FillRect(hdc, &r, brush);
          DeleteObject(brush);
          r.top+=4;
        }

        if (header->bg_textcolor != -1)
        {
          HFONT newFont = CreateFontIndirect((LOGFONT *) header->blocks[NB_BGFONT].offset);
          if (newFont)
          {
            HFONT oldFont;
            r.left=16;
            r.top=8;
            SetBkMode(hdc,TRANSPARENT);
            SetTextColor(hdc,header->bg_textcolor);
            oldFont = SelectObject(hdc,newFont);
            DrawText(hdc,g_caption,-1,&r,DT_TOP|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
            SelectObject(hdc,oldFont);
            DeleteObject(newFont);
          }
        }
        EndPaint(hwnd,&ps);
      }
      return 0;
  }
  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

#endif //NSIS_SUPPORT_BGBG
