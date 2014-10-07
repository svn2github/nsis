/*
 * ResourceEditor.h
 * 
 * This file is a part of NSIS.
 * 
 * Copyright (C) 2002-2014 Amir Szekely <kichik@users.sourceforge.net>
 * 
 * Licensed under the zlib/libpng license (the "License");
 * you may not use this file except in compliance with the License.
 * 
 * Licence details can be found in the file COPYING.
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.
 *
 * Reviewed for Unicode support by Jim Park -- 08/21/2007
 */

#if !defined(AFX_RESOURCEEDITOR_H__683BF710_E805_4093_975B_D5729186A89A__INCLUDED_)
#define AFX_RESOURCEEDITOR_H__683BF710_E805_4093_975B_D5729186A89A__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Platform.h"
#include "winchar.h"
#include <vector>
#include <cassert>

#ifdef _WIN32
#include <winnt.h>
#else
// all definitions for non Win32 platforms were taken from MinGW's free Win32 library
#  define IMAGE_DIRECTORY_ENTRY_RESOURCE  2
#  define IMAGE_SCN_MEM_DISCARDABLE 0x2000000
#  pragma pack(4)
typedef struct _IMAGE_RESOURCE_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD MajorVersion;
  WORD MinorVersion;
  WORD NumberOfNamedEntries;
  WORD NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;
typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
  DWORD OffsetToData;
  DWORD Size;
  DWORD CodePage;
  DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;
typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
  WORD Length;
  CHAR NameString[1];
} IMAGE_RESOURCE_DIRECTORY_STRING,*PIMAGE_RESOURCE_DIRECTORY_STRING;
typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
  WORD Length;
  WCHAR NameString[1];
} IMAGE_RESOURCE_DIR_STRING_U,*PIMAGE_RESOURCE_DIR_STRING_U;
#  pragma pack()
#endif

#pragma pack(4)
typedef struct _MY_IMAGE_RESOURCE_DIRECTORY_ENTRY {
  union {
    struct {
#ifndef __BIG_ENDIAN__
      DWORD NameOffset:31;
      DWORD NameIsString:1;
#else
      DWORD NameIsString:1;
      DWORD NameOffset:31;
#endif
    } NameString;
    DWORD Name;
    WORD Id;
  } UName;
  union {
    DWORD OffsetToData;
    struct {
#ifndef __BIG_ENDIAN__
      DWORD OffsetToDirectory:31;
      DWORD DataIsDirectory:1;
#else
      DWORD DataIsDirectory:1;
      DWORD OffsetToDirectory:31;
#endif
    } DirectoryOffset;
  } UOffset;
} MY_IMAGE_RESOURCE_DIRECTORY_ENTRY,*PMY_IMAGE_RESOURCE_DIRECTORY_ENTRY;

#pragma pack()

#include <stdexcept>

// classes
class CResourceDirectory;
class CResourceDirectoryEntry;
class CResourceDataEntry;

// Resource directory with entries
typedef struct RESOURCE_DIRECTORY {
  IMAGE_RESOURCE_DIRECTORY Header;
  MY_IMAGE_RESOURCE_DIRECTORY_ENTRY Entries[1];
} *PRESOURCE_DIRECTORY;

#define GetMemberFromOptionalHeader(optionalHeader, member) \
    ( (optionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) ? \
      &((PIMAGE_OPTIONAL_HEADER32)&optionalHeader)->member : \
      &((PIMAGE_OPTIONAL_HEADER64)&optionalHeader)->member \
    )
class CResourceEditor {
public:
  CResourceEditor(BYTE* pbPE, int iSize, bool bKeepData = true);
  virtual ~CResourceEditor();

  // On POSIX+Unicode GetResource(RT_VERSION,..) is not TCHAR nor WINWCHAR, it is WCHAR/UINT16.
  // If it passes IS_INTRESOURCE we must allow it.
  // Use TCHAR* for real strings. If you need to pass in a WINWCHAR*, make GetResourceW public...
  template<class T> bool UpdateResource(const T*Type, WORD Name, LANGID Lang, BYTE*Data, DWORD Size)
  {
    if (sizeof(T) != sizeof(TCHAR) && !IS_INTRESOURCE(Type))
    {
      assert(IS_INTRESOURCE(Type));
      return false;
    }
    return UpdateResourceT((const TCHAR*) Type, Name, Lang, Data, Size);
  }
  template<class T> BYTE* GetResource(const T*Type, WORD Name, LANGID Lang)
  {
    if (sizeof(T) != sizeof(TCHAR) && !IS_INTRESOURCE(Type))
    {
      assert(IS_INTRESOURCE(Type));
      return NULL;
    }
    return GetResourceT((const TCHAR*) Type, Name, Lang);
  }
  template<class T> int GetResourceSize(const T*Type, WORD Name, LANGID Lang)
  {
    if (sizeof(T) != sizeof(TCHAR) && !IS_INTRESOURCE(Type))
    {
      assert(IS_INTRESOURCE(Type));
      return -1;
    }
    return GetResourceSizeT((const TCHAR*) Type, Name, Lang);
  }
  template<class T> DWORD GetResourceOffset(const T*Type, WORD Name, LANGID Lang)
  {
    if (sizeof(T) != sizeof(TCHAR) && !IS_INTRESOURCE(Type))
    {
      assert(IS_INTRESOURCE(Type));
      return -1;
    }
    return GetResourceOffsetT((const TCHAR*) Type, Name, Lang);
  }

  bool  UpdateResourceT   (const TCHAR* szType, WORD szName, LANGID wLanguage, BYTE* lpData, DWORD dwSize);
  BYTE* GetResourceT      (const TCHAR* szType, WORD szName, LANGID wLanguage);
  int   GetResourceSizeT  (const TCHAR* szType, WORD szName, LANGID wLanguage);
  DWORD GetResourceOffsetT(const TCHAR* szType, WORD szName, LANGID wLanguage);
  void  FreeResource(BYTE* pbResource);

  // The section name must be in ASCII.
  bool  SetPESectionVirtualSize(const char* pszSectionName, DWORD newsize);
  DWORD Save(BYTE* pbBuf, DWORD &dwSize);

  // utitlity functions
  static PIMAGE_NT_HEADERS GetNTHeaders(BYTE* pbPE);

  static PRESOURCE_DIRECTORY GetResourceDirectory(
    BYTE* pbPE,
    DWORD dwSize,
    PIMAGE_NT_HEADERS ntHeaders,
    DWORD *pdwResSecVA = NULL,
    DWORD *pdwSectionIndex = NULL
  );
private:
  bool  UpdateResourceW(const WINWCHAR* szType, WINWCHAR* szName, LANGID wLanguage, BYTE* lpData, DWORD dwSize);
  BYTE* GetResourceW(const WINWCHAR* szType, WINWCHAR* szName, LANGID wLanguage);
  int   GetResourceSizeW(const WINWCHAR* szType, WINWCHAR* szName, LANGID wLanguage);
  DWORD GetResourceOffsetW(const WINWCHAR* szType, WINWCHAR* szName, LANGID wLanguage);

private:
  BYTE* m_pbPE;
  int   m_iSize;
  bool  m_bKeepData;

  PIMAGE_NT_HEADERS m_ntHeaders;

  DWORD m_dwResourceSectionIndex;
  DWORD m_dwResourceSectionVA;

  CResourceDirectory* m_cResDir;

  CResourceDirectory* ScanDirectory(PRESOURCE_DIRECTORY rdRoot, PRESOURCE_DIRECTORY rdToScan);

  void WriteRsrcSec(BYTE* pbRsrcSec);
  void SetOffsets(CResourceDirectory* resDir, ULONG_PTR newResDirAt);

  DWORD AdjustVA(DWORD dwVirtualAddress, DWORD dwAdjustment);
  DWORD AlignVA(DWORD dwVirtualAddress);
};

class CResourceDirectory {
public:
  CResourceDirectory(PIMAGE_RESOURCE_DIRECTORY prd);
  virtual ~CResourceDirectory();

  IMAGE_RESOURCE_DIRECTORY GetInfo();

  CResourceDirectoryEntry* GetEntry(unsigned int i);
  bool AddEntry(CResourceDirectoryEntry* entry);
  void RemoveEntry(int i);
  unsigned int  CountEntries();
  int  Find(const WINWCHAR* szName);
  int  Find(WORD wId);

  DWORD GetSize();

  void Destroy();

  ULONG_PTR m_ulWrittenAt;

private:
  IMAGE_RESOURCE_DIRECTORY m_rdDir;
  std::vector<CResourceDirectoryEntry*> m_vEntries;
};

class CResourceDirectoryEntry {
public:
  CResourceDirectoryEntry(const WINWCHAR* szName, CResourceDirectory* rdSubDir);
  CResourceDirectoryEntry(const WINWCHAR* szName, CResourceDataEntry* rdeData);
  virtual ~CResourceDirectoryEntry();

  bool HasName();
  WINWCHAR* GetName();
  int GetNameLength();

  WORD GetId();

  bool IsDataDirectory();
  CResourceDirectory* GetSubDirectory();

  CResourceDataEntry* GetDataEntry();

  ULONG_PTR m_ulWrittenAt;

private:
  bool m_bHasName;
  WINWCHAR* m_szName;
  WORD m_wId;

  bool m_bIsDataDirectory;
  union {
    CResourceDirectory* m_rdSubDir;
    CResourceDataEntry* m_rdeData;
  };
};

class CResourceDataEntry {
public:
  CResourceDataEntry(BYTE* pbData, DWORD dwSize, DWORD dwCodePage = 0, DWORD dwOffset = DWORD(-1));
  ~CResourceDataEntry();

  BYTE* GetData();

  void SetData(BYTE* pbData, DWORD dwSize);
  void SetData(BYTE* pbData, DWORD dwSize, DWORD dwCodePage);

  DWORD GetSize();
  DWORD GetCodePage();
  DWORD GetOffset();

  ULONG_PTR m_ulWrittenAt;

private:
  BYTE* m_pbData;
  DWORD m_dwSize;
  DWORD m_dwCodePage;
  DWORD m_dwOffset;
};

#endif // !defined(AFX_RESOURCEEDITOR_H__683BF710_E805_4093_975B_D5729186A89A__INCLUDED_)
