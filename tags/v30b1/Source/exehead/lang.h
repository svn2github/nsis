/*
 * lang.c
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

#ifndef _NSIS_LANG_H_
#define _NSIS_LANG_H_


// generic startup strings (these will never be overridable)
#define _LANG_INVALIDCRC _T("Installer integrity check has failed. Common causes include\n") \
                         _T("incomplete download and damaged media. Contact the\n") \
                         _T("installer's author to obtain a new copy.\n\n") \
                         _T("More information at:\n") \
                         _T("http://nsis.sf.net/NSIS_Error")

#define _LANG_ERRORWRITINGTEMP _T("Error writing temporary file. Make sure your temp folder is valid.")

#define _LANG_UNINSTINITERROR _T("Error launching installer")

#define _LANG_VERIFYINGINST _T("verifying installer: %d%%")

#define _LANG_UNPACKING _T("unpacking data: %d%%")

#define _LANG_CANTOPENSELF _T("Error launching installer") // same as uninstiniterror for size

#define _LANG_GENERIC_ERROR _T("NSIS Error")

// We store index to the current language table as a negative
// index value - 1.  So this macro, undoes that into a valid
// index.
#define LANG_STR_TAB(x)             cur_langtable[-((int)x+1)]

#define LANG_BRANDING               -1
#define LANG_CAPTION                -2
#define LANG_NAME                   -3
#define LANG_SPACE_AVAIL            -4
#define LANG_SPACE_REQ              -5
#define LANG_CANTWRITE              -6
#define LANG_COPYFAILED             -7
#define LANG_COPYTO                 -8
#define LANG_CANNOTFINDSYMBOL       -9
#define LANG_COULDNOTLOAD           -10
#define LANG_CREATEDIR              -11
#define LANG_CREATESHORTCUT         -12
#define LANG_CREATEDUNINST          -13
#define LANG_DELETEFILE             -14
#define LANG_DELETEONREBOOT         -15
#define LANG_ERRORCREATINGSHORTCUT  -16
#define LANG_ERRORCREATING          -17
#define LANG_ERRORDECOMPRESSING     -18
#define LANG_DLLREGERROR            -19
#define LANG_EXECSHELL              -20
#define LANG_EXECUTE                -21
#define LANG_EXTRACT                -22
#define LANG_ERRORWRITING           -23
#define LANG_INSTCORRUPTED          -24
#define LANG_NOOLE                  -25
#define LANG_OUTPUTDIR              -26
#define LANG_REMOVEDIR              -27
#define LANG_RENAMEONREBOOT         -28
#define LANG_RENAME                 -29
#define LANG_SKIPPED                -30
#define LANG_COPYDETAILS            -31
#define LANG_LOG_INSTALL_PROCESS    -32
#define LANG_BYTE                   -33
#define LANG_KILO                   -34
#define LANG_MEGA                   -35
#define LANG_GIGA                   -36

#endif//_NSIS_LANG_H_
