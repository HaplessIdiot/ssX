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
 *           Mark Vojkovich, <mvojkovi@ucsd.edu>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 * 
 * Permedia 2 accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_accel.c,v 1.3 1998/07/31 10:41:21 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#include "xaalocal.h"		/* For replacements */

static void Permedia2Sync(ScrnInfoPtr pScrn);
static void Permedia2SetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Permedia2SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
static void Permedia2SetupForFillRectSolid24bpp(ScrnInfoPtr pScrn,int color,
				int rop, unsigned int planemask);
static void Permedia2SubsequentFillRectSolid24bpp(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
static void Permedia2SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);
static void Permedia2SetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void Permedia2SubsequentScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, int w, int h);
static void Permedia2SetupForScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop,
				unsigned int planemask,
				int transparency_color);
static void Permedia2SetClippingRectangle(ScrnInfoPtr pScrn, int x, int y,
				int w, int h);
static void Permedia2SetupForHorVertLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Permedia2SubsequentHorVertLine(ScrnInfoPtr pScrn, int x, int y, 
				int len, int dir);
static void Permedia2WriteBitmap(ScrnInfoPtr pScrn, int x, int y, int w, int h, 
				unsigned char *src, int srcwidth, int skipleft, 
				int fg, int bg, int rop,unsigned int planemask);
static void Permedia2SetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void Permedia2SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void Permedia2SetupForMono8x8PatternFill24bpp(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void Permedia2SubsequentMono8x8PatternFillRect24bpp(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void Permedia2WritePixmap8bpp(ScrnInfoPtr pScrn, int x, int y, int w, 
				int h, unsigned char *src, int srcwidth, 
				int rop, unsigned int planemask, 
				int transparency_color, int bpp, int depth);
static void Permedia2WritePixmap16bpp(ScrnInfoPtr pScrn, int x, int y, int w, 
				int h, unsigned char *src, int srcwidth, 
				int rop, unsigned int planemask, 
				int transparency_color, int bpp, int depth);
static void Permedia2WritePixmap24bpp(ScrnInfoPtr pScrn, int x, int y, int w, 
				int h, unsigned char *src, int srcwidth, 
				int rop, unsigned int planemask, 
				int transparency_color, int bpp, int depth);
static void Permedia2WritePixmap32bpp(ScrnInfoPtr pScrn, int x, int y, int w, 
				int h, unsigned char *src, int srcwidth, 
				int rop, unsigned int planemask, 
				int transparency_color, int bpp, int depth);
static void Permedia2SetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
				int fg, int bg, int rop,unsigned int planemask);
static void Permedia2SubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, 
				int x, int y, int w, int h, int skipleft);
static void Permedia2TEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int w,
    				int h, int skipleft, int startline, 
    				unsigned int **glyphs, int glyphWidth,
    				int fg, int bg, int rop, unsigned planemask);
static void Permedia2NonTEGlyphRenderer(ScrnInfoPtr pScrn, int xBack, int wBack,
    				int xText, int wText, 
    				int y, int h, int skipleft, int startline, 
    				NonTEGlyphInfo *glyphp,
    				int fg, int bg, int rop,
    				unsigned int planemask);
static void Permedia2FillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg,
				int rop, unsigned int planemask, int nBox,
				BoxPtr pBox, int xorg, int yorg,PixmapPtr pPix);
static void Permedia2FillColorExpandSpans(ScrnInfoPtr pScrn, int fg, int bg,
   				int rop, unsigned int planemask, int n,
   				DDXPointPtr ppt, int *pwidth, int fSorted,
   				int xorg, int yorg, PixmapPtr pPix);
static void Permedia2LoadCoord(ScrnInfoPtr pScrn, int x, int y, int w, int h);

#define MAX_FIFO_ENTRIES 256

static void
Permedia2InitializeEngine(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    /* Initialize the Accelerator Engine to defaults */

    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	ScissorMode);
    GLINT_SLOW_WRITE_REG(UNIT_ENABLE,	FBWriteMode);
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
	case 24:
 	    GLINT_SLOW_WRITE_REG(0x4, FBReadPixel); /* 24 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 2<<19,	PMTextureMapFormat);
	    break;
	case 32:
	    GLINT_SLOW_WRITE_REG(0x2, FBReadPixel); /* 32 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 2<<19,	PMTextureMapFormat);
  	    break;
    }
    pGlint->ClippingOn = FALSE;
    pGlint->startxdom = 0;
    pGlint->startxsub = 0;
    pGlint->starty = 0;
    pGlint->count = 0;
    pGlint->dy = 1;
    pGlint->dxdom = 0;
    pGlint->rectxy = 0;
    pGlint->rectwh = 0;
    pGlint->ROP = 0xFF;
    GLINT_SLOW_WRITE_REG(0, RectangleSize);
    GLINT_SLOW_WRITE_REG(0, RectangleOrigin);
    GLINT_SLOW_WRITE_REG(0, dXDom);
    GLINT_SLOW_WRITE_REG(1<<16, dY);
    GLINT_SLOW_WRITE_REG(0, StartXDom);
    GLINT_SLOW_WRITE_REG(0, StartXSub);
    GLINT_SLOW_WRITE_REG(0, StartY);
    GLINT_SLOW_WRITE_REG(0, GLINTCount);
}

Bool
Permedia2AccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    BoxRec AvailFBArea;

    pGlint->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    Permedia2InitializeEngine(pScrn);

    infoPtr->Flags = PIXMAP_CACHE;
 
    infoPtr->Sync = Permedia2Sync;

    infoPtr->SolidFillFlags = 0;
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY; 
    infoPtr->WriteBitmapFlags = 0;
    if (pScrn->bitsPerPixel == 24) {
    	infoPtr->SetupForSolidFill = 
				Permedia2SetupForFillRectSolid24bpp;
    	infoPtr->SubsequentSolidFillRect = 	
				Permedia2SubsequentFillRectSolid24bpp;
    } else {
        infoPtr->SolidLineFlags = 0;
        infoPtr->SetupForSolidLine = Permedia2SetupForHorVertLine;
        infoPtr->SubsequentSolidHorVertLine = Permedia2SubsequentHorVertLine;
    	infoPtr->SetupForSolidFill = Permedia2SetupForFillRectSolid;
    	infoPtr->SubsequentSolidFillRect = Permedia2SubsequentFillRectSolid;
    }
    
    if (pScrn->bitsPerPixel >= 24) {
	infoPtr->SetupForScreenToScreenCopy = 	
				Permedia2SetupForScreenToScreenCopy2432bpp;
    	infoPtr->SubsequentScreenToScreenCopy = 		
				Permedia2SubsequentScreenToScreenCopy2432bpp;
    } else {
    	infoPtr->SetupForScreenToScreenCopy = 	
				Permedia2SetupForScreenToScreenCopy;
    	infoPtr->SubsequentScreenToScreenCopy = 		
				Permedia2SubsequentScreenToScreenCopy;
    }

    infoPtr->SetClippingRectangle = Permedia2SetClippingRectangle;

    infoPtr->Mono8x8PatternFillFlags = 
    				HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    				HARDWARE_PATTERN_PROGRAMMED_BITS |
    				HARDWARE_PATTERN_SCREEN_ORIGIN;

    if (pScrn->bitsPerPixel == 24) {
	infoPtr->SetupForMono8x8PatternFill =
				Permedia2SetupForMono8x8PatternFill24bpp;
	infoPtr->SubsequentMono8x8PatternFillRect = 
				Permedia2SubsequentMono8x8PatternFillRect24bpp;
    } else {
	infoPtr->SetupForMono8x8PatternFill =
				Permedia2SetupForMono8x8PatternFill;
	infoPtr->SubsequentMono8x8PatternFillRect = 
				Permedia2SubsequentMono8x8PatternFillRect;
    }

    infoPtr->TEGlyphRenderer = Permedia2TEGlyphRenderer;
    infoPtr->NonTEGlyphRenderer = Permedia2NonTEGlyphRenderer;
    infoPtr->FillColorExpandRects = Permedia2FillColorExpandRects;
    infoPtr->FillColorExpandSpans = Permedia2FillColorExpandSpans;
    infoPtr->CPUToScreenColorExpandFillFlags = TRANSPARENCY_ONLY |
      					       CPU_TRANSFER_PAD_DWORD |
      					       BIT_ORDER_IN_BYTE_LSBFIRST |
					       LEFT_EDGE_CLIPPING |
					       LEFT_EDGE_CLIPPING_NEGATIVE_X;

    infoPtr->ColorExpandRange = MAX_FIFO_ENTRIES;
    infoPtr->ColorExpandBase = pGlint->IOBase + OutputFIFO + 4;
    infoPtr->SetupForCPUToScreenColorExpandFill =
				Permedia2SetupForCPUToScreenColorExpandFill;
    infoPtr->SubsequentCPUToScreenColorExpandFill = 
				Permedia2SubsequentCPUToScreenColorExpandFill;
    infoPtr->WriteBitmap = Permedia2WriteBitmap;

    if (pScrn->bitsPerPixel == 8)
  	infoPtr->WritePixmap = Permedia2WritePixmap8bpp;
    else
    if (pScrn->bitsPerPixel == 16)
  	infoPtr->WritePixmap = Permedia2WritePixmap16bpp;
    else
#if 0
    if (pScrn->bitsPerPixel == 24) {
  	infoPtr->WritePixmap = Permedia2WritePixmap24bpp;
	infoPtr->WritePixmapFlags |= NO_PLANEMASK;
    }
    else
#endif
    if (pScrn->bitsPerPixel == 32)
  	infoPtr->WritePixmap = Permedia2WritePixmap32bpp;

    /* Now fixup if we are 24bpp */
    if (pScrn->bitsPerPixel == 24) {
	infoPtr->SolidFillFlags |= NO_PLANEMASK;
	infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;
        infoPtr->WriteBitmapFlags |= NO_PLANEMASK;
        infoPtr->CPUToScreenColorExpandFillFlags |= NO_PLANEMASK;
        infoPtr->Mono8x8PatternFillFlags |= NO_PLANEMASK;
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pGlint->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);
    if (AvailFBArea.x2 > 2048) AvailFBArea.x2 = 2047;
    if (AvailFBArea.y2 > 2048) AvailFBArea.y2 = 2047;

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void Permedia2LoadCoord(
	ScrnInfoPtr pScrn,
	int x, int y,
	int w, int h
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    if (((h<<16)|w) != pGlint->rectwh) {
	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	pGlint->rectwh = (h<<16)|w;
    }
    if (((y<<16)|x) != pGlint->rectxy) {
	GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
	pGlint->rectxy = (y<<16)|x;
    }
}


static void
Permedia2Sync(ScrnInfoPtr pScrn)
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
Permedia2SetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(3);
    GLINT_WRITE_REG(((y1&0x0fff)<<16)|(x1&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG(((y2&0x0fff)<<16)|(x2&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG(1, ScissorMode);
    pGlint->ClippingOn = TRUE;
}

static void
Permedia2SetupForScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn, 
				 int xdir, int ydir, int rop,
				 unsigned int planemask, int transparency_color)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    pGlint->BltScanDirection = 0;
    if (xdir == 1) pGlint->BltScanDirection |= XPositive;
    if (ydir == 1) pGlint->BltScanDirection |= YPositive;
  
    if (pScrn->bitsPerPixel == 24) {
	GLINT_WAIT(2);
    } else {
        GLINT_WAIT(3);
        DO_PLANEMASK(planemask);
    }

    if ((rop == GXset) || (rop == GXclear)) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
    	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	    GLINT_WRITE_REG(pGlint->pprod|FBRM_SrcEnable, FBReadMode);
        } else {
	    GLINT_WRITE_REG(pGlint->pprod|FBRM_SrcEnable|FBRM_DstEnable, 
								FBReadMode);
        }
    }
    LOADROP(rop);
}

static void
Permedia2SubsequentScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn, int x1,
					int y1, int x2, int y2, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    int srcaddr = y1 * pScrn->displayWidth + x1;
    int dstaddr = y2 * pScrn->displayWidth + x2;
    GLINT_WAIT(4);
    Permedia2LoadCoord(pScrn, x2, y2, w, h);
    GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
    GLINT_WRITE_REG(PrimitiveRectangle | pGlint->BltScanDirection, Render);
}

static void
Permedia2SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    pGlint->BltScanDirection = 0;
    if (xdir == 1) pGlint->BltScanDirection |= XPositive;
    if (ydir == 1) pGlint->BltScanDirection |= YPositive;
  
    GLINT_WAIT(2);
    DO_PLANEMASK(planemask);

    if ((rop == GXset) || (rop == GXclear)) {
	pGlint->FrameBufferReadMode = pGlint->pprod;
    } else
    if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	pGlint->FrameBufferReadMode = pGlint->pprod |FBRM_SrcEnable;
    } else {
	pGlint->FrameBufferReadMode = pGlint->pprod | FBRM_SrcEnable |
							FBRM_DstEnable;
    }
    LOADROP(rop);
}

static void
Permedia2SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int srcaddr;
    int dstaddr;
    char align;

    /* We can only use GXcopy for Packed modes */
    if (pGlint->ROP != GXcopy) {
  	srcaddr = y1 * pScrn->displayWidth + x1;
  	dstaddr = y2 * pScrn->displayWidth + x2;
	GLINT_WAIT(6);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode, FBReadMode);
        Permedia2LoadCoord(pScrn, x2, y2, w, h);
    } else {
  	srcaddr = y1 * pScrn->displayWidth + (x1 & ~pGlint->bppalign);
  	dstaddr = y2 * pScrn->displayWidth + (x2 & ~pGlint->bppalign);
  	align = (x2 & pGlint->bppalign) - (x1 & pGlint->bppalign);
	GLINT_WAIT(7);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode|FBRM_Packed, FBReadMode);
        Permedia2LoadCoord(pScrn, x2>>pGlint->BppShift, y2, 
						(w+7)>>pGlint->BppShift, h);
  	GLINT_WRITE_REG(align<<29|x2<<16|(x2+w), PackedDataLimits);
    }

    GLINT_WRITE_REG(0, WaitForCompletion);
    GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
    GLINT_WRITE_REG(PrimitiveRectangle | pGlint->BltScanDirection, Render);
}

static void
Permedia2SetupForHorVertLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    GLINT_WAIT(4);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(color, GLINTColor);
    REPLICATE(color);
    GLINT_WRITE_REG(color, FBBlockColor);
    if (rop == GXcopy) {
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
  	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
}

static void
Permedia2SetupForFillRectSolid24bpp(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    pGlint->ForeGroundColor = color;

    CHECKCLIPPING;
    GLINT_WAIT(4);
    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    GLINT_WRITE_REG(color, ConstantColor);
    if (rop == GXcopy) {
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
  	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
}

static void
Permedia2SetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    REPLICATE(color);
    CHECKCLIPPING;

    GLINT_WAIT(5);
    DO_PLANEMASK(planemask);
    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(color, FBBlockColor);
    } else {
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
      	GLINT_WRITE_REG(color, ConstantColor);
	/* We can use Packed mode for filling solid non-GXcopy rasters */
	GLINT_WRITE_REG(pGlint->pprod|FBRM_DstEnable|FBRM_Packed, FBReadMode);
    }
    LOADROP(rop);
}

static void
Permedia2SubsequentFillRectSolid24bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(3);
    Permedia2LoadCoord(pScrn, x, y, w, h);
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive, Render);
}

static void
Permedia2SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int speed = 0;

    if (pGlint->ROP == GXcopy) {
	GLINT_WAIT(3);
        Permedia2LoadCoord(pScrn, x, y, w, h);
  	speed = FastFillEnable;
    } else {
	GLINT_WAIT(4);
        Permedia2LoadCoord(pScrn, x>>pGlint->BppShift, y, 
						(w+7)>>pGlint->BppShift, h);
  	GLINT_WRITE_REG(x<<16|(x+w), PackedDataLimits);
  	speed = 0;
    }
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | speed, Render);
}

static void MoveBYTE(
   register CARD32* dest,
   register unsigned char* src,
   register int dwords
)
{
     while(dwords) {
	*dest = *src;
	src += 1;
	dest += 1;
	dwords -= 1;
     }	
}

static void MoveWORDS(
   register CARD32* dest,
   register unsigned short* src,
   register int dwords
)
{
     while(dwords & ~0x01) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	src += 2;
	dest += 2;
	dwords -= 2;
     }	
     switch(dwords) {
	case 0:	return;
	case 1: *dest = *src;
		return;
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
Permedia2SetupForMono8x8PatternFill24bpp(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    if (bg == -1) pGlint->FrameBufferReadMode = -1;
	else    pGlint->FrameBufferReadMode = 0;

    pGlint->ForeGroundColor = fg;
    pGlint->BackGroundColor = bg;
    REPLICATE(pGlint->ForeGroundColor);
    REPLICATE(pGlint->BackGroundColor);
  
    GLINT_WAIT(11);
    GLINT_WRITE_REG((patternx & 0xFF), AreaStipplePattern0);
    GLINT_WRITE_REG((patternx & 0xFF00) >> 8, AreaStipplePattern1);
    GLINT_WRITE_REG((patternx & 0xFF0000) >> 16, AreaStipplePattern2);
    GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
    GLINT_WRITE_REG((patterny & 0xFF), AreaStipplePattern4);
    GLINT_WRITE_REG((patterny & 0xFF00) >> 8, AreaStipplePattern5);
    GLINT_WRITE_REG((patterny & 0xFF0000) >> 16, AreaStipplePattern6);
    GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
  
    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    LOADROP(rop);
}

static void 
Permedia2SetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    if (bg == -1) pGlint->FrameBufferReadMode = -1;
	else    pGlint->FrameBufferReadMode = 0;

    CHECKCLIPPING;
    pGlint->ForeGroundColor = fg;
    pGlint->BackGroundColor = bg;
    REPLICATE(pGlint->ForeGroundColor);
    REPLICATE(pGlint->BackGroundColor);
  
    GLINT_WAIT(12);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG((patternx & 0xFF), AreaStipplePattern0);
    GLINT_WRITE_REG((patternx & 0xFF00) >> 8, AreaStipplePattern1);
    GLINT_WRITE_REG((patternx & 0xFF0000) >> 16, AreaStipplePattern2);
    GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
    GLINT_WRITE_REG((patterny & 0xFF), AreaStipplePattern4);
    GLINT_WRITE_REG((patterny & 0xFF00) >> 8, AreaStipplePattern5);
    GLINT_WRITE_REG((patterny & 0xFF0000) >> 16, AreaStipplePattern6);
    GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);

    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    }
    LOADROP(rop);
}

static void 
Permedia2SubsequentMono8x8PatternFillRect24bpp(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
  
    GLINT_WAIT(8);
    Permedia2LoadCoord(pScrn, x, y, w, h);

    if (pGlint->FrameBufferReadMode != -1) {
	GLINT_WRITE_REG(pGlint->BackGroundColor, ConstantColor);
	GLINT_WRITE_REG(patternx<<7|patterny<<12| ASM_InvertPattern |
						UNIT_ENABLE, AreaStippleMode);
	GLINT_WRITE_REG(AreaStippleEnable | XPositive | 
					YPositive | PrimitiveRectangle, Render);
    }

    GLINT_WRITE_REG(pGlint->ForeGroundColor, ConstantColor);
    GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
    GLINT_WRITE_REG(AreaStippleEnable | XPositive | YPositive |
						PrimitiveRectangle, Render);
}

static void 
Permedia2SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
  
    GLINT_WAIT(8);
    Permedia2LoadCoord(pScrn, x, y, w, h);

    if (pGlint->FrameBufferReadMode != -1) {
	if (pGlint->ROP == GXcopy) {
      	    GLINT_WRITE_REG(pGlint->BackGroundColor, FBBlockColor);
	    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
					FastFillEnable, Render);
	} else { 
      	    GLINT_WRITE_REG(pGlint->BackGroundColor, ConstantColor);
	    GLINT_WRITE_REG(patternx<<7|patterny<<12| ASM_InvertPattern |
			UNIT_ENABLE, AreaStippleMode);
  	    GLINT_WRITE_REG(AreaStippleEnable | XPositive | 
				YPositive | PrimitiveRectangle, Render);
	}
    }

    if (pGlint->ROP == GXcopy) {
	GLINT_WRITE_REG(pGlint->ForeGroundColor, FBBlockColor);
	pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
  	GLINT_WRITE_REG(pGlint->ForeGroundColor, ConstantColor);
	pGlint->FrameBufferReadMode = 0;
    }
    GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
    GLINT_WRITE_REG(AreaStippleEnable | pGlint->FrameBufferReadMode | 
			XPositive | YPositive | PrimitiveRectangle, Render);
}

static void
Permedia2SubsequentHorVertLine(ScrnInfoPtr pScrn,int x,int y,int len,int dir)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
  
    GLINT_WAIT(6);
    if (x != pGlint->startxdom) {
	GLINT_WRITE_REG(x<<16, StartXDom);
	pGlint->startxdom = x;
    }
    if (y != pGlint->starty) {
	GLINT_WRITE_REG(y<<16, StartY);
	pGlint->starty = y;
    }
	if (dir == DEGREES_0) {
	    if (pGlint->dxdom != 1) {
	        GLINT_WRITE_REG(1<<16, dXDom);
		pGlint->dxdom = 1;
	    }
	    if (pGlint->dy != 0) {
	        GLINT_WRITE_REG(0<<16, dY);
		pGlint->dy = 0;
	    }
        } else {
	    if (pGlint->dxdom != 0) {
	        GLINT_WRITE_REG(0<<16, dXDom);
		pGlint->dxdom = 0;
	    }
	    if (pGlint->dy != 1) {
	        GLINT_WRITE_REG(1<<16, dY);
		pGlint->dy = 1;
	    }
        }

	if (len != pGlint->count) {
            GLINT_WRITE_REG(len, GLINTCount);
	    pGlint->count = len;
	}
        GLINT_WRITE_REG(PrimitiveLine, Render);
}

static void
Permedia2SetupForCPUToScreenColorExpandFill(
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
    } else {
        GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
        GLINT_WRITE_REG(fg, FBBlockColor);
	GLINT_WRITE_REG(0, RasterizerMode);
        pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
        GLINT_WRITE_REG(fg, ConstantColor);
        pGlint->FrameBufferReadMode = 0;
    }
    LOADROP(rop);
}

static void
Permedia2SubsequentCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int dwords = ((w + 31) >> 5) * h;

    if (skipleft)
	Permedia2SetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    else
	CHECKCLIPPING;

    GLINT_WAIT(6);
    Permedia2LoadCoord(pScrn, x, y, w, h);
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | SyncOnBitMask |
					pGlint->FrameBufferReadMode, Render);
    GLINT_WRITE_REG((dwords - 1)<<16 | 0x0D, OutputFIFO);
}

static void
Permedia2WriteBitmap(ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned char *srcpntr;
    int dwords, height, mode;
    Bool SecondPass = FALSE;

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    if (skipleft)
	Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    else
	CHECKCLIPPING;
 
    if (pScrn->bitsPerPixel == 24) {
	GLINT_WAIT(9);
    } else {
        GLINT_WAIT(10);
        DO_PLANEMASK(planemask);
    }
    LOADROP(rop);
    Permedia2LoadCoord(pScrn, x&0xFFFF, y, w, h);
    if (rop == GXcopy) {
    	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
	mode = FastFillEnable;
    	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    } else {
	mode = 0;
	GLINT_WRITE_REG(BitMaskPackingEachScanline,RasterizerMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    }

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |mode,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    }

   /* >>>>>>>>> initiate transfer (x,y,w,h).  Skipleft pixels on the
	left edge will be clipped <<<<<< */

SECOND_PASS:
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | mode | SyncOnBitMask, Render);
    
    height = h;
    srcpntr = src;
    while(height--) {
    	GLINT_WAIT(dwords);
    	/* 0x0D is the TAG value for BitMaskPattern */
    	GLINT_WRITE_REG(((dwords - 1) << 16) | 0x0D, OutputFIFO);
   	MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
		(CARD32 *)srcpntr, dwords);
	srcpntr += srcwidth;
    }   

    if(SecondPass) {
	SecondPass = FALSE;
	/* >>>>>> invert bitmask and set bg <<<<<<<< */
	REPLICATE(bg);
	GLINT_WAIT(2);
    if ((pScrn->bitsPerPixel != 24) && (rop == GXcopy)) {
    	    GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
    	    GLINT_WRITE_REG(InvertBitMask|BitMaskPackingEachScanline, 
						RasterizerMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	goto SECOND_PASS;
    }

    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG(infoRec);
}


static void
Permedia2WritePixmap8bpp(
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
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int skipleft, dwords, count;
    CARD32* srcp;
    unsigned char *srcpbyte;
    Bool FastTexLoad = FALSE;

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {      
	GLINT_WRITE_REG(pGlint->pprod | FBRM_Packed, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }

    dwords = (w + 3) >> 2;
    if((!(x&3)) && (!(w&3))) FastTexLoad = TRUE;	
    if((rop != GXcopy) || (planemask != ~0))
	FastTexLoad = FALSE;

    if (rop == GXcopy) {
	skipleft = 0;
    } else {
	if((skipleft = (long)src & 0x03)) {
	    	skipleft /= (bpp>>3);

	    x -= skipleft;	     
	    w += skipleft;
	
	       src = (unsigned char*)((long)src & ~0x03); 
	}
    }
	

        if(FastTexLoad) {
	  int address;

	  CHECKCLIPPING;
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  Permedia2Sync(pScrn);	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = ((y * pScrn->displayWidth) + x) >> 2;
	      srcp = (CARD32*)src;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		address += MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x11 << 4) | 0x0D,
					 OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
  	   char align = (x & pGlint->bppalign);
		
    	   if (skipleft)
		Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
	   else
		CHECKCLIPPING;

	   if (rop == GXcopy) {
	     GLINT_WAIT(6);
             Permedia2LoadCoord(pScrn, (x&0xFFFF)>>pGlint->BppShift, y, 
				(w+pGlint->bppalign)>>pGlint->BppShift, h);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
             Permedia2LoadCoord(pScrn, x&0xFFFF, y, w, h);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   if (rop == GXcopy) {
	    while(h--) {
	      count = dwords;
	      srcp = (CARD32*)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	    }  
	   } else {
	    while(h--) {
	      count = w;
	      srcpbyte = (unsigned char *)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcpbyte += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, count);
	      }
	      src += srcwidth;
	    }  
	   }
	}

    SET_SYNC_FLAG(infoRec);
}

static void
Permedia2WritePixmap16bpp(
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
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int skipleft, dwords, count;
    CARD32* srcp;
    unsigned short* srcpword;
    Bool FastTexLoad;

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {      
	GLINT_WRITE_REG(pGlint->pprod | FBRM_Packed, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }

	FastTexLoad = FALSE;
	dwords = (w + 1) >> 1;
	if((!(x&1)) && (!(w&1))) FastTexLoad = TRUE;
	if((rop != GXcopy) || (planemask != ~0))
		FastTexLoad = FALSE;

	if (rop == GXcopy) {
	  skipleft = 0;
	} else {
	  if((skipleft = (long)src & 0x03L)) {
	    		skipleft /= (bpp>>3);

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	   src = (unsigned char*)((long)src & ~0x03L); 
	  }
	}
	
        if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  Permedia2Sync(pScrn);	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = ((y * pScrn->displayWidth) + x) >> 1;
	      srcp = (CARD32*)src;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		address += MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x11 << 4) | 0x0D,
					 OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
  	   char align = (x & pGlint->bppalign);
		
    	   if (skipleft)
		Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    	   else
		CHECKCLIPPING;

	   GLINT_WAIT(6);
	   if (rop == GXcopy) {
	     GLINT_WAIT(6);
             Permedia2LoadCoord(pScrn, (x&0xFFFF)>>pGlint->BppShift, y, 
				(w+pGlint->bppalign)>>pGlint->BppShift, h);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
             Permedia2LoadCoord(pScrn, x&0xFFFF, y, w, h);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   if (rop == GXcopy) {
	    while(h--) {
	      count = dwords;
	      srcp = (CARD32*)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	    }  
	   } else {
	    while(h--) {
	      count = w;
	      srcpword = (unsigned short *)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned short *)srcpword, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcpword += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned short *)srcpword, count);
	      }
	      src += srcwidth;
	    }  
	   }
	}

    SET_SYNC_FLAG(infoRec);
}

static void
Permedia2WritePixmap24bpp(
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
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int skipleft = 0, dwords, count;
    unsigned char* srcpbyte;
    CARD32* srcp;

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {      
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }

	dwords = ((w+1)*3)>>2;
	  if((skipleft = (long)src & 0x03L)) {
			skipleft = 4 - skipleft;

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	   src = (unsigned char*)(src - (3*skipleft));  
	}
	
	{
    	   if (skipleft)
		Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    	   else
		CHECKCLIPPING;

	   GLINT_WAIT(5);
           Permedia2LoadCoord(pScrn, x&0xFFFF, y, w, h);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

#if 1
	    while(h--) {
	      count = w;
	      srcpbyte = (unsigned char *)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcpbyte += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, count);
	      }
	      src += srcwidth;
#else
	   while(h--) {
	      count = dwords;
	      srcp = (CARD32*)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
#endif
	   }  
	}

    SET_SYNC_FLAG(infoRec);
}


static void
Permedia2WritePixmap32bpp(
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
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int skipleft, dwords, count;
    CARD32* srcp;
    Bool FastTexLoad;

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {      
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }

	FastTexLoad = TRUE;
	dwords = w;
	if((rop != GXcopy) || (planemask != ~0))
		FastTexLoad = FALSE;
	
	if (!FastTexLoad) {
	  if((skipleft = (long)src & 0x03L)) {
	    		skipleft /= (bpp>>3);

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	   src = (unsigned char*)((long)src & ~0x03L); 
	  }
	}
	
        if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  Permedia2Sync(pScrn);	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = (y * pScrn->displayWidth) + x;
	      srcp = (CARD32*)src;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		address += MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x11 << 4) | 0x0D,
					 OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
    	   if (skipleft)
		Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    	   else
		CHECKCLIPPING;

	   GLINT_WAIT(6);
           Permedia2LoadCoord(pScrn, x&0xFFFF, y, w, h);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   while(h--) {
	      count = dwords;
	      srcp = (CARD32*)src;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)pGlint->IOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      src += srcwidth;
	   }  
	}

    SET_SYNC_FLAG(infoRec);
}

static void 
Permedia2TEGlyphRenderer(
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
Permedia2NonTEGlyphRenderer(
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
Permedia2FillColorExpandSpans(
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
Permedia2FillColorExpandRects(ScrnInfoPtr pScrn,
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
