/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: netinc.h - network includes and portability defines (used internally)
** License: zlib
**
** Reviewed for Unicode Support by Jim Park -- 08/17/2007
** Note: The functions that work on char's should be explicitely set to use the
** ANSI versions.  Some of the functions like lstrcpy() are #defined to be
** the wide-char versions when _UNICODE is defined.  So these must be explictly
** set to use the ANSI versions.
*/

#ifndef _NETINC_H_
#define _NETINC_H_

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <time.h>
#define strcasecmp(x,y) stricmp(x,y)
#define ERRNO (WSAGetLastError())
#define PORTABLE_SOCKET SOCKET
#define SET_SOCK_BLOCK(s,block) { unsigned long __i=block?0:1; ioctlsocket(s,FIONBIO,&__i); }
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEWOULDBLOCK
typedef int socklen_t;

#else

#ifndef THREAD_SAFE
#define THREAD_SAFE
#endif
#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define ERRNO errno
#define PORTABLE_SOCKET int
#define closesocket(s) close(s)
#define SET_SOCK_BLOCK(s,block) { int __flags; if ((__flags = fcntl(s, F_GETFL, 0)) != -1) { if (!block) __flags |= O_NONBLOCK; else __flags &= ~O_NONBLOCK; fcntl(s, F_SETFL, __flags);  } }

#define stricmp(x,y) strcasecmp(x,y)
#define strnicmp(x,y,z) strncasecmp(x,y,z)  
#define wsprintf sprintf

#endif // !_WIN32

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef INADDR_ANY
#define INADDR_ANY 0
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

extern void mini_memset(void *,char,int);
extern void mini_memcpy(void *,void*,int);
#define memset mini_memset
#define memcpy mini_memcpy
#define strcpy lstrcpyA
#define strncpy lstrcpynA
#define strcat lstrcatA
#define strlen lstrlenA
#define malloc(x) GlobalAlloc(GPTR,(x))
#define free(x) { if (x) GlobalFree(x); }

#endif //_NETINC_H_
