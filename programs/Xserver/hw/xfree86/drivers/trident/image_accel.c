/*
 * Copyright 1997,1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 * 
 * Trident 3DImage' accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/image_accel.c,v 1.11 1999/06/20 07:14:32 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "trident_regs.h"
#include "trident.h"

#include "xaarop.h"
#include "xaalocal.h"

static void ImageSync(ScrnInfoPtr pScrn);
static void ImageSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void ImageSubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
static void ImageSetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void ImageSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
static void ImageSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);
static void ImageSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void ImageSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void ImageSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    				int x, int y, int w, int h, int srcx, int srcy,
				int offset);
static void ImageSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void ImageSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void ImageSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
				int x2, int y2);
static void ImageDisableClipping(ScrnInfoPtr pScrn);
static void ImageSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void ImageSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void ImageSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, 
				int rop, unsigned int planemask, int trans_col);
static void ImageSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void ImageSetupForImageWrite(	
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
);
static void ImageSubsequentImageWriteRect(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int skipleft
);

static void
ImageInitializeAccelerator(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x2120, 0xF0000000);
    switch (pScrn->depth) {
	case 8:
	    pTrident->EngineOperation = 0;
	    break;
	case 15:
	    pTrident->EngineOperation = 5;
	    break;
	case 16:
	    pTrident->EngineOperation = 1;
	    break;
	case 24:
	    pTrident->EngineOperation = 2;
	    break;
    }
    IMAGE_OUT(0x2120, 0x40000000 | pTrident->EngineOperation);
    IMAGE_OUT(0x2120, 0x80000000);
    IMAGE_OUT(0x2144, 0x00000000);
    IMAGE_OUT(0x2148, 0x00000000);
    IMAGE_OUT(0x2120, 0x60000000 | pScrn->displayWidth<<16 | pScrn->displayWidth);
    IMAGE_OUT(0x216C, 0x00000000);
    IMAGE_OUT(0x2170, 0x00000000);
    IMAGE_OUT(0x217C, 0x00000000);
    IMAGE_OUT(0x2120, 0x10000000 | 4095 << 16 | 4095);
    IMAGE_OUT(0x2130, 4095 << 16 | 4095);
    pTrident->DstEnable = FALSE;
}

Bool
ImageAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    BoxRec AvailFBArea;

    pTrident->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    ImageInitializeAccelerator(pScrn);

    infoPtr->Flags = PIXMAP_CACHE |
		     LINEAR_FRAMEBUFFER |
		     OFFSCREEN_PIXMAPS;
 
    infoPtr->Sync = ImageSync;

    infoPtr->SetClippingRectangle = ImageSetClippingRectangle;
    infoPtr->DisableClipping = ImageDisableClipping;
    infoPtr->ClippingFlags = HARDWARE_CLIP_SOLID_FILL |
			     HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY |
			     HARDWARE_CLIP_MONO_8x8_FILL;

#if 0
    infoPtr->SolidLineFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidLine = ImageSetupForFillRectSolid;
    infoPtr->SolidBresenhamLineErrorTermBits = 11;
    infoPtr->SubsequentSolidBresenhamLine = ImageSubsequentSolidBresenhamLine;
#endif

    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = ImageSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = ImageSubsequentFillRectSolid;
    
    infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK |
				       NO_TRANSPARENCY |
				       ONLY_TWO_BITBLT_DIRECTIONS;

    infoPtr->SetupForScreenToScreenCopy = 	
				ImageSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				ImageSubsequentScreenToScreenCopy;

    infoPtr->Mono8x8PatternFillFlags =  NO_PLANEMASK | 
					BIT_ORDER_IN_BYTE_MSBFIRST |
					HARDWARE_PATTERN_SCREEN_ORIGIN |
					HARDWARE_PATTERN_PROGRAMMED_BITS;

    infoPtr->SetupForMono8x8PatternFill =
				ImageSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				ImageSubsequentMono8x8PatternFillRect;

#if 0
    infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK | 
					HARDWARE_PATTERN_SCREEN_ORIGIN | 
					NO_TRANSPARENCY | 
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForColor8x8PatternFill =
				TridentSetupForColor8x8PatternFill;
    infoPtr->SubsequentColor8x8PatternFillRect = 
				TridentSubsequentColor8x8PatternFillRect;

    infoPtr->ScreenToScreenColorExpandFillFlags = NO_TRANSPARENCY |NO_PLANEMASK;

    infoPtr->SetupForScreenToScreenColorExpandFill = 	
				ImageSetupForScreenToScreenColorExpand;
    infoPtr->SubsequentScreenToScreenColorExpandFill = 		
				ImageSubsequentScreenToScreenColorExpand;
#endif

#if 0
    infoPtr->CPUToScreenColorExpandFillFlags = CPU_TRANSFER_PAD_QWORD |
				NO_TRANSPARENCY |
				SYNC_AFTER_COLOR_EXPAND |
				BIT_ORDER_IN_BYTE_MSBFIRST |
			        SCANLINE_PAD_DWORD |
			        NO_PLANEMASK;
    infoPtr->ColorExpandRange = 0x8000;
    infoPtr->ColorExpandBase = pTrident->IOBase + 0x10000;
    infoPtr->SetupForCPUToScreenColorExpandFill = 	
				ImageSetupForCPUToScreenColorExpand;
    infoPtr->SubsequentCPUToScreenColorExpandFill = 		
				ImageSubsequentCPUToScreenColorExpand;
#endif

#if 0
    infoPtr->SetupForImageWrite = ImageSetupForImageWrite;
    infoPtr->SubsequentImageWriteRect = ImageSubsequentImageWriteRect;
    infoPtr->ImageWriteFlags = NO_TRANSPARENCY | NO_PLANEMASK |
				CPU_TRANSFER_PAD_QWORD |
				SYNC_AFTER_IMAGE_WRITE;
    infoPtr->ImageWriteBase = pTrident->IOBase + 0x10000;
    infoPtr->ImageWriteRange = 0x8000;
#endif

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pTrident->FbMapSize - 4096) / (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);
    if (AvailFBArea.y2 > 4095) AvailFBArea.y2 = 4095;

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void
ImageSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int busy;
    int cnt = 500000;

    if (pTrident->Clipping) ImageDisableClipping(pScrn);

    IMAGEBUSY(busy);
    while (busy != 0) {
	if (--cnt < 0) {
	    ErrorF("GE timeout\n");
	    IMAGE_OUT(0x2164, 0x00000000);
	    ImageInitializeAccelerator(pScrn);
	    break;
	}
    	IMAGEBUSY(busy);
    }
}

static void
ImageSyncClip(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int busy;
    int cnt = 500000;

    IMAGEBUSY(busy);
    while (busy != 0) {
	if (--cnt < 0) {
	    ErrorF("GE timeout\n");
	    IMAGE_OUT(0x2164, 0x00000000);
	    ImageInitializeAccelerator(pScrn);
	    break;
	}
    	IMAGEBUSY(busy);
    }
}

static void
ImageSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->BltScanDirection = 0;
    if ((xdir < 0) || (ydir < 0)) pTrident->BltScanDirection |= 1<<2;

    IMAGE_OUT(0x2120, 0x80000000);
    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
}

static void
ImageSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    if (pTrident->BltScanDirection) {
	IMAGE_OUT(0x2100, (y1+h-1)<<16 | (x1+w-1));
	IMAGE_OUT(0x2104, y1<<16 | x1);
	IMAGE_OUT(0x2108, (y2+h-1)<<16 | (x2+w-1));
	IMAGE_OUT(0x210C, y2<<16 | x2);
    } else {
	IMAGE_OUT(0x2100, y1<<16 | x1);
	IMAGE_OUT(0x2104, (y1+h-1)<<16 | (x1+w-1));
	IMAGE_OUT(0x2108, y2<<16 | x2);
	IMAGE_OUT(0x210C, (y2+h-1)<<16 | (x2+w-1));
    }

    IMAGE_OUT(0x2124, 0x80000000 | 1<<22 | 1<<7 | 1<<10 | pTrident->BltScanDirection | (pTrident->Clipping ? 1 : 0));

    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void
ImageSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x2120, 0x10000000 | y1<<16 | x1);
    IMAGE_OUT(0x2130, y2<<16 | x2);
    pTrident->Clipping = TRUE;
}

static void
ImageDisableClipping(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    pTrident->Clipping = FALSE;
}
    
static void
ImageSetupForSolidLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
    IMAGE_OUT(0x2144, color);
}

static void 
ImageSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int direction = 0;

    pTrident->BltScanDirection = 0;
    if (octant & YMAJOR) pTrident->BltScanDirection = 1<<18;
    if (octant & XDECREASING) direction |= 1<<30;
    if (octant & YDECREASING) direction |= 1<<31;

    IMAGE_OUT(0x2124, 0x20000000 | 1<<22 | 1<<10 | 1<<9);
    IMAGE_OUT(0x21FC, 0x20000000 | 1<<19 | pTrident->BltScanDirection);
    IMAGE_OUT(0x2200, y<<16 | x);
    IMAGE_OUT(0x2204, direction | dmaj << 16 | dmin);
    IMAGE_OUT(0x2208, (e << 16) | len);

    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void
ImageSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x2120, 0x80000000);
    REPLICATE(color);
    pTrident->ROP = rop;
    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
    IMAGE_OUT(0x2144, color);
}

static void
ImageSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int clip = 0;

    if (pTrident->Clipping) clip = 1;

    IMAGE_OUT(0x2108, y<<16 | x);
    IMAGE_OUT(0x210C, ((y+h-1)<<16) | (x+w-1));
    IMAGE_OUT(0x2124, 0x80000000 | 1<<22 | 1<<10 | 1<<9 | clip);
    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void 
ImageSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int fg, int bg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->ROP = rop;

    REPLICATE(bg);
    REPLICATE(fg);
    IMAGE_OUT(0x2144, fg);
    IMAGE_OUT(0x2148, bg);
    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
}

static void 
ImageSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int srcx, int srcy, int offset)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

	IMAGE_OUT(0x2100, srcy<<16 | srcx);
	IMAGE_OUT(0x2104, ((srcy+h-1)<<16) | (srcx+w-1));
	IMAGE_OUT(0x2108, y<<16 | x);
	IMAGE_OUT(0x210C, ((y+h-1)<<16) | (x+w-1));

    IMAGE_OUT(0x2124, 0x80000000 | 3<<22 | 1<<7 | 1<<10 | offset<<18);

    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void 
ImageSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
    int fg, int bg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    if (bg == -1) pTrident->ROP = 2<<22;
    else	  pTrident->ROP = 3<<22;
    REPLICATE(fg);
    REPLICATE(bg);
    IMAGE_OUT(0x2120, 0x80000000);
    IMAGE_OUT(0x2144, fg);
    IMAGE_OUT(0x2148, bg);
    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
}

static void
ImageSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
	int x, int y, int w, int h, int skipleft)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

if ((w==0) || (h==0))return;
    IMAGE_OUT(0x2108, y<<16 | x);
    IMAGE_OUT(0x210C, ((y+h-1)<<16) | (x+w-1));
    IMAGE_OUT(0x2124, 0x80000000 | pTrident->ROP | 1<<10);
}

static void MoveBYTE(
   register unsigned char* dest,
   register unsigned char* src,
   register int bytes
)
{
     while(bytes) {
	*dest = *src;
	src += 1;
	dest += 1;
	bytes -= 1;
     }	
}

static void MoveDWORDS(
   register CARD32* dest,
   register CARD32* src,
   register int dwords )
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
	*(dest + 3) = *(src + 3);
	src += 4;
	dest += 4;
	dwords -= 4;
     }	
     if (!dwords) return;
     *dest = *src;
     dest += 1;
     src += 1;
     if (dwords == 1) return;
     *dest = *src;
     dest += 1;
     src += 1;
     if (dwords == 2) return;
     *dest = *src;
     dest += 1;
     src += 1;
}

static void 
ImageSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int mix = XAAHelpPatternROP(pScrn, &fg, &bg, planemask, &rop);

    IMAGE_OUT(0x2120, 0x90000000 | rop);
    if (bg == -1) {
	REPLICATE(fg);
	IMAGE_OUT(0x2120, 0x80000000 | 1<<27);
	IMAGE_OUT(0x2130, patternx);
	IMAGE_OUT(0x2134, patterny);
	IMAGE_OUT(0x2150, fg);
    } else {
	REPLICATE(bg);
	REPLICATE(fg);
	IMAGE_OUT(0x2120, 0x80000000 | 1<<27 | 1<<26);
	IMAGE_OUT(0x2130, patternx);
	IMAGE_OUT(0x2134, patterny);
	IMAGE_OUT(0x2150, fg);
	IMAGE_OUT(0x2154, bg);
    }
}

static void 
ImageSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int clip = 0;

    if (pTrident->Clipping) clip = 1;
    IMAGE_OUT(0x2108, y<<16 | x);
    IMAGE_OUT(0x210C, (y+h-1)<<16 | (x+w-1));
    IMAGE_OUT(0x2124, 0x80000000 | 7<<18 | 1<<22 | 1<<10 | 1<<9 | clip);
    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void 
ImageSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int rop,
					   unsigned int planemask,
					   int trans_col)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_PATLOC(((patterny * pScrn->displayWidth * pScrn->bitsPerPixel / 8) +
		 (patternx * pScrn->bitsPerPixel / 8)) >> 6);
    if (trans_col == -1) {
	REPLICATE(trans_col);
	TGUI_BCOLOUR(trans_col);
	TGUI_DRAWFLAG(PAT2SCR | 1<<12);
    } else
    	TGUI_DRAWFLAG(PAT2SCR);
    TGUI_FMIX(XAAPatternROP[rop]);
}

static void 
ImageSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UsePCIRetry)
    	ImageSyncClip(pScrn);
}

static void ImageSetupForImageWrite(	
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->ROP = rop;
    IMAGE_OUT(0x2120, 0x80000000);
    IMAGE_OUT(0x2120, 0x90000000 | XAACopyROP[rop]);
}

static void ImageSubsequentImageWriteRect(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int skipleft
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x2108, y<<16 | x);
    IMAGE_OUT(0x210C, (y+h-1)<<16 | (x+w-1));
    IMAGE_OUT(0x2124, 0x80000000 | 1<<22 | ((pTrident->ROP == GXcopy) ? 0 : 1<<10));
}
