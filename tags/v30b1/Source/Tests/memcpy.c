// Reviewed for Unicode support by Jim Park -- 08/24/2007

#include "../Platform.h"

void NSISCALL mini_memcpy(void *out, const void *in, int len)
{
  char *c_out=(char*)out;
  char *c_in=(char *)in;
  while (len-- > 0)
  {
    *c_out++=*c_in++;
  }
}
