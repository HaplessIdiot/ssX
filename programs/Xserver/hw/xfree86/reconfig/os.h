/* $XFree86$ */

/* from <X11/Xosdefs.h> */
#ifdef NOSTDHDRS
#define X_NOT_STDC_ENV
#endif
#if defined(USL) && defined(SYSV)
#define X_NOT_STDC_ENV
#endif
#if defined(SYSV) && defined(i386)
#define X_NOT_STDC_ENV
#endif

/* from <X11/Xlibint.h> */
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#include <string.h>
#else
char *malloc(), *realloc(), *calloc();
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#endif

