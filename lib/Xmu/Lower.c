/* $XConsortium: Lower.c,v 1.7 94/04/17 20:16:11 converse Exp $ */

/* 
 
Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

/* $XFree86: xc/lib/Xmu/Lower.c,v 1.2 1998/06/28 08:59:57 dawes Exp $ */

#define  XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/Xmu/CharSet.h>

#include <stdio.h>

#if NeedVarargsPrototypes
#include <stdarg.h>
#define Va_start(a,b) va_start(a,b)
#else
#include <varargs.h>
#define Va_start(a,b) va_start(a)
#endif

/*
 * ISO Latin-1 case conversion routine
 */

#if NeedFunctionPrototypes
void XmuCopyISOLatin1Lowered(char *dst, _Xconst char *src)
#else
void XmuCopyISOLatin1Lowered(dst, src)
    char *dst, *src;
#endif
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	 *source;
	 source++, dest++)
    {
	if ((*source >= XK_A) && (*source <= XK_Z))
	    *dest = *source + (XK_a - XK_A);
	else if ((*source >= XK_Agrave) && (*source <= XK_Odiaeresis))
	    *dest = *source + (XK_agrave - XK_Agrave);
	else if ((*source >= XK_Ooblique) && (*source <= XK_Thorn))
	    *dest = *source + (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}

#if NeedFunctionPrototypes
void XmuCopyISOLatin1Uppered(char *dst, _Xconst char *src)
#else
void XmuCopyISOLatin1Uppered(dst, src)
    char *dst, *src;
#endif
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	 *source;
	 source++, dest++)
    {
	if ((*source >= XK_a) && (*source <= XK_z))
	    *dest = *source - (XK_a - XK_A);
	else if ((*source >= XK_agrave) && (*source <= XK_odiaeresis))
	    *dest = *source - (XK_agrave - XK_Agrave);
	else if ((*source >= XK_slash) && (*source <= XK_thorn))
	    *dest = *source - (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}

#if NeedFunctionPrototypes
int XmuCompareISOLatin1 (_Xconst char *first, _Xconst char *second)
#else
int XmuCompareISOLatin1 (first, second)
    char *first, *second;
#endif
{
    register unsigned char *ap, *bp;

    for (ap = (unsigned char *) first, bp = (unsigned char *) second;
	 *ap && *bp; ap++, bp++) {
	register unsigned char a, b;

	if ((a = *ap) != (b = *bp)) {
	    /* try lowercasing and try again */

	    if ((a >= XK_A) && (a <= XK_Z))
	      a += (XK_a - XK_A);
	    else if ((a >= XK_Agrave) && (a <= XK_Odiaeresis))
	      a += (XK_agrave - XK_Agrave);
	    else if ((a >= XK_Ooblique) && (a <= XK_Thorn))
	      a += (XK_oslash - XK_Ooblique);

	    if ((b >= XK_A) && (b <= XK_Z))
	      b += (XK_a - XK_A);
	    else if ((b >= XK_Agrave) && (b <= XK_Odiaeresis))
	      b += (XK_agrave - XK_Agrave);
	    else if ((b >= XK_Ooblique) && (b <= XK_Thorn))
	      b += (XK_oslash - XK_Ooblique);

	    if (a != b) return (((int) a) - ((int) b));
	}
    }
    return (((int) *ap) - ((int) *bp));
}

#if NeedFunctionPrototypes
void XmuNCopyISOLatin1Lowered(char *dst, _Xconst char *src, register int size)
#else
void XmuNCopyISOLatin1Lowered(dst, src, size)
    char *dst, *src;
    register int size;
#endif
{
  register unsigned char *dest, *source;
  register int c;

  if (size > 0)
    {
      for (dest = (unsigned char *)dst, source = (unsigned char *)src;
	   (c = *source) && size > 1;
	   source++, dest++, size--)
	{
	  if (c >= XK_A && c <= XK_Z)
	    *dest = c + (XK_a - XK_A);
	  else if (c >= XK_Agrave && c <= XK_Odiaeresis)
	    *dest = c + (XK_agrave - XK_Agrave);
	  else if (c >= XK_Ooblique && c <= XK_Thorn)
	    *dest = c + (XK_oslash - XK_Ooblique);
	  else
	    *dest = c;
	}
      *dest = '\0';
    }
}

#if NeedVarargsPrototypes
int
XmuSnprintf(char *str, int size, char *fmt, ...)
#else 
/* VARARGS3 */
int
XmuSnprintf(str, size, fmt, va_alist)
     char *str, *fmt;
     int size;
     va_dcl
#endif
{
  va_list ap;
  int retval;

  if (size <= 0)
    return (size);

  Va_start(ap, fmt);

#ifndef HAS_SNPRINTF
  retval = vsprintf(str, fmt, ap);
  if (retval >= size)
    {
      fprintf(stderr, "WARNING: buffer overflow detected!\n");
      fflush(stderr);
      abort();
    }
#else
  retval = vsnprintf(str, size, fmt, ap);
#endif

  va_end(ap);

  return (retval);
}

