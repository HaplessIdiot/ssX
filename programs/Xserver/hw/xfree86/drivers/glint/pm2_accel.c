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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_accel.c,v 1.2 1998/07/25 16:55:47 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

static void	Permedia2Sync(ScrnInfoPtr pScrn);
static void	Permedia2SetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
					int rop, unsigned int planemask);
static void	Permedia2SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
					int y, int w, int h);
static void    	Permedia2SetupForFillRectSolid24bpp(ScrnInfoPtr pScrn,int color,
					int rop, unsigned int planemask);
static void	Permedia2SubsequentFillRectSolid24bpp(ScrnInfoPtr pScrn, int x,
						int y, int w, int h);
static void 	Permedia2SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
						int x1, int y1, int x2,
						int y2, int w, int h);
static void	Permedia2SetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void 	Permedia2SubsequentScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, int w, int h);
static void	Permedia2SetupForScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop,
				unsigned int planemask,
				int transparency_color);
static void	Permedia2SetClippingRectangle(ScrnInfoPtr pScrn, int x, int y,
				int w, int h);
static void    	Permedia2SetupForHorVertLine(ScrnInfoPtr pScrn,int color,
					int rop, unsigned int planemask);
static void	Permedia2SubsequentHorVertLine(ScrnInfoPtr pScrn, int x, int y, int len, int dir);
#if 0
static void	Permedia2FillRectStippled();
static void	Permedia2ImageTextTECPUToScreenColorExpand();
static void	Permedia2PolyTextTECPUToScreenColorExpand();
#endif
static void	Permedia2WriteBitmap(ScrnInfoPtr pScrn, int x, int y, int w, int h, unsigned char *src, int srcwidth, int skipleft, int fg, int bg, int rop, unsigned int planemask);
static void	Permedia2FillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop, unsigned int planemask, int nBox, BoxPtr pBox, int xorg, int yorg, PixmapPtr pPixmap);
static void	Permedia2SetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patternx, int patterny, int fg, int bg, int rop, unsigned int planemask);
static void	Permedia2SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patternx, int patterny, int x, int y, int w, int h);
static void	Permedia2SetupForMono8x8PatternFill24bpp(ScrnInfoPtr pScrn, int patternx, int patterny, int fg, int bg, int rop, unsigned int planemask);
static void	Permedia2SubsequentMono8x8PatternFillRect24bpp(ScrnInfoPtr pScrn, int patternx, int patterny, int x, int y, int w, int h);
static void 	Permedia2WritePixmap8bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h, unsigned char *src, int srcwidth, int rop, unsigned int planemask, int transparency_color, int bpp, int depth);
static void 	Permedia2WritePixmap16bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h, unsigned char *src, int srcwidth, int rop, unsigned int planemask, int transparency_color, int bpp, int depth);
static void 	Permedia2WritePixmap24bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h, unsigned char *src, int srcwidth, int rop, unsigned int planemask, int transparency_color, int bpp, int depth);
static void 	Permedia2WritePixmap32bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h, unsigned char *src, int srcwidth, int rop, unsigned int planemask, int transparency_color, int bpp, int depth);

static void
Permedia2InitializeEngine(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    /* Initialize the Accelerator Engine to defaults */

    GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	ScissorMode);
    GLINT_SLOW_WRITE_REG(UNIT_ENABLE,	FBWriteMode);
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
	case 24:
 	    GLINT_SLOW_WRITE_REG(0x4, FBReadPixel); /* 24 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 2<<19,	PMTextureMapFormat);
	    break;
	case 32:
	    GLINT_SLOW_WRITE_REG(0x2, FBReadPixel); /* 32 Bits */
	    GLINT_SLOW_WRITE_REG(pGlint->pprod | 2<<19,	PMTextureMapFormat);
  	    break;
    }
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

    if (pScrn->bitsPerPixel == 24) {
    	infoPtr->SetupForSolidFill = 
				Permedia2SetupForFillRectSolid24bpp;
    	infoPtr->SubsequentSolidFillRect = 	
				Permedia2SubsequentFillRectSolid24bpp;
    } else {
    	infoPtr->SetupForSolidFill = Permedia2SetupForFillRectSolid;
    	infoPtr->SubsequentSolidFillRect = Permedia2SubsequentFillRectSolid;
    }
    infoPtr->SetupForSolidLine = Permedia2SetupForHorVertLine;
    infoPtr->SubsequentSolidHorVertLine = Permedia2SubsequentHorVertLine;
    
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
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY; 

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

  if (pScrn->bitsPerPixel != 24) {
	/* Performance loss ??? */
	/* infoPtr->FillColorExpandRects = Permedia2FillColorExpandRects; */
  	infoPtr->WriteBitmap = Permedia2WriteBitmap;

#if 0
  	xf86AccelInfoRec.ImageTextTE=Permedia2ImageTextTECPUToScreenColorExpand;
  	xf86GCInfoRec.ImageGlyphBltTE = xf86ImageGlyphBltTE;
 
  	xf86AccelInfoRec.PolyTextTE = Permedia2PolyTextTECPUToScreenColorExpand;
  	xf86GCInfoRec.PolyGlyphBltTE = xf86PolyGlyphBltTE;
#endif
    }

    if (pScrn->bitsPerPixel == 8)
  	infoPtr->WritePixmap = Permedia2WritePixmap8bpp;
    else
    if (pScrn->bitsPerPixel == 16)
  	infoPtr->WritePixmap = Permedia2WritePixmap16bpp;
    else
#if 0
    if (pScrn->bitsPerPixel == 24)
  	infoPtr->WritePixmap = Permedia2WritePixmap24bpp;
    else
#endif
    if (pScrn->bitsPerPixel == 32)
  	infoPtr->WritePixmap = Permedia2WritePixmap32bpp;

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

static void
Permedia2Sync(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    while (GLINT_READ_REG(DMACount) != 0);
    GLINT_WAIT(2);
    GLINT_WRITE_REG(0xc00, FilterMode);
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
    GLINT_WRITE_REG (((y1&0x0fff)<<16)|(x1&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG (((y2&0x0fff)<<16)|(x2&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG (1, ScissorMode);
}

static void
Permedia2SetupForScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn, 
				 int xdir, int ydir, int rop,
				 unsigned int planemask, int transparency_color)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    pGlint->BltScanDirection = 0;
    if (xdir == 1) pGlint->BltScanDirection |= XPositive;
    if (ydir == 1) pGlint->BltScanDirection |= YPositive;
  
    pGlint->ROP = rop;

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);

    if ((rop == GXset) || (rop == GXclear)) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
    	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	    GLINT_WRITE_REG(pGlint->pprod|FBRM_SrcEnable, FBReadMode);
        } else {
	    GLINT_WRITE_REG(pGlint->pprod|FBRM_SrcEnable|pGlint->BlitMode, 
								FBReadMode);
        }
    }
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

static void
Permedia2SubsequentScreenToScreenCopy2432bpp(ScrnInfoPtr pScrn, int x1,
					int y1, int x2, int y2, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    int srcaddr = y1 * pScrn->displayWidth + x1;
    int dstaddr = y2 * pScrn->displayWidth + x2;
    GLINT_WAIT(4);
    GLINT_WRITE_REG((y2<<16)|x2, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
    GLINT_WRITE_REG(PrimitiveRectangle | pGlint->BltScanDirection, Render);
}

static void
Permedia2SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    pGlint->BltScanDirection = 0;
    if (xdir == 1) pGlint->BltScanDirection |= XPositive;
    if (ydir == 1) pGlint->BltScanDirection |= YPositive;
  
    pGlint->ROP = rop;

    GLINT_WAIT(2);
    DO_PLANEMASK(planemask);

    if ((rop == GXset) || (rop == GXclear)) {
	pGlint->FrameBufferReadMode = pGlint->pprod;
    } else
    if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	pGlint->FrameBufferReadMode = pGlint->pprod |FBRM_SrcEnable;
    } else {
	pGlint->FrameBufferReadMode = pGlint->pprod | FBRM_SrcEnable |
							pGlint->BlitMode;
    }
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
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
  	GLINT_WRITE_REG((y2<<16)|x2, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    } else {
  	srcaddr = y1 * pScrn->displayWidth + (x1 & ~pGlint->bppalign);
  	dstaddr = y2 * pScrn->displayWidth + (x2 & ~pGlint->bppalign);
  	align = (x2 & pGlint->bppalign) - (x1 & pGlint->bppalign);
	GLINT_WAIT(7);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode|FBRM_Packed, FBReadMode);
  	GLINT_WRITE_REG((y2<<16)|(x2>>pGlint->BppShift), RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|((w+7)>>pGlint->BppShift), RectangleSize);
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
    pGlint->ROP = rop;

    GLINT_WAIT(4);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(color, GLINTColor);
    REPLICATE(color);
    GLINT_WRITE_REG(color, FBBlockColor);
    if (rop == GXcopy) {
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
  	GLINT_WRITE_REG(pGlint->pprod|pGlint->BlitMode, FBReadMode);
    }
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

static void
Permedia2SetupForFillRectSolid24bpp(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    pGlint->ForeGroundColor = color;
    pGlint->ROP = rop;

    GLINT_WAIT(5);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    GLINT_WRITE_REG(color, ConstantColor);
    if (rop == GXcopy) {
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
  	GLINT_WRITE_REG(pGlint->pprod|pGlint->BlitMode, FBReadMode);
    }
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

static void
Permedia2SetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    REPLICATE(color);

    GLINT_WAIT(5);
    DO_PLANEMASK(planemask);
    pGlint->ROP = rop;
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
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

static void
Permedia2SubsequentFillRectSolid24bpp(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(3);
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);

    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive, Render);
}

static void
Permedia2SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int speed = 0;

    if (pGlint->ROP == GXcopy) {
	GLINT_WAIT(3);
  	GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  	speed = FastFillEnable;
    } else {
	GLINT_WAIT(4);
  	GLINT_WRITE_REG((y<<16)|x>>pGlint->BppShift, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|(w+7)>>pGlint->BppShift, RectangleSize);
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

    if (bg == -1) pGlint->FrameBufferReadMode = -1;
	else    pGlint->FrameBufferReadMode = 0;

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
  
    pGlint->ROP = rop;

    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
    }
    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
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

    pGlint->ROP = rop;

    if (rop == GXcopy) {
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    }
    GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
}

static void 
Permedia2SubsequentMono8x8PatternFillRect24bpp(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
  
    GLINT_WAIT(8);
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);

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
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);

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
  
    if (pGlint->ROP == GXcopy) {
	GLINT_WAIT(3);
	if (dir == DEGREES_0) {
            GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
            GLINT_WRITE_REG((1<<16)|len, RectangleSize);
	} else {
            GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
            GLINT_WRITE_REG((len<<16)|1, RectangleSize);
	}
        GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
							FastFillEnable, Render);
    } else {
	GLINT_WAIT(6);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG(y<<16, StartY);

	if (dir == DEGREES_0) {
	    GLINT_WRITE_REG(1<<16, dXDom);
	    GLINT_WRITE_REG(0, dY);
        } else {
	    GLINT_WRITE_REG(0, dXDom);
	    GLINT_WRITE_REG(1<<16, dY);
        }

        GLINT_WRITE_REG(len, GLINTCount);
        GLINT_WRITE_REG(PrimitiveLine, Render);
    }
}

static unsigned int ShiftMasks[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
    0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 
    0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
    0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
    0xFFFFFFFF
};

static void
Permedia2StippleCPUToScreen(ScrnInfoPtr pScrn, int x, int y, int w, int h, 
				unsigned char *src, int srcwidth,
    				int stipplewidth, int stippleheight,
    				int srcx, int srcy)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned char *srcp = (srcwidth * srcy) + src;
    int dwords = (w + 31) >> 5; 
    int count, offset; 

    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
			pGlint->FrameBufferReadMode | SyncOnBitMask, Render);

    if(stipplewidth < 32) {
	register int width;
	register CARD32 pattern;

	while(h--) {
	   width = stipplewidth;
	   pattern = *((CARD32*)srcp) & ShiftMasks[width];  
	   while(!(width & ~15)) {
		pattern |= (pattern << width);
		width <<= 1;	
	   }
	   pattern |= (pattern << width);
 
	   offset = srcx;
	   count = dwords;

	   GLINT_WAIT(dwords);
	   while(count--) {
	   	GLINT_WRITE_REG(
		    (pattern >> offset) | 
		    (pattern << (width - offset)), BitMaskPattern);
		offset += 32;
		while(offset >= width) 
		    offset -= width;
	   }

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
     } else {
	register CARD32* scratch;
	register int scratch2;
	int shift;

	while(h--) {
	   count = dwords;
	   offset = srcx;
	   	
	   GLINT_WAIT(dwords);
	   while(count--) {
	   	shift = stipplewidth - offset;
		scratch = (CARD32*)(srcp + (offset >> 3));
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		    if(scratch2) {
		        GLINT_WRITE_REG((*scratch >> scratch2) |
			     (scratch[1] << (32 - scratch2)), BitMaskPattern);
		    } else {
			GLINT_WRITE_REG(*scratch, BitMaskPattern); 
		    }
		} else {
			GLINT_WRITE_REG((*((CARD32*)srcp) << shift) |
			((*scratch >> scratch2) & ShiftMasks[shift]), 
								BitMaskPattern);
		}

		offset += 32;
		while(offset >= stipplewidth) 
		    offset -= stipplewidth;
	   }	

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
    }
}

static void
Permedia2FillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop, 
				unsigned int planemask, int nBox, BoxPtr pBox,
				int xorg, int yorg, PixmapPtr pPixmap)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int rectWidth, rectHeight;
    int StippleType;

    if (bg == -1) {	/* Transparent Stipples */
	StippleType = 0;
    } else if(rop == GXcopy)  /* OpaqueEasyStipples */
	StippleType = 1;
    else 				/* OpaqueHardStipples */
	StippleType = 2;

    GLINT_WAIT(10);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
    if (rop == GXcopy) {
	pGlint->FrameBufferReadMode = FastFillEnable;
 	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	pGlint->FrameBufferReadMode = 0;
 	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
    }
    rectWidth = pBox->x2 - pBox->x1;
    rectHeight = pBox->y2 - pBox->y1;
    GLINT_WRITE_REG((pBox->y1<<16)|pBox->x1, RectangleOrigin);
    GLINT_WRITE_REG((rectHeight<<16)|rectWidth, RectangleSize);

    if ((rectWidth > 0) && (rectHeight > 0)) {
	switch(StippleType) {
	      case 0:
		REPLICATE(fg);
		if (rop == GXcopy) {
		    GLINT_WRITE_REG(fg, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(fg, ConstantColor);
		}
		Permedia2StippleCPUToScreen(pScrn,
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xorg, yorg);
		break;
	      case 1:
		REPLICATE(bg);
		GLINT_WAIT(4);
		GLINT_WRITE_REG(0, RasterizerMode);
		GLINT_WRITE_REG(bg, FBBlockColor);
		GLINT_WRITE_REG(PrimitiveRectangle|XPositive|YPositive|
							FastFillEnable,Render);
		REPLICATE(fg);
		if (rop == GXcopy) {
		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(fg, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
		    GLINT_WRITE_REG(fg, ConstantColor);
		}
		Permedia2StippleCPUToScreen(pScrn,
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xorg, yorg);
		break;
	      case 2:
		REPLICATE(fg);
		GLINT_WAIT(3);
		if (rop == GXcopy) {
		    GLINT_WRITE_REG(fg, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(fg, ConstantColor);
		}
		Permedia2StippleCPUToScreen(pScrn,
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xorg, yorg);
		REPLICATE(bg);
		if (rop == GXcopy) {
		    GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
		    GLINT_WRITE_REG(bg, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(InvertBitMask|
				BitMaskPackingEachScanline,RasterizerMode);
		    GLINT_WRITE_REG(bg, ConstantColor);
		}
		Permedia2StippleCPUToScreen(pScrn,
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xorg, yorg);
		break;
	}
    }
    GLINT_WRITE_REG(0, RasterizerMode);
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
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned char *srcpntr;
    int dwords, height, mode;
    Bool SecondPass = FALSE;

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    /* >>>>> Set rop, planemask, left edge clip skipleft pixels right
	of x (skipleft is sometimes 0 and clipping isn't needed). <<<<<< */
    if (skipleft)
	Permedia2SetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
 
    GLINT_WAIT(10);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    if (rop == GXcopy) {
	mode = FastFillEnable;
    	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	mode = 0;
	GLINT_WRITE_REG(BitMaskPackingEachScanline,RasterizerMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
    }

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
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |mode,Render);
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
	if (rop == GXcopy) {
    	    GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
    	    GLINT_WRITE_REG(InvertBitMask|BitMaskPackingEachScanline, 
						RasterizerMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	goto SECOND_PASS;
    }

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
}

#if 0
#define MAX_GLYPHS	1024			/* that's gotta be enough */
static unsigned int* Glyphs[MAX_GLYPHS]; 

static void
Permedia2ImageTextTECPUToScreenColorExpand(pDrawable, pGC, xInit, yInit,
					nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    int w, h, x, y;
    int glyphWidth, count, dwords;		       

    glyphWidth = FONTMAXBOUNDS(pGC->font, characterWidth);
    
    /*
     * Check for non-standard glyphs, glyphs that are too wide.
     */
    if ((GLYPHWIDTHBYTESPADDED(*ppci) != 4) || glyphWidth > 32) {
        xf86GCInfoRec.ImageGlyphBltFallBack(
            pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
	return;
    }
 
    h = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
    if (!(h && glyphWidth))
	return;
 
    x = xInit + FONTMAXBOUNDS(pGC->font, leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pGC->font) + pDrawable->y;
    w = nglyph * glyphWidth;
 
    /*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
    gfg =  pGC->fgPixel;
    gbg = pGC->bgPixel;
    REPLICATE (gfg);
    REPLICATE (gbg);
 
    GLINT_WAIT(8);
    DO_PLANEMASK(pGC->planemask);
    GLINT_WRITE_REG((GXcopy<<1)|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(pprod, FBReadMode);
    GLINT_WRITE_REG(gbg, FBBlockColor);
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
			FastFillEnable, Render);
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

    for(count = 0; count < nglyph; count++) 
	Glyphs[count] = (unsigned int*)FONTGLYPHBITS(pglyphBase,*ppci++);	

    dwords = ((w + 31) >> 5) - 1;

    /*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
    GLINT_WAIT(2);
    GLINT_WRITE_REG(gfg, FBBlockColor);
    GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
 			SyncOnBitMask | FastFillEnable, Render);
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

#if 1
    for(count = 0; count < h; count++) {
        GLINT_WAIT(dwords + 1);
        /* 0x0D is the TAG value for BitMaskPattern */
        GLINT_WRITE_REG((dwords << 16) | 0x0D, OutputFIFO);
	xf86DrawTextScanline(
		(unsigned int*)((char*)pGlint->IOBase + OutputFIFO + 4),
 		Glyphs, count, nglyph, glyphWidth);
    }
#else
    for(count = 0; count < h; count++) {
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)pGlint->IOBase + BitMaskPattern),
 		Glyphs, count, nglyph, glyphWidth);
    }
#endif
    SET_SYNC_FLAG;	
}


static void
Permedia2PolyTextTECPUToScreenColorExpand(pDrawable, pGC, xInit, yInit,
 					nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    int w, h, x, y;
    int glyphWidth, count, dwords;		       
 
    glyphWidth = FONTMAXBOUNDS(pGC->font, characterWidth);
     
    /*
     * Check for non-standard glyphs, glyphs that are too wide.
     */
    if ((GLYPHWIDTHBYTESPADDED(*ppci) != 4) || glyphWidth > 32) {
        xf86GCInfoRec.PolyGlyphBltFallBack(
            pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
	return;
    }
 
    h = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
    if (!(h && glyphWidth))
 	return;
 
    x = xInit + FONTMAXBOUNDS(pGC->font, leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pGC->font) + pDrawable->y;
    w = nglyph * glyphWidth;
 
    for(count = 0; count < nglyph; count++) 
 	Glyphs[count] = (unsigned int*)FONTGLYPHBITS(pglyphBase,*ppci++);	
 
    dwords = ((w + 31) >> 5) - 1;

    /*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
    gfg = pGC->fgPixel;
    REPLICATE (gfg);
 
    GLINT_WAIT(9);
    DO_PLANEMASK(pGC->planemask);
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    if (pGC->alu == GXcopy) {
	GLINT_WRITE_REG(0, RasterizerMode);
 	GLINT_WRITE_REG(pprod, FBReadMode);
 	GLINT_WRITE_REG(gfg, FBBlockColor);
 	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
 				SyncOnBitMask | FastFillEnable, Render);
    } else {
 	GLINT_WRITE_REG(pprod | pGlint->BlitMode, FBReadMode);
     	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
 	GLINT_WRITE_REG(gfg, ConstantColor);
   	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
 	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
 						SyncOnBitMask, Render);
    }
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
 
#if 1
     for(count = 0; count < h; count++) {
	GLINT_WAIT(dwords + 1);
	/* 0x0D is the TAG value for BitMaskPattern */
	GLINT_WRITE_REG((dwords << 16) | 0x0D, OutputFIFO);
	xf86DrawTextScanline(
		(unsigned int*)((char*)pGlint->IOBase + OutputFIFO + 4),
 		Glyphs, count, nglyph, glyphWidth);
    }
#else
    for(count = 0; count < h; count++) { 
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)pGlint->IOBase + BitMaskPattern),
 		Glyphs, count, nglyph, glyphWidth);
    }
#endif
    SET_SYNC_FLAG;	
}
#endif

#define MAX_FIFO_ENTRIES 256

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
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
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

	   if (rop == GXcopy) {
	     GLINT_WAIT(6);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF)>>pGlint->BppShift, RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|(w+pGlint->bppalign)>>pGlint->BppShift, RectangleSize);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
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

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
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
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
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

	   GLINT_WAIT(6);
	   if (rop == GXcopy) {
	     GLINT_WAIT(6);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF)>>pGlint->BppShift, RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|(w+pGlint->bppalign)>>pGlint->BppShift, RectangleSize);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
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

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
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
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int skipleft = 0, dwords, count;
    unsigned char* srcpbyte;
    CARD32* srcp;

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (rop == GXcopy) {      
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
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

	   GLINT_WAIT(5);
	   GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	   GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

#if 0
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

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
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
	GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
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

	   GLINT_WAIT(6);
	   GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	   GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
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

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
}

