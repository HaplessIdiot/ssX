/**********************************************************************
Copyright 1998, 1999 by Precision Insight, Inc., Cedar Park, Texas.

                        All Rights Reserved

Permission to use, copy, modify, distribute, and sell this software and
its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Precision Insight not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  Precision Insight
and its suppliers make no representations about the suitability of this
software for any purpose.  It is provided "as is" without express or 
implied warranty.

PRECISION INSIGHT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**********************************************************************/
/* $XFree86$ */

/*
 * The original Precision Insight driver for
 * XFree86 v.3.3 has been sponsored by Red Hat.
 *
 * Authors:
 *   Jens Owen (jens@precisioninsight.com)
 *   Kevin E. Martin (kevin@precisioninsight.com)
 *
 * Port to Xfree86 v.4.0
 *   1998, 1999 by Egbert Eich (Egbert.Eich@Physik.TU-Darmstadt.DE)
 */

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"   

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h" 

#include "xf86Cursor.h"
#include "cursorstr.h"
/* Driver specific headers */
#include "neo.h"

static void _neoLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src,
				int xoff, int yoff);

void
NeoShowCursor(ScrnInfoPtr pScrn)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    /* turn cursor on */
    OUTREG(NEOREG_CURSCNTL, NEO_CURS_ENABLE);
    nPtr->NeoHWCursorShown = TRUE;    
}

void
NeoHideCursor(ScrnInfoPtr pScrn)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    /*
     * turn cursor off 
     *
     * Sometimes we loose the I/O map, so directly use I/O here
     */
    outb(GRAX,0x82);
    outb(GRAX+1,0x0);
    nPtr->NeoHWCursorShown = FALSE;
}

static void
neoSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    int xoff = 0, yoff = 0;
    
    if (y < 0) {
	yoff = -y;
	y = 0;
    }
    if (x < 0) {
	xoff = -x;
	x = 0;
    }
    if (yoff != nPtr->NeoCursorPrevY || xoff !=nPtr->NeoCursorPrevX) {
	_neoLoadCursorImage(pScrn, nPtr->NeoCursorImage
			    + ((nPtr->CursorInfo->MaxWidth >> 2) * yoff)
			    + ((xoff + 4) >> 3),(xoff + 4),yoff);
	nPtr->NeoCursorPrevY = yoff;
	nPtr->NeoCursorPrevX = xoff;
    }

    /* Move the cursor */
    OUTREG(NEOREG_CURSX, x);
    OUTREG(NEOREG_CURSY, y);
}

static void
neoSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    /* swap blue and red */
    fg = ((fg & 0xff0000) >> 16) | ((fg & 0xff) << 16) | (fg & 0xff00);
    bg = ((bg & 0xff0000) >> 16) | ((bg & 0xff) << 16) | (bg & 0xff00);

    /* load colors */
    OUTREG(NEOREG_CURSFGCOLOR, fg);
    OUTREG(NEOREG_CURSBGCOLOR, bg);
}

static void
_neoLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src, int xoff, int yoff)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);
    int i;
    unsigned char *_dest, *_src;
    int _width, _fill;
    
    if (!nPtr->noLinear) {
	for (i = 0; i< nPtr->CursorInfo->MaxHeight - yoff; i++) {
	    _dest = ((unsigned char *)nPtr->NeoFbBase
		     + nAcl->CursorAddress
		     + ((nPtr->CursorInfo->MaxWidth >> 2) * i));
	    _width = (nPtr->CursorInfo->MaxWidth
		      - (xoff & 0x38)) >> 3;
	    _src = (src + ((nPtr->CursorInfo->MaxWidth >> 2) * i));
	    _fill = (xoff & 0x38) >> 3;

	    memcpy(_dest,_src,_width);
	    memset(_dest + _width, 0, _fill);

	    _dest += (nPtr->CursorInfo->MaxWidth >> 3);
	    _src += (nPtr->CursorInfo->MaxWidth >> 3);
	    memcpy(_dest,_src,_width);
	    memset(_dest + _width, 0, _fill);
	}
	memset(nPtr->NeoFbBase + nAcl->CursorAddress 
	       + ((nPtr->CursorInfo->MaxWidth >> 2) * i),
	       0, (nPtr->CursorInfo->MaxHeight - i)
	       * (nPtr->CursorInfo->MaxWidth >> 2));
    } else {
	/*
	 * The cursor can only be in the last 16K of video memory,
	 * which fits in the last banking window.
	 */
	for (i = 0; i<nPtr->CursorInfo->MaxHeight; i++) {
	    _dest = ((unsigned char *)nPtr->NeoFbBase
		     + (nAcl->CursorAddress & 0xFFFF)
		     + ((nPtr->CursorInfo->MaxWidth >> 2) * i));
	    _width = (nPtr->CursorInfo->MaxWidth
		      - ((xoff & 0x38) >> 3));
	    _src = (src + ((nPtr->CursorInfo->MaxWidth >> 2) * i));
	    _fill = (xoff & 0x38) >> 3;

	    NEOSetWrite(pScrn->pScreen, (int)(nAcl->CursorAddress >> 16));
	    memcpy(_dest,_src,_width);
	    memset(_dest + _width, 0, _fill);

	    _dest += (nPtr->CursorInfo->MaxWidth >> 3);
	    _src += (nPtr->CursorInfo->MaxWidth >> 3);
	    memcpy(_dest,_src,_width);
	    memset(_dest + _width, 0, _fill);
	}
    }
    /* set cursor address here or we loose the cursor on video mode change */
    /* Load storage location.  */
    OUTREG(NEOREG_CURSMEMPOS, ((0x000f & (nAcl->CursorAddress >> 10)) << 8)  | 
	   ((0x0ff0 & (nAcl->CursorAddress >> 10)) >> 4));
}

static void
neoLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    nPtr->NeoCursorImage = src;  /* store src address for later use */
    _neoLoadCursorImage(pScrn,src,0,0);
}

static Bool
neoUseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    NEOACLPtr nAcl = NEOACLPTR(xf86Screens[pScr->myNum]);

    return nAcl->UseHWCursor;
}

static unsigned char*
neoRealizeCursor(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *SrcS, *SrcM, *DstS, *DstM;
    CARD32 *pSrc, *pMsk;
    unsigned char *mem;
    int SrcPitch, DstPitch, y, x, z;

    mem = (unsigned char*)xnfcalloc(4096,1);
    SrcPitch = (pCurs->bits->width + 31) >> 5;
    DstPitch = infoPtr->MaxWidth >> 4;
    SrcS = (CARD32*)pCurs->bits->source;
    SrcM = (CARD32*)pCurs->bits->mask;
    DstS = (CARD32*)mem;
    DstM = DstS + (DstPitch >> 1);
    
    for(y = pCurs->bits->height, pSrc = DstS, pMsk = DstM; 
	y--; 
	pSrc+=DstPitch, pMsk+=DstPitch, SrcS+=SrcPitch, SrcM+=SrcPitch) {
	for(x = 0; x < SrcPitch; x++) {
	    pSrc[x] = ~SrcS[x] & SrcM[x];
	    pMsk[x] = SrcM[x];
	    for (z = 0; z < 4; z++) { 
		((char *)pSrc)[x*4+z] =
		    byte_reversed[((char *)pSrc)[x*4+z] & 0xFF];
		((char *)pMsk)[x*4+z] =
		    byte_reversed[((char *)pMsk)[x*4+z] & 0xFF];
	    }
	}
#if 0
	for (;x < DstPitch; x++) {
	    pSrc[x] = 0;
	    pMsk[x] = 0;
	}
#endif
    }

    return (unsigned char *)mem;
}

Bool
NeoCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    NEOPtr nPtr = NEOPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;

    nPtr->CursorInfo = infoPtr;

    infoPtr->MaxHeight = 64;
    infoPtr->MaxWidth = 64;
    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;

    infoPtr->SetCursorColors = neoSetCursorColors;
    infoPtr->SetCursorPosition = neoSetCursorPosition;
    infoPtr->LoadCursorImage = neoLoadCursorImage;
    infoPtr->HideCursor = NeoHideCursor;
    infoPtr->ShowCursor = NeoShowCursor;
    infoPtr->UseHWCursor = neoUseHWCursor;
    infoPtr->RealizeCursor = neoRealizeCursor;
    
    return(xf86InitCursor(pScreen, infoPtr));
}



