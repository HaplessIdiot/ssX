/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_accel.c,v 1.6 1999/03/22 13:40:00 dawes Exp $ */


#include "apm.h"
#include "miline.h"

extern int xf86Exiting;

/* Defines */
#define MAXLOOP 1000000

/* Translation from X ROP's to APM ROP's. */
static unsigned char apmROP[] = {
  0,
  0x88,
  0x44,
  0xCC,
  0x22,
  0xAA,
  0x66,
  0xEE,
  0x11,
  0x99,
  0x55,
  0xDD,
  0x33,
  0xBB,
  0x77,
  0xFF
};

extern void apm_stopit(void);

static void Dump(void* start, u32 len);

#include "apm_funcs.c"

#define IOP_ACCESS
#include "apm_funcs.c"

#undef IOP_ACCESS
#undef PSZ
#define PSZ	24
#include "apm_funcs.c"

#define IOP_ACCESS
#include "apm_funcs.c"

static void
ApmRemoveStipple(FBAreaPtr area)
{
    ((ApmPtr)area->devPrivate.ptr)->apmStippleCached = FALSE;
}

static void
ApmMoveStipple(FBAreaPtr from, FBAreaPtr to)
{
    ApmPtr pApm = (ApmPtr)to->devPrivate.ptr;

    pApm->apmStippleCache.x = to->box.x1;
    pApm->apmStippleCache.y = to->box.y1 + pApm->displayHeight;
}

/*
 * ApmCacheMonoStipple
 * because my poor AT3D needs stipples stored linearly in memory.
 */
static XAACacheInfoPtr
ApmCacheMonoStipple(ScrnInfoPtr pScrn, PixmapPtr pPix)
{
    APMDECL(pScrn);
    int		w = pPix->drawable.width;
    int		h = pPix->drawable.height, ch;
    int		i, dwords;
    FBAreaPtr	draw;

    if ((pApm->apmStippleCache.serialNumber == pPix->drawable.serialNumber) &&
	pApm->apmStippleCached &&
	(pApm->apmStippleCache.fg == -1) && (pApm->apmStippleCache.bg == -1)) {
	pApm->apmStippleCache.trans_color = -1;
	return &pApm->apmStippleCache;
    }
    else if (pApm->apmStippleCached) {
	pApm->apmStippleCached = FALSE;
	xf86FreeOffscreenArea(pApm->area);
    }

    draw = xf86AllocateLinearOffscreenArea(pApm->pScreen, (w * h + 7) / 8, 8,
				    ApmMoveStipple, ApmRemoveStipple, pApm);
    if (!draw)
	return NULL;

    pApm->area = draw;
    pApm->apmStippleCache.serialNumber = pPix->drawable.serialNumber;
    pApm->apmStippleCache.trans_color = pApm->apmStippleCache.bg =
	pApm->apmStippleCache.fg = -1;
    pApm->apmStippleCache.orig_w = w;
    pApm->apmStippleCache.orig_h = h;
    pApm->apmStippleCache.x = draw->box.x1;
    pApm->apmStippleCache.y = draw->box.y1 + pApm->displayHeight;
    pApm->apmStippleCache.w = w;
    pApm->apmStippleCache.h = ((draw->box.x2 - draw->box.x1) *
				(draw->box.y2 - draw->box.y1) *
				pApm->bitsPerPixel) / pApm->apmStippleCache.w;
    pApm->apmStippleCached = TRUE;

    w = (w + 7) / 8;

    if (pPix->devKind == w) {
	w *= h;
	ch = pApm->apmStippleCache.h / h;
	h = 1;
    }
    else
	ch = pApm->apmStippleCache.h;

    if (((w & 3) == 0 && h == 1) || h == 1) {
	CARD32	*srcPtr, *dstPtr;

	/*
	 * I guess this will be used more often
	 */
	dwords = w >> 2;
	dstPtr = ((CARD32 *)pApm->FbBase) + (draw->box.x1 +
		draw->box.y1 * pApm->bytesPerScanline) / 4;

	while (ch-- > 0) {
	    srcPtr = (CARD32 *)pPix->devPrivate.ptr;
	    for(i = dwords; i-- > 0; )
		*dstPtr++ = XAAReverseBitOrder(*srcPtr++);

	    if (w & 3) {
		CARD16 *dstPtr2 = (CARD16 *)dstPtr;
		CARD16 *srcPtr2 = (CARD16 *)srcPtr;

		if (w & 2)
		    *dstPtr2++ = *srcPtr2++;
		if (w & 1)
		    *(CARD8 *)dstPtr2 = *(CARD8 *)srcPtr2;
	    }
	}
    }
    else if ((w & 3) == 0) {
	int offset = pPix->devKind, h2;
	CARD32	*srcPtr, *dstPtr;

	dstPtr = ((CARD32 *)pApm->FbBase) + (draw->box.x1 +
		draw->box.y1 * pApm->bytesPerScanline) / 4;
	dwords = w >> 2;

	while (ch-- > 0) {
	    srcPtr = (CARD32 *)pPix->devPrivate.ptr;

	    for (h2 = h; h2-- > 0; ) {
		for(i = dwords; i-- > 0; )
		    dstPtr[i] = XAAReverseBitOrder(srcPtr[i]);
		
		dstPtr += w;
		srcPtr += offset;
	    }
	}
    }
    else
    {
	int offset = pPix->devKind, h2;
	CARD32 *dstPtr;
	unsigned char *srcPtr;
	CARD32 tmp;
	char *ptmp = (char *)&tmp;
#define PUT(b)	do {	*ptmp++ = byte_reversed[(b)];		\
			if (ptmp == (char *)((&tmp) + 1)) {	\
			    *dstPtr++ = tmp;			\
			    ptmp = (char *)&tmp;		\
			}					\
		    } while(0)

	dstPtr = ((CARD32 *)pApm->FbBase) + (draw->box.x1 +
		draw->box.y1 * pApm->bytesPerScanline) / 4;
	while (ch-- > 0) {
	    srcPtr = (unsigned char *)pPix->devPrivate.ptr;
	    for (h2 = h; h2-- > 0; ) {
		for(i = 0; i < w; i++)
		    PUT(srcPtr[i]);
		
		srcPtr += offset;
	    }
	}
	while (ptmp-- != (char *)&tmp)
	    ((char *)dstPtr)[ptmp - (char *)&tmp] = *ptmp;
    }

    return &pApm->apmStippleCache;
}

/*********************************************************************************************/

int
ApmAccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    XAAInfoRecPtr	pXAAinfo;
    BoxRec		AvailFBArea;

    pApm->AccelInfoRec	= pXAAinfo = XAACreateInfoRec();
    if (!pXAAinfo)
	return FALSE;

    pApm->pScreen	= pScreen;
    pApm->displayWidth	= pScrn->display->virtualX;
    pApm->displayHeight	= pScrn->display->virtualY;
    pApm->bitsPerPixel	= pScrn->bitsPerPixel;
    pApm->bytesPerScanline= (pApm->displayWidth * pApm->bitsPerPixel) >> 3;

    /*
     * Set up the main acceleration flags.
     */
    pXAAinfo->Flags = PIXMAP_CACHE | LINEAR_FRAMEBUFFER | OFFSCREEN_PIXMAPS;
    pApm->OffscreenReserved += 1024;
    pXAAinfo->CacheMonoStipple = ApmCacheMonoStipple;

    if (pApm->bitsPerPixel != 24) {
	if (!pApm->noLinear) {
#define	XAA(s)		pXAAinfo->s = Apm##s
	    XAA(Sync);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    if (pApm->Chipset == AT3D && pApm->ChipRev >= 4) {
		pXAAinfo->CPUToScreenColorExpandFillFlags =
		  NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
		  | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
		  LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
		XAA(SetupForCPUToScreenColorExpandFill);
		XAA(SubsequentCPUToScreenColorExpandFill);
		pXAAinfo->ColorExpandBase	= pApm->BltMap;
		pXAAinfo->ColorExpandRange	= 32*1024;
	    }

	    /* Accelerated image transfers */
	    pXAAinfo->ImageWriteFlags	=
			    LEFT_EDGE_CLIPPING | NO_PLANEMASK |
			    SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD |
			    SYNC_AFTER_IMAGE_WRITE;
	    pXAAinfo->ImageWriteBase	= pApm->BltMap;
	    pXAAinfo->ImageWriteRange	= 32*1024;
	    XAA(SetupForImageWrite);
	    XAA(SubsequentImageWriteRect);
	    XAA(WritePixmap);

	    /* Accelerated screen-screen bitblts */
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenCopy);
	    XAA(SubsequentScreenToScreenCopy);

	    /* Accelerated Line drawing */
	    pXAAinfo->SolidLineFlags = NO_PLANEMASK | HARDWARE_CLIP_LINE;
	    XAA(SubsequentSolidBresenhamLine);
	    XAA(SetClippingRectangle);
	    pXAAinfo->SolidBresenhamLineErrorTermBits = 15;

	    /* Pattern fill */
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK |
			    HARDWARE_PATTERN_PROGRAMMED_BITS |
			    HARDWARE_PATTERN_SCREEN_ORIGIN;
	    XAA(SetupForMono8x8PatternFill);
	    XAA(SubsequentMono8x8PatternFillRect);
	    if (pApm->bitsPerPixel == 8) {
		pXAAinfo->Color8x8PatternFillFlags = NO_PLANEMASK |
				HARDWARE_PATTERN_SCREEN_ORIGIN;
		XAA(SetupForColor8x8PatternFill);
		XAA(SubsequentColor8x8PatternFillRect);
	    }
#undef XAA
	}
	else {
#define	XAA(s)		pXAAinfo->s = Apm##s##_IOP
	    XAA(Sync);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    if (pApm->Chipset == AT3D && pApm->ChipRev >= 4) {
		pXAAinfo->CPUToScreenColorExpandFillFlags =
		  NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
		  | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
		  LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
		XAA(SetupForCPUToScreenColorExpandFill);
		XAA(SubsequentCPUToScreenColorExpandFill);
		pXAAinfo->ColorExpandBase	= pApm->BltMap;
		pXAAinfo->ColorExpandRange	= 32*1024;
	    }

	    /* Accelerated image transfers */
	    pXAAinfo->ImageWriteFlags	=
			    LEFT_EDGE_CLIPPING | NO_PLANEMASK |
			    SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD;
	    pXAAinfo->ImageWriteBase	= pApm->BltMap;
	    pXAAinfo->ImageWriteRange	= 32*1024;
	    XAA(SetupForImageWrite);
	    XAA(SubsequentImageWriteRect);
	    XAA(WritePixmap);

	    /* Accelerated screen-screen bitblts */
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenCopy);
	    XAA(SubsequentScreenToScreenCopy);

	    /* Accelerated Line drawing */
	    pXAAinfo->SolidLineFlags = NO_PLANEMASK | HARDWARE_CLIP_LINE;
	    XAA(SubsequentSolidBresenhamLine);
	    XAA(SetClippingRectangle);
	    pXAAinfo->SolidBresenhamLineErrorTermBits = 15;

	    /* Pattern fill */
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK |
			    HARDWARE_PATTERN_PROGRAMMED_BITS |
			    HARDWARE_PATTERN_SCREEN_ORIGIN;
	    XAA(SetupForMono8x8PatternFill);
	    XAA(SubsequentMono8x8PatternFillRect);
	    if (pApm->bitsPerPixel == 8) {
		pXAAinfo->Color8x8PatternFillFlags = NO_PLANEMASK |
				HARDWARE_PATTERN_SCREEN_ORIGIN;
		XAA(SetupForColor8x8PatternFill);
		XAA(SubsequentColor8x8PatternFillRect);
	    }
#undef XAA
	}
    }
    else {
	if (!pApm->noLinear) {
#define	XAA(s)		pXAAinfo->s = Apm##s##24
	    XAA(Sync);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

#if 0
	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    if (pApm->Chipset == AT3D && pApm->ChipRev >= 4) {
		pXAAinfo->CPUToScreenColorExpandFillFlags =
		  NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
		  | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
		  LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
		XAA(SetupForCPUToScreenColorExpandFill);
		XAA(SubsequentCPUToScreenColorExpandFill);
		pXAAinfo->ColorExpandBase	= pApm->BltMap;
		pXAAinfo->ColorExpandRange	= 32*1024;
	    }

	    /* Accelerated image transfers */
	    pXAAinfo->ImageWriteFlags	=
			    LEFT_EDGE_CLIPPING | NO_PLANEMASK |
			    SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD |
			    SYNC_AFTER_IMAGE_WRITE;
	    pXAAinfo->ImageWriteBase	= pApm->BltMap;
	    pXAAinfo->ImageWriteRange	= 32*1024;
	    XAA(SetupForImageWrite);
	    XAA(SubsequentImageWriteRect);
	    XAA(WritePixmap);
#endif

	    /* Accelerated screen-screen bitblts */
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenCopy);
	    XAA(SubsequentScreenToScreenCopy);

#if 0
	    /* Accelerated Line drawing */
	    pXAAinfo->SolidLineFlags = NO_PLANEMASK | HARDWARE_CLIP_LINE;
	    XAA(SubsequentSolidBresenhamLine);
	    XAA(SetClippingRectangle);
	    pXAAinfo->SolidBresenhamLineErrorTermBits = 15;

	    /* Pattern fill */
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK |
			    HARDWARE_PATTERN_PROGRAMMED_BITS |
			    HARDWARE_PATTERN_SCREEN_ORIGIN;
	    XAA(SetupForMono8x8PatternFill);
	    XAA(SubsequentMono8x8PatternFillRect);
#endif
#undef XAA
	}
	else {
#define	XAA(s)		pXAAinfo->s = Apm##s##24##_IOP
	    XAA(Sync);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

#if 0
	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    if (pApm->Chipset == AT3D && pApm->ChipRev >= 4) {
		pXAAinfo->CPUToScreenColorExpandFillFlags =
		  NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
		  | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
		  LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
		XAA(SetupForCPUToScreenColorExpandFill);
		XAA(SubsequentCPUToScreenColorExpandFill);
		pXAAinfo->ColorExpandBase	= pApm->BltMap;
		pXAAinfo->ColorExpandRange	= 32*1024;
	    }

	    /* Accelerated image transfers */
	    pXAAinfo->ImageWriteFlags	=
			    LEFT_EDGE_CLIPPING | NO_PLANEMASK |
			    SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD;
	    pXAAinfo->ImageWriteBase	= pApm->BltMap;
	    pXAAinfo->ImageWriteRange	= 32*1024;
	    XAA(SetupForImageWrite);
	    XAA(SubsequentImageWriteRect);
	    XAA(WritePixmap);
#endif

	    /* Accelerated screen-screen bitblts */
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenCopy);
	    XAA(SubsequentScreenToScreenCopy);

#if 0
	    /* Accelerated Line drawing */
	    pXAAinfo->SolidLineFlags = NO_PLANEMASK | HARDWARE_CLIP_LINE;
	    XAA(SubsequentSolidBresenhamLine);
	    XAA(SetClippingRectangle);
	    pXAAinfo->SolidBresenhamLineErrorTermBits = 15;

	    /* Pattern fill */
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK |
			    HARDWARE_PATTERN_PROGRAMMED_BITS |
			    HARDWARE_PATTERN_SCREEN_ORIGIN;
	    XAA(SetupForMono8x8PatternFill);
	    XAA(SubsequentMono8x8PatternFillRect);
#endif
#undef XAA
	}
    }

    /* Pixmap cache setup */
    pXAAinfo->CachePixelGranularity = 64 / pApm->bitsPerPixel;
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pApm->displayWidth;
    AvailFBArea.y2 = (pScrn->videoRam * 1024 - pApm->OffscreenReserved) /
	(pApm->displayWidth * ((pApm->bitsPerPixel + 7) >> 3));

    xf86InitFBManager(pScreen, &AvailFBArea);

    return XAAInit(pScreen, pXAAinfo);
}


static void
Dump(void* start, u32 len)
{
  u8* i;
  int c = 0;
  ErrorF("Memory Dump. Start 0x%x length %d\n", (u32)start, len);
  for (i = (u8*)start; i < ((u8*)start+len); i++)
  {
    ErrorF("%02x ", *i);
    if (c++ % 25 == 24)
      ErrorF("\n");
  }
  ErrorF("\n");
}

#ifdef __GNUC__
void apm_stopit(void){__asm__ __volatile__("": : : "memory");}
#else
void apm_stopit(void){}
#endif
