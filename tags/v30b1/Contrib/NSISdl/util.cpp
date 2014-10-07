/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: util.cpp - JNL implementation of basic network utilities
** License: see License.txt
**
** Unicode support by Jim Park -- 08/24/2007
**   Keep everything here strictly ANSI.  No TCHAR style stuff.
*/

#include "netinc.h"

#include "util.h"

int my_atoi(char *s)
{
  int sign=0;
  int v=0;
  if (*s == '-') { s++; sign++; }
  for (;;)
  {
    int c=*s++ - '0';
    if (c < 0 || c > 9) break;
    v*=10;
    v+=c;
  }
  if (sign) return -(int) v;
  return (int)v;
}

__int64 myatoi64(const char *s)
{
  __int64 v=0;
  int sign=0;

  if (*s == '-')
    sign++;
  else
    s--;

  for (;;)
  {
    int c=*(++s) - '0';
    if (c < 0 || c > 9) break;
    v*=10;
    v+=c;
  }

  if (sign)
    v = -v;

  return v;
}

void myitoa64(__int64 i, char *buffer)
{
    char buf[128], *b = buf;

    if (i < 0)
    {
        *(buffer++) = '-';
        i = -i;
    }
    if (i == 0) *(buffer++) = '0';
    else 
    {
        while (i > 0) 
        {
            *(b++) = '0' + ((char) (i%10));
            i /= 10;
        }
        while (b > buf) *(buffer++) = *(--b);
    }
    *buffer = 0;
}

void mini_memset(void *o,char i,int l)
{
  char *oo=(char*)o;
  while (l-- > 0) *oo++=i;
}
void mini_memcpy(void *o,void*i,int l)
{
  char *oo=(char*)o;
  char *ii=(char*)i;
  while (l-- > 0) *oo++=*ii++;
}
