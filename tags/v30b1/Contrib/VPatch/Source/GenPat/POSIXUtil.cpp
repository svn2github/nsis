//---------------------------------------------------------------------------
// POSIXUtil.cpp
//---------------------------------------------------------------------------
//                           -=* VPatch *=-
//---------------------------------------------------------------------------
// Copyright (C) 2001-2005 Koen van de Sande / Van de Sande Productions
//---------------------------------------------------------------------------
// Website: http://www.tibed.net/vpatch
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Unicode support by Jim Park -- 08/29/2007


#include "POSIXUtil.h"
#include <iostream>
#include <fstream>

#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* My Borland/VC++ compiler do not have this file */
#if !defined(__BORLANDC__) && !defined(_MSC_VER)
  #include <unistd.h>
#endif

#ifdef __WIN32__     /* The Windows way of getting a temp file needs windows.h*/
  #include <windows.h>
#endif

using namespace std;

namespace POSIX {

  void TimeT_To_FILETIME(time_t t, ALT_FILETIME *pft ) {
      uint64_t ll = (((uint64_t)t) *10000000L) + (((uint64_t)116444736L) * 1000000000L);
      pft->dwLowDateTime = (uint32_t)(ll & 0xFFFFFFFF);
      pft->dwHighDateTime = (uint32_t)((ll>>32)&0xFFFFFFFF);
  }

#ifdef __WIN32__
  /* do it the old way on Win32, because POSIX does not get timezone stuff right */
  ALT_FILETIME getFileTime(const TCHAR* sFileName) {
    FILETIME temp;
    GetSystemTimeAsFileTime(&temp);
    HANDLE h = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {
      terr << _T("Cannot read file time of ") << sFileName << _T("\n");
    } else {
      GetFileTime(h, NULL, NULL, &temp);
      CloseHandle(h);
    }
    ALT_FILETIME result;
    result.dwHighDateTime = temp.dwHighDateTime;
    result.dwLowDateTime = temp.dwLowDateTime;
    return result;
  }
#else
  ALT_FILETIME getFileTime(const char* sFileName) {
    struct stat buf;
    /* get current time first as a fall-back */
    time_t currentTime = time(NULL);

    if(stat(sFileName, &buf)) {
      terr << _T("Cannot read file time of ") << sFileName << _T("\n");
    } else {
      /* get the time from the file */
      currentTime = buf.st_mtime;
    }
    ALT_FILETIME result;
    TimeT_To_FILETIME(currentTime, &result);
    return result;
  }
#endif

  uint32_t getFileSize(const TCHAR* sFileName) {
    bifstream f;
    f.open(sFileName, std::ios_base::binary | std::ios_base::in);
    if (!f.good() || f.eof() || !f.is_open()) {
      throw _T("File could not be read (getFileSize)");
    }
    f.seekg(0, std::ios_base::beg);
    std::ifstream::pos_type begin_pos = f.tellg();
    f.seekg(0, std::ios_base::end);
    return static_cast<int>(f.tellg() - begin_pos);
  }

#ifdef __WIN32__
  tstring getTempFile() {
    TCHAR buffer[MAX_PATH];
    if(GetTempFileName(_T("."),_T("vpatch"),0,buffer) == 0) {
      terr << _T("Cannot create temporary filename");
    }
    return tstring(buffer);
  }
#else
  tstring getTempFile() {
    TCHAR t[] = _T("/tmp/genpatXXXXXX");

    mode_t old_umask = umask(0077);

    int fd = mkstemp(t);
    if (fd == -1) {
      terr << _T("Cannot create temporary filename");
      return _T("");
    }
    close(fd);

    umask(old_umask);

    return tstring(t);
  }
#endif

}
