/* $TOG: fontmisc.h /main/11 1998/05/07 14:12:58 kaleb $ */

/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/font/include/fontmisc.h,v 3.4 1998/10/03 09:07:30 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FONTMISC_H_
#define _FONTMISC_H_

#ifndef FONTMODULE
#include <X11/Xfuncs.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern int rand();
#endif
#include <stdio.h>

#ifndef X_NOT_POSIX
#include <unistd.h>
#else
extern int close();
#endif

#endif /* FONTMODULE */

typedef unsigned char	*pointer;
typedef int		Bool;

#ifndef X_PROTOCOL
#ifndef _XSERVER64
typedef unsigned long	Atom;
typedef unsigned long	XID;
#else
#include <X11/Xmd.h>
typedef CARD32 XID;
typedef CARD32 Atom;
#endif 
#endif

#ifndef LSBFirst
#define LSBFirst	0
#define MSBFirst	1
#endif

#ifndef None
#define None	0l
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern char	    *NameForAtom ();

extern pointer Xalloc(unsigned);
extern pointer Xrealloc(pointer, unsigned);
extern void Xfree(pointer);

#define xalloc(n)   Xalloc ((unsigned) n)
#define xfree(p)    Xfree ((pointer) p)
#define xrealloc(p,n)	Xrealloc ((pointer)p,n)
#define lowbit(x) ((x) & (~(x) + 1))

#define assert(x)	((void)0)

extern void
BitOrderInvert(
    register unsigned char *,
    register int
);

extern void
TwoByteSwap(
    register unsigned char *,
    register int
);

extern void
FourByteSwap(
    register unsigned char *,
    register int
);

extern int
RepadBitmap (
    char*, 
    char*,
    unsigned, 
    unsigned,
    int, 
    int
);

#endif /* _FONTMISC_H_ */
