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
 * Permedia accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm_accel.c,v 1.3 1998/07/31 10:41:23 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#include "xaalocal.h"		/* For replacements */

static void PermediaSync(ScrnInfoPtr pScrn);
static void PermediaSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
static void PermediaSubsequentFillRectSolid();
static void PermediaSubsequentScreenToScreenCopy ();
static void PermediaSetupForScreenToScreenCopy ();
static void PermediaSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
				int x2, int y2);
static void PermediaSetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
				int fg, int bg, int rop,unsigned int planemask);
static void PermediaSubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void PermediaTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int w,
    				int h, int skipleft, int startline, 
    				unsigned int **glyphs, int glyphWidth,
    				int fg, int bg, int rop, unsigned planemask);
static void PermediaNonTEGlyphRenderer(ScrnInfoPtr pScrn, int xBack, int wBack,
    				int xText, int wText, 
    				int y, int h, int skipleft, int startline, 
    				NonTEGlyphInfo *glyphp,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void PermediaFillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg,
				int rop, unsigned int planemask, int nBox,
				BoxPtr pBox,int xorg, int yorg, PixmapPtr pPix);
static void PermediaFillColorExpandSpans(ScrnInfoPtr pScrn, int fg, int bg,
   				int rop, unsigned int planemask, int n,
   				DDXPointPtr ppt, int *pwidth, int fSorted,
   				int xorg, int yorg, PixmapPtr pPix);
static void PermediaWriteBitmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				unsigned char *src, int srcwidth, int srcx,
    				int srcy, int fg, int bg, int rop,
    				unsigned int planemask);
static void PermediaLoadCoord(ScrnInfoPtr pScrn, int x, int y, int w, int h,
				int a, int d);

#define MAX_FIFO_ENTRIES 1023

static void
PermediaInitializeEngine(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    /* Initialize the Accelerator Engine to defaults */

    GLINT_SLOW_WRITE_REG(0, 		ScissorMode);
    GLINT_SLOW_WRITE_REG(1, 		FBWriteMode);
    GLINT_SLOW_WRITE_REG(0, 		dXDom);
    GLINT_SLOW_WRITE_REG(0, 		dXSub);
    GLINT_SLOW_WRITE_REG(GWIN_DisableLBUpdate,   GLINTWindow);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	DitherMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaBlendMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	ColorDDAMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureColorMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureReadMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	PMTextureReadMode);
    GLINT_SLOW_WRITE_REG(pGlint->pprod,	LBReadMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaBlendMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TexelLUTMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	YUVMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	DepthMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	RouterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	FogMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AntialiasMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaTestMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	StencilMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AreaStippleMode);
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

    switch (pScrn->bitsPerPixel) {
	case 8:
	    GLINT_SLOW_WRITE_REG(0x0, FBReadPixel); /* 8 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod,	PMTextureMapFormat);
	    break;
	case 16:
	    GLINT_SLOW_WRITE_REG(0x1, FBReadPixel); /* 16 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 1<<19,	PMTextureMapFormat);
	    break;
	case 32:
	    GLINT_SLOW_WRITE_REG(0x2, FBReadPixel); /* 32 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 2<<19,	PMTextureMapFormat);
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
    GLINT_WRITE_REG(0, StartXSub);
    GLINT_WRITE_REG(0,StartXDom);
    GLINT_WRITE_REG(0,StartY);
    GLINT_WRITE_REG(0,GLINTCount);
    GLINT_WRITE_REG(0,dXDom);
    GLINT_WRITE_REG(1<<16,dY);
}

Bool
PermediaAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    BoxRec AvailFBArea;

    pGlint->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    PermediaInitializeEngine(pScrn);

    infoPtr->Flags = PIXMAP_CACHE;
 
    infoPtr->Sync = PermediaSync;

    infoPtr->SetupForSolidFill = PermediaSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = PermediaSubsequentFillRectSolid;
#if 0
    infoPtr->SubsequentTwoPointLine = Permedia2SubsequentTwoPointLine;
#endif
  
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY; 

    infoPtr->SetupForScreenToScreenCopy = PermediaSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = PermediaSubsequentScreenToScreenCopy;
    infoPtr->SetClippingRectangle = PermediaSetClippingRectangle;

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
				PermediaSetupForCPUToScreenColorExpandFill;
        infoPtr->SubsequentCPUToScreenColorExpandFill = 
				PermediaSubsequentCPUToScreenColorExpandFill;

        infoPtr->TEGlyphRenderer = PermediaTEGlyphRenderer;
        infoPtr->NonTEGlyphRenderer = PermediaNonTEGlyphRenderer;
	infoPtr->WriteBitmap = PermediaWriteBitmap;
#if 0
        infoPtr->WritePixmap = PermediaWritePixmap;
#endif
        infoPtr->FillColorExpandRects = PermediaFillColorExpandRects;
        infoPtr->FillColorExpandSpans = PermediaFillColorExpandSpans;
    }
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pGlint->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return (XAAInit(pScreen, infoPtr));
}

static void PermediaLoadCoord(
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

static void
PermediaSync(ScrnInfoPtr pScrn)
{
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
PermediaSetClippingRectangle(
	ScrnInfoPtr pScrn,
	int x1, int y1, 
	int x2, int y2
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(3);
    GLINT_WRITE_REG (((y1&0x0FFF) << 16) | (x1&0x0FFF), ScissorMinXY);
    GLINT_WRITE_REG (((y2&0x0FFF) << 16) | (x2&0x0FFF), ScissorMaxXY);
    GLINT_WRITE_REG (1, ScissorMode);
    pGlint->ClippingOn = TRUE;
}

static void
PermediaSetupForScreenToScreenCopy(
	ScrnInfoPtr pScrn, 
	int xdir, int ydir, int rop,
	unsigned int planemask, int transparency_color
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    pGlint->BltScanDirection = 0;
    if (ydir == 1) pGlint->BltScanDirection |= YPositive;

    GLINT_WAIT(5);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    GLINT_WRITE_REG(0, RasterizerMode);

    if ((rop == GXset) || (rop == GXclear)) {
	pGlint->FrameBufferReadMode = pGlint->pprod;
    } else
    if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	pGlint->FrameBufferReadMode = pGlint->pprod | FBRM_SrcEnable;
    } else {
	pGlint->FrameBufferReadMode = pGlint->pprod | FBRM_SrcEnable | 
				      FBRM_DstEnable;
    }
    LOADROP(rop);
}

void
PermediaSubsequentScreenToScreenCopy (ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2,
				   int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int srcaddr;
    int dstaddr;
    char align;
    int direction;

    if (!(pGlint->BltScanDirection & YPositive)) {
	y1 = y1 + h - 1;
	y2 = y2 + h - 1;
	direction = -1;
    } else {
	direction = 1;
    }

    /* We can only use GXcopy for Packed modes, and less than 32 width
     * gives us speed for small blits. */
    if ((w < 32) || (pGlint->ROP != GXcopy)) {
  	srcaddr = y1 * pScrn->displayWidth + x1;
  	dstaddr = y2 * pScrn->displayWidth + x2;
  	GLINT_WAIT(7);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode, FBReadMode);
	PermediaLoadCoord(pScrn, x2, y2, x2+w, h, 0, direction);
  } else {
  	srcaddr = y1 * pScrn->displayWidth + (x1 & ~pGlint->bppalign);
  	dstaddr = y2 * pScrn->displayWidth + (x2 & ~pGlint->bppalign);
  	align = (x2 & pGlint->bppalign) - (x1 & pGlint->bppalign);
  	GLINT_WAIT(8);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode | FBRM_Packed | 
						(align&7)<<20, FBReadMode);
	PermediaLoadCoord(pScrn, x2>>pGlint->BppShift, y2, 
				(x2+w+7)>>pGlint->BppShift, h, 0, direction);
  	GLINT_WRITE_REG(x2<<16|(x2+w), PackedDataLimits);
  }

  GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
  GLINT_WRITE_REG(PrimitiveTrapezoid, Render);
}

static void 
PermediaSetupForFillRectSolid(
	ScrnInfoPtr pScrn, 
	int color, int rop, 
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    REPLICATE(color);
#if 0
  gcolor = color;
#endif

    GLINT_WAIT(7);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {
	pGlint->FrameBufferReadMode = pGlint->pprod;
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
  	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(color, FBBlockColor);
    } else {
	pGlint->FrameBufferReadMode = pGlint->pprod|FBRM_DstEnable;
      	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
      	GLINT_WRITE_REG(color, ConstantColor);
    }
    LOADROP(rop);
}

void PermediaSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int speed = 0;
    if (pGlint->ROP == GXcopy) {
	GLINT_WAIT(5);
	PermediaLoadCoord(pScrn, x, y, x+w, h, 0, 1);
  	speed = FastFillEnable;
    } else {
	GLINT_WAIT(7);
      	GLINT_WRITE_REG(pGlint->pprod | FBRM_Packed | FBRM_DstEnable, FBReadMode);
	PermediaLoadCoord(pScrn, x>>pGlint->BppShift, y, 
					(x+w+7)>>pGlint->BppShift, h, 0, 1);
  	GLINT_WRITE_REG(x<<16|(x+w), PackedDataLimits);
    }
    GLINT_WRITE_REG(PrimitiveTrapezoid | speed, Render);
}

#if 0
static void
PermediaSetupFor8x8PatternColorExpand (int patternx, int patterny,
				       int bg, int fg, int rop,
				       unsigned planemask)
{
  gfg = fg;
  gbg = bg;
  REPLICATE(gfg);
  REPLICATE(gbg);
  grop = rop;

  GLINT_WAIT(16);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG ((patternx & 0x000000ff), AreaStipplePattern0);
  GLINT_WRITE_REG ((patternx & 0x0000ff00) >> 8, AreaStipplePattern1);
  GLINT_WRITE_REG ((patternx & 0x00ff0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG ((patternx & 0xff000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG ((patterny & 0x000000ff), AreaStipplePattern4);
  GLINT_WRITE_REG ((patterny & 0x0000ff00) >> 8, AreaStipplePattern5);
  GLINT_WRITE_REG ((patterny & 0x00ff0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG ((patterny & 0xff000000) >> 24, AreaStipplePattern7);

  if (rop == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
	GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
  }
  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  LOADROP(rop);
  GLINT_WRITE_REG(1 << 16, dY);
   GLINT_WRITE_REG(gfg, ConstantColor);
   	GLINT_WRITE_REG(gbg, Texel0);
}

void 
PermediaSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
   GLINT_WAIT(6);
   GLINT_WRITE_REG((x+w)<<16, StartXSub);
   GLINT_WRITE_REG(x<<16,     StartXDom);
   GLINT_WRITE_REG(y<<16,     StartY);
   GLINT_WRITE_REG(h,         GLINTCount);
   if (gbg != -1) {
   	GLINT_WRITE_REG(1<<20|patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  	GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid, Render);
  } else { 
  	GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  	GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid, Render);
  }
}

void
PermediaSubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
{
  int dy = y2 - y1 + 1;

  if ((dy < 16) && (grop == GXcopy)) {
	GLINT_WAIT(5);
  	GLINT_WRITE_REG(x1<<16, StartXDom);
  	GLINT_WRITE_REG(y1<<16, StartY);
  	GLINT_WRITE_REG(dy, GLINTCount);
  	GLINT_WRITE_REG((x1+1)<<16, StartXSub);
  	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable, Render);
	return;
  }
  
  GLINT_WAIT(6);
  GLINT_WRITE_REG(mode, FBReadMode);
  GLINT_WRITE_REG (gcolor, GLINTColor);
  GLINT_WRITE_REG (x1 << 16, StartXDom);
  GLINT_WRITE_REG (y1 << 16, StartY);
  GLINT_WRITE_REG (dy, GLINTCount);

  GLINT_WRITE_REG (PrimitiveLine, Render);
}
#endif

static void 
PermediaWriteBitmap(ScrnInfoPtr pScrn, 
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int srcx, int srcy,
    int fg, int bg,
    int rop,
    unsigned int planemask
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned char *srcp;
    unsigned char *srcpntr;
    int dwords, skipleft, height;
    register int count; 
    register CARD32* pattern;
    Bool SecondPass = FALSE;

    srcp = (srcwidth * srcy) + (srcx >> 3) + src; 
    srcx &= 0x07;
    if((skipleft = (long)srcp & 0x03)) { 
	skipleft = (skipleft << 3) + srcx;	
	srcp = (unsigned char *)((long)srcp & ~0x03);
    } else
	skipleft = srcx;    

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    /* >>>>> Set rop, planemask, left edge clip skipleft pixels right
	of x (skipleft is sometimes 0 and clipping isn't needed). <<<<<< */
    if (skipleft) 
	PermediaSetClippingRectangle(pScrn, x+skipleft, y, x+w, y+h);
    else
	CHECKCLIPPING;
    GLINT_WAIT(13);
    DO_PLANEMASK(planemask);
    LOADROP(rop);
    if (rop == GXcopy) {
	pGlint->FrameBufferReadMode = FastFillEnable;
    	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    } else {
	pGlint->FrameBufferReadMode = 0;
    	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    }
    GLINT_WRITE_REG(x<<16, StartXDom);
    GLINT_WRITE_REG(y<<16, StartY);
    GLINT_WRITE_REG((x+w)<<16, StartXSub);
    GLINT_WRITE_REG(h, GLINTCount);
    GLINT_WRITE_REG(1<<16, dY);

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	GLINT_WRITE_REG(PrimitiveTrapezoid |pGlint->FrameBufferReadMode,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    }

   /* >>>>>>>>> initiate transfer (x,y,w,h).  Skipleft pixels on the
	left edge will be clipped <<<<<< */

SECOND_PASS:
    GLINT_WRITE_REG(PrimitiveTrapezoid | pGlint->FrameBufferReadMode | SyncOnBitMask, Render);
    
    height = h;
    srcpntr = srcp;
    while(height--) {
	count = dwords >> 3;
	pattern = (CARD32*)srcpntr;
	while(count--) {
		GLINT_WAIT(8);
		GLINT_WRITE_REG(*(pattern), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+1), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+2), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+3), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+4), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+5), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+6), BitMaskPattern);
		GLINT_WRITE_REG(*(pattern+7), BitMaskPattern);
		pattern+=8;
	}
	count = dwords & 0x07;
	GLINT_WAIT(count);
	while (count--)
		GLINT_WRITE_REG(*(pattern++), BitMaskPattern);
	srcpntr += srcwidth;
    }    

    if(SecondPass) {
	SecondPass = FALSE;
	/* >>>>>> invert bitmask and set bg <<<<<<<< */
	REPLICATE(bg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(InvertBitMask|
				BitMaskPackingEachScanline, RasterizerMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	goto SECOND_PASS;
    }

    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG(infoRec);	
}

static void
PermediaSetupForCPUToScreenColorExpandFill(
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
        GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
        GLINT_WRITE_REG(fg, FBBlockColor);
        pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
        GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
        GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
        GLINT_WRITE_REG(fg, ConstantColor);
        pGlint->FrameBufferReadMode = 0;
    }
    LOADROP(rop);
}

static void
PermediaSubsequentCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int dwords = ((w + 31) >> 5) * h;

    if (skipleft) {
	PermediaSetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    }

    GLINT_WAIT(6);
    PermediaLoadCoord(pScrn, x, y, x+w, h, 0, 1);
    GLINT_WRITE_REG(PrimitiveTrapezoid | pGlint->FrameBufferReadMode | 
							SyncOnBitMask, Render);
    GLINT_WRITE_REG((dwords - 1)<<16 | 0x0D, OutputFIFO);
}

static void 
PermediaTEGlyphRenderer(
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

static void 
PermediaNonTEGlyphRenderer(
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

static void 
PermediaFillColorExpandSpans(
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
PermediaFillColorExpandRects(ScrnInfoPtr pScrn,
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
