/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_accel.c,v 1.8 1999/08/28 09:00:57 dawes Exp $ */


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
    ((struct ApmStippleCacheRec *)area->devPrivate.ptr)->apmStippleCached = FALSE;
}

static void
ApmMoveStipple(FBAreaPtr from, FBAreaPtr to)
{
    struct ApmStippleCacheRec *pApm = (struct ApmStippleCacheRec *)to->devPrivate.ptr;

    pApm->apmStippleCache.x = to->box.x1;
    pApm->apmStippleCache.y += to->box.y1 - from->box.y1;
    /* TODO : move data */
}

/*
 * ApmCacheMonoStipple
 * because my poor AT3D needs stipples stored linearly in memory.
 */
static XAACacheInfoPtr
ApmCacheMonoStipple(ScrnInfoPtr pScrn, PixmapPtr pPix)
{
    APMDECL(pScrn);
    int		w = pPix->drawable.width, W = (w + 31) & ~31;
    int		h = pPix->drawable.height;
    int		i, j, dwords, mem, width, funcNo;
    FBAreaPtr	draw;
    struct ApmStippleCacheRec	*pCache;
    unsigned char	*srcPtr;
    CARD32		*dstPtr;

    for (i = 0; i < APM_CACHE_NUMBER; i++)
	if ((pApm->apmCache[i].apmStippleCache.serialNumber == pPix->drawable.serialNumber)
		&& pApm->apmCache[i].apmStippleCached &&
		(pApm->apmCache[i].apmStippleCache.fg == -1) &&
		(pApm->apmCache[i].apmStippleCache.bg == -1)) {
	    pApm->apmCache[i].apmStippleCache.trans_color = -1;
	    return &pApm->apmCache[i].apmStippleCache;
	}
    if ((i = ++pApm->apmCachePtr) >= APM_CACHE_NUMBER)
	i = pApm->apmCachePtr = 0;
    pCache = &pApm->apmCache[i];
    if (pCache->apmStippleCached) {
	pCache->apmStippleCached = FALSE;
	xf86FreeOffscreenArea(pCache->area);
    }

    draw = xf86AllocateLinearOffscreenArea(pApm->pScreen, (W * h + 7) / 8, 8,
				    ApmMoveStipple, ApmRemoveStipple, pCache);
    if (!draw)
	return NULL;	/* Let's hope this will never happen... */

    pCache->area = draw;
    pCache->apmStippleCache.serialNumber = pPix->drawable.serialNumber;
    pCache->apmStippleCache.trans_color =
	pCache->apmStippleCache.bg =
	pCache->apmStippleCache.fg = -1;
    pCache->apmStippleCache.orig_w = w;
    pCache->apmStippleCache.orig_h = h;
    pCache->apmStippleCache.x = draw->box.x1;
    pCache->apmStippleCache.y = draw->box.y1 + ((pCache - pApm->apmCache) + 1) * pApm->Scanlines;
    mem = ((draw->box.x2 - draw->box.x1) * (draw->box.y2 - draw->box.y1) *
			pApm->bitsPerPixel) / (W * h);
    width = 2;
    while (width * width <= mem)
	width++;
    width--;
    pCache->apmStippleCache.w = (width * W + pApm->bitsPerPixel - 1) /
			pApm->bitsPerPixel;
    pCache->apmStippleCache.h = ((draw->box.x2 - draw->box.x1) *
			(draw->box.y2 - draw->box.y1)) /
			pCache->apmStippleCache.w;
    pCache->apmStippleCached = TRUE;

    if (w < 32) {
	if (w & (w - 1))	funcNo = 1;
	else			funcNo = 0;
    } else			funcNo = 2;

    dstPtr = ((CARD32 *)pApm->FbBase) + (draw->box.x1 +
			draw->box.y1 * pApm->bytesPerScanline) / 4;
    j = 0;
    dwords = (pCache->apmStippleCache.w * pApm->bitsPerPixel) / 32;
    while (j + h <= pCache->apmStippleCache.h) {
	srcPtr = (unsigned char *)pPix->devPrivate.ptr;
	for (i = h; --i >= 0; ) {
	    (*XAAStippleScanlineFuncMSBFirst[funcNo])(dstPtr, (CARD32 *)srcPtr, 0, w, dwords);
	    srcPtr += pPix->devKind;
	    dstPtr += dwords;
	}
	j += h;
    }
    srcPtr = (unsigned char *)pPix->devPrivate.ptr;
    for (i = pCache->apmStippleCache.h - j ; --i >= 0; ) {
	(*XAAStippleScanlineFuncMSBFirst[funcNo])(dstPtr, (CARD32 *)srcPtr, 0, w, dwords);
	srcPtr += pPix->devKind;
	dstPtr += dwords;
    }

    return &pCache->apmStippleCache;
}

/*********************************************************************************************/

int
ApmAccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    XAAInfoRecPtr	pXAAinfo;
    BoxRec		AvailFBArea;
    int			mem, ScratchMemOffset, i, stat;

    pApm->AccelInfoRec	= pXAAinfo = XAACreateInfoRec();
    if (!pXAAinfo)
	return FALSE;

    pApm->pScreen	= pScreen;
    pApm->displayWidth	= pScrn->display->virtualX;
    pApm->displayHeight	= pScrn->display->virtualY;
    pApm->bitsPerPixel	= pScrn->bitsPerPixel;
    pApm->bytesPerScanline= (pApm->displayWidth * pApm->bitsPerPixel) >> 3;
    mem			= pScrn->videoRam << 10;
    pApm->Scanlines	= mem / pApm->bytesPerScanline + 1;
    /*
     * Reserve at list one line for transparent 8x8 mono2color expansion
     */
    ScratchMemOffset	= ((mem - pApm->OffscreenReserved) /
			pApm->bytesPerScanline - 1) * pApm->bytesPerScanline;
    pApm->ScratchMemSize= mem - ScratchMemOffset - pApm->OffscreenReserved;
    pApm->ScratchMemOffset	= ScratchMemOffset;
    switch (pApm->bitsPerPixel) {
    case 8:
    case 24:
	pApm->ScratchMemWidth =
		(mem - ScratchMemOffset - pApm->OffscreenReserved) / 1;
	pApm->ScratchMem =
		((ScratchMemOffset & 0xFFF000) << 4) |
		    (ScratchMemOffset & 0xFFF);
	break;

    case 16:
	pApm->ScratchMemWidth =
		(mem - ScratchMemOffset - pApm->OffscreenReserved) / 2;
	pApm->ScratchMem =
		((ScratchMemOffset & 0xFFE000) << 3) |
		    ((ScratchMemOffset & 0x1FFE) >> 1);
	break;

    case 32:
	pApm->ScratchMemWidth =
		(mem - ScratchMemOffset - pApm->OffscreenReserved) / 4;
	pApm->ScratchMem =
		((ScratchMemOffset & 0xFFC000U) << 2) |
		    ((ScratchMemOffset & 0x3FFC) >> 2);
	break;
    }
    pApm->OffscreenReserved = mem - ScratchMemOffset;

    pApm->apmMMIO_Init = TRUE;

    /*
     * Abort
     */
    if (pApm->noLinear) {
	stat = RDXL_IOP(0x1FC);
	while ((stat & (STATUS_HOSTBLTBUSY | STATUS_ENGINEBUSY)) ||
		((stat & STATUS_FIFO) < 8)) {
	    WRXB_IOP(0x1FC, 0);
	    stat = RDXL_IOP(0x1FC);
	}
    }
    else {
	stat = RDXL_M(0x1FC);
	while ((stat & (STATUS_HOSTBLTBUSY | STATUS_ENGINEBUSY)) ||
		((stat & STATUS_FIFO) < 8)) {
	    WRXB_M(0x1FC, 0);
	    stat = RDXL_M(0x1FC);
	}
    }

    pApm->Setup_DEC = 0;
    switch(pApm->bitsPerPixel)
    {
      case 8:
           pApm->Setup_DEC |= DEC_BITDEPTH_8;
           break;
      case 16:
           pApm->Setup_DEC |= DEC_BITDEPTH_16;
           break;
      case 24:
           pApm->Setup_DEC |= DEC_BITDEPTH_24;
           break;
      case 32:
           pApm->Setup_DEC |= DEC_BITDEPTH_32;
           break;
      default:
           xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Cannot set up drawing engine control for bpp = %d\n",
		    pScrn->bitsPerPixel);
           break;
    }

    switch(pApm->displayWidth)
    {
      case 640:
           pApm->Setup_DEC |= DEC_WIDTH_640;
           break;
      case 800:
           pApm->Setup_DEC |= DEC_WIDTH_800;
           break;
      case 1024:
           pApm->Setup_DEC |= DEC_WIDTH_1024;
           break;
      case 1152:
           pApm->Setup_DEC |= DEC_WIDTH_1152;
           break;
      case 1280:
           pApm->Setup_DEC |= DEC_WIDTH_1280;
           break;
      case 1600:
           pApm->Setup_DEC |= DEC_WIDTH_1600;
           break;
      default:
           xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Cannot set up drawing engine control "
		       "for screen width = %d\n", pApm->displayWidth);
           break;
    }

    /* Setup current register values */
    for (i = 0; i < sizeof(pApm->regcurr) / 4; i++)
	((CARD32 *)curr)[i] = RDXL(0x30 + 4*i);

    SETCLIP_CTRL(1);
    SETCLIP_CTRL(0);
    SETBYTEMASK(0x00);
    SETBYTEMASK(0xFF);
    SETROP(ROP_S_xor_D);
    SETROP(ROP_S);
    pApm->apmMMIO_Init = TRUE;

    /*
     * Set up the main acceleration flags.
     */
    pXAAinfo->Flags = PIXMAP_CACHE | LINEAR_FRAMEBUFFER | OFFSCREEN_PIXMAPS;
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
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK | NO_TRANSPARENCY |
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
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK | NO_TRANSPARENCY |
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
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK | NO_TRANSPARENCY |
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
	    pXAAinfo->Mono8x8PatternFillFlags = NO_PLANEMASK | NO_TRANSPARENCY |
			    HARDWARE_PATTERN_PROGRAMMED_BITS |
			    HARDWARE_PATTERN_SCREEN_ORIGIN;
	    XAA(SetupForMono8x8PatternFill);
	    XAA(SubsequentMono8x8PatternFillRect);
#endif
#undef XAA
	}
    }

    /*
     * Init Rush extension
     */
    REGION_INIT(pApm->pScreen, &pApm->apmLockedRegion, (BoxRec *)0, 1);
#ifdef XF86RUSH_EXT
    /* XXX Should this be called once per screen? */
    XFree86RushExtensionInit();
#endif

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
