/*
 * Copyright 2000 by Sven Luther <luther@dpt-info.u-strasbg.fr>.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Sven Luther not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Sven Luther makes no representations
 * about the suitability of this software for any purpose. It is provided
 * "as is" without express or implied warranty.
 *
 * SVEN LUTHER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SVEN LUTHER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Sven Luther, <luther@dpt-info.u-strasbg.fr>
 *           Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * this work is sponsored by Appian Graphics.
 * 
 * Permedia 3 accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm3_accel.c,v 1.3 2000/09/11 16:58:56 alanh Exp $ */

#include "Xarch.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "fb.h"

#include "glint_regs.h"
#include "pm3_regs.h"
#include "glint.h"

#include "xaalocal.h"		/* For replacements */

#define DEBUG 0

#if DEBUG
# define TRACE_ENTER(str)       ErrorF("pm3_accel: " str " %d\n",pScrn->scrnIndex)
# define TRACE_EXIT(str)        ErrorF("pm3_accel: " str " done\n")
# define TRACE(str)             ErrorF("pm3_accel trace: " str "\n")
#else
# define TRACE_ENTER(str)
# define TRACE_EXIT(str)
# define TRACE(str)
#endif

/* Sync */
void Permedia3Sync(ScrnInfoPtr pScrn);
/* Clipping */
static void Permedia3SetClippingRectangle(ScrnInfoPtr pScrn, int x, int y,
				int w, int h);
static void Permedia3DisableClipping(ScrnInfoPtr pScrn);
/* ScreenToScreenCopy */
static void Permedia3SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);
static void Permedia3SetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
/* SolidFill */
static void Permedia3SetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Permedia3SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
/* 8x8 Mono Pattern Fills */
static void Permedia3SetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg,
				int rop, unsigned int planemask);
static void Permedia3SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int x_offset, int y_offset, int x, int y, 
				int w, int h);
/* Color Expansion Fills */
static void Permedia3SetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
				int fg, int bg, int rop,unsigned int planemask);
static void Permedia3SubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, 
				int x, int y, int w, int h, int skipleft);
/* Images Writes */
static void Permedia3SetupForImageWrite(ScrnInfoPtr pScrn, int rop,
				unsigned int planemask, int trans_color,
				int bpp, int depth);
static void Permedia3SubsequentImageWriteRect(ScrnInfoPtr pScrn, 
				int x, int y, int w, int h, int skipleft);
/* SolidLines */
/*
static void Permedia3PolylinesThinSolidWrapper(DrawablePtr pDraw, GCPtr pGC,
   				int mode, int npt, DDXPointPtr pPts);
static void Permedia3PolySegmentThinSolidWrapper(DrawablePtr pDraw, GCPtr pGC,
 				int nseg, xSegment *pSeg);
static void Permedia3SetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void Permedia3SubsequentSolidHorVertLine(ScrnInfoPtr pScrn,
				int x, int y, int len, int dir);
static void Permedia3SubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
*/
/* DashedLines */
/*
static void Permedia3PolylinesThinDashedWrapper(DrawablePtr pDraw, GCPtr pGC,
   				int mode, int npt, DDXPointPtr pPts);
static void Permedia3PolySegmentThinDashedWrapper(DrawablePtr pDraw, GCPtr pGC,
 				int nseg, xSegment *pSeg);
static void Permedia3SetupForDashedLine(ScrnInfoPtr pScrn, int fg, int bg,
				int rop, unsigned int planemask,
				int length, unsigned char *pattern);
static void Permedia3SubsequentDashedBresenhamLine(ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant, int phase);
*/

#define MAX_FIFO_ENTRIES 256

/* Mirror stipple pattern horizontally */
#if X_BYTE_ORDER == X_BIG_ENDIAN
# define STIPPLE_SWAP	1<<18
#else
# define STIPPLE_SWAP	0
#endif


void
Permedia3InitializeEngine(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int colorformat = 0;

    /* Initialize the Accelerator Engine to defaults */
    TRACE_ENTER("Permedia3InitializeEngine");

    /* Host out PreInit */
    /* Set filter mode to enable sync tag & data output */
    GLINT_SLOW_WRITE_REG(0xc00,		FilterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, StatisticMode);
    Permedia3Sync(pScrn);

    TRACE("Permedia3InitializeEngine : first sync");
    /* Disable most units by default */
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3DeltaMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, RasterizerMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, ScissorMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, LineStippleMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, AreaStippleMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3GIDMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, DepthMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, StencilMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3TextureCoordMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3TextureIndexMode0);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3TextureIndexMode1);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, TextureReadMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3LUTMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, TextureFilterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3TextureCompositeMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3TextureApplicationMode);
    GLINT_SLOW_WRITE_REG(0, PM3TextureCompositeColorMode1);
    GLINT_SLOW_WRITE_REG(0, PM3TextureCompositeAlphaMode1);
    GLINT_SLOW_WRITE_REG(0, PM3TextureCompositeColorMode0);
    GLINT_SLOW_WRITE_REG(0, PM3TextureCompositeAlphaMode0);

    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, FogMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, ChromaTestMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, AlphaTestMode);
    /* Not done in P3Lib ??? */
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, AntialiasMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3AlphaTestMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, YUVMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3AlphaBlendColorMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3AlphaBlendAlphaMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, DitherMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, LogicalOpMode);
    
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, StatisticMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, RouterMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, PM3Window);

    GLINT_SLOW_WRITE_REG(0, PM3Config2D);
    
    GLINT_SLOW_WRITE_REG(0xffffffff, PM3SpanColorMask);

    GLINT_SLOW_WRITE_REG(0, PM3XBias);
    GLINT_SLOW_WRITE_REG(0, PM3YBias);

    GLINT_SLOW_WRITE_REG(0, PM3DeltaControl);

    GLINT_SLOW_WRITE_REG(0xffffffff, BitMaskPattern);
    Permedia3Sync(pScrn);

    /* ScissorStippleUnit Initialization (is it needed ?) */
    pGlint->ClippingOn = FALSE;
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, ScissorMode);
    /* We never use Screen Scissor ...
    GLINT_SLOW_WRITE_REG(
	(pScrn->virtualX&0xffff)|((pScrn->virtualY&0xffff)<<16),
	ScreenSize);
    GLINT_SLOW_WRITE_REG(
	(0&0xffff)|((0&0xffff)<<16),
	WindowOrigin);
    */
    Permedia3Sync(pScrn);

    /* StencilDepthUnit Initialization */
    GLINT_SLOW_WRITE_REG(0, PM3Window);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, DepthMode);
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE, StencilMode);
    GLINT_SLOW_WRITE_REG(0, StencilData);
    Permedia3Sync(pScrn);

    /* FBReadUnit Initialization */
    TRACE("Permedia3InitializeEngine : only syncs upto now");
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadEnables_E(0xff) |
	PM3FBDestReadEnables_R(0xff) |
	PM3FBDestReadEnables_ReferenceAlpha(0xff),
	PM3FBDestReadEnables);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferAddr0);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferOffset0);
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadBufferWidth_Width(pScrn->displayWidth),
	PM3FBDestReadBufferWidth0);
    /*
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferAddr1);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferOffset1);
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadBufferWidth_Width(pScrn->displayWidth),
	PM3FBDestReadBufferWidth1);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferAddr2);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferOffset2);
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadBufferWidth_Width(pScrn->displayWidth),
	PM3FBDestReadBufferWidth2);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferAddr3);
    GLINT_SLOW_WRITE_REG(0, PM3FBDestReadBufferOffset3);
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadBufferWidth_Width(pScrn->displayWidth),
	PM3FBDestReadBufferWidth3);
    */
    GLINT_SLOW_WRITE_REG(
	PM3FBDestReadMode_ReadEnable |
	/* Not needed, since FBDestRead is the same as FBWrite.
	PM3FBDestReadMode_Blocking |
	*/ 
	PM3FBDestReadMode_Enable0,
	PM3FBDestReadMode);
    TRACE("Permedia3InitializeEngine : DestRead");
    GLINT_SLOW_WRITE_REG(0, PM3FBSourceReadBufferAddr);
    GLINT_SLOW_WRITE_REG(0, PM3FBSourceReadBufferOffset);
    GLINT_SLOW_WRITE_REG(
	PM3FBSourceReadBufferWidth_Width(pScrn->displayWidth),
	PM3FBSourceReadBufferWidth);
    GLINT_SLOW_WRITE_REG(
	PM3FBSourceReadMode_Blocking |
	PM3FBSourceReadMode_ReadEnable,
	PM3FBSourceReadMode);
    TRACE("Permedia3InitializeEngine : SourceRead");
#if X_BYTE_ORDER == X_BIG_ENDIAN
    pGlint->RasterizerSwap = 1;
#else
    pGlint->RasterizerSwap = 0;
#endif
    switch (pScrn->bitsPerPixel) {
	case 8:
	    GLINT_SLOW_WRITE_REG(0x2, PixelSize);
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    pGlint->RasterizerSwap |= 3<<15;	/* Swap host data */
#endif
	    break;
	case 16:
	    GLINT_SLOW_WRITE_REG(0x1, PixelSize);
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    pGlint->RasterizerSwap |= 2<<15;	/* Swap host data */
#endif
	    break;
	case 32:
	    GLINT_SLOW_WRITE_REG(0x0, PixelSize);
	    break;
    }
    TRACE("Permedia3InitializeEngine : PixelSize");
    Permedia3Sync(pScrn);

    /* LogicalOpUnit Initialization */
    GLINT_SLOW_WRITE_REG(0xffffffff,	FBSoftwareWriteMask);
    Permedia3Sync(pScrn);

    /* FBWriteUnit Initialization */
    GLINT_SLOW_WRITE_REG(
	PM3FBWriteMode_WriteEnable|
	PM3FBWriteMode_OpaqueSpan|
	PM3FBWriteMode_Enable0,
	PM3FBWriteMode);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferAddr0);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferOffset0);
    GLINT_SLOW_WRITE_REG(
	PM3FBWriteBufferWidth_Width(pScrn->displayWidth),
	PM3FBWriteBufferWidth0);
    Permedia3Sync(pScrn);
    /*
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferAddr1);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferOffset1);
    GLINT_SLOW_WRITE_REG(
	PM3FBWriteBufferWidth_Width(pScrn->displayWidth),
	PM3FBWriteBufferWidth1);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferAddr2);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferOffset2);
    GLINT_SLOW_WRITE_REG(
	PM3FBWriteBufferWidth_Width(pScrn->displayWidth),
	PM3FBWriteBufferWidth2);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferAddr3);
    GLINT_SLOW_WRITE_REG(0, PM3FBWriteBufferOffset3);
    GLINT_SLOW_WRITE_REG(
	PM3FBWriteBufferWidth_Width(pScrn->displayWidth),
	PM3FBWriteBufferWidth3);
    */
    TRACE("Permedia3InitializeEngine : FBWrite");
    /* SizeOfframebuffer */
    GLINT_SLOW_WRITE_REG(
	pScrn->displayWidth *
	(8 * pGlint->FbMapSize / (pScrn->bitsPerPixel * pScrn->displayWidth)
	    >4095?4095: 8 * pGlint->FbMapSize /
	    (pScrn->bitsPerPixel * pScrn->displayWidth)),
	PM3SizeOfFramebuffer);
    GLINT_SLOW_WRITE_REG(0xffffffff,	FBHardwareWriteMask);
    Permedia3Sync(pScrn);
    TRACE("Permedia3InitializeEngine : FBHardwareWriteMask & SizeOfFramebuffer");
    /* Color Format */
    switch (pScrn->depth) {
	case 8:
	    colorformat = 4;
	    break;
	case 15:
	    colorformat = 2;
	    break;
	case 16:
	    colorformat = 3;
	    break;
	case 24:
	case 32:
	    colorformat = 0;
	    break;
    }
    GLINT_SLOW_WRITE_REG(UNIT_DISABLE|
	((colorformat&0xf)<<2)|(1<<10),
	DitherMode);

    /* Other stuff */
    pGlint->startxdom = 0;
    pGlint->startxsub = 0;
    pGlint->starty = 0;
    pGlint->count = 0;
    pGlint->dy = 1<<16;
    pGlint->dxdom = 0;
    pGlint->x = 0;
    pGlint->y = 0;
    pGlint->h = 0;
    pGlint->w = 0;
    pGlint->ROP = 0xFF;
    GLINT_SLOW_WRITE_REG(0, dXDom);
    GLINT_SLOW_WRITE_REG(0, dXSub);
    GLINT_SLOW_WRITE_REG(1<<16, dY);
    GLINT_SLOW_WRITE_REG(0, StartXDom);
    GLINT_SLOW_WRITE_REG(0, StartXSub);
    GLINT_SLOW_WRITE_REG(0, StartY);
    GLINT_SLOW_WRITE_REG(0, GLINTCount);
    TRACE_EXIT("Permedia3InitializeEngine");
}

Bool
Permedia3AccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    BoxRec AvailFBArea;

    pGlint->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    Permedia3InitializeEngine(pScrn);

    /* Generic accel engine flags */
    infoPtr->Flags =
	PIXMAP_CACHE |
	OFFSCREEN_PIXMAPS |
	LINEAR_FRAMEBUFFER;

    /* Synchronization of the accel engine */
    infoPtr->Sync = Permedia3Sync;

    /* Clipping Setup */
    infoPtr->ClippingFlags =
	HARDWARE_CLIP_MONO_8x8_FILL |
	HARDWARE_CLIP_SOLID_FILL;
    infoPtr->SetClippingRectangle = Permedia3SetClippingRectangle;
    infoPtr->DisableClipping = Permedia3DisableClipping;

    /* ScreenToScreenCopy */
    infoPtr->ScreenToScreenCopyFlags =
	NO_TRANSPARENCY; 
    infoPtr->SetupForScreenToScreenCopy =
	Permedia3SetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy =
	Permedia3SubsequentScreenToScreenCopy;

    /* SolidFill */
    infoPtr->SolidFillFlags = 0;
    infoPtr->SetupForSolidFill = Permedia3SetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = Permedia3SubsequentFillRectSolid;

    /* 8x8 Mono Pattern Fills */
    infoPtr->Mono8x8PatternFillFlags = 
    	HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    	HARDWARE_PATTERN_PROGRAMMED_BITS |
    	HARDWARE_PATTERN_SCREEN_ORIGIN |
	BIT_ORDER_IN_BYTE_LSBFIRST;
    infoPtr->SetupForMono8x8PatternFill =
	Permedia3SetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
	Permedia3SubsequentMono8x8PatternFillRect;

    /* Color Expand Fills */
    infoPtr->CPUToScreenColorExpandFillFlags =
	SYNC_AFTER_COLOR_EXPAND |
	LEFT_EDGE_CLIPPING |
	BIT_ORDER_IN_BYTE_LSBFIRST |
	CPU_TRANSFER_BASE_FIXED |
	CPU_TRANSFER_PAD_DWORD;
    infoPtr->ColorExpandBase = pGlint->IOBase + BitMaskPattern;
    infoPtr->ColorExpandRange = 4;
    infoPtr->SetupForCPUToScreenColorExpandFill =
	    Permedia3SetupForCPUToScreenColorExpandFill;
    infoPtr->SubsequentCPUToScreenColorExpandFill = 
	    Permedia3SubsequentCPUToScreenColorExpandFill;

    /* Images Writes */
    infoPtr->ImageWriteFlags = 
	NO_GXCOPY |
	SYNC_AFTER_IMAGE_WRITE |
	LEFT_EDGE_CLIPPING |
	BIT_ORDER_IN_BYTE_LSBFIRST |
	CPU_TRANSFER_BASE_FIXED |
	CPU_TRANSFER_PAD_DWORD;
    infoPtr->ImageWriteBase = pGlint->IOBase + PM3FBSourceData;
    infoPtr->ImageWriteRange = 8;
    infoPtr->SetupForImageWrite =
	    Permedia3SetupForImageWrite;
    infoPtr->SubsequentImageWriteRect =
	    Permedia3SubsequentImageWriteRect;
    
    /* SolidLines */
    /*
    infoPtr->SolidLineFlags = 0;
    infoPtr->PolySegmentThinSolidFlags = 0;
    infoPtr->PolylinesThinSolidFlags = 0;
    infoPtr->PolySegmentThinSolid = Permedia3PolySegmentThinSolidWrapper;
    infoPtr->PolylinesThinSolid = Permedia3PolylinesThinSolidWrapper;
    infoPtr->SetupForSolidLine = Permedia3SetupForSolidLine;
    infoPtr->SubsequentSolidHorVertLine = Permedia3SubsequentSolidHorVertLine;
    infoPtr->SubsequentSolidBresenhamLine =
	Permedia3SubsequentSolidBresenhamLine;
    */

    /* DashedLines */
#if 0
    infoPtr->DashedLineFlags =
	/* TRANSPARENCY_ONLY could be dropped if we manage 
	 * to do the background color also ... :((( */
	TRANSPARENCY_ONLY |
	LINE_PATTERN_POWER_OF_2_ONLY |
	LINE_PATTERN_LSBFIRST_LSBJUSTIFIED;
    infoPtr-> DashPatternMaxLength = 16;
    infoPtr->PolySegmentThinDashedFlags = 0;
    infoPtr->PolylinesThinDashedFlags = 0;
    /* This needs to be set so as to use the Dashed lines instead
     * of full ones. */
    infoPtr->PolySegmentThinDashed = Permedia3PolySegmentThinDashedWrapper;
    infoPtr->PolylinesThinDashed = Permedia3PolylinesThinDashedWrapper;
    infoPtr->SetupForDashedLine = Permedia3SetupForDashedLine;
    infoPtr->SubsequentDashedBresenhamLine =
	Permedia3SubsequentDashedBresenhamLine;
#endif

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pGlint->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);
    /* Alan does this ???
    AvailFBArea.y2 = ((pGlint->FbMapSize > 16384*1024) ? 16384*1024 :
	pGlint->FbMapSize)  / (pScrn->displayWidth 
	pScrn->bitsPerPixel / 8);
    */

    /* Permedia3 has a maximum 4096x4096 framebuffer */
    if (AvailFBArea.y2 > 4095) AvailFBArea.y2 = 4095;

    xf86InitFBManager(pScreen, &AvailFBArea);

    Permedia3Sync(pScrn);
    return(XAAInit(pScreen, infoPtr));
}

#define CHECKCLIPPING				\
{						\
    if (pGlint->ClippingOn) {			\
	pGlint->ClippingOn = FALSE;		\
	GLINT_WAIT(1);				\
	GLINT_WRITE_REG(0, ScissorMode);	\
    }						\
}

void
Permedia3Sync(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    CHECKCLIPPING;

    while (GLINT_READ_REG(DMACount) != 0);
    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, GlintSync);
    do {
   	while(GLINT_READ_REG(OutFIFOWords) == 0);
    } while (GLINT_READ_REG(OutputFIFO) != PM3SyncTag);
}

static void
Permedia3SetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINT_WAIT(3);
    GLINT_WRITE_REG(((y1&0x0fff)<<16)|(x1&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG(((y2&0x0fff)<<16)|(x2&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG(1, ScissorMode);
    pGlint->ClippingOn = TRUE;
}

static void
Permedia3DisableClipping(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CHECKCLIPPING;
}

/* ScreenToScreenCopy definition */
static void
Permedia3SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForScreenToScreenCopy");
    pGlint->PM3_Render2D =
	PM3Render2D_SpanOperation |
	PM3Render2D_Operation_Normal;
    pGlint->ClippingOn = TRUE;
    pGlint->PM3_Config2D =
	PM3Config2D_UserScissorEnable |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    if (xdir == 1) pGlint->PM3_Render2D |= PM3Render2D_XPositive;
    if (ydir == 1) pGlint->PM3_Render2D |= PM3Render2D_YPositive;
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXnoop)&&(rop!=GXinvert)) {
	pGlint->PM3_Render2D |= PM3Render2D_FBSourceReadEnable;
	pGlint->PM3_Config2D |= PM3Config2D_Blocking;
    }
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
    GLINT_WAIT(2);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    TRACE_EXIT("Permedia3SetupForScreenToScreenCopy");
}

static void
Permedia3SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* Spans needs to be 32 bit aligned. */
    int x_align = x1 & 0x1f;
    TRACE_ENTER("Permedia3SubsequentScreenToScreenCopy");
    GLINT_WAIT(5);
    GLINT_WRITE_REG(((y2&0x0fff)<<16)|(x2&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG((((y2+h)&0x0fff)<<16)|((x2+w)&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG(
	PM3RectanglePosition_XOffset(x2-x_align) |
	PM3RectanglePosition_YOffset(y2),
	PM3RectanglePosition);
    GLINT_WRITE_REG(
	PM3FBSourceReadBufferOffset_XOffset(x1-x2)|
	PM3FBSourceReadBufferOffset_YOffset(y1-y2),
	PM3FBSourceReadBufferOffset);
    GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(w+x_align)|
	PM3Render2D_Height(h),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentScreenToScreenCopy");
}

/* Solid Fills */
static void
Permedia3SetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForFillRectSolid");
    REPLICATE(color);
    /* Prepapre Common Render2D & Config2D data */
    pGlint->PM3_Render2D =
	PM3Render2D_SpanOperation |
	PM3Render2D_XPositive |
	PM3Render2D_YPositive |
	PM3Render2D_Operation_Normal;
    pGlint->PM3_Config2D =
	PM3Config2D_UseConstantSource |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
    GLINT_WAIT(3);
    /* Using FBClockColor (have to disable SpanOperation) will fill only the
     * first 32 pixels of the 64 pixels of a span. Lets use ForegroundColor 
     * instead (from the LogicOps unit)
    GLINT_WRITE_REG(color, PM3FBBlockColor);
     */
    GLINT_WRITE_REG(color, PM3ForegroundColor);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    TRACE_EXIT("Permedia3SetupForFillRectSolid");
}

static void
Permedia3SubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentFillRectSolid");
    GLINT_WAIT(2);
    GLINT_WRITE_REG(
	PM3RectanglePosition_XOffset(x) |
	PM3RectanglePosition_YOffset(y),
	PM3RectanglePosition);
    GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(w) | PM3Render2D_Height(h),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentFillRectSolid");
}
/* 8x8 Mono Pattern Fills */
static void 
Permedia3SetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
			   int patternx, int patterny, 
			   int fg, int bg, int rop,
			   unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForMono8x8PatternFill");
    REPLICATE(fg);
    pGlint->PM3_Render2D =
	PM3Render2D_AreaStippleEnable |
	PM3Render2D_SpanOperation |
	PM3Render2D_XPositive |
	PM3Render2D_YPositive |
	PM3Render2D_Operation_Normal;
    pGlint->PM3_Config2D =
	PM3Config2D_UseConstantSource |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_BackgroundROPEnable |
	PM3Config2D_BackgroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
    pGlint->PM3_AreaStippleMode = 1;
    pGlint->PM3_AreaStippleMode |= (2<<1);
    pGlint->PM3_AreaStippleMode |= (2<<4);
    if (bg != -1) {
	REPLICATE(bg);
	pGlint->PM3_Config2D |= PM3Config2D_OpaqueSpan;
	pGlint->PM3_AreaStippleMode |= 1<<20;
	GLINT_WAIT(12);
    	GLINT_WRITE_REG(bg, BackgroundColor);
    }
    else GLINT_WAIT(11);
    GLINT_WRITE_REG((patternx & 0xFF), AreaStipplePattern0);
    GLINT_WRITE_REG((patternx & 0xFF00) >> 8, AreaStipplePattern1);
    GLINT_WRITE_REG((patternx & 0xFF0000) >> 16, AreaStipplePattern2);
    GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
    GLINT_WRITE_REG((patterny & 0xFF), AreaStipplePattern4);
    GLINT_WRITE_REG((patterny & 0xFF00) >> 8, AreaStipplePattern5);
    GLINT_WRITE_REG((patterny & 0xFF0000) >> 16, AreaStipplePattern6);
    GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
    GLINT_WRITE_REG(fg, PM3ForegroundColor);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    TRACE_EXIT("Permedia3SetupForMono8x8PatternFill");
}

static void 
Permedia3SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
			   int x_offset, int y_offset,
			   int x, int y, int w, int h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentMono8x8PatternFillRect");
    GLINT_WAIT(3);
    GLINT_WRITE_REG(
	PM3RectanglePosition_XOffset(x) |
	PM3RectanglePosition_YOffset(y),
	PM3RectanglePosition);
    GLINT_WRITE_REG(
	(x_offset&0x7)<<7 | (y_offset&0x7)<<12 |
	pGlint->PM3_AreaStippleMode,
	AreaStippleMode);
    GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(w) | PM3Render2D_Height(h),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentMono8x8PatternFillRect");
}

/* Color Expansion Fills */
static void Permedia3SetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
	int fg, int bg, int rop,unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForCPUToScreenColorExpandFill");
    REPLICATE(fg);
    pGlint->PM3_Render2D =
	PM3Render2D_SpanOperation |
	PM3Render2D_XPositive |
	PM3Render2D_YPositive |
	PM3Render2D_Operation_SyncOnBitMask;
    pGlint->PM3_Config2D =
	PM3Config2D_UserScissorEnable |
	PM3Config2D_UseConstantSource |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_BackgroundROPEnable |
	PM3Config2D_BackgroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
    if (bg != -1) {
	REPLICATE(bg);
	pGlint->PM3_Config2D |= PM3Config2D_OpaqueSpan;
	GLINT_WAIT(4);
    	GLINT_WRITE_REG(bg, BackgroundColor);
    }
    else GLINT_WAIT(3);
    GLINT_WRITE_REG(fg, PM3ForegroundColor);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    TRACE_EXIT("Permedia3SetupForCPUToScreenColorExpandFill");

}
static void Permedia3SubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, 
	int x, int y, int w, int h, int skipleft)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentCPUToScreenColorExpandFill");
    GLINT_WAIT(4);
    GLINT_WRITE_REG(((y&0x0fff)<<16)|(x&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG((((y+h)&0x0fff)<<16)|((x+w)&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG(
	PM3RectanglePosition_XOffset(x-skipleft) |
	PM3RectanglePosition_YOffset(y),
	PM3RectanglePosition);
    GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(w+skipleft) | PM3Render2D_Height(h),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentCPUToScreenColorExpandFill");
}
/* Images Writes */
static void Permedia3SetupForImageWrite(ScrnInfoPtr pScrn, int rop,
	unsigned int planemask, int trans_color, int bpp, int depth)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForImageWrite");
    pGlint->PM3_Render2D =
	PM3Render2D_SpanOperation |
	PM3Render2D_XPositive |
	PM3Render2D_YPositive |
	PM3Render2D_Operation_SyncOnHostData;
    pGlint->PM3_Config2D =
	PM3Config2D_UserScissorEnable |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
    GLINT_WAIT(2);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    TRACE_EXIT("Permedia3SetupForImageWrite");

}
static void Permedia3SubsequentImageWriteRect(ScrnInfoPtr pScrn, 
	int x, int y, int w, int h, int skipleft)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentImageWrite");
    GLINT_WAIT(4);
    GLINT_WRITE_REG(((y&0x0fff)<<16)|(x&0x0fff), ScissorMinXY);
    GLINT_WRITE_REG((((y+h)&0x0fff)<<16)|((x+w)&0x0fff), ScissorMaxXY);
    GLINT_WRITE_REG(
	PM3RectanglePosition_XOffset(x-skipleft) |
	PM3RectanglePosition_YOffset(y),
	PM3RectanglePosition);
    GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(w+skipleft) | PM3Render2D_Height(h),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentImageWrite");
}

/* Solid Lines */
#if 0
static void 
Permedia3PolylinesThinSolidWrapper(
   DrawablePtr     pDraw,
   GCPtr           pGC,
   int             mode,
   int             npt,
   DDXPointPtr     pPts
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    GLINTPtr pGlint = GLINTPTR(infoRec->pScrn);
    pGlint->CurrentGC = pGC;
    pGlint->CurrentDrawable = pDraw;
    if(infoRec->NeedToSync) (*infoRec->Sync)(infoRec->pScrn);
    XAAPolyLines(pDraw, pGC, mode, npt, pPts);
}

static void 
Permedia3PolySegmentThinSolidWrapper(
   DrawablePtr     pDraw,
   GCPtr           pGC,
   int             nseg,
   xSegment        *pSeg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    GLINTPtr pGlint = GLINTPTR(infoRec->pScrn);
    pGlint->CurrentGC = pGC;
    pGlint->CurrentDrawable = pDraw;
    if(infoRec->NeedToSync) (*infoRec->Sync)(infoRec->pScrn);
    XAAPolySegment(pDraw, pGC, nseg, pSeg);
}
static void
Permedia3SetupForSolidLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForSolidLine");
    REPLICATE(color);
    pGlint->PM3_Render2D =
	PM3Render2D_SpanOperation |
	PM3Render2D_XPositive |
	PM3Render2D_YPositive |
	PM3Render2D_Operation_Normal;
    pGlint->PM3_Config2D =
	PM3Config2D_UseConstantSource |
	PM3Config2D_ForegroundROPEnable |
	PM3Config2D_ForegroundROP(rop) |
	PM3Config2D_FBWriteEnable;
    /*
    if (pGlint->ClippingOn)
	pGlint->PM3_Config2D |= PM3Config2D_UserScissorEnable;
    */
    GLINT_WAIT(6);
    GLINT_WRITE_REG(pGlint->PM3_Config2D, PM3Config2D);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE|1<<11, LogicalOpMode);
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted)) {
	pGlint->PM3_Config2D |= PM3Config2D_FBDestReadEnable;
	GLINT_WRITE_REG(
	    PM3FBDestReadMode_ReadEnable,
	    PM3FBDestReadMode);
    }
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(color, PM3ForegroundColor);
    GLINT_WRITE_REG(UNIT_DISABLE, ScissorMode);
    TRACE_EXIT("Permedia3SetupForSolidLine");
}

static void 
Permedia3SubsequentSolidHorVertLine( ScrnInfoPtr pScrn,
        int x, int y, int len, int dir)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentSolidHorVertLine");
    /* Set the Rectangle Position */
    if ((y != pGlint->y) || (x != pGlint->x)) {
	pGlint->x = x;
	pGlint->y = y;
	GLINT_WAIT(2);
	GLINT_WRITE_REG(
	    PM3RectanglePosition_XOffset(x)|PM3RectanglePosition_YOffset(y),
	    PM3RectanglePosition);
    }
    else GLINT_WAIT(1);
    if (dir == DEGREES_0) GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(len) | PM3Render2D_Height(1),
	PM3Render2D);
    else GLINT_WRITE_REG(pGlint->PM3_Render2D |
	PM3Render2D_Width(1) | PM3Render2D_Height(len),
	PM3Render2D);
    TRACE_EXIT("Permedia3SubsequentSolidHorVertLine");
}

static void 
Permedia3SubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentSolidBresenhamLine");
    GLINT_WAIT(6);
    if (octant & YMAJOR) {
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG((x<<16) + ((e<<16) / dmaj), StartXDom);
	GLINT_WRITE_REG(((octant & YDECREASING) ? -1 : 1)<<16, dY);
	GLINT_WRITE_REG(
	    ((((octant & XDECREASING) ? -1 : 1) * dmin)<<16) / dmaj,
	    dXDom);
    }
    else {
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((y<<16) + ((e<<16) / dmaj), StartY);
	GLINT_WRITE_REG(((octant & XDECREASING) ? -1 : 1)<<16, dXDom);
	GLINT_WRITE_REG(
	    ((((octant & YDECREASING) ? -1 : 1) * dmin)<<16) / dmaj,
	    dY);
    }
    GLINT_WRITE_REG(len,GLINTCount);
    GLINT_WRITE_REG(0, Render);
    TRACE_EXIT("Permedia3SubsequentSolidBresenhamLine");
}
#endif

/* Dashed Lines */
#if 0
static void 
Permedia3PolylinesThinDashedWrapper(
   DrawablePtr     pDraw,
   GCPtr           pGC,
   int             mode,
   int             npt,
   DDXPointPtr     pPts
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    GLINTPtr pGlint = GLINTPTR(infoRec->pScrn);
    pGlint->CurrentGC = pGC;
    pGlint->CurrentDrawable = pDraw;
    if(infoRec->NeedToSync) (*infoRec->Sync)(infoRec->pScrn);
    XAAPolyLines(pDraw, pGC, mode, npt, pPts);
}

static void 
Permedia3PolySegmentThinDashedWrapper(
   DrawablePtr     pDraw,
   GCPtr           pGC,
   int             nseg,
   xSegment        *pSeg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    GLINTPtr pGlint = GLINTPTR(infoRec->pScrn);
    pGlint->CurrentGC = pGC;
    pGlint->CurrentDrawable = pDraw;
    if(infoRec->NeedToSync) (*infoRec->Sync)(infoRec->pScrn);
    XAAPolySegment(pDraw, pGC, nseg, pSeg);
}

static void
Permedia3SetupForDashedLine(ScrnInfoPtr pScrn,
	int fg, int bg, int rop, unsigned int planemask,
	int length, unsigned char *pattern)
{
    int p = 0;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SetupForDashedLine");

    switch (length) {
	case 16:
	    p = pattern[0] | (pattern[1]<<8);
	    break;
	case 8:
	    p = (pattern[0]<<8) | (pattern[0]);
	    break;
	case 4:
	    p &= 0x0f;
	    p |= (p<<4);
	    p |= (p<<8);
	    break;
	case 2:
	    p &= 0x03;
	    p |= (p<<2);
	    p |= (p<<4);
	    p |= (p<<8);
	    break;
	case 1:
	    if (pattern[0]&0x01) p = -1;
	    else p = 0;
	    break;
    }
    REPLICATE(fg);
    if (bg != -1) {
	REPLICATE(bg);
	GLINT_WAIT(9);
    	GLINT_WRITE_REG(bg, BackgroundColor);
	GLINT_WRITE_REG(
	    PM3FBWriteMode_WriteEnable|
	    PM3FBWriteMode_OpaqueSpan|
	    PM3FBWriteMode_Enable0,
	    PM3FBWriteMode);
	GLINT_WRITE_REG(1<<24, RasterizerMode);
    }
    else GLINT_WAIT(6);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE|1<<11, LogicalOpMode);
    if ((rop!=GXclear)&&(rop!=GXset)&&(rop!=GXcopy)&&(rop!=GXcopyInverted))
	GLINT_WRITE_REG(
	    PM3FBDestReadMode_ReadEnable,
	    PM3FBDestReadMode);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG( 1<<0|(p&0xffff)<<10, LineStippleMode);
    GLINT_WRITE_REG(fg, PM3ForegroundColor);
    /*
    GLINT_WRITE_REG(UNIT_DISABLE, ScissorMode);
    */
    TRACE_EXIT("Permedia3SetupForDashedLine");
}

static void 
Permedia3SubsequentDashedBresenhamLine( ScrnInfoPtr pScrn, int x, int y,
	int dmaj, int dmin, int e, int len, int octant, int phase)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    TRACE_ENTER("Permedia3SubsequentDashedBresenhamLine");
    ErrorF ("SVEN SVEN : Doing Bresenham Line from %d,%d, len = %d"
	" major = %d, minor %d, octant = %d, e = %d.\n",
	x, y, len, dmaj, dmin, len, e);
    GLINT_WAIT(7);
    if (octant & YMAJOR) {
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG((x<<16) + ((e<<16) / dmaj), StartXDom);
	GLINT_WRITE_REG(((octant & YDECREASING) ? -1 : 1)<<16, dY);
	GLINT_WRITE_REG(
	    ((((octant & XDECREASING) ? -1 : 1) * dmin)<<16) / dmaj,
	    dXDom);
    }
    else {
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((y<<16) + ((e<<16) / dmaj), StartY);
	GLINT_WRITE_REG(((octant & XDECREASING) ? -1 : 1)<<16, dXDom);
	GLINT_WRITE_REG(
	    ((((octant & YDECREASING) ? -1 : 1) * dmin)<<16) / dmaj,
	    dY);
    }
    GLINT_WRITE_REG(len,GLINTCount);
    /* I am not satisfied by this, 3<<1 in the Render Command
     * will reset the phase to 0 always, ... */
    GLINT_WRITE_REG((phase&0xf)<<0|(phase&0xf)<<13, PM3LoadLineStippleCounters);
    GLINT_WRITE_REG(3<<1, Render);
    TRACE_EXIT("Permedia3SubsequentDashedBresenhamLine");
}
#endif
