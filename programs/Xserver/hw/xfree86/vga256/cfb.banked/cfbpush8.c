/*
 * Push Pixels for 8 bit displays.
 */

/*

Copyright (c) 1989  X Consortium

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
/* $XConsortium: cfbpush8.c,v 5.10 94/04/17 20:32:23 dpw Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include        "vgaBank.h"

#if PPW == 4

extern void mfbPushPixels();

void
cfbPushPixels8 (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitmap;
    DrawablePtr	pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    register unsigned long   *src, *dst;
    register unsigned long   pixel;
    register unsigned long   c, bits;
    unsigned long   *pdstLine, *psrcLine;
    unsigned long   *pdstBase;
    int		    srcWidth;
    int		    dstWidth;
    int		    xoff;
    int		    nBitmapLongs, nPixmapLongs;
    int		    nBitmapTmp, nPixmapTmp;
    unsigned long   rightMask;
    BoxRec	    bbox;
    cfbPrivGCPtr    devPriv;

    bbox.x1 = xOrg;
    bbox.y1 = yOrg;
    bbox.x2 = bbox.x1 + dx;
    bbox.y2 = bbox.y1 + dy;
    devPriv = (cfbPrivGC *)pGC->devPrivates[cfbGCPrivateIndex].ptr;
    
    switch ((*pGC->pScreen->RectIn)(devPriv->pCompositeClip, &bbox))
    {
      case rgnPART:
	mfbPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
      case rgnOUT:
	return;
    }

    cfbGetLongWidthAndPointer (pDrawable, dstWidth, pdstBase)

    BANK_FLAG(pdstBase)

    psrcLine = (unsigned long *) pBitmap->devPrivate.ptr;
    srcWidth = (int) pBitmap->devKind >> 2;
    
    pixel = devPriv->xor;
    xoff = xOrg & 03;
    nBitmapLongs = (dx + xoff) >> 5;
    nPixmapLongs = (dx + 3 + xoff) >> 2;

    rightMask = ~cfb8BitLenMasks[((dx + xoff) & 0x1f)];

    pdstLine = pdstBase + (yOrg * dstWidth) + (xOrg >> 2);

    while (dy--)
    {
	c = 0;
	nPixmapTmp = nPixmapLongs;
	nBitmapTmp = nBitmapLongs;
	src = psrcLine;
	dst = pdstLine;
	SETRW(dst);
	while (nBitmapTmp--)
	{
	    bits = *src++;
	    c |= BitRight (bits, xoff);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    nPixmapTmp -= 8;
	    c = 0;
	    if (xoff)
		c = BitLeft (bits, 32 - xoff);
	}
	if (BitLeft (rightMask, xoff))
	    c |= BitRight (*src, xoff);
	c &= rightMask;
	switch (nPixmapTmp) {
	case 8:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 7:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 6:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 5:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 4:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 3:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 2:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 1:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 0:
	    break;
	}
	pdstLine += dstWidth;
	psrcLine += srcWidth;
    }
}

#endif
