/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_accel.c,v 1.5 1999/03/21 07:35:03 dawes Exp $ */


#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Version.h"
#include "miline.h"
#include "apm.h"


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

unsigned char curr[0x54];

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

/*********************************************************************************************/

int
ApmAccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    XAAInfoRecPtr	pXAAinfo;
    BoxRec		AvailFBArea;
    int			i;

    pApm->AccelInfoRec = pXAAinfo = XAACreateInfoRec();
    if (!pXAAinfo)
	return FALSE;

    /*
     * Set up the main acceleration flags.
     */
    pXAAinfo->Flags = PIXMAP_CACHE;
    pApm->OffscreenReserved += 1024;

    if (pScrn->depth != 24) {
	if (!pApm->noLinear) {
#define	XAA(s)		pXAAinfo->s = Apm##s
	    XAA(Sync);

	    /* Setup current register values */
	    for (i = 0; i < sizeof curr / 4; i++)
		((CARD32 *)curr)[i] = RDXL(0x30 + 4*i);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    pXAAinfo->CPUToScreenColorExpandFillFlags =
	      NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
	      | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
	      LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
	    XAA(SetupForCPUToScreenColorExpandFill);
	    XAA(SubsequentCPUToScreenColorExpandFill);
	    pXAAinfo->ColorExpandBase	= pApm->BltMap;
	    pXAAinfo->ColorExpandRange	= 32*1024;

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
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY;
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
	    if (pScrn->bitsPerPixel == 8) {
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

	    /* Setup current register values */
	    for (i = 0; i < sizeof curr / 4; i++)
		((CARD32 *)curr)[i] = RDXL_IOP(0x30 + 4*i);

	    /* Accelerated filled rectangles */
	    pXAAinfo->SolidFillFlags = NO_PLANEMASK;
	    XAA(SetupForSolidFill);
	    XAA(SubsequentSolidFillRect);

	    /* Accelerated screen to screen color expansion */
	    pXAAinfo->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK;
	    XAA(SetupForScreenToScreenColorExpandFill);
	    XAA(SubsequentScreenToScreenColorExpandFill);

	    /* Accelerated CPU to screen color expansion */
	    pXAAinfo->CPUToScreenColorExpandFillFlags =
	      NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
	      | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
	      LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
	    XAA(SetupForCPUToScreenColorExpandFill);
	    XAA(SubsequentCPUToScreenColorExpandFill);
	    pXAAinfo->ColorExpandBase	= pApm->BltMap;
	    pXAAinfo->ColorExpandRange	= 32*1024;

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
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY;
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
	    if (pScrn->bitsPerPixel == 8) {
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

	    /* Setup current register values */
	    for (i = 0; i < sizeof curr / 4; i++)
		((CARD32 *)curr)[i] = RDXL(0x30 + 4*i);

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
	    pXAAinfo->CPUToScreenColorExpandFillFlags =
	      NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
	      | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
	      LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
	    XAA(SetupForCPUToScreenColorExpandFill);
	    XAA(SubsequentCPUToScreenColorExpandFill);
	    pXAAinfo->ColorExpandBase	= pApm->BltMap;
	    pXAAinfo->ColorExpandRange	= 32*1024;

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
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY;
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

	    /* Setup current register values */
	    for (i = 0; i < sizeof curr / 4; i++)
		((CARD32 *)curr)[i] = RDXL_IOP(0x30 + 4*i);

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
	    pXAAinfo->CPUToScreenColorExpandFillFlags =
	      NO_PLANEMASK | SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_QWORD
	      | BIT_ORDER_IN_BYTE_MSBFIRST | LEFT_EDGE_CLIPPING |
	      LEFT_EDGE_CLIPPING_NEGATIVE_X | SYNC_AFTER_COLOR_EXPAND;
	    XAA(SetupForCPUToScreenColorExpandFill);
	    XAA(SubsequentCPUToScreenColorExpandFill);
	    pXAAinfo->ColorExpandBase	= pApm->BltMap;
	    pXAAinfo->ColorExpandRange	= 32*1024;

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
	    pXAAinfo->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY;
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
    pXAAinfo->CachePixelGranularity = 1;
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pScrn->videoRam * 1024 - pApm->OffscreenReserved) /
	(pScrn->displayWidth * ((pScrn->bitsPerPixel + 7) >> 3));

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
