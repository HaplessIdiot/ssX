/* $XFree86: xc/programs/Xserver/hw/xfree86/reconfig/os.h,v 3.0 1994/09/04 10:49:15 dawes Exp $ */

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

#ifdef MACH
#include <ctype.h>
#endif
