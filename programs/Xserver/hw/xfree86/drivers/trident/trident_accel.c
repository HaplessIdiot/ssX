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
 * Trident accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_accel.c,v 1.4 1999/01/11 05:13:31 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "trident_regs.h"
#include "trident.h"

#include "xaalocal.h"

extern int TGUIRops_alu[];
extern int TGUIRops_Pixalu[];

#ifdef TRIDENT_MMIO

#define MMIONAME(x)			x##MMIO

#define TridentAccelInit		MMIONAME(TridentAccelInit)
#define TridentInitializeAccelerator	MMIONAME(TridentInitializeAccelerator)
#define TridentSync			MMIONAME(TridentSync)
#define TridentSetupForSolidLine	MMIONAME(TridentSetupForSolidLine)
#define Trident9685SetupForSolidLine	MMIONAME(Trident9685SetupForSolidLine)
#define TridentSubsequentSolidBresenhamLine \
			MMIONAME(TridentSubsequentSolidBresenhamLine)
#define TridentSetupForFillRectSolid 	MMIONAME(TridentSetupForFillRectSolid)
#define Trident9685SetupForFillRectSolid \
			MMIONAME(Trident9685SetupForFillRectSolid)
#define TridentSubsequentFillRectSolid	MMIONAME(TridentSubsequentFillRectSolid)
#define TridentSetupForScreenToScreenCopy \
			MMIONAME(TridentSetupForScreenToScreenCopy)
#define TridentSubsequentScreenToScreenCopy \
			MMIONAME(TridentSubsequentScreenToScreenCopy)
#define TridentSetupForScreenToScreenColorExpand \
			MMIONAME(TridentSetupForScreenToScreenColorExpand)
#define TridentSubsequentScreenToScreenColorExpand \
			MMIONAME(TridentSubsequentScreenToScreenColorExpand)
#define TridentSetClippingRectangle	MMIONAME(TridentSetClippingRectangle)
#define TridentWritePixmap		MMIONAME(TridentWritePixmap)
#define TridentSetupForMono8x8PatternFill \
			MMIONAME(TridentSetupForMono8x8PatternFill)
#define TridentSubsequentMono8x8PatternFillRect \
			MMIONAME(TridentSubsequentMono8x8PatternFillRect)
#define TridentSetupForColor8x8PatternFill \
			MMIONAME(TridentSetupForColor8x8PatternFill)
#define TridentSubsequentColor8x8PatternFillRect \
			MMIONAME(TridentSubsequentColor8x8PatternFillRect)

#endif

static void TridentSync(ScrnInfoPtr pScrn);
static void TridentSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Trident9685SetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void TridentSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
static void TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Trident9685SetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void TridentSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
static void TridentSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);
static void TridentSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void TridentSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void TridentSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    				int x, int y, int w, int h, int srcx, int srcy,
				int offset);
static void TridentSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void TridentSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void TridentSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
				int x2, int y2);
static void TridentWritePixmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				unsigned char *src, int srcwidth, int rop,
    				unsigned int planemask, int transparency_color,
    				int bpp, int depth);
static void TridentSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void TridentSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void TridentSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, 
				int rop, unsigned int planemask, int trans_col);
static void TridentSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);

static void
TridentInitializeAccelerator(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    CHECKCLIPPING;
    if (pTrident->Chipset == PROVIDIA9682)
    	pTrident->EngineOperation |= 0x100; /* Disable Clipping */
    TGUI_OPERMODE(pTrident->EngineOperation);
}

Bool
TridentAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    BoxRec AvailFBArea;

    pTrident->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    TridentInitializeAccelerator(pScrn);

    infoPtr->Flags = PIXMAP_CACHE |
		     OFFSCREEN_PIXMAPS |
		     LINEAR_FRAMEBUFFER;
 
    infoPtr->Sync = TridentSync;

#if 0 /* Lines are slower than cfb/mi */
    infoPtr->SolidLineFlags = NO_PLANEMASK;
    if (pTrident->Chipset == PROVIDIA9685)
    	infoPtr->SetupForSolidLine = Trident9685SetupForSolidLine;
    else
    	infoPtr->SetupForSolidLine = TridentSetupForSolidLine;
    
    infoPtr->SolidBresenhamLineErrorTermBits = 11;
    infoPtr->SubsequentSolidBresenhamLine = TridentSubsequentSolidBresenhamLine;
#endif

    infoPtr->SolidFillFlags = NO_PLANEMASK;
    if (pTrident->Chipset == PROVIDIA9685)
    	infoPtr->SetupForSolidFill = Trident9685SetupForFillRectSolid;
    else
    	infoPtr->SetupForSolidFill = TridentSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = TridentSubsequentFillRectSolid;
    
    infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK;

    if (!HAS_DST_TRANS) infoPtr->ScreenToScreenCopyFlags |= NO_TRANSPARENCY;

    infoPtr->SetupForScreenToScreenCopy = 	
				TridentSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				TridentSubsequentScreenToScreenCopy;

    infoPtr->Mono8x8PatternFillFlags =  NO_PLANEMASK | 
					NO_TRANSPARENCY |
					HARDWARE_PATTERN_SCREEN_ORIGIN | 
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForMono8x8PatternFill =
				TridentSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				TridentSubsequentMono8x8PatternFillRect;
    infoPtr->MonoPatternPitch = 64;

#if 0
    infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK | 
					HARDWARE_PATTERN_SCREEN_ORIGIN | 
					NO_TRANSPARENCY | 
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForColor8x8PatternFill =
				TridentSetupForColor8x8PatternFill;
    infoPtr->SubsequentColor8x8PatternFillRect = 
				TridentSubsequentColor8x8PatternFillRect;
#endif

#if 0
    infoPtr->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK |
						  NO_TRANSPARENCY;

    infoPtr->SetupForScreenToScreenColorExpandFill = 	
				TridentSetupForScreenToScreenColorExpand;
    infoPtr->SubsequentScreenToScreenColorExpandFill = 		
				TridentSubsequentScreenToScreenColorExpand;
#endif

    if (pTrident->Chipset == PROVIDIA9685) {
        infoPtr->CPUToScreenColorExpandFillFlags = CPU_TRANSFER_PAD_DWORD |
				LEFT_EDGE_CLIPPING |
				LEFT_EDGE_CLIPPING_NEGATIVE_X |
				SYNC_AFTER_COLOR_EXPAND |
				BIT_ORDER_IN_BYTE_MSBFIRST |
			        SCANLINE_PAD_DWORD |
				NO_TRANSPARENCY |
			        NO_PLANEMASK;
        infoPtr->ColorExpandRange = pScrn->displayWidth * pScrn->bitsPerPixel/8;
    	infoPtr->ColorExpandBase = pTrident->FbBase;
    	infoPtr->SetupForCPUToScreenColorExpandFill = 	
				TridentSetupForCPUToScreenColorExpand;
    	infoPtr->SubsequentCPUToScreenColorExpandFill = 		
				TridentSubsequentCPUToScreenColorExpand;
    }

    /* Requires clipping, therefore only 9660's or above */
    if (pTrident->Chipset > CYBER9320) {
    	infoPtr->WritePixmap = TridentWritePixmap;
    	infoPtr->WritePixmapFlags = NO_PLANEMASK;
   
    	if (!HAS_DST_TRANS) infoPtr->WritePixmapFlags |= NO_TRANSPARENCY;
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pTrident->FbMapSize - 4096) / (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);

    if (AvailFBArea.y2 > 2048) AvailFBArea.y2 = 2047;

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void
TridentSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int count = 0, timeout = 0;
    int busy;

    infoRec->NeedToSync = FALSE;

    for (;;) {
	BLTBUSY(busy);
	if (busy != GE_BUSY) {
	    return;
	}
	count++;
	if (count == 10000000) {
	    ErrorF("Trident: BitBLT engine time-out.\n");
	    count = 9990000;
	    timeout++;
	    if (timeout == 8) {
		/* Reset BitBLT Engine */
		TGUI_STATUS(0x00);
		return;
	    }
	}
    }
}

static void
TridentSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int dst = 0;

    pTrident->BltScanDirection = 0;
    if (xdir < 0) pTrident->BltScanDirection |= XNEG;
    if (ydir < 0) pTrident->BltScanDirection |= YNEG;

    if (transparency_color != -1) {
	pTrident->DstEnable = TRUE;
	if (pTrident->Chipset == PROVIDIA9685) {
	    dst = 3<<16;
	} else {
    	    TGUI_OPERMODE(pTrident->EngineOperation | DST_ENABLE);
	}
	REPLICATE(transparency_color);
	TGUI_CKEY(transparency_color);
    }

    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy)
	dst = FASTMODE;

    if (pTrident->Chipset == PROVIDIA9685 && rop == GXcopy) dst |= 1<<21;

    TGUI_DRAWFLAG(pTrident->BltScanDirection | SCR2SCR | dst);
    TGUI_FMIX(TGUIRops_alu[rop]);
}

static void
TridentSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    if (pTrident->BltScanDirection & YNEG) {
        y1 = y1 + h - 1;
	y2 = y2 + h - 1;
    }
    if (pTrident->BltScanDirection & XNEG) {
	x1 = x1 + w - 1;
	x2 = x2 + w - 1;
    }
    TGUI_SRC_XY(x1,y1);
    TGUI_DEST_XY(x2,y2);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
    if (pTrident->DstEnable) {
	if (pTrident->Chipset != PROVIDIA9685) {
	    TGUI_OPERMODE(pTrident->EngineOperation);
	}
	pTrident->DstEnable = FALSE;
    }
}

static void
TridentSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_SRCCLIP_XY(x1, y1);
    TGUI_DSTCLIP_XY(x2, y2);
    pTrident->Clipping = TRUE;
}

static void
TridentSetupForSolidLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    REPLICATE(color);
    TGUI_FCOLOUR(color);
    TGUI_BCOLOUR(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
}

static void
Trident9685SetupForSolidLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    REPLICATE(color);
    TGUI_FPATCOL(color);
    TGUI_BPATCOL(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
}

static void 
TridentSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
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
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
}

static void
TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = 0;

    REPLICATE(color);
    TGUI_FCOLOUR(color);
    TGUI_BCOLOUR(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy) drawflag = FASTMODE;
    if (pTrident->Chipset == PROVIDIA9685 && rop == GXcopy) drawflag |= 1<<21;
    TGUI_DRAWFLAG(SOLIDFILL | PATMONO | drawflag);
}

static void
Trident9685SetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    REPLICATE(color);
    TGUI_FPATCOL(color);
    TGUI_BPATCOL(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
    TGUI_DRAWFLAG(SOLIDFILL | PATMONO);
}

static void
TridentSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_DIM_XY(w,h);
    TGUI_DEST_XY(x,y);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
}

static void 
TridentSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int fg, int bg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    REPLICATE(fg);
    REPLICATE(bg);
    TGUI_FCOLOUR(fg);
    TGUI_BCOLOUR(bg);

    TGUI_DRAWFLAG(SRCMONO | SCR2SCR);
    TGUI_FMIX(TGUIRops_alu[rop]);
}

static void 
TridentSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int srcx, int srcy, int offset)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_OUTL(0x44, (offset << 28) | ((srcy * pScrn->displayWidth + srcx) * pScrn->bitsPerPixel * 8));
    TGUI_SRC_XY(srcx,srcy);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
}

static void 
TridentSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
    int fg, int bg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = 0;

    if (bg != -1)
	drawflag |= SRCMONO;
    else
	drawflag |= SRCMONO | 1<<12;

    if (pTrident->Chipset == PROVIDIA9685) 
	drawflag |= 1<<30;
  
    REPLICATE(fg);
    REPLICATE(bg);
    TGUI_FCOLOUR(fg);
    TGUI_BCOLOUR(bg);
    TGUI_SRC_XY(0,0);
    TGUI_DRAWFLAG(drawflag);
    TGUI_FMIX(TGUIRops_alu[rop]);
}

static void
TridentSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
	int x, int y, int w, int h, int skipleft)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int bpp = pScrn->bitsPerPixel >> 3;

    TridentSetClippingRectangle(pScrn,((x+skipleft)*bpp),y,((w+x)*bpp)-1,y+h-1);

    if (pTrident->Chipset == PROVIDIA9682) 
    	TGUI_OPERMODE(pTrident->EngineOperation & 0xFEFF);

    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);

    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);

    if (pTrident->Chipset == PROVIDIA9682) 
    	TGUI_OPERMODE(pTrident->EngineOperation);
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
TridentWritePixmap(
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
    int dst = 0;

    TridentSetClippingRectangle(pScrn,(x*(bpp>>3)),y,((w+x)*(bpp>>3))-1,y+h-1);

    if (w & 7)
    	w += 8 - (w & 7);

    if (transparency_color != -1) {
	pTrident->DstEnable = TRUE;
	if (pTrident->Chipset == PROVIDIA9685) {
	    dst = 3<<16;
	} else {
    	    TGUI_OPERMODE(pTrident->EngineOperation | DST_ENABLE);
	}
	REPLICATE(transparency_color);
	TGUI_CKEY(transparency_color);
    }
    TGUI_SRC_XY(0,0);
    TGUI_FMIX(TGUIRops_alu[rop]);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    if (pTrident->Chipset == PROVIDIA9685) {
	if (rop == GXcopy) dst |= 1<<21;
    	TGUI_DRAWFLAG(1<<30 | dst); /* Enable Clipping */
    } else {
	if ((pTrident->Chipset == PROVIDIA9682 ||
	     pTrident->Chipset == TGUI9680) && rop == GXcopy) {
	    TGUI_DRAWFLAG(FASTMODE);
	} else {
    	    TGUI_DRAWFLAG(0);
	}
    }
    if (pTrident->Chipset == PROVIDIA9682) 
    	TGUI_OPERMODE(pTrident->EngineOperation & 0xFEFF); /* Enable Clipping */
    TGUI_COMMAND(GE_BLT);

    if (pScrn->bitsPerPixel == 8)
    	w >>= 2;
    if (pScrn->bitsPerPixel == 16)
    	w >>= 1;

    while (h--) {
	MoveDWORDS((CARD32*)pTrident->FbBase,(CARD32*)src,w);
	src += srcwidth;
    }

    if (pTrident->UseGERetry)
    	SET_SYNC_FLAG(infoRec);
    else 
    	TridentSync(pScrn);

    if (pTrident->DstEnable) {
	if (pTrident->Chipset != PROVIDIA9685) {
	    TGUI_OPERMODE(pTrident->EngineOperation);
	}
	pTrident->DstEnable = FALSE;
    }
    if (pTrident->Chipset == PROVIDIA9682) 
	TGUI_OPERMODE(pTrident->EngineOperation);

    CHECKCLIPPING;
}

static void 
TridentSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = 0;

    REPLICATE(bg);
    REPLICATE(fg);
    if (pTrident->Chipset == PROVIDIA9685) {
	drawflag |= 7<<18;
    	TGUI_BPATCOL(bg);
    	TGUI_FPATCOL(fg);
        if (rop == GXcopy) drawflag |= 1<<21;
    } else {
    	TGUI_BCOLOUR(bg);
    	TGUI_FCOLOUR(fg);
        if ((pTrident->Chipset == PROVIDIA9682 ||
	     pTrident->Chipset == TGUI9680) && rop == GXcopy) 
		drawflag |= FASTMODE;
    }
    TGUI_DRAWFLAG(PAT2SCR | PATMONO | drawflag);
    TGUI_PATLOC(((patterny * pScrn->displayWidth * pScrn->bitsPerPixel / 8) +
		 (patternx * pScrn->bitsPerPixel / 8)) >> 6);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
}

static void 
TridentSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
}

static void 
TridentSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
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
TridentSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
    CHECKCLIPPING;
}
