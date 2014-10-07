/*
 * ui.h
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
 * Unicode support by Jim Park -- 08/13/2007
 */


#ifndef _UI_H_
#define _UI_H_

extern int *cur_langtable;

extern int NSISCALL ui_doinstall(void);
void NSISCALL update_status_text(int strtab, const TCHAR *text2);

extern int ui_dlg_visible;
extern HWND m_curwnd;

#ifdef NSIS_CONFIG_LOG
void NSISCALL build_g_logfile(void);
#endif

// sent to the last child window to tell it that the install thread is done
#define WM_NOTIFY_INSTPROC_DONE (WM_USER+0x4)

// sent to every child window to tell it it can start executing NSIS code
#define WM_NOTIFY_START (WM_USER+0x5)

// sent to every child window to tell it it is closing soon
#define WM_NOTIFY_INIGO_MONTOYA (WM_USER+0xb)

// update message used by DirProc and SelProc for space display
#define WM_IN_UPDATEMSG (WM_USER+0xf)

// simulates clicking on the tree
#define WM_TREEVIEW_KEYHACK (WM_USER+0x13)

// notifies a component selection change (.onMouseOverSection)
#define WM_NOTIFY_SELCHANGE (WM_USER+0x19)

// Notifies the installation type has changed by the user
#define WM_NOTIFY_INSTTYPE_CHANGED (WM_USER+0x20)

#endif//_UI_H_
