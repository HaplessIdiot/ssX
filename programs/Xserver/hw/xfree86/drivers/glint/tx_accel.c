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
 *           Dirk Hohndel, <hohndel@suse.de>
 *           Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 * 
 * GLINT 500TX / MX accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/tx_accel.c,v 1.6 1998/10/04 14:35:54 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#include "xaalocal.h"	/* For replacements */

static void TXSync(ScrnInfoPtr pScrn);
static void TXSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, int rop,
						unsigned int planemask);
static void TXSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y,
						int w, int h);
static void TXSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patternx, 
						int patterny, 
					   	int fg, int bg, int rop,
					   	unsigned int planemask);
static void TXSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patternx,
						int patterny, int x, int y,
				   		int w, int h);
static void TXSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir,
						int rop, unsigned int planemask,
				    		int transparency_color);
static void TXSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
						int x2, int y2, int w, int h);
static void TXWriteBitmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				unsigned char *src, int srcwidth,
				int skipleft, int fg, int bg, int rop,
    				unsigned int planemask);
static void TXSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
						int x2,int y2);
static void TXFillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop,
				unsigned int planemask, int nBox, BoxPtr pBox,
				int xorg, int yorg, PixmapPtr pPix);
static void TXFillColorExpandSpans(ScrnInfoPtr pScrn, int fg, int bg, int rop,
   				unsigned int planemask, int n, DDXPointPtr ppt,
   				int *pwidth, int fSorted, int xorg, int yorg,
   				PixmapPtr pPix);
static void TXSetupForSolidLine(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
static void TXSubsequentSolidHorVertLine(ScrnInfoPtr pScrn, int x1, int y1,
				int len, int dir);
static void TXWritePixmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
   				unsigned char *src, int srcwidth, int rop,
   				unsigned int planemask, int trans,
   				int bpp, int depth);
static void TXSetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, int fg,
				int bg, int rop, unsigned int planemask);
static void TXSubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, int x,
				int y, int w, int h, int skipleft);
static void TXTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				int skipleft, int startline, 
    				unsigned int **glyphs, int glyphWidth,
    				int fg, int bg, int rop, unsigned planemask);
static void TXNonTEGlyphRenderer(ScrnInfoPtr pScrn, int xBack, int wBack,
    				int xText, int wText, 
    				int y, int h, int skipleft, int startline, 
    				NonTEGlyphInfo *glyphp,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void TXLoadCoord(ScrnInfoPtr pScrn, int x, int y, int w, int h,
				int a, int d);

#define MAX_FIFO_ENTRIES	1023

static void
TXInitializeEngine(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* Initialize the Accelerator Engine to defaults */

    GLINT_SLOW_WRITE_REG(pGlint->pprod,	LBReadMode);
    GLINT_SLOW_WRITE_REG(pGlint->pprod,	FBReadMode);
    GLINT_SLOW_WRITE_REG(0, dXSub);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	LBWriteMode);
    GLINT_SLOW_WRITE_REG(UNIT_ENABLE,	FBWriteMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	DitherMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaBlendMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	ColorDDAMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureColorMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,  TextureReadMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,  GLINTWindow);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,  AlphaBlendMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,  DepthMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,  RouterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	FogMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AntialiasMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaTestMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	StencilMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AreaStippleMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	LineStippleMode);
    GLINT_SLOW_WRITE_REG(0,		UpdateLineStippleCounters);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	LogicalOpMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	DepthMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	StatisticMode);
    GLINT_SLOW_WRITE_REG(0xc00,		FilterMode);
    GLINT_SLOW_WRITE_REG(0xffffffff,	FBHardwareWriteMask);
    GLINT_SLOW_WRITE_REG(0xffffffff,	FBSoftwareWriteMask);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	RasterizerMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	GLINTDepth);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	FBSourceOffset);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	FBPixelOffset);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	LBSourceOffset);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	WindowOrigin);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	FBWindowBase);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	LBWindowBase);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	RouterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	PatternRamMode);

    switch (pScrn->bitsPerPixel) {
    	case 8:
	    GLINT_SLOW_WRITE_REG(0x2,	PixelSize);
	    break;
	case 16:
	    GLINT_SLOW_WRITE_REG(0x1,	PixelSize);
	    break;
	case 32:
	    GLINT_SLOW_WRITE_REG(0x0,	PixelSize);
	    break;
    }
    pGlint->ROP = 0xFF;
    pGlint->ClippingOn = FALSE;
    pGlint->startxsub = 0;
    pGlint->startxdom = 0;
    pGlint->starty = 0;
    pGlint->count = 0;
    pGlint->dxdom = 0;
    pGlint->dy = 1;
    GLINT_SLOW_WRITE_REG(0, StartXSub);
    GLINT_SLOW_WRITE_REG(0, StartXDom);
    GLINT_SLOW_WRITE_REG(0, StartY);
    GLINT_SLOW_WRITE_REG(0, GLINTCount);
    GLINT_SLOW_WRITE_REG(0, dXDom);
    GLINT_SLOW_WRITE_REG(1<<16, dY);
}

Bool
TXAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    long memory = pGlint->FbMapSize;
    BoxRec AvailFBArea;

    pGlint->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    TXInitializeEngine(pScrn);

    infoPtr->Flags = PIXMAP_CACHE;
 
    infoPtr->Sync = TXSync;

    infoPtr->SolidFillFlags = 0;
    infoPtr->SetupForSolidFill = TXSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = TXSubsequentFillRectSolid;

    infoPtr->SolidLineFlags = 0;
    infoPtr->SetupForSolidLine = TXSetupForSolidLine;
    infoPtr->SubsequentSolidHorVertLine = TXSubsequentSolidHorVertLine;

    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY |
				       ONLY_LEFT_TO_RIGHT_BITBLT;
    infoPtr->SetupForScreenToScreenCopy = TXSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = TXSubsequentScreenToScreenCopy;

    infoPtr->Mono8x8PatternFillFlags = HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
				       HARDWARE_PATTERN_SCREEN_ORIGIN |
				       HARDWARE_PATTERN_PROGRAMMED_BITS;
    infoPtr->SetupForMono8x8PatternFill = TXSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = TXSubsequentMono8x8PatternFillRect;

    infoPtr->SetClippingRectangle = TXSetClippingRectangle;

    /* We need to break up scanlines to use these without PCI retries */
    if (pGlint->UsePCIRetry) {
        infoPtr->CPUToScreenColorExpandFillFlags = TRANSPARENCY_ONLY |
      					       CPU_TRANSFER_PAD_DWORD |
      					       BIT_ORDER_IN_BYTE_LSBFIRST |
					       LEFT_EDGE_CLIPPING |
					       LEFT_EDGE_CLIPPING_NEGATIVE_X;
        infoPtr->ColorExpandRange = MAX_FIFO_ENTRIES;
        infoPtr->ColorExpandBase = pGlint->IOBase + OutputFIFO + 4;
        infoPtr->SetupForCPUToScreenColorExpandFill =
				TXSetupForCPUToScreenColorExpandFill;
        infoPtr->SubsequentCPUToScreenColorExpandFill = 
				TXSubsequentCPUToScreenColorExpandFill;

        infoPtr->WriteBitmap = TXWriteBitmap;
        infoPtr->WritePixmap = TXWritePixmap;
        infoPtr->TEGlyphRenderer = TXTEGlyphRenderer;
#if 0
        infoPtr->NonTEGlyphRenderer = TXNonTEGlyphRenderer;
#endif
        infoPtr->FillColorExpandRects = TXFillColorExpandRects;
        infoPtr->FillColorExpandSpans = TXFillColorExpandSpans;
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    if (memory > 16777216) memory = 16777216; /* Cater for >16MB of videoRAM */
    AvailFBArea.y2 = memory / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return (XAAInit(pScreen, infoPtr));
}

static void TXLoadCoord(
	ScrnInfoPtr pScrn,
	int x, int y,
	int w, int h,
	int a, int d
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    if (w != pGlint->startxsub) {
    	GLINT_WRITE_REG(w<<16, StartXSub);
	pGlint->startxsub = w;
    }
    if (x != pGlint->startxdom) {
    	GLINT_WRITE_REG(x<<16,StartXDom);
	pGlint->startxdom = x;
    }
    if (y != pGlint->starty) {
    	GLINT_WRITE_REG(y<<16,StartY);
	pGlint->starty = y;
    }
    if (h != pGlint->count) {
    	GLINT_WRITE_REG(h,GLINTCount);
	pGlint->count = h;
    }
    if (a != pGlint->dxdom) {
    	GLINT_WRITE_REG(a<<16,dXDom);
	pGlint->dxdom = a;
    }
    if (d != pGlint->dy) {
    	GLINT_WRITE_REG(d<<16,dY);
	pGlint->dy = d;
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
     if (dwords == 1) return;
     *(dest + 1) = *(src + 1);
     if (dwords == 2) return;
     *(dest + 2) = *(src + 2);
}

static void
TXSync(
	ScrnInfoPtr pScrn
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    while (GLINT_READ_REG(DMACount) != 0);
    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, GlintSync);
    do {
   	while(GLINT_READ_REG(OutFIFOWords) == 0);
#define Sync_tag 0x188
    } while (GLINT_READ_REG(OutputFIFO) != Sync_tag);
}

static void
TXSetupForFillRectSolid(
	ScrnInfoPtr pScrn, 
	int color, int rop, 
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    pGlint->ForeGroundColor = color;
    CHECKCLIPPING;
	
    GLINT_WAIT(5);
    REPLICATE(color);
    DO_PLANEMASK(planemask);
    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
	GLINT_WRITE_REG(color, FBBlockColor);
	pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
	GLINT_WRITE_REG(color, PatternRamData0);
	pGlint->FrameBufferReadMode = FastFillEnable | SpanOperation;
    }
    LOADROP(rop);
}

static void
TXSubsequentFillRectSolid(
	ScrnInfoPtr pScrn, 
	int x, int y, 
	int w, int h
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    TXLoadCoord(pScrn, x, y, x+w, h, 0, 1);
    GLINT_WRITE_REG(PrimitiveTrapezoid | pGlint->FrameBufferReadMode,Render);
}

static void
TXSetupForSolidLine(
	ScrnInfoPtr pScrn, 
	int color, int rop, 
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
	
    CHECKCLIPPING;
    if (rop == GXcopy) {
        GLINT_WAIT(6);
        GLINT_WRITE_REG(color, GLINTColor);
    	REPLICATE(color);
	GLINT_WRITE_REG(color, FBBlockColor);
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
        GLINT_WRITE_REG(0, dXDom);
    } else {
        GLINT_WAIT(4);
        GLINT_WRITE_REG(color, GLINTColor);
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    DO_PLANEMASK(planemask);
    LOADROP(rop);
}

static void
TXSubsequentSolidHorVertLine(
	ScrnInfoPtr pScrn,
	int x1, int y1, 
	int len, int dir
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    if ((len < 16) && (pGlint->ROP == GXcopy)) {
    GLINT_WAIT(5);
    if (dir == DEGREES_0) {
    TXLoadCoord(pScrn, x1, y1, x1+len, 1, 0, 1);
    } else {
    TXLoadCoord(pScrn, x1, y1, x1+1, len, 0, 1);
    }
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable,Render);
    } else {
    GLINT_WAIT(6);
    if (dir == DEGREES_0) {
    TXLoadCoord(pScrn, x1, y1, pGlint->startxsub, len, 1, 0);
    } else {
    TXLoadCoord(pScrn, x1, y1, pGlint->startxsub, len, 0, 1);
    }
    GLINT_WRITE_REG (PrimitiveLine, Render);
    }
}

static void
TXSetClippingRectangle(
	ScrnInfoPtr pScrn, 	
	int x1, int y1, 
	int x2, int y2
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    GLINT_WAIT(3);
    GLINT_WRITE_REG((y1&0x0FFF)<<16|(x1&0x0FFF), ScissorMinXY);
    GLINT_WRITE_REG((y2&0x0FFF)<<16|(x2&0x0FFF), ScissorMaxXY);
    GLINT_WRITE_REG(1, ScissorMode); /* Enable Scissor Mode */
    pGlint->ClippingOn = TRUE;
}

static void
TXSetupForScreenToScreenCopy(
	ScrnInfoPtr pScrn,
	int xdir, int  ydir, 	
	int rop,
	unsigned int planemask, 
	int transparency_color
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    CHECKCLIPPING;
    pGlint->BltScanDirection = ydir;

    GLINT_WAIT(4);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);

    if ((rop == GXclear) || (rop == GXset)) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else 
    if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_SrcEnable, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_SrcEnable | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
}

static void
TXSubsequentScreenToScreenCopy(
	ScrnInfoPtr pScrn, 
	int x1, int y1, 
	int x2, int y2,
	int w, int h
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int srcaddr, dstaddr;

    GLINT_WAIT(7);
    if (pGlint->BltScanDirection != 1) {
	y1 += h - 1;
	y2 += h - 1;
        TXLoadCoord(pScrn, x2, y2, x2+w, h, 0, -1);
    } else {
        TXLoadCoord(pScrn, x2, y2, x2+w, h, 0, 1);
    }	

    srcaddr = y1 * pScrn->displayWidth + x1;
    dstaddr = y2 * pScrn->displayWidth + x2;

    GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
    GLINT_WRITE_REG(PrimitiveTrapezoid| FastFillEnable | SpanOperation, Render);
}

static void
TXSetupForCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int fg, int bg, 
	int rop, 
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    REPLICATE(fg);
    CHECKCLIPPING;

    if (bg != -1) ErrorF("OUCH CPUToScreen called with BG = %d\n",bg);

    GLINT_WAIT(5);
    DO_PLANEMASK(planemask);
    if (rop == GXcopy) {
        GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
        GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
        GLINT_WRITE_REG(fg, FBBlockColor);
        pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
        GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
        GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
        GLINT_WRITE_REG(fg, PatternRamData0);
        pGlint->FrameBufferReadMode = FastFillEnable | SpanOperation;
    }
    LOADROP(rop);
}

static void
TXSubsequentCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int dwords = ((w + 31) >> 5) * h;

    if (skipleft) {
	TXSetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    }

    GLINT_WAIT(6);
    TXLoadCoord(pScrn, x, y, x+w, h, 0, 1);
    GLINT_WRITE_REG(PrimitiveTrapezoid | pGlint->FrameBufferReadMode | 
							SyncOnBitMask, Render);
    GLINT_WRITE_REG((dwords - 1)<<16 | 0x0D, OutputFIFO);
}

void TXSetupForMono8x8PatternFill(
	ScrnInfoPtr pScrn,
	int patternx, int patterny, 
	int fg, int bg, int rop,
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    if (bg == -1) pGlint->FrameBufferReadMode = -1;
	else    pGlint->FrameBufferReadMode = 0;
    pGlint->ForeGroundColor = fg;
    pGlint->BackGroundColor = bg;
    REPLICATE(pGlint->ForeGroundColor);
    REPLICATE(pGlint->BackGroundColor);

    GLINT_WAIT(12);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG((patternx & 0x000000FF),       AreaStipplePattern0);
    GLINT_WRITE_REG((patternx & 0x0000FF00) >> 8,  AreaStipplePattern1);
    GLINT_WRITE_REG((patternx & 0x00FF0000) >> 16, AreaStipplePattern2);
    GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
    GLINT_WRITE_REG((patterny & 0x000000FF),       AreaStipplePattern4);
    GLINT_WRITE_REG((patterny & 0x0000FF00) >> 8,  AreaStipplePattern5);
    GLINT_WRITE_REG((patterny & 0x00FF0000) >> 16, AreaStipplePattern6);
    GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
  
    if (rop == GXcopy) {
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
}

static void 
TXSubsequentMono8x8PatternFillRect(
	ScrnInfoPtr pScrn, 
	int patternx, int patterny, 
	int x, int y,
	int w, int h
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int span = 0;
  
    GLINT_WAIT(10);
    TXLoadCoord(pScrn, x, y, x+w, h, 0, 1);

    if (pGlint->FrameBufferReadMode != -1) {
	if (pGlint->ROP == GXcopy) {
	  GLINT_WRITE_REG(pGlint->BackGroundColor, FBBlockColor);
	  GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable,Render);
	} else {
  	  GLINT_WRITE_REG(pGlint->BackGroundColor, PatternRamData0);
	  GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12|ASM_InvertPattern |
					UNIT_ENABLE, AreaStippleMode);
	  GLINT_WRITE_REG(AreaStippleEnable | SpanOperation | FastFillEnable | 
						PrimitiveTrapezoid, Render);
	}
    }

    if (pGlint->ROP == GXcopy) {
	GLINT_WRITE_REG(pGlint->ForeGroundColor, FBBlockColor);
	span = 0;
    } else {
  	GLINT_WRITE_REG(pGlint->ForeGroundColor, PatternRamData0);
	span = SpanOperation;
    }
    GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12|
  						UNIT_ENABLE, AreaStippleMode);
    GLINT_WRITE_REG(AreaStippleEnable | span | FastFillEnable | 
						PrimitiveTrapezoid, Render);
}

static void 
TXFillColorExpandSpans(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    Bool TwoPass = FALSE, FirstPass = TRUE;
    StippleScanlineProcPtr StippleFunc;
    int stipplewidth = pPix->drawable.width;
    int stippleheight = pPix->drawable.height;
    int dwords, srcy, srcx, funcNo = 2;
    unsigned char *srcp;

    CHECKCLIPPING;
    if(stipplewidth <= 32) {
	if(stipplewidth & (stipplewidth - 1))	
	  funcNo = 1;
	else	
	  funcNo = 0;
    } 
    StippleFunc = XAAStippleScanlineFuncLSBFirst[funcNo];

    if(bg == -1) {
	/* one pass */
    } else if((rop == GXcopy) && infoRec->FillSolidSpans) {
	/* one pass but we fill *all* background spans first */
	(*infoRec->FillSolidSpans)(
		pScrn, bg, rop, planemask, n, ppt, pwidth, fSorted);
	bg = -1;
    } else {
	/* gotta do two passes */
	TwoPass = TRUE;
    }

    if(!TwoPass)
	(*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    while(n--) {
	dwords = (*pwidth + 31) >> 5;

	srcy = (ppt->y - yorg) % stippleheight;
	if(srcy < 0) srcy += stippleheight;
	srcx = (ppt->x - xorg) % stipplewidth;
	if(srcx < 0) srcx += stipplewidth;

	srcp = (pPix->devKind * srcy) + (unsigned char*)pPix->devPrivate.ptr;

SECOND_PASS:
	if(TwoPass) {
	    GLINT_WAIT(1);
	    if (FirstPass) {
	    	GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	    } else {
	    	GLINT_WRITE_REG(0, RasterizerMode);
 	    }

	    (*infoRec->SetupForCPUToScreenColorExpandFill)(pScrn, 
			(FirstPass) ? bg : fg, -1, rop, planemask);
	}

        (*infoRec->SubsequentCPUToScreenColorExpandFill)(pScrn, ppt->x, ppt->y,
 			*pwidth, 1, 0);

	(*StippleFunc)((CARD32*)infoRec->ColorExpandBase, 
		(CARD32*)srcp, srcx, stipplewidth, dwords);
    
	if(TwoPass) {
	   if(FirstPass) {
		FirstPass = FALSE;
		goto SECOND_PASS;
	   } else FirstPass = TRUE;
	}

	ppt++; pwidth++;
     }

     SET_SYNC_FLAG(infoRec);
}

static void
TXFillColorExpandRects(ScrnInfoPtr pScrn,
	int fg, int bg,
	int rop,
	unsigned int planemask,
	int nBox, BoxPtr pBox,
	int xorg, int yorg,
	PixmapPtr pPix
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    Bool TwoPass = FALSE, FirstPass = TRUE;
    StippleScanlineProcPtr StippleFunc;
    int stipplewidth = pPix->drawable.width;
    int stippleheight = pPix->drawable.height;
    int srcwidth = pPix->devKind;
    int dwords, srcy, srcx, funcNo = 2, h;
    unsigned char *src = (unsigned char*)pPix->devPrivate.ptr;
    unsigned char *srcp;

    CHECKCLIPPING;
    if(stipplewidth <= 32) {
	if(stipplewidth & (stipplewidth - 1))	
	  funcNo = 1;
	else	
	  funcNo = 0;
    } 
    StippleFunc = XAAStippleScanlineFuncLSBFirst[funcNo];

    if(bg == -1) {
	/* one pass */
    } else if((rop == GXcopy) && infoRec->FillSolidRects) {
	/* one pass but we fill *all* background rects first */
	(*infoRec->FillSolidRects)(pScrn, bg, rop, planemask, nBox, pBox);
	bg = -1;
    } else {
	/* gotta do two passes */
	TwoPass = TRUE;
    }

    if(!TwoPass)
	(*infoRec->SetupForCPUToScreenColorExpandFill)(
					pScrn, fg, bg, rop, planemask);

    while(nBox--) {
	dwords = (pBox->x2 - pBox->x1 + 31) >> 5;

SECOND_PASS:
	if(TwoPass) {
	    GLINT_WAIT(1);
	    if (FirstPass) {
	    	GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	    } else {
	    	GLINT_WRITE_REG(0, RasterizerMode);
	    }

	    (*infoRec->SetupForCPUToScreenColorExpandFill)(pScrn, 
			(FirstPass) ? bg : fg, -1, rop, planemask);
	}

	h = pBox->y2 - pBox->y1;

        (*infoRec->SubsequentCPUToScreenColorExpandFill)(
			pScrn, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, h, 0);

	srcy = (pBox->y1 - yorg) % stippleheight;
	if(srcy < 0) srcy += stippleheight;
	srcx = (pBox->x1 - xorg) % stipplewidth;
	if(srcx < 0) srcx += stipplewidth;

	srcp = (srcwidth * srcy) + src;
	
	while(h--) {
	  /* same */
	
	   (*StippleFunc)((CARD32*)infoRec->ColorExpandBase, 
			(CARD32*)srcp, srcx, stipplewidth, dwords);
	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
		srcy = 0;
		srcp = src;
	   }
	}
    
	if(TwoPass) {
	   if(FirstPass) {
		FirstPass = FALSE;
		goto SECOND_PASS;
	   } else FirstPass = TRUE;
	}

	pBox++;
    }
    SET_SYNC_FLAG(infoRec);
}

static void 
TXWriteBitmap(ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned char *srcpntr;
    int dwords, height, mode;
    Bool SecondPass = FALSE;

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    if (skipleft) 
	TXSetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    else
	CHECKCLIPPING;

    GLINT_WAIT(11);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    LOADROP(rop);
    if (rop == GXcopy) {
	mode = 0;
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	mode = SpanOperation;
	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    TXLoadCoord(pScrn, x, y, x+w, h, 0, 1);

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else if(rop == GXcopy) {
	REPLICATE(bg);
	GLINT_WAIT(5);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	GLINT_WRITE_REG(PrimitiveTrapezoid |mode|FastFillEnable,Render);
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else {
	SecondPass = TRUE;
	REPLICATE(fg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    }

SECOND_PASS:
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | mode | SyncOnBitMask, Render);
    
    height = h;
    srcpntr = src;
    while(height--) {
	GLINT_WRITE_REG((dwords - 1)<<16 | 0x0D, OutputFIFO);
        MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
			(CARD32*)srcpntr, dwords);
	srcpntr += srcwidth;
    }    

    if(SecondPass) {
	SecondPass = FALSE;
	REPLICATE(bg);
	GLINT_WAIT(4);
	GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	goto SECOND_PASS;
    }

    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG(infoRec);
}

static void 
TXTEGlyphRenderer(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GlyphScanlineFuncPtr GlyphFunc = 
		XAAGlyphScanlineFuncLSBFirst[glyphWidth - 1];

    if(bg != -1) {
    	(*infoRec->SetupForSolidFill)(pScrn, bg, rop, planemask);
        (*infoRec->SubsequentSolidFillRect)(pScrn, x, y, w, h);
	bg = -1;
    }

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    /* I assume you can do all skiplefts */
    w += skipleft;
    x -= skipleft;

    (*infoRec->SubsequentCPUToScreenColorExpandFill)(
				pScrn, x, y, w, h, skipleft);

    while(h--) {
	(*GlyphFunc)((CARD32*)infoRec->ColorExpandBase, 
			glyphs, startline++, w, glyphWidth);
    }

    if (skipleft) {
	GLINT_WAIT(1);
	GLINT_WRITE_REG(0, ScissorMode);
    }
    SET_SYNC_FLAG(infoRec);
}

#if 0
static void 
TXNonTEGlyphRenderer(
    ScrnInfoPtr pScrn,
    int xBack, int wBack, int xText, int wText, 
    int y, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphp,
    int fg, int bg, int rop,
    unsigned int planemask
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    if(wBack) {
    	(*infoRec->SetupForSolidFill)(pScrn, bg, rop, planemask);
        (*infoRec->SubsequentSolidFillRect)(pScrn, xBack, y, wBack, h);
	bg = -1;
    }

    if(wText) {
	(*infoRec->SetupForCPUToScreenColorExpandFill)(
					pScrn, fg, bg, rop, planemask);
    	(*infoRec->SubsequentCPUToScreenColorExpandFill)(
					pScrn, xText, y, wText, h, 0);

   	while(h--) {
	    XAANonTEGlyphScanlineFuncLSBFirst(
		(CARD32*)infoRec->ColorExpandBase, 
		glyphp, startline++, wText, skipleft);

    	}
    }

    SET_SYNC_FLAG(infoRec);
}
#endif

static void 
TXWritePixmap(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,	
   int srcwidth,	/* bytes */
   int rop,
   unsigned int planemask,
   int trans,
   int bpp, int depth
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CARD32 *srcp;
    int count,dwords, skipleft, Bpp = bpp >> 3; 

    if((skipleft = (long)src & 0x03L)) {
	skipleft /= Bpp;

	x -= skipleft;	     
	w += skipleft;
	
	src = (unsigned char*)((long)src & ~0x03L);     
    }

    switch(Bpp) {
    case 1:	dwords = (w + 3) >> 2;
		break;
    case 2:	dwords = (w + 1) >> 1;
		break;
    case 4:	dwords = w;
		break;
    default: return; 
    }

    if (skipleft)
	TXSetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    else
	CHECKCLIPPING;

    GLINT_WAIT(11);
    DO_PLANEMASK(planemask);
    if (rop == GXcopy) {
        GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
        GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
    TXLoadCoord(pScrn, x, y, x+w, h, 0, 1);
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | SpanOperation | 
						SyncOnHostData, Render);

    while(h--) {
      count = dwords;
      srcp = (CARD32*)src;
      while(count >= MAX_FIFO_ENTRIES) {
	/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
       	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
				0x05, OutputFIFO);
	MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
	count -= MAX_FIFO_ENTRIES - 1;
	srcp += MAX_FIFO_ENTRIES - 1;
      }
      if(count) {
	/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
       	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
				0x05, OutputFIFO);
	MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
 		(CARD32*)srcp, count);
      }
      src += srcwidth;
    }  
    SET_SYNC_FLAG(infoRec);
}
