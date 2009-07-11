/*
	compat: Some compatibility functions and header inclusions.
	Basic standard C stuff, that may barely be above/around C89.

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	It is envisioned to include this compat header instead of any of the "standard" headers, to catch compatibility issues.
	So, don't include stdlib.h or string.h ... include compat.h.

	copyright 2007-8 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#ifndef MPG123_COMPAT_H
#define MPG123_COMPAT_H

#include "config.h"
#include "mpg123.h"

#ifdef INCOMPLETE_TCHAR
#ifdef _UNICODE
#define _tstof _wtof
#define _tcsncicmp _wcsnicmp
#define _tstoi64 _wtoi64
#define _tstol _wtol
#define _tstoi _wtoi
#define _tperror _wperror
#else
#define _tstof atof
#define _tcsncicmp strncmp
#define _tstoi64 atoi64
#define _tstol atol
#define _tstoi atoi
#define _tperror perror
#endif
#endif

#ifdef _UNICODE
#define strz "ws"
#else
#define strz "s"
#endif

#ifndef __T
#define __T(X) _T(X)
#endif

#ifndef HAVE_TCHAR_H
#define _TDIR DIR
#define _T(x) x
#define _fgetts fgets
#define _ftprintf fprintf
#define _sntprintf snprintf
#define _stat64i32 stat /**< mingw-w64 seems to use _stat64i32 struct by default.*/
#define _stscanf sscanf
#define _tchdir chdir
#define _tclosedir closedir
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcscspn strcspn
#define _tcsdup strdup
#define _tcsicmp strcasecmp
#define _tcslen strlen
#define _tcsncicmp strncasecmp
#define _tcsncmp strncmp
#define _tcsncpy strncpy
#define _tcsrchr strrchr
#define _tcstok strtok
#define _tdirent dirent
#define _tfopen fopen
#define _tgetcwd    getcwd
#define _tgetenv getenv
#define _tmain main
#define _topen open
#define _topendir opendir
#define _tperror perror
#define _tprintf printf
#define _treaddir readdir
#define _tstat stat
#define _tstof atof
#define _tstoi atoi
#define _tstoi64 atoll
#define _tstol atol
#define _vftprintf vfprintf
#define strz "s"
#endif

#ifdef HAVE_STDLIB_H
/* realloc, size_t */
#include <stdlib.h>
#endif

#include        <stdio.h>
#include        <math.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Types, types, types. */
/* Do we actually need these two in addition to sys/types.h? As replacement? */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
/* We want SIZE_MAX, etc. */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
 
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)-1)
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef OS2
#include <float.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
/* For select(), I need select.h according to POSIX 2001, else: sys/time.h sys/types.h unistd.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* To parse big numbers... */
#ifdef HAVE_ATOLL
#define atobigint _tstoi64
#else
#define atobigint _tstol
#endif

typedef unsigned char byte;

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *safe_realloc(void *ptr, size_t size);
#ifndef HAVE_STRERROR
const char *strerror(int errnum);
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif

/* If we have the size checks enabled, try to derive some sane printfs.
   Simple start: Use max integer type and format if long is not big enough.
   I am hesitating to use %ll without making sure that it's there... */
#if !(defined PLAIN_C89) && (defined SIZEOF_OFF_T) && (SIZEOF_OFF_T > SIZEOF_LONG) && (defined PRIiMAX)
# define OFF_P PRIiMAX
typedef intmax_t off_p;
#else
# define OFF_P "li"
typedef long off_p;
#endif

#if !(defined PLAIN_C89) && (defined SIZEOF_SIZE_T) && (SIZEOF_SIZE_T > SIZEOF_LONG) && (defined PRIuMAX)
# define SIZE_P PRIuMAX
typedef uintmax_t size_p;
#else
# define SIZE_P "lu"
typedef unsigned long size_p;
#endif

#if !(defined PLAIN_C89) && (defined SIZEOF_SSIZE_T) && (SIZEOF_SSIZE_T > SIZEOF_LONG) && (defined PRIiMAX)
# define SSIZE_P PRIuMAX
typedef intmax_t ssize_p;
#else
# define SSIZE_P "li"
typedef long ssize_p;
#endif

#endif
