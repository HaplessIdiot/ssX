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
/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "trident_regs.h"
#include "trident.h"

#include "xaalocal.h"

int TGUIRops_alu[16] = {
	TGUIROP_0,
	TGUIROP_AND,
	TGUIROP_SRC_AND_NOT_DST,
	TGUIROP_SRC,
	TGUIROP_NOT_SRC_AND_DST,
	TGUIROP_DST,
	TGUIROP_XOR,
	TGUIROP_OR,
	TGUIROP_NOR,
	TGUIROP_XNOR,
	TGUIROP_NOT_DST,
	TGUIROP_SRC_OR_NOT_DST,
	TGUIROP_NOT_SRC,
	TGUIROP_NOT_SRC_OR_DST,
	TGUIROP_NAND,
	TGUIROP_1
};

int TGUIRops_Pixalu[16] = {
	TGUIROP_0,
	TGUIROP_AND_PAT,
	TGUIROP_PAT_AND_NOT_DST,
	TGUIROP_PAT,
	TGUIROP_NOT_PAT_AND_DST,
	TGUIROP_DST,
	TGUIROP_XOR_PAT,
	TGUIROP_OR_PAT,
	TGUIROP_NOR_PAT,
	TGUIROP_XNOR_PAT,
	TGUIROP_NOT_DST,
	TGUIROP_PAT_OR_NOT_DST,
	TGUIROP_NOT_PAT,
	TGUIROP_NOT_PAT_OR_DST,
	TGUIROP_NAND_PAT,
	TGUIROP_1
};

static void TridentSync(ScrnInfoPtr pScrn);
static void TridentSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void TridentSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
static void TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
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
    				int bg, int fg, int rop,
    				unsigned int planemask);
static void TridentSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    				int srcx, int srcy, int x, int y, int w, int h);
static void TridentSetupForImageWrite(ScrnInfoPtr pScrn,
 				int rop, unsigned int planemask,
				int transparency_color, int bpp, int depth);
static void TridentSubsequentImageWrite(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void TridentSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
				int x2, int y2);

static void
TridentInitializeAccelerator(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->Clipping = FALSE;
    TGUI_SRCCLIP_XY(0,0);
    TGUI_DSTCLIP_XY(4095,2047);
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

    infoPtr->Flags = PIXMAP_CACHE;
 
    infoPtr->Sync = TridentSync;

#if 0
    infoPtr->SolidLineFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidLine = TridentSetupForSolidLine;
    infoPtr->SolidBresenhamLineErrorTermBits = 11;
    infoPtr->SubsequentSolidBresenhamLine = TridentSubsequentSolidBresenhamLine;
    infoPtr->SetClippingRectangle = TridentSetClippingRectangle;
#endif

    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = TridentSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = TridentSubsequentFillRectSolid;
    
    infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY; 
    infoPtr->SetupForScreenToScreenCopy = 	
				TridentSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				TridentSubsequentScreenToScreenCopy;

#if 0
    infoPtr->ScreenToScreenColorExpandFillFlags = NO_PLANEMASK |NO_TRANSPARENCY; 
    infoPtr->SetupForScreenToScreenColorExpandFill = 	
				TridentSetupForScreenToScreenColorExpand;
    infoPtr->SubsequentScreenToScreenColorExpandFill = 		
				TridentSubsequentScreenToScreenColorExpand;
#endif

#if 0
    infoPtr->ImageWriteFlags = CPU_TRANSFER_PAD_DWORD |
			       SCANLINE_PAD_DWORD |
			       NO_PLANEMASK |
			       NO_TRANSPARENCY;
    infoPtr->ImageWriteRange = 0;
    infoPtr->ImageWriteBase = pTrident->FbBase; 
    infoPtr->SetupForImageWrite = TridentSetupForImageWrite;
    infoPtr->SubsequentImageWriteRect = TridentSubsequentImageWrite;
#endif

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pTrident->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void
TridentSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int count = 0, timeout = 0;
    int busy;

    for (;;) {
	BLTBUSY(busy);
	if (busy != GE_BUSY) return;
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

    CHECKCLIPPING;

    pTrident->BltScanDirection = 0;
    if (xdir < 0) pTrident->BltScanDirection |= XNEG;
    if (ydir < 0) pTrident->BltScanDirection |= YNEG;
    TGUI_DRAWFLAG(pTrident->BltScanDirection | SCR2SCR);
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
    TridentSync(pScrn);
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
TridentSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->BltScanDirection = 0;
    if (octant & YMAJOR) pTrident->BltScanDirection |= YMAJ;
    if (octant & XDECREASING) pTrident->BltScanDirection |= XNEG;
    if (octant & YDECREASING) pTrident->BltScanDirection |= YNEG;
    TGUI_DRAWFLAG(SOLIDFILL | STENCIL | pTrident->BltScanDirection);
    TGUI_SRC_XY(dmaj,dmin);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(e,len);
    TGUI_COMMAND(GE_BRESLINE);
    TridentSync(pScrn);
}

static void
TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    CHECKCLIPPING;

    REPLICATE(color);
    TGUI_FCOLOUR(color);
    TGUI_BCOLOUR(color);
    TGUI_FMIX(TGUIRops_Pixalu[rop]);
}

static void
TridentSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_DIM_XY(w,h);
    TGUI_DEST_XY(x,y);
    TGUI_DRAWFLAG(SOLIDFILL | PATMONO);
    TGUI_COMMAND(GE_BLT);
    TridentSync(pScrn);
}

static void 
TridentSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int bg, int fg, int rop,
    unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    CHECKCLIPPING;

    REPLICATE(fg);
    REPLICATE(bg);

    TGUI_FCOLOUR(fg);
    TGUI_BCOLOUR(bg);
    TGUI_DRAWFLAG(SCR2SCR | SRCMONO);
    TGUI_FMIX(TGUIRops_alu[rop]);
}

static void 
TridentSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
    int srcx, int srcy, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_SRC_XY(srcx,srcy);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    TridentSync(pScrn);
}

static void 
TridentSetupForImageWrite(ScrnInfoPtr pScrn,
 	int rop, unsigned int planemask,
 	int transparency_color, int bpp, int depth)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    
    CHECKCLIPPING;

    TGUI_DRAWFLAG(0);
    TGUI_SRC_XY(0,0);
    TGUI_FMIX(TGUIRops_alu[rop]);
}
 
static void
TridentSubsequentImageWrite(ScrnInfoPtr pScrn,
	int x, int y, int w, int h, int skipleft)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
}
