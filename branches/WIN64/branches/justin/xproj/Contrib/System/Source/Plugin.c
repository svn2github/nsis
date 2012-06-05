#include "stdafx.h"
#include "Plugin.h"
#include "Buffers.h"
#include "System.h"

HWND g_hwndParent;

char *AllocString()
{
    return (char*) GlobalAlloc(GPTR,g_stringsize);
}

char *AllocStr(char *str)
{
    return lstrcpy(AllocString(), str);
}

char* system_popstring()
{
        char *str;
        nsis_stack_t *th;

        if (!g_stacktop || !*g_stacktop) return NULL;
        th=(*g_stacktop);

        str = AllocString();
        lstrcpy(str,th->text);

        *g_stacktop = th->next;
        GlobalFree((HGLOBAL)th);
        return str;
}

char *system_pushstring(char *str)
{
        nsis_stack_t *th;
        if (!g_stacktop) return str;
        th=(nsis_stack_t*)GlobalAlloc(GPTR,sizeof(nsis_stack_t)+g_stringsize);
        lstrcpyn(th->text,str,g_stringsize);
        th->next=*g_stacktop;
        *g_stacktop=th;
        return str;
}

char *system_getuservariable(int varnum)
{
        if (varnum < 0 || varnum >= __INST_LAST) return AllocString();
        return AllocStr(g_variables+varnum*g_stringsize);
}

char *system_setuservariable(int varnum, char *var)
{
        if (var != NULL && varnum >= 0 && varnum < __INST_LAST) {
                lstrcpy (g_variables + varnum*g_stringsize, var);
        }
        return var;
}

// Updated for int64 and simple bitwise operations
__int64 myatoi64(char *s)
{
  __int64 v=0;
  // Check for right input
  if (!s) return 0;
  if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
  {
    s++;
    for (;;)
    {
      int c=*(++s);
      if (c >= '0' && c <= '9') c-='0';
      else if (c >= 'a' && c <= 'f') c-='a'-10;
      else if (c >= 'A' && c <= 'F') c-='A'-10;
      else break;
      v<<=4;
      v+=c;
    }
  }
  else if (*s == '0' && s[1] <= '7' && s[1] >= '0')
  {
    for (;;)
    {
      int c=*(++s);
      if (c >= '0' && c <= '7') c-='0';
      else break;
      v<<=3;
      v+=c;
    }
  }
  else
  {
    int sign=0;
    if (*s == '-') sign++; else s--;
    for (;;)
    {
      int c=*(++s) - '0';
      if (c < 0 || c > 9) break;
      v*=10;
      v+=c;
    }
    if (sign) v = -v;
  }

  // Support for simple ORed expressions
  if (*s == '|') 
  {
      v |= myatoi64(s+1);
  }

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

int popint64()
{
    int value;
	char *str;
	if ((str = system_popstring()) == NULL) return -1;
	value = (int) myatoi64(str);
    GlobalFree(str);
	return value;
}

void system_pushint(int value)
{
	char buffer[1024];
	wsprintf(buffer, "%d", value);
	system_pushstring(buffer);
}

char *copymem(char *output, char *input, int size)
{
    char *out = output;
    if ((input != NULL) && (output != NULL))
        while (size-- > 0) *(out++) = *(input++);
    return output;
}

HANDLE GlobalCopy(HANDLE Old)
{
	SIZE_T size = GlobalSize(Old);
    return copymem(GlobalAlloc(GPTR, size), Old, (int) size);
}

UINT_PTR NSISCallback(enum NSPIM msg)
{
  return 0;
}

#ifdef _DEBUG
void main()
{
}
#endif
