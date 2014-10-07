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
 * Unicode support by Jim Park -- 08/10/2007
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include "tstring.h" // for std::string

#include "boost/scoped_ptr.hpp" // for boost::scoped_ptr
#include "ResourceEditor.h"

#ifndef _WIN32
#  include <iconv.h>
#  include <stdio.h>
#  include <unistd.h>
#endif

#include <stdarg.h>

extern double my_wtof(const wchar_t *str);
extern size_t my_strncpy(TCHAR*Dest, const TCHAR*Src, size_t cchMax);
size_t my_strftime(TCHAR *s, size_t max, const TCHAR  *fmt, const struct tm *tm);

// Adds the bitmap in filename using resource editor re as id id.
// If width or height are specified it will also make sure the bitmap is in that size
int update_bitmap(CResourceEditor* re, WORD id, const TCHAR* filename, int width=0, int height=0, int maxbpp=0);

bool GetDLLVersion(const tstring& filepath, DWORD& high, DWORD& low);

tstring get_full_path(const tstring& path);
tstring get_dir_name(const tstring& path);
tstring get_file_name(const tstring& path);
tstring get_executable_path(const TCHAR* argv0);
tstring get_executable_dir(const TCHAR *argv0);
tstring remove_file_extension(const tstring& path);
tstring& path_append_separator(tstring& path);
tstring& path_append(tstring& base, const TCHAR* more);
inline tstring& path_append(tstring& base, const tstring& more) { return path_append(base, more.c_str()); }
#ifdef _WIN32
#define IsPathSeparator IsAgnosticPathSeparator
#else
#define IsPathSeparator(c) ( PLATFORM_PATH_SEPARATOR_C == (c) )
#endif
inline bool IsAgnosticPathSeparator(const TCHAR c) { return _T('\\') == c || _T('/') == c; }
bool IsWindowsPathRelative(const TCHAR *p);

tstring lowercase(const tstring&);
tstring get_string_prefix(const tstring& str, const tstring& separator);
tstring get_string_suffix(const tstring& str, const tstring& separator);
void RawTStrToASCII(const TCHAR*in,char*out,UINT maxcch);
size_t ExpandoStrFmtVaList(wchar_t*Stack, size_t cchStack, wchar_t**ppMalloc, const wchar_t*FmtStr, va_list Args);

template<typename T, size_t S> class ExpandoString {
  T m_stack[S ? S : 1], *m_heap;
public:
  ExpandoString() : m_heap(0) {}
  ~ExpandoString() { free(m_heap); }
  void Reserve(size_t cch)
  {
    if (S && cch <= S) return ;
    void *p = realloc(m_heap, cch * sizeof(T));
    if (!p) throw std::bad_alloc();
    m_heap = (T*) p;
  }
  size_t StrFmt(const T*FmtStr, va_list Args, bool throwonerr = true)
  {
    size_t n = ExpandoStrFmtVaList(m_stack, COUNTOF(m_stack), &m_heap, FmtStr, Args);
    if (throwonerr && !n && *FmtStr) throw std::bad_alloc();
    return n;
  }
  T* GetPtr() { return m_heap ? m_heap : m_stack; }
  operator T*() { return GetPtr(); }
};

typedef enum {
  GFSF_BYTESIFPOSSIBLE = 0x1,
  GFSF_HIDEBYTESCALE   = 0x2,
  GFSF_DEFAULT = 0
} GETFRIENDLYSIZEFLAGS;
const TCHAR* GetFriendlySize(UINT64 n, unsigned int&fn, GETFRIENDLYSIZEFLAGS f = GFSF_DEFAULT);
class FriendlySize {
  const TCHAR* m_s;
  unsigned int m_n;
public:
  FriendlySize(UINT64 n, size_t f = GFSF_DEFAULT) { Calculate(n, f); }
  void Calculate(UINT64 n, size_t f) { m_s = GetFriendlySize(n, m_n, (GETFRIENDLYSIZEFLAGS)f); }
  unsigned int UInt() const { return m_n; }
  const TCHAR* Scale() const { return m_s; }
};

int sane_system(const TCHAR *command);

void PrintColorFmtMsg(unsigned int type, const TCHAR *fmtstr, va_list args);
void FlushOutputAndResetPrintColor();
#ifdef _WIN32
#ifdef _UNICODE
int RunChildProcessRedirected(LPCWSTR cmdprefix, LPCWSTR cmdmain);
inline int RunChildProcessRedirected(LPCWSTR cmd) { return RunChildProcessRedirected(0, cmd); }
#ifdef MAKENSIS
typedef struct {
  HANDLE hNative;
  FILE*hCRT;
  WORD cp;
  signed char mode; // -1 = redirected, 0 = unknown, 1 = console
  bool mustwritebom;
} WINSIO_OSDATA;
inline bool WinStdIO_IsConsole(WINSIO_OSDATA&osd) { return osd.mode > 0; }
inline bool WinStdIO_IsRedirected(WINSIO_OSDATA&osd) { return osd.mode < 0; }
bool WINAPI WinStdIO_OStreamInit(WINSIO_OSDATA&osd, FILE*strm, WORD cp, int bom = 1);
bool WINAPI WinStdIO_OStreamWrite(WINSIO_OSDATA&osd, const wchar_t *Str, UINT cch = -1);
int WINAPI WinStdIO_vfwprintf(FILE*strm, const wchar_t*Fmt, va_list val);
int WinStdIO_fwprintf(FILE*strm, const wchar_t*Fmt, ...);
int WinStdIO_wprintf(const wchar_t*Fmt, ...);
#ifndef _MSC_VER // our tchar.h already defined everything...
#include <tchar.h> // Make sure we include the CRTs tchar.h in case it is pulled in by something else later.
#endif
// We don't hook fflush since the native handle is only used with WriteConsoleW
#undef _vsntprintf
#define _vsntprintf Error: TODO
#undef _tprintf
#define _tprintf WinStdIO_wprintf
#undef _ftprintf
#define _ftprintf WinStdIO_fwprintf
#undef _vftprintf
#define _vftprintf WinStdIO_vfwprintf
#endif // ~MAKENSIS
#else
int RunChildProcessRedirected(LPCWSTR cmd);
#endif // ~_UNICODE
#define ResetPrintColor() FlushOutputAndResetPrintColor() // For reset ONLY, use PrintColorFmtMsg(0,NULL ...
#define SetPrintColorWARN() PrintColorFmtMsg(1|0x10, NULL, (va_list)NULL)
#define SetPrintColorERR() PrintColorFmtMsg(2|0x10, NULL, (va_list)NULL)
#else
#define ResetPrintColor()
#define SetPrintColorWARN()
#define SetPrintColorERR()
#endif // ~_WIN32
inline void PrintColorFmtMsg_WARN(const TCHAR *fmtstr, ...)
{
  va_list val;
  va_start(val,fmtstr);
  PrintColorFmtMsg(1, fmtstr, val);
  va_end(val);
}
inline void PrintColorFmtMsg_ERR(const TCHAR *fmtstr, ...)
{
  va_list val;
  va_start(val,fmtstr);
  PrintColorFmtMsg(2, fmtstr, val);
  va_end(val);
}


#ifndef _WIN32
// iconv const inconsistency workaround by Alexandre Oliva
template <typename T>
inline size_t nsis_iconv_adaptor
  (size_t (*iconv_func)(iconv_t, T, size_t *, char**,size_t*),
  iconv_t cd, char **inbuf, size_t *inbytesleft,
  char **outbuf, size_t *outbytesleft)
{
  return iconv_func (cd, (T)inbuf, inbytesleft, outbuf, outbytesleft);
}

const char* nsis_iconv_get_host_endian_ucs4_code();
bool nsis_iconv_reallociconv(iconv_t CD, char**In, size_t*cbInLeft, char**Mem, size_t&cbConverted);
void create_code_page_string(char*buf, size_t len, UINT code_page); // Create iconv code from Windows codepage

class iconvdescriptor {
  iconv_t m_cd;
public:
  iconvdescriptor(iconv_t cd = (iconv_t)-1) : m_cd(cd) {}
  ~iconvdescriptor() { Close(); }
  void Close()
  {
    if ((iconv_t)-1 != m_cd)
    {
      iconv_close(m_cd);
      m_cd = (iconv_t)-1;
    }
  }
  bool Open(const char*tocode, const char*fromcode)
  {
    m_cd = iconv_open(tocode, fromcode);
    return (iconv_t)-1 != m_cd;
  }
  UINT Convert(void*inbuf, size_t*pInLeft, void*outbuf, size_t outLeft)
  {
    char *in = (char*) inbuf, *out = (char*) outbuf;
    size_t orgOutLeft = outLeft;
    size_t ret = nsis_iconv_adaptor(iconv, *this, &in, pInLeft, &out, &outLeft);
    if ((size_t)(-1) == ret) 
      ret = 0, *pInLeft = 1;
    else
      ret = orgOutLeft - outLeft;
    return ret;
  }
  iconv_t GetDescriptor() const { return m_cd; }
  operator iconv_t() const { return m_cd; }

  static const char* GetHostEndianUCS4Code() { return nsis_iconv_get_host_endian_ucs4_code(); }
};

BOOL IsDBCSLeadByteEx(unsigned int CodePage, unsigned char TestChar);
TCHAR *CharPrev(const TCHAR *s, const TCHAR *p);
char *CharNextA(const char *s);
wchar_t *CharNextW(const wchar_t *s);
char *CharNextExA(WORD codepage, const char *s, int flags);
int wsprintf(TCHAR *s, const TCHAR *format, ...);
int WideCharToMultiByte(UINT CodePage, DWORD dwFlags, const wchar_t* lpWideCharStr,
    int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar);
int MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
    int cbMultiByte, wchar_t* lpWideCharStr, int cchWideChar);
BOOL IsValidCodePage(UINT CodePage);
#ifdef _UNICODE
#define CharNext CharNextW
#else
#define CharNext CharNextA
#endif
bool NSISRT_Initialize();
#define NSISRT_free(p) ( free((void*)(p)) )
wchar_t* NSISRT_mbtowc(const char *Str);
char* NSISRT_wctomb(const wchar_t *Str);
char* NSISRT_wctombpath(const wchar_t *Path);
char* NSISRT_ttombpath(const TCHAR *Path);
const char* NSISRT_setlocale_wincp(int cat, unsigned int cp);
int _wcsicmp(const wchar_t *a, const wchar_t *b);
int _wcsnicmp(const wchar_t *a, const wchar_t *b, size_t n);
long _wtol(const wchar_t *s);
int _wtoi(const wchar_t *s);
int _swprintf(wchar_t *d, const wchar_t *f, ...);
wchar_t* _wcsdup(const wchar_t *s);
wchar_t* _wgetenv(const wchar_t *wname);
int _wremove(const wchar_t *Path);
#define _wunlink _wremove // BUGBUG: This is not 100% correct, _wremove can also call rmdir
int _wchdir(const wchar_t *Path);
int _wstat(const wchar_t *Path, struct stat *pS);

TCHAR *my_convert(const TCHAR *path);
void my_convert_free(TCHAR *converted_path);
int my_open(const TCHAR *pathname, int flags);

#define OPEN(a, b) my_open(a, b)

#else // _WIN32

#define NSISRT_Initialize() (true)

#define my_convert(x) (x)
#define my_convert_free(x)

#define OPEN(a, b) _topen(a, b)

#endif // ~_WIN32

FILE* my_fopen(const TCHAR *path, const char *mode);
#define FOPEN(a, b) my_fopen((a), (b))

// round a value up to be a multiple of 512
// assumption: T is an int type
template <class T>
inline T align_to_512(const T x) {
  return (x+511) & ~511;
}

// ================
// ResourceManagers
// ================

// When a ResourceManager instance goes out of scope, it will run
// _FREE_RESOURCE on the resource.
// Example use:
// int fd = open(..);
// assert(fd != -1);
// MANAGE_WITH(fd, close);

class BaseResourceManager {
protected:
  BaseResourceManager() {}
public:
  virtual ~BaseResourceManager() {}
};

template <typename _RESOURCE, typename _FREE_RESOURCE>
class ResourceManager : public BaseResourceManager {
public:
  ResourceManager(_RESOURCE& resource) : m_resource(resource) {}
  virtual ~ResourceManager() { m_free_resource(m_resource); };
private: // members
  _RESOURCE& m_resource;
  _FREE_RESOURCE m_free_resource;
private: // don't copy instances
  ResourceManager(const ResourceManager&);
  void operator=(const ResourceManager&);
};

#define RM_MANGLE_FREEFUNC(freefunc) __free_with_##freefunc
#define RM_DEFINE_FREEFUNC(freefunc) \
  struct RM_MANGLE_FREEFUNC(freefunc) { \
    template <typename T> void operator()(T& x) { freefunc(x); } \
  }

typedef boost::scoped_ptr<BaseResourceManager> ResourceManagerPtr;

template<typename _FREE_RESOURCE, typename _RESOURCE>
void createResourceManager(_RESOURCE& resource, ResourceManagerPtr& ptr) {
  ptr.reset(new ResourceManager<_RESOURCE, _FREE_RESOURCE>(resource));
}

#define RM_MANGLE_RESOURCE(resource) resource##_autoManager
#define MANAGE_WITH(resource, freefunc) \
  ResourceManagerPtr RM_MANGLE_RESOURCE(resource); \
    createResourceManager<RM_MANGLE_FREEFUNC(freefunc)>( \
      resource, RM_MANGLE_RESOURCE(resource))

// Add more resource-freeing functions here when you need them
RM_DEFINE_FREEFUNC(close);
RM_DEFINE_FREEFUNC(CloseHandle);
RM_DEFINE_FREEFUNC(fclose);
RM_DEFINE_FREEFUNC(free);
RM_DEFINE_FREEFUNC(my_convert_free);

// Auto path conversion
#ifndef _WIN32
#  define PATH_CONVERT(x) x = my_convert(x); MANAGE_WITH(x, my_convert_free);
#else
#  define PATH_CONVERT(x)
#endif

// Platform detection
inline bool Platform_IsBigEndian() { return FIX_ENDIAN_INT16(0x00ff) != 0x00ff; }
unsigned char Platform_SupportsUTF8Conversion();

#endif //_UTIL_H_
