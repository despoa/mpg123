#if (WIN32 && _UNICODE && UNICODE)
#define WIN32_LEAN_AND_MEAN 1
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <winnls.h>

#include "config.h"
#include "mpg123.h"
#include "debug.h"
#include "win32conv.h"

int
win32_uni2mbc (const wchar_t * const wptr, const char **const mbptr,
	       size_t * const buflen)
{
  size_t len;
  char *buf;
  int ret;
  len = wcslen (wptr) + 1;
  buf = calloc (len, sizeof (char));	/* Can we assume sizeof char always = 1 */
  debug2("win32_uni2mbc allocated %u bytes at %p", len, buf);
  if (buf)
    {
      ret =
	WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wptr, -1, buf, len,
			     NULL, NULL);
      *mbptr = buf;
      if (buflen)
	*buflen = len * sizeof (char);
      return ret;
    }
  else
    {
      if (buflen)
	*buflen = 0;
      return 0;
    }
}

/**
 * win32_mbc2uni
 * Converts a null terminated string to a wide equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[in] wptr Pointer to wide string.
 * @param[out] mbptr Pointer to multibyte string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * MultiByteToWideChar - http://msdn.microsoft.com/en-us/library/dd319072(VS.85).aspx
 */

int
win32_mbc2uni (const char *const mbptr, const wchar_t ** const wptr,
	       size_t * const buflen)
{
  size_t len;
  wchar_t *buf;
  int ret;
  len = strlen (mbptr) + 1;
  buf = calloc (len, sizeof (wchar_t));
  debug2("win32_uni2mbc allocated %u bytes at %p", len, buf);
  if (buf)
    {
      ret = MultiByteToWideChar (CP_UTF8, MB_PRECOMPOSED, mbptr, -1, buf, len);
      *wptr = buf;
      if (buflen)
	*buflen = len * sizeof (wchar_t);
      return ret;
    }
  else
    {
      if (buflen)
	*buflen = 0;
      return 0;
    }
}

#endif /* WIN32 && _UNICODE && UNICODE */
