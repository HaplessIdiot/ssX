/* $TOG: private.c /main/5 1998/02/09 10:55:41 kaleb $ */

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
/* $XFree86$ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include    "fontmisc.h"
#include    "fontstruct.h"

static int _FontPrivateAllocateIndex = 0;

int
AllocateFontPrivateIndex ()
{
    return _FontPrivateAllocateIndex++;
}

FontPtr 
CreateFontRec( void )
{
    FontPtr pFont;
    int size;

    size = sizeof(FontRec) + (sizeof(pointer) * _FontPrivateAllocateIndex);

    pFont = (FontPtr)xalloc(size);
    bzero((char*)pFont, size);
    
    if(pFont) {
	pFont->maxPrivate = _FontPrivateAllocateIndex - 1;
	if(_FontPrivateAllocateIndex)
	    pFont->devPrivates = (pointer)(&pFont[1]);
    }

    return pFont;
}

void DestroyFontRec (FontPtr pFont)
{
   xfree(pFont);
}

void
ResetFontPrivateIndex ()
{
    _FontPrivateAllocateIndex = 0;
}

Bool
_FontSetNewPrivate (pFont, n, ptr)
    FontPtr pFont;
    int	    n;
    pointer ptr;
{
    pointer *new;

    if (n > pFont->maxPrivate)
    {
	new = (pointer *) xrealloc (pFont->devPrivates, (n + 1) * sizeof (pointer));
	if (!new)
	    return FALSE;
	pFont->devPrivates = new;
	/* zero out new, uninitialized privates */
	while(++pFont->maxPrivate < n)
	    pFont->devPrivates[pFont->maxPrivate] = (pointer)0;
    }
    pFont->devPrivates[n] = ptr;
    return TRUE;
}

