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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/image_accel.c,v 1.6 1999/03/14 14:10:48 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#define TRIDENT_MMIO
#include "trident_regs.h"
#include "trident.h"

#include "xaalocal.h"

extern int TGUIRops_alu[];
extern int TGUIRops_Pixalu[];

static void ImageSync(ScrnInfoPtr pScrn);
static void ImageSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void ImageSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
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
static void ImageWritePixmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				unsigned char *src, int srcwidth, int rop,
    				unsigned int planemask, int transparency_color,
    				int bpp, int depth);
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

    IMAGE_OUT(0x20, 0xF0000000);
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
    IMAGE_OUT(0x20, 0x40000000 | pTrident->EngineOperation);
    IMAGE_OUT(0x44, 0x00000000);
    IMAGE_OUT(0x48, 0x00000000);
    IMAGE_OUT(0x20, 0x60000000 | pScrn->displayWidth<<16 | pScrn->displayWidth);
    IMAGE_OUT(0x6C, 0x00000000);
    IMAGE_OUT(0x70, 0x00000000);
    IMAGE_OUT(0x7C, 0x00000000);
    IMAGE_OUT(0x20, 0x10000000 | 4095 << 16 | 4095);
    IMAGE_OUT(0x30, 4095 << 16 | 4095);
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
			     HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY;

#if 0 /* Lines are slower than cfb/mi */
    infoPtr->SolidLineFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidLine = TridentSetupForSolidLine;
    infoPtr->SolidBresenhamLineErrorTermBits = 11;
    infoPtr->SubsequentSolidBresenhamLine = TridentSubsequentSolidBresenhamLine;
#endif

    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = ImageSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = ImageSubsequentFillRectSolid;
    
    infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK |
				       ONLY_TWO_BITBLT_DIRECTIONS |
				       NO_TRANSPARENCY;

    infoPtr->SetupForScreenToScreenCopy = 	
				ImageSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				ImageSubsequentScreenToScreenCopy;

#if 0
    infoPtr->Mono8x8PatternFillFlags =  NO_PLANEMASK | 
					NO_TRANSPARENCY |
					HARDWARE_PATTERN_SCREEN_ORIGIN |
					HARDWARE_PATTERN_PROGRAMMED_BITS;

    infoPtr->SetupForMono8x8PatternFill =
				ImageSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				ImageSubsequentMono8x8PatternFillRect;

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

    infoPtr->CPUToScreenColorExpandFillFlags = CPU_TRANSFER_PAD_DWORD |
				NO_TRANSPARENCY |
				SYNC_AFTER_COLOR_EXPAND |
				BIT_ORDER_IN_BYTE_MSBFIRST |
			        SCANLINE_PAD_DWORD |
			        NO_PLANEMASK;
    infoPtr->ColorExpandRange = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
    infoPtr->ColorExpandBase = pTrident->FbBase;
    infoPtr->SetupForCPUToScreenColorExpandFill = 	
				TridentSetupForCPUToScreenColorExpand;
    infoPtr->SubsequentCPUToScreenColorExpandFill = 		
				TridentSubsequentCPUToScreenColorExpand;

    infoPtr->WritePixmap = ImageWritePixmap;
    infoPtr->WritePixmapFlags = NO_PLANEMASK;
    infoPtr->SetupForImageWrite = ImageSetupForImageWrite;
    infoPtr->SubsequentImageWriteRect = ImageSubsequentImageWriteRect;
    infoPtr->ImageWriteFlags = NO_TRANSPARENCY | NO_PLANEMASK |
				CPU_TRANSFER_PAD_QWORD |
				SYNC_AFTER_IMAGE_WRITE;
    infoPtr->ImageWriteBase = pTrident->IOBase + 0x56;
    infoPtr->ImageWriteRange = 8;
#endif

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pTrident->FbMapSize - 4096) / (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void
ImageSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int count = 0, timeout = 0;
    int busy;

    for (;;) {
	IMAGEBUSY(busy);
	if (busy == 0x00800000) {
	    return;
	}
	count++;
	if (count == 10000000) {
	    ErrorF("Trident: BitBLT engine time-out.\n");
	    count = 9990000;
	    timeout++;
	    if (timeout == 8) {
		/* Reset BitBLT Engine */
		IMAGE_STATUS(0x00);
		return;
	    }
	}
    }
}

static void
ImageSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->ROP = rop;
    pTrident->BltScanDirection = 0;
    if ((xdir < 0) || (ydir < 0)) pTrident->BltScanDirection |= 1<<2;

    if (transparency_color != -1) {
	pTrident->DstEnable = TRUE;
	IMAGE_OUT(0x20, 0x70000000 | 1<<25 | (transparency_color&0xFFFFFF));
    } else {
	pTrident->DstEnable = FALSE;
	IMAGE_OUT(0x20, 0x70000000);
    }

    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_alu[rop]);
}

static void
ImageSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int clip = 0;

    if (pTrident->BltScanDirection) {
	IMAGE_OUT(0x00, (y1+h-1)<<16 | (x1+w-1));
	IMAGE_OUT(0x04, y1<<16 | x1);
	IMAGE_OUT(0x08, (y2+h-1)<<16 | (x2+w-1));
	IMAGE_OUT(0x0C, y2<<16 | x2);
    } else {
	IMAGE_OUT(0x00, y1<<16 | x1);
	IMAGE_OUT(0x04, (y1+h-1)<<16 | (x1+w-1));
	IMAGE_OUT(0x08, y2<<16 | x2);
	IMAGE_OUT(0x0C, (y2+h-1)<<16 | (x2+w-1));
    }

    if (pTrident->Clipping) clip = 1;

    IMAGE_OUT(0x24, 0x80000000 | 1<<22 | 1<<7 | (pTrident->ROP == GXcopy ? 0 : 1<<10) | pTrident->BltScanDirection | clip);

    if (!pTrident->UsePCIRetry)
    	ImageSync(pScrn);
}

static void
ImageSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x20, 0x10000000 | y1<<16 | x1);
    IMAGE_OUT(0x30, y2<<16 | x2);
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

    REPLICATE(color);
    TGUI_FCOLOUR(color);
    TGUI_BCOLOUR(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
}

static void 
ImageSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->BltScanDirection = 0;
    if (octant & YMAJOR) pTrident->BltScanDirection |= YMAJ;
    if (octant & XDECREASING) pTrident->BltScanDirection |= XNEG;
    if (octant & YDECREASING) pTrident->BltScanDirection |= YNEG;
    TGUI_DRAWFLAG(SOLIDFILL | STENCIL | pTrident->BltScanDirection);
    TGUI_SRC_XY(dmin-dmaj,dmin);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(dmin+e,len);
    TGUI_COMMAND(GE_BRESLINE);
    if (!pTrident->UsePCIRetry)
    	ImageSync(pScrn);
    CHECKCLIPPING;
}

static void
ImageSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    if (pTrident->DstEnable) {
	IMAGE_OUT(0x20, 0x70000000);
	pTrident->DstEnable = FALSE;
    }
    REPLICATE(color);
    pTrident->ROP = rop;
    IMAGE_OUT(0x20, 0x40000000 | pTrident->EngineOperation);
    IMAGE_OUT(0x44, color);
    IMAGE_OUT(0x48, color);
    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_alu[rop]);
}

static void
ImageSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int clip = 0;

    if (pTrident->Clipping) clip = 1;
    IMAGE_OUT(0x08, y<<16 | x);
    IMAGE_OUT(0x0C, (y+h-1)<<16 | x+w-1);
    IMAGE_OUT(0x24, 0x80000000 | 3<<22 | (pTrident->ROP == GXcopy ? 0 : 1<<10) | 1<<9 | clip);
    if (!pTrident->UsePCIRetry)
    	ImageSync(pScrn);
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
    IMAGE_OUT(0x44, fg);
    IMAGE_OUT(0x48, bg);
    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_alu[rop]);
}

static void 
ImageSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int srcx, int srcy, int offset)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

	IMAGE_OUT(0x00, srcy<<16 | srcx);
	IMAGE_OUT(0x04, (srcy+h-1)<<16 | (srcx+w-1));
	IMAGE_OUT(0x08, y<<16 | x);
	IMAGE_OUT(0x0C, (y+h-1)<<16 | (x+w-1));

    IMAGE_OUT(0x24, 0x80000000 | 3<<22 | 1<<7 | (pTrident->ROP == GXcopy ? 0 : 1<<10) | offset<<25);

    if (!pTrident->UsePCIRetry)
    	ImageSync(pScrn);
}

static void 
ImageSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
    int fg, int bg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    REPLICATE(fg);
    REPLICATE(bg);
    TGUI_FCOLOUR(fg);
    TGUI_BCOLOUR(bg);

    if (bg != -1) {
    	TGUI_DRAWFLAG(SRCMONO);
    } else {
	TGUI_DRAWFLAG(SRCMONO | 1<<12);
    }
    TGUI_SRC_XY(0,0);
    TGUI_FMIX(TGUIRops_alu[rop]);
}

static void
ImageSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
	int x, int y, int w, int h, int skipleft)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    ImageSetClippingRectangle(pScrn,x,y,((w+x)*(pScrn->bitsPerPixel>>3))-1,y+h-1);

#if 0
ErrorF("%d %d %d %d\n",x,y,w,h);
#endif
if (w == 0) return;
if (h == 0) return;

    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
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
ImageWritePixmap(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int rop,
    unsigned int planemask,
    int transparency_color,
    int bpp, int depth
)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    CARD32 *srcp;

    ImageSetClippingRectangle(pScrn,x,y,w+x-1,y+h-1);

#if 0
    if (w & 7)
    	w += 8 - (w & 7);
#endif

    if (transparency_color != -1) {
	pTrident->DstEnable = TRUE;
	IMAGE_OUT(0x20, 0x70000000 | 1<<27 | transparency_color);
    }
    IMAGE_OUT(0x00, 0);
    IMAGE_OUT(0x04, 0);
    IMAGE_OUT(0x08, y<<16 | x);
    IMAGE_OUT(0x0C, (y+h-1)<<16 | x+w-1);
    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_alu[rop]);
    IMAGE_OUT(0x24, 0x80000000 | 1<<22 | 1<<10 | 1);

#if 0
    if (pScrn->bitsPerPixel == 8)
    	w >>= 2;
    if (pScrn->bitsPerPixel == 16)
    	w >>= 1;
#endif
	w = (w + 3) >> 2;

    if (w&1) w+=1;
    w /= 2;
    while (h--) {
    	srcp = (CARD32 *)src;	/* Is *src really on a 4-byte boundary ?!?! */
	while (w--) {
	    IMAGE_OUT(0x56, *srcp++);
	    IMAGE_OUT(0x60, *srcp++);
	}
	src += srcwidth;
    }

    if (pTrident->UsePCIRetry)
    	SET_SYNC_FLAG(infoRec);
    else 
    	ImageSync(pScrn);

    if (pTrident->DstEnable) {
	IMAGE_OUT(0x20, 0x70000000);
	pTrident->DstEnable = FALSE;
    }
}

static void 
ImageSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_Pixalu[rop]);
    REPLICATE(fg);
    if (bg != -1) {
    REPLICATE(bg);
    IMAGE_OUT(0x20, 0x80000000 | 1<<27 |1<<26);
    } else {
    IMAGE_OUT(0x20, 0x80000000 | 1<<27);
    }
    IMAGE_OUT(0x30, (patternx & 0xFF000000)>>24);
    IMAGE_OUT(0x30, (patternx & 0xFF0000)>>16);
    IMAGE_OUT(0x30, (patternx & 0xFF00)>>8);
    IMAGE_OUT(0x30, (patternx & 0xFF));
    IMAGE_OUT(0x30, (patterny & 0xFF000000)>>24);
    IMAGE_OUT(0x30, (patterny & 0xFF0000)>>16);
    IMAGE_OUT(0x30, (patterny & 0xFF00)>>8);
    IMAGE_OUT(0x30, (patterny & 0xFF));
    IMAGE_OUT(0x50, fg);
    IMAGE_OUT(0x54, bg);
}

static void 
ImageSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    IMAGE_OUT(0x08, y<<16 | x);
    IMAGE_OUT(0x0C, (y+h-1)<<16 | x+w-1);
    IMAGE_OUT(0x24, 0x80000000 | 7<<18 | 1<<10 | 3<<22 | 1<<9);
    if (!pTrident->UsePCIRetry)
    	ImageSync(pScrn);
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
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
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
    	ImageSync(pScrn);
    CHECKCLIPPING;
}

static void ImageSetupForImageWrite(	
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x20, 0x90000000 | TGUIRops_alu[rop]);
}

static void ImageSubsequentImageWriteRect(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int skipleft
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    IMAGE_OUT(0x08, y<<16 | x);
    IMAGE_OUT(0x0C, (y+h-1)<<16 | x+w-1);
    IMAGE_OUT(0x24, 0x80000000 | 1<<22 | 1<<10 | 1<<9);
}
