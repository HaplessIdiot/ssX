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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm_accel.c,v 1.4 1998/08/13 14:45:53 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#include "miline.h"		/* for octants */
#include "xaalocal.h"		/* For replacements */

static void PermediaSync(ScrnInfoPtr pScrn);
static void PermediaSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
static void PermediaSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y,
				int w, int h);
static void PermediaSetupForSolidLine(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
static void PermediaSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
static void PermediaSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, 
				int y1, int x2, int y2, int w, int h);
static void PermediaSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int transparency_color);
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
    				unsigned char *src, int srcwidth, int skipleft,
    				int fg, int bg, int rop,unsigned int planemask);
static void PermediaWritePixmap8bpp(ScrnInfoPtr pScrn, int x, int y, int w,
    				int h, unsigned char *src, int srcwidth,
    				int rop, unsigned int planemask,
    				int transparency_color, int bpp, int depth);
static void PermediaWritePixmap16bpp(ScrnInfoPtr pScrn, int x, int y, int w,
    				int h, unsigned char *src, int srcwidth,
    				int rop, unsigned int planemask,
    				int transparency_color, int bpp, int depth);
static void PermediaWritePixmap32bpp(ScrnInfoPtr pScrn, int x, int y, int w,
    				int h, unsigned char *src, int srcwidth,
    				int rop, unsigned int planemask,
    				int transparency_color, int bpp, int depth);
static void PermediaSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny,
				       int fg, int bg, int rop,
				       unsigned planemask);
static void PermediaSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn,int patternx, int patterny, int x, int y,
				   int w, int h);
static void PermediaLoadCoord(ScrnInfoPtr pScrn, int x, int y, int w, int h,
				int a, int d);

#define MAX_FIFO_ENTRIES 1023

static void
PermediaInitializeEngine(ScrnInfoPtr pScrn)
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
    pGlint->dy = 1<<16;
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

    infoPtr->Flags = PIXMAP_CACHE /*| MICROSOFT_ZERO_LINE_BIAS*/;
 
    infoPtr->Sync = PermediaSync;

    infoPtr->SetupForSolidFill = PermediaSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = PermediaSubsequentFillRectSolid;
#if 0
    infoPtr->SetupForSolidLine = PermediaSetupForSolidLine;
    infoPtr->SubsequentSolidBresenhamLine = PermediaSubsequentSolidBresenhamLine;
#endif
  
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY |
				       ONLY_LEFT_TO_RIGHT_BITBLT; 

    infoPtr->SetupForScreenToScreenCopy = PermediaSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = PermediaSubsequentScreenToScreenCopy;

#if 0
    infoPtr->SetClippingRectangle = PermediaSetClippingRectangle;
#endif
  
    infoPtr->Mono8x8PatternFillFlags = 
    				HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    				HARDWARE_PATTERN_PROGRAMMED_BITS |
    				HARDWARE_PATTERN_SCREEN_ORIGIN;

    infoPtr->SetupForMono8x8PatternFill =
				PermediaSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				PermediaSubsequentMono8x8PatternFillRect;

    /* We need to break up scanlines to use these without PCI retries */
    if (pGlint->UsePCIRetry) {
        infoPtr->CPUToScreenColorExpandFillFlags = /* TRANSPARENCY_ONLY | */
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

	if (pScrn->bitsPerPixel == 8)
        infoPtr->WritePixmap = PermediaWritePixmap8bpp;
	else
	if (pScrn->bitsPerPixel == 16)
        infoPtr->WritePixmap = PermediaWritePixmap16bpp;
	else
	if (pScrn->bitsPerPixel == 32)
        infoPtr->WritePixmap = PermediaWritePixmap32bpp;
	infoPtr->WriteBitmap = PermediaWriteBitmap;
#if 0
        infoPtr->TEGlyphRenderer = PermediaTEGlyphRenderer;
        infoPtr->NonTEGlyphRenderer = PermediaNonTEGlyphRenderer;
        infoPtr->FillColorExpandRects = PermediaFillColorExpandRects;
        infoPtr->FillColorExpandSpans = PermediaFillColorExpandSpans;
#endif
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pGlint->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);
  
    if (AvailFBArea.y2 > 1024) AvailFBArea.y2 = 1024;

    xf86InitFBManager(pScreen, &AvailFBArea);

    return (XAAInit(pScreen, infoPtr));
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

static void PermediaLoadCoord(
	ScrnInfoPtr pScrn,
	int x, int y,
	int w, int h,
	int a, int d
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    if (w != pGlint->startxsub) {
    	GLINT_WRITE_REG(w, StartXSub);
	pGlint->startxsub = w;
    }
    if (x != pGlint->startxdom) {
    	GLINT_WRITE_REG(x,StartXDom);
	pGlint->startxdom = x;
    }
    if (y != pGlint->starty) {
    	GLINT_WRITE_REG(y,StartY);
	pGlint->starty = y;
    }
    if (h != pGlint->count) {
    	GLINT_WRITE_REG(h,GLINTCount);
	pGlint->count = h;
    }
    if (a != pGlint->dxdom) {
    	GLINT_WRITE_REG(a,dXDom);
	pGlint->dxdom = a;
    }
    if (d != pGlint->dy) {
    	GLINT_WRITE_REG(d,dY);
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
PermediaSetupForSolidLine(
	ScrnInfoPtr pScrn, 
	int color, int rop, 
	unsigned int planemask
){
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;
    GLINT_WAIT(6);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(color, GLINTColor);
    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    if (rop == GXcopy) {
  	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
  	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }
    LOADROP(rop);
}

static void 
PermediaSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
   int dy, dxdom, startxdom, starty;
   double min_off = 0.0;

#if 0
   dmaj >>= 1;  
   dmin >>= 1;
#endif
   x <<= 16;
   y <<= 16;

   /* translate the error term into a minor axis offset  
	in units of 1/(2^16) of a pixel. */
   if(dmaj != -e)
      min_off = (((double)e/(double)dmaj) + 1.0) * 0.5 * 65536.0;

   if(octant & YMAJOR) {
	if(octant & YDECREASING)
	  dy = -1 << 16;  
	else
	  dy = 1 << 16;
        
        if(octant & XDECREASING)
            dxdom = (-dmin << 16)/dmaj;
	else
            dxdom = (dmin << 16)/dmaj;
	/* We have to shift up one, as bit 0 isn't used for precision */
	startxdom = x | (((int)min_off<<1) & 0xFFFE); 	
	starty = y;
   } else {
        if(octant & XDECREASING)
          dxdom = -1 << 16;  
        else
          dxdom = 1 << 16;

	if(octant & YDECREASING)
            dy = (-dmin << 16)/dmaj;
	else
            dy = (dmin << 16)/dmaj;
        startxdom = x;
	/* We have to shift up one, as bit 0 isn't used for precision */
	starty = y | (((int)min_off<<1) & 0xFFFE);
   }

   PermediaLoadCoord(pScrn, startxdom, starty, pGlint->startxsub, len, dxdom, dy);
   GLINT_WRITE_REG(PrimitiveLine, Render);
}

static void
PermediaSetupForScreenToScreenCopy(
	ScrnInfoPtr pScrn, 
	int xdir, int ydir, int rop,
	unsigned int planemask, int transparency_color
){
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CHECKCLIPPING;

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

static void
PermediaSubsequentScreenToScreenCopy(
	ScrnInfoPtr pScrn, 
	int x1, int y1, 
	int x2, int y2,
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
	direction = -1<<16;
    } else {
	direction = 1<<16;
    }

    /* We can only use GXcopy for Packed modes, and less than 32 width
     * gives us speed for small blits. */
    if ((w < 32) || (pGlint->ROP != GXcopy)) {
	PermediaLoadCoord(pScrn, x2<<16, y2<<16, (x2+w)<<16, h, 0, direction);
  	srcaddr = x1;
  	dstaddr = x2;
  	GLINT_WAIT(7);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode, FBReadMode);
  } else {
	PermediaLoadCoord(pScrn, (x2>>pGlint->BppShift)<<16, y2<<16, 
				((x2+w+7)>>pGlint->BppShift)<<16, h, 0, 
				direction);
  	srcaddr = (x1 & ~pGlint->bppalign);
  	dstaddr = (x2 & ~pGlint->bppalign);
  	align = (x2 & pGlint->bppalign) - (x1 & pGlint->bppalign);
  	GLINT_WAIT(8);
	GLINT_WRITE_REG(pGlint->FrameBufferReadMode | FBRM_Packed | 
						(align&7)<<20, FBReadMode);
  	GLINT_WRITE_REG(x2<<16|(x2+w), PackedDataLimits);
  }
  srcaddr += y1 * pScrn->displayWidth;
  dstaddr += y2 * pScrn->displayWidth;
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
    CHECKCLIPPING;

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

static void
PermediaSubsequentFillRectSolid(
	ScrnInfoPtr pScrn, 
	int x, int y, 
	int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int speed = 0;
    if (pGlint->ROP == GXcopy) {
	GLINT_WAIT(5);
	PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
  	speed = FastFillEnable;
    } else {
	GLINT_WAIT(7);
      	GLINT_WRITE_REG(pGlint->pprod | FBRM_Packed | FBRM_DstEnable, FBReadMode);
	PermediaLoadCoord(pScrn, (x>>pGlint->BppShift)<<16, y<<16, 
				((x+w+7)>>pGlint->BppShift)<<16, h, 0, 1<<16);
  	GLINT_WRITE_REG(x<<16|(x+w), PackedDataLimits);
    }
    GLINT_WRITE_REG(PrimitiveTrapezoid | speed, Render);
}

static void
PermediaSetupForMono8x8PatternFill(
	ScrnInfoPtr pScrn, 
	int patternx, int patterny,
	int fg, int bg, int rop,
	unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
  pGlint->ForeGroundColor = fg;
  pGlint->BackGroundColor = bg;
  REPLICATE(pGlint->ForeGroundColor);
  REPLICATE(pGlint->BackGroundColor);

  CHECKCLIPPING;
  GLINT_WAIT(15);
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
	GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
  } else {
	GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
  }
  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  GLINT_WRITE_REG(pGlint->ForeGroundColor, ConstantColor);
  GLINT_WRITE_REG(pGlint->BackGroundColor, Texel0);
  LOADROP(rop);
}

static void 
PermediaSubsequentMono8x8PatternFillRect(
	ScrnInfoPtr pScrn,
	int patternx, int patterny, 
	int x, int y,
	int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(6);
    PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
    if (pGlint->BackGroundColor != -1) {
   	GLINT_WRITE_REG(1<<20|patternx<<7|patterny<<12|UNIT_ENABLE, 
							AreaStippleMode);
  	GLINT_WRITE_REG(AreaStippleEnable | TextureEnable | PrimitiveTrapezoid, 
							Render);
    } else { 
  	GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  	GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid, Render);
    }
}

static void 
PermediaWriteBitmap(ScrnInfoPtr pScrn, 
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
    int dwords, height;
    register int count; 
    register CARD32* pattern;
    Bool SecondPass = FALSE;

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
    PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);

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
    srcpntr = src;
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
    int dobackground = 0;

    if (bg != -1) dobackground |= ForceBackgroundColor;

    GLINT_WAIT(6);
    DO_PLANEMASK(planemask);
    if (rop == GXcopy) {
        GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    } else {
        GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
    }

    pGlint->ForeGroundColor = fg;
    pGlint->BackGroundColor = bg;
    REPLICATE(fg);
    REPLICATE(bg);

    if (rop == GXcopy) {
        GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	if (dobackground) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	}
	GLINT_WRITE_REG(0, RasterizerMode);
        pGlint->FrameBufferReadMode = FastFillEnable;
    } else {
        GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(BitMaskPackingEachScanline|dobackground,RasterizerMode);
        GLINT_WRITE_REG(fg, ConstantColor);
	if (dobackground) {
	    GLINT_WRITE_REG(bg, Texel0);
	    pGlint->FrameBufferReadMode = TextureEnable;
	} else
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

    if (skipleft)
	PermediaSetClippingRectangle(pScrn,x+skipleft, y, x+w, y+h);
    else
	CHECKCLIPPING;

    if ((pGlint->ROP == GXcopy) && (pGlint->BackGroundColor != -1)) {
        PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
        GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable, Render);
	REPLICATE(pGlint->ForeGroundColor);
	GLINT_WRITE_REG(pGlint->ForeGroundColor, FBBlockColor);
    }
	
    GLINT_WAIT(6);
    PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
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

static void
PermediaWritePixmap8bpp(
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
	  PermediaSync(pScrn);	/* we are not using the rasterizer */
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
    	   if (skipleft)
		PermediaSetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
	   else
		CHECKCLIPPING;

	   if (rop == GXcopy) {
	     GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	   } else {
	     GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
	   }
           PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveTrapezoid | SyncOnHostData, Render);

	   {
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
PermediaWritePixmap16bpp(
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
	  PermediaSync(pScrn);	/* we are not using the rasterizer */
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
    	   if (skipleft)
		PermediaSetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    	   else
		CHECKCLIPPING;

	   if (rop == GXcopy) {
	     GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
	   } else {
	     GLINT_WRITE_REG(pGlint->pprod | FBRM_DstEnable, FBReadMode);
	   }
           PermediaLoadCoord(pScrn, x<<16, y<<16, (x+w)<<16, h, 0, 1<<16);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   LOADROP(rop);
  	   GLINT_WRITE_REG(PrimitiveTrapezoid | SyncOnHostData, Render);

	   {
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
PermediaWritePixmap32bpp(
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
	  PermediaSync(pScrn);	/* we are not using the rasterizer */
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
		PermediaSetClippingRectangle(pScrn,x+skipleft,y,x+w,y+h);
    	   else
		CHECKCLIPPING;

	   GLINT_WAIT(6);
           PermediaLoadCoord(pScrn, (x&0xFFFF)<<16, y<<16, ((x&0xFFFF)+w)<<16, h, 0, 1<<16);
	   LOADROP(rop);
	   if (rop == GXcopy) {
		GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   } else {
		GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	   }
  	   GLINT_WRITE_REG(PrimitiveTrapezoid | SyncOnHostData, Render);

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
