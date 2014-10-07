//---------------------------------------------------------------------------
// GlobalTypes.h
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
// Reviewed for Unicode support by Jim Park -- 08/29/2007

#if !defined(GlobalTypes_H)
  #define GlobalTypes_H

  #ifndef _MSC_VER
    #include <stdint.h>
  #endif
  #include <iostream>
  #include <fstream>
  #include <ios>
  #include <string>

  using namespace std;

  #ifdef _MSC_VER
    typedef unsigned char uint8_t;
    typedef unsigned __int32 uint32_t;
    typedef unsigned __int64 uint64_t;
    #define CHECKSUM_BLOCK unsigned __int64
    #define __WIN32__
  #else
    #define CHECKSUM_BLOCK unsigned long long
  #endif

  typedef uint32_t TFileOffset;

  // This is a hacky partial replacement for <i|o>[f]stream so we can open wchar_t*
  #include "tchar.h"
  #include <stdio.h>
  #include <assert.h>

class simplebfstream {
  FILE*m_File;
  ios_base::iostate m_state;
  streamsize m_LastReadCount;
public:
  simplebfstream() : m_File(0), m_state(ios_base::badbit|ios_base::failbit) {}
  ~simplebfstream() 
  {
    if (m_File) fclose(m_File);
  }

  bool open(const TCHAR*filename, ios_base::openmode mode)
  {
    TCHAR mAcc, mFmt = _T('b');
    if (ios::in&mode) mAcc = _T('r');
    if (ios::out&mode) mAcc = _T('w');
    assert(0==(mode&~(ios::in|ios::binary|ios::out)));

    TCHAR modestr[3] = {mAcc, mFmt, _T('\0')};
    m_File = FOPEN(filename, modestr);
    m_state = m_File ? ios_base::goodbit : ios_base::badbit|ios_base::failbit;

    return good();
  }

  void close() 
  {
    if (!m_File || fclose(m_File))
    {
      m_state |= ios_base::failbit;
    }
    m_File = 0;
  }

  bool is_open() const {return !!m_File;}
  bool eof() const {return !!(ios_base::eofbit & m_state);}
  bool bad() const {return !!(ios_base::badbit & m_state);}
  bool fail() const {return !!((ios_base::failbit|ios_base::badbit) & m_state);}
  bool good() const {return ios_base::goodbit==m_state;}

  streamsize gcount() const {return m_LastReadCount;}
  streampos tellg() const {return ftell(m_File);}

  simplebfstream& read(char*s,streamsize n) 
  {
    size_t cbio = fread(s, 1, n, m_File);
    m_LastReadCount = cbio;
    if (cbio != (size_t)n)
    {
      m_state |= ferror(m_File) ? ios_base::badbit : (ios_base::eofbit|ios_base::failbit);
    }
    return *this;
  }

  simplebfstream& seekg(streamoff off, ios_base::seekdir dir)
  {
    int origin = ios_base::beg==dir ? SEEK_SET : ios_base::cur==dir ? SEEK_CUR : SEEK_END;
    if (fseek(m_File, off, origin))
    {
      // BUGBUG: Does not follow standard 
       m_state |= ios_base::badbit|ios_base::failbit;
    }
    return *this;
  }

  simplebfstream& seekp(streamoff off, ios_base::seekdir dir) {return seekg(off, dir);}
  streampos tellp() const {return tellg();}

  simplebfstream& write(const char* s, streamsize n)
  {
    size_t cbio = fwrite(s, 1, n, m_File);
    if (cbio != (size_t)n) m_state |= ios_base::badbit;
    return *this;
  }

  bool operator ! () const {return fail();}
};

  typedef simplebfstream bistream;
  typedef bistream bifstream;
  typedef simplebfstream bostream;
  typedef bostream bofstream;

#endif // GlobalTypes_H
