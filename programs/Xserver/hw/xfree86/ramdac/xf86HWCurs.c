/* $XFree86$ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "cursorstr.h"
#include "mi.h"
#include "mipointer.h"
#include "xf86Cursor.h"

static unsigned char* RealizeCursorInterleave0(XAACursorInfoPtr, CursorPtr);
static unsigned char* RealizeCursorInterleave1(XAACursorInfoPtr, CursorPtr);
static unsigned char* RealizeCursorInterleave8(XAACursorInfoPtr, CursorPtr);
static unsigned char* RealizeCursorInterleave16(XAACursorInfoPtr, CursorPtr);
static unsigned char* RealizeCursorInterleave32(XAACursorInfoPtr, CursorPtr);
static unsigned char* RealizeCursorInterleave64(XAACursorInfoPtr, CursorPtr);


Bool 
XAAInitHardwareCursor(ScreenPtr pScreen, XAACursorInfoPtr infoPtr)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    if((infoPtr->MaxWidth <= 0) || (infoPtr->MaxHeight <= 0))
	return FALSE;

    if(	!infoPtr->SetCursorPosition ||
	!infoPtr->LoadCursorImage ||
	!infoPtr->HideCursor ||
	!infoPtr->ShowCursor ||
	!infoPtr->SetCursorColors) /* these are required for now */
	return FALSE;

    if (infoPtr->RealizeCursor) {
	/* Don't overwrite a driver provided Realize Cursor function */
    } else
    if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1 & infoPtr->Flags) {
	infoPtr->RealizeCursor = RealizeCursorInterleave1;
    } else
    if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8 & infoPtr->Flags) {
	infoPtr->RealizeCursor = RealizeCursorInterleave8;
    } else
    if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16 & infoPtr->Flags) {
	infoPtr->RealizeCursor = RealizeCursorInterleave16;
    } else 
    if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32 & infoPtr->Flags) {
	infoPtr->RealizeCursor = RealizeCursorInterleave32;
    } else 
    if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 & infoPtr->Flags) {
	infoPtr->RealizeCursor = RealizeCursorInterleave64;
    } else {	/* not interleaved */
	infoPtr->RealizeCursor = RealizeCursorInterleave0;
    }

    infoPtr->pScrn = pScrn;

    return TRUE;
}


void
XAASetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;
    XAACursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;
    unsigned char *bits;

    if(!pCurs) {
	(*infoPtr->HideCursor)(infoPtr->pScrn);
	return;
    }

    bits = (unsigned char*)pCurs->devPriv[pScreen->myNum];

    x -= infoPtr->pScrn->frameX0 + ScreenPriv->HotX;
    y -= infoPtr->pScrn->frameY0 + ScreenPriv->HotY;

    if(!bits) {
	bits = (*infoPtr->RealizeCursor)(infoPtr, pCurs);
	pCurs->devPriv[pScreen->myNum] = (pointer)bits;
    }

    (*infoPtr->HideCursor)(infoPtr->pScrn);

    if(bits)
	(*infoPtr->LoadCursorImage)(infoPtr->pScrn, bits);

    XAARecolorCursor(pScreen, pCurs, 1);

    (*infoPtr->SetCursorPosition)(infoPtr->pScrn, x, y);

    (*infoPtr->ShowCursor)(infoPtr->pScrn);
}


void
XAAMoveCursor(ScreenPtr pScreen, int x, int y)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;
    XAACursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    x -= infoPtr->pScrn->frameX0 + ScreenPriv->HotX;
    y -= infoPtr->pScrn->frameY0 + ScreenPriv->HotY;
     
    (*infoPtr->SetCursorPosition)(infoPtr->pScrn, x, y);
}

void 
XAARecolorCursor(ScreenPtr pScreen, CursorPtr pCurs, Bool displayed)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;
    XAACursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    if(ScreenPriv->PalettedCursor) {
        xColorItem sourceColor, maskColor;
	ColormapPtr pmap = ScreenPriv->pInstalledMap;
	
	if(!pmap) return;

	sourceColor.red = pCurs->foreRed;
	sourceColor.green = pCurs->foreGreen;
	sourceColor.blue = pCurs->foreBlue;
	FakeAllocColor(pmap, &sourceColor);
	maskColor.red = pCurs->backRed;
	maskColor.green = pCurs->backGreen;
	maskColor.blue = pCurs->backBlue;
	FakeAllocColor(pmap, &maskColor);
	FakeFreeColor(pmap, sourceColor.pixel);
	FakeFreeColor(pmap, maskColor.pixel);
	(*infoPtr->SetCursorColors)(infoPtr->pScrn,
		maskColor.pixel, sourceColor.pixel);
    } else {	/* Pass colors in 8-8-8 RGB format */
	(*infoPtr->SetCursorColors)(infoPtr->pScrn,
            (pCurs->backBlue >> 8) |
            ((pCurs->backGreen >> 8) << 8) |
            ((pCurs->backRed >> 8) << 16),
            (pCurs->foreBlue >> 8) |
            ((pCurs->foreGreen >> 8) << 8) |
            ((pCurs->foreRed >> 8) << 16)
        );
    }
}

/* These functions assume that MaxWidth is a multiple of 32 */

static unsigned char* 
RealizeCursorInterleave0(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *SrcS, *SrcM, *DstS, *DstM;
    CARD32 *pSrc, *pMsk;
    unsigned char *mem;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;
    int SrcPitch, DstPitch, y, x;
    int dwords = size >> 3;

    if(!(mem = (unsigned char *)xcalloc(1, size)))
	return NULL;

    SrcPitch = (pCurs->bits->width + 31) >> 5;
    DstPitch = infoPtr->MaxWidth >> 5;

    SrcS = (CARD32*)pCurs->bits->source;
    SrcM = (CARD32*)pCurs->bits->mask;
    DstS = (CARD32*)mem;
    DstM = DstS + dwords;

    if(infoPtr->Flags & HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK) {
	CARD32 *tmp;
	tmp = DstS; DstS = DstM; DstM = tmp;
    }

    for(y = pCurs->bits->height, pSrc = DstS, pMsk = DstM; 
	y--; 
	pSrc+=DstPitch, pMsk+=DstPitch, SrcS+=SrcPitch, SrcM+=SrcPitch) {
	for(x = 0; x < SrcPitch; x++) {
	    pSrc[x] = SrcS[x];
	    pMsk[x] = SrcM[x];
	}
    }

    if(infoPtr->Flags & HARDWARE_CURSOR_AND_SOURCE_WITH_MASK) {
	int count = dwords;
	CARD32* pntr = DstS;
	CARD32* pntr2 = DstM;
	while(count--) {
	   *pntr &= *pntr2;
	    pntr++; pntr2++;
	}
    }

    /*
     * Must be _after_ HARDWARE_CURSOR_AND_SOURCE_WITH_MASK to avoid wiping
     * out entire source mask.
     */
    if(infoPtr->Flags & HARDWARE_CURSOR_INVERT_MASK) {
	int count = dwords;
	CARD32* pntr = DstM;
	while(count--) {
	   *pntr = ~(*pntr);
	    pntr++;
	}
    }

    if(infoPtr->Flags & HARDWARE_CURSOR_BIT_ORDER_MSBFIRST) {
	for(y = pCurs->bits->height, pSrc = DstS, pMsk = DstM; 
	    y--; 
	    pSrc+=DstPitch, pMsk+=DstPitch) {
	    for(x = 0; x < SrcPitch; x++) {
		pSrc[x] = XAAReverseBitOrder(pSrc[x]);
		pMsk[x] = XAAReverseBitOrder(pMsk[x]);
	    }
	}
    }

    return mem;
}

static unsigned char* 
RealizeCursorInterleave1(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned char *DstS, *DstM;
    unsigned char *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
	return NULL;

    if(!(mem = (unsigned char *)xcalloc(1, size))) {
        xfree(mem2);
	return NULL;
    }
    
     /* 1 bit interleave */
    DstS = (unsigned char *)mem2;
    DstM = DstS + (size >> 1);
    pntr = (unsigned char *)mem;
    count = size;
    while(count) {
	*pntr++ = ((*DstS&0x01)     ) | ((*DstM&0x01) << 1) |
		  ((*DstS&0x02) << 1) | ((*DstM&0x02) << 2) |
		  ((*DstS&0x04) << 2) | ((*DstM&0x04) << 3) |
		  ((*DstS&0x08) << 3) | ((*DstM&0x08) << 4);
	*pntr++ = ((*DstS&0x10) >> 4) | ((*DstM&0x10) >> 3) |
		  ((*DstS&0x20) >> 3) | ((*DstM&0x20) >> 2) |
		  ((*DstS&0x40) >> 2) | ((*DstM&0x40) >> 1) |
		  ((*DstS&0x80) >> 1) | ((*DstM&0x80)     );
	DstS++;
	DstM++;
	count-=2;
    }

    /* Free the uninterleaved cursor */
    xfree(mem2);

    return mem;
}


static unsigned char* 
RealizeCursorInterleave8(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned char *DstS, *DstM;
    unsigned char *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
	return NULL;

    if(!(mem = (unsigned char *)xcalloc(1, size))) {
        xfree(mem2);
	return NULL;
    }
    
    /* 8 bit interleave */
    DstS = (unsigned char *)mem2;
    DstM = DstS + (size >> 1);
    pntr = (unsigned char *)mem;
    count = size;
    while(count) {
	*pntr++ = *DstS++;
	*pntr++ = *DstM++;
	count-=2;
    }

    /* Free the uninterleaved cursor */
    xfree(mem2);

    return mem;
}

static unsigned char* 
RealizeCursorInterleave16(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned short *DstS, *DstM;
    unsigned short *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
	return NULL;

    if(!(mem = (unsigned char *)xcalloc(1, size))) {
        xfree(mem2);
	return NULL;
    }
    
    /* 16 bit interleave */
    DstS = (unsigned short *)mem2;
    DstM = DstS + (size >> 2);
    pntr = (unsigned short *)mem;
    count = (size >> 1);
    while(count) {
	*pntr++ = *DstS++;
	*pntr++ = *DstM++;
	count-=2;
    }

    /* Free the uninterleaved cursor */
    xfree(mem2);

    return mem;
}

static unsigned char* 
RealizeCursorInterleave32(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *DstS, *DstM;
    CARD32 *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
	return NULL;

    if(!(mem = (unsigned char *)xcalloc(1, size))) {
        xfree(mem2);
	return NULL;
    }
    
    /* 32 bit interleave */
    DstS = (CARD32*)mem2;
    DstM = DstS + (size >> 3);
    pntr = (CARD32*)mem;
    count = (size >> 2);
    while(count) {
	*pntr++ = *DstS++;
	*pntr++ = *DstM++;
	count-=2;
    }

    /* Free the uninterleaved cursor */
    xfree(mem2);

    return mem;
}

static unsigned char* 
RealizeCursorInterleave64(XAACursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *DstS, *DstM;
    CARD32 *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
	return NULL;

    if(!(mem = (unsigned char *)xcalloc(1, size))) {
        xfree(mem2);
	return NULL;
    }

    /* 64 bit interleave */
    DstS = (CARD32*)mem2;
    DstM = DstS + (size >> 3);
    pntr = (CARD32*)mem;
    count = (size >> 2);
    while(count) {
	*pntr++ = *DstS++;
	*pntr++ = *DstS++;
	*pntr++ = *DstM++;
	*pntr++ = *DstM++;
	count-=4;
    }

    /* Free the uninterleaved cursor */
    xfree(mem2);

    return mem;
}


