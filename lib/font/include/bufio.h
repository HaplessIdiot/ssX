/* $TOG: bufio.h /main/8 1998/05/07 13:44:07 kaleb $ */

/*

Copyright 1993, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/lib/font/include/bufio.h,v 1.0tsi Exp $ */

#ifndef ___BUFIO_H___
#define ___BUFIO_H___ 1

#include <X11/Xfuncproto.h>

#ifdef TEST

#define xalloc(s)   malloc(s)
#define xfree(s)    free(s)

#endif

#define BUFFILESIZE	8192
#define BUFFILEEOF	-1

typedef unsigned char BufChar;

typedef struct _buffile {
    BufChar *bufp;
    int	    left;
    BufChar buffer[BUFFILESIZE];
    int	    (*io)(/* BufFilePtr f */);
    int	    (*skip)(/* BufFilePtr f, int count */);
    int	    (*close)(/* BufFilePtr f */);
    char    *private;
} BufFileRec, *BufFilePtr;

extern BufFilePtr   BufFileCreate (
#if NeedFunctionPrototypes
    char*,
    int (*)(),
    int (*)(),
    int (*)()
#endif
);
extern BufFilePtr   BufFileOpenRead (
#if NeedFunctionPrototypes
    int
#endif
);

extern BufFilePtr BufFileOpenWrite (
#if NeedFunctionPrototypes
    int
#endif
);

extern BufFilePtr   BufFilePushCompressed (
#if NeedFunctionPrototypes
    BufFilePtr    
#endif
);
#ifdef X_GZIP_FONT_COMPRESSION
extern BufFilePtr   BufFilePushZIP (
#if NeedFunctionPrototypes
    BufFilePtr    
#endif
);
#endif
extern int	    BufFileClose (
#if NeedFunctionPrototypes
    BufFilePtr,
    int
#endif
);
extern int	    BufFileFlush (
#if NeedFunctionPrototypes
    BufFilePtr
#endif
);

extern int BufFileRead (
#if NeedFunctionPrototypes
    BufFilePtr,
    char*,
    int	
#endif
);

extern int BufFileWrite (
#if NeedFunctionPrototypes
    BufFilePtr,
    char*,
    int	
#endif
);

#define BufFileGet(f)	((f)->left-- ? *(f)->bufp++ : (*(f)->io) (f))
#define BufFilePut(c,f)	(--(f)->left ? *(f)->bufp++ = (c) : (*(f)->io) (c,f))
#define BufFileSkip(f,c)    ((*(f)->skip) (f, c))

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* ___BUFIO_H___ */
