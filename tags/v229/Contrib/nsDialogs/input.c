#include <windows.h>

#include "input.h"
#include "defs.h"
#include "nsis.h"

extern struct nsDialog g_dialog;

static int NSDFUNC ConvertPlacement(char *str, int total, int height)
{
  char unit = *CharPrev(str, str + lstrlen(str));
  int x = myatoi(str);

  if (unit == '%')
  {
    if (x < 0)
    {
      return MulDiv(total, 100 + x, 100);
    }

    return MulDiv(total, x, 100);
  }
  else if (unit == 'u')
  {
    RECT r;

    r.left = r.top = x;

    MapDialogRect(g_dialog.hwParent, &r);

    if (height)
      return x >= 0 ? r.top : total + r.top;
    else
      return x >= 0 ? r.left : total + r.left;
  }

  if (x < 0)
  {
    return total + x;
  }

  return x;
}

int NSDFUNC PopPlacement(int *x, int *y, int *width, int *height)
{
  RECT dialogRect;
  int  dialogWidth;
  int  dialogHeight;
  char buf[1024];

  GetClientRect(g_dialog.hwDialog, &dialogRect);
  dialogWidth = dialogRect.right;
  dialogHeight = dialogRect.bottom;

  if (popstring(buf, 1024))
    return 1;

  *x = ConvertPlacement(buf, dialogWidth, 0);

  if (popstring(buf, 1024))
    return 1;

  *y = ConvertPlacement(buf, dialogHeight, 1);

  if (popstring(buf, 1024))
    return 1;

  *width = ConvertPlacement(buf, dialogWidth, 0);

  if (popstring(buf, 1024))
    return 1;

  *height = ConvertPlacement(buf, dialogHeight, 1);

  return 0;
}
