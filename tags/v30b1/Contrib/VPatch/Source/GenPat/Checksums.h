//---------------------------------------------------------------------------
// Checksums.h
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

#if !defined(Checksums_H)
  #define Checksums_H

  #include "md5.h"
  #include <string>
  #include "GlobalTypes.h"
  #include "tchar.h"

  typedef uint32_t crc32_t;

  class TChecksum {
  public:
    md5_byte_t digest[16];
    crc32_t crc;
    enum { CRC32, MD5 } mode;

    TChecksum() : mode(MD5) { }
    TChecksum(tstring& fileName);

    void loadMD5(md5_byte_t newdigest[16]);
    void loadCRC32(crc32_t newcrc);

    bool operator==(const TChecksum& b);
  };

#endif // Checksums_H
