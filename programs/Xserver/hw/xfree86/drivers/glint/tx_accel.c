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
 * GLINT 500TX accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/tx_accel.c,v 1.2 1998/07/25 16:55:50 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#if 0
extern int pprod;
extern int coprotype;
extern GLINTWindowBase;
int ScanlineWordCount;
int savedplanemask = 0xffffffff;
int savedrop = UNIT_DISABLE;
int savedreadmode = 0;
int savedrasterizer = UNIT_DISABLE;
int savedcolorddamode = UNIT_DISABLE;
static int gcolor;
int blitmode = FBRM_DstEnable;
static int grop;
static int mode;
static int span;
static int gbg, gfg;
CARD32 ScratchBuffer[512];
static int blitxdir, blitydir;

extern void xf86ImageGlyphBltTE();
extern void xf86PolyGlyphBltTE();
#endif

void TXSync();
void TXSetupForFillRectSolid();
void TXSubsequentFillRectSolid();
#if 0
void GLINTSubsequentFillTrapezoidSolid();
void GLINTSubsequentTwoPointLine();
void GLINTSetupForScreenToScreenCopy();
void GLINTSubsequentScreenToScreenCopy();
void GLINTSetClippingRectangle();
void GLINTSetupFor8x8PatternColorExpand();
void GLINTSubsequent8x8PatternColorExpand();
void GLINTSetupForScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenCopy();
void GLINTFillRectStippled();
void GLINTWriteBitmap();
void GLINTImageTextTECPUToScreenColorExpand();
void GLINTPolyTextTECPUToScreenColorExpand();
#endif

void TXInitializeEngine(ScrnInfoPtr pScrn)
{
	GLINTPtr pGlint = GLINTPTR(pScrn);
    /* Initialize the Accelerator Engine to defaults */

	GLINT_SLOW_WRITE_REG(pGlint->pprod,		LBReadMode);
	GLINT_SLOW_WRITE_REG(0x01,		LBWriteMode);
#if 1	/* For the GLINT MX - Added by Dirk */
	GLINT_SLOW_WRITE_REG(0x0,		0x6008);
	GLINT_SLOW_WRITE_REG(0x0,		0x6000);
#endif
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	DitherMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	AlphaBlendMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	ColorDDAMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureColorMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   TextureReadMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   GLINTWindow);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   AlphaBlendMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   LogicalOpMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   DepthMode);
	GLINT_SLOW_WRITE_REG(UNIT_DISABLE,   RouterMode);
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
}

Bool
TXAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    BoxRec AvailFBArea;

    pGlint->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    TXInitializeEngine(pScrn);

    infoPtr->Flags = PIXMAP_CACHE;
 
    infoPtr->Sync = TXSync;

    infoPtr->SetupForSolidFill = TXSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = TXSubsequentFillRectSolid;
#if 0
    infoPtr->SubsequentTwoPointLine = TXSubsequentTwoPointLine;
    xf86AccelInfoRec.SetClippingRectangle = GLINTSetClippingRectangle;

    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
      GLINTSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
      GLINTSubsequentScreenToScreenCopy;
  
    xf86AccelInfoRec.ColorExpandFlags = VIDEO_SOURCE_GRANULARITY_PIXEL |
      					CPU_TRANSFER_PAD_DWORD |
					ONLY_TRANSPARENCY_SUPPORTED |
      					BIT_ORDER_IN_BYTE_LSBFIRST;
  
 /* This is just used for text now, as we replace XAA for bitmaps & stipples */
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
      GLINTSetupForScanlineScreenToScreenColorExpand;
    xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
      GLINTSubsequentScanlineScreenToScreenColorExpand;
    xf86AccelInfoRec.ScratchBufferAddr = 1;
    xf86AccelInfoRec.ScratchBufferSize = 512;
    xf86AccelInfoRec.ScratchBufferBase = (void*)(ScratchBuffer);
    xf86AccelInfoRec.PingPongBuffers = 1;

    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
      					GLINTSetupFor8x8PatternColorExpand;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
      					GLINTSubsequent8x8PatternColorExpand;

    if (pGlint->UsePCIRetry) {
	xf86AccelInfoRec.ImageTextTE = GLINTImageTextTECPUToScreenColorExpand;
	xf86GCInfoRec.ImageGlyphBltTE = xf86ImageGlyphBltTE;
	xf86AccelInfoRec.PolyTextTE = GLINTPolyTextTECPUToScreenColorExpand;
	xf86GCInfoRec.PolyGlyphBltTE = xf86PolyGlyphBltTE;
    }

    /* Replace XAA functions */
    xf86AccelInfoRec.FillRectOpaqueStippled = GLINTFillRectStippled;
    xf86AccelInfoRec.FillRectStippled = GLINTFillRectStippled;
    xf86GCInfoRec.SecondaryPolyFillRectOpaqueStippledFlags = 0;
    xf86GCInfoRec.SecondaryPolyFillRectStippledFlags = 0;
    xf86AccelInfoRec.WriteBitmap = GLINTWriteBitmap;

    xf86AccelInfoRec.ImageWriteOffset = glintInfoRec.displayWidth *
			glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel / 8);
    /* 4 scanlines should be enough */
    xf86AccelInfoRec.ImageWriteRange = glintInfoRec.displayWidth * 4 *
			(glintInfoRec.bitsPerPixel / 8);

    xf86AccelInfoRec.PixmapCacheMemoryStart =
	xf86AccelInfoRec.ImageWriteOffset + xf86AccelInfoRec.ImageWriteRange;
  
    xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;

    /* Check Pixmap cache and if enough room, enable ImageWrites */
    if (!OFLG_ISSET(OPTION_NO_PIXMAP_CACHE, &glintInfoRec.options)) {
      if ((xf86AccelInfoRec.PixmapCacheMemoryEnd - 
		xf86AccelInfoRec.ImageWriteOffset) > 
				xf86AccelInfoRec.ImageWriteRange) {

    	xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY;
    	xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy =
			GLINTSubsequentScanlineScreenToScreenCopy;
      }
    } else {
      xf86AccelInfoRec.ImageWriteRange = 0;
      xf86AccelInfoRec.ImageWriteOffset = 0;
    }
#endif
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pGlint->FbMapSize / (pScrn->displayWidth * 
					  pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return (XAAInit(pScreen, infoPtr));
}

void TXSync(ScrnInfoPtr pScrn) {
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

void TXSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, int rop, unsigned planemask)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
#if 0
	gcolor = color;
#endif
	REPLICATE(color);
	
	pGlint->ROP = rop;
	GLINT_WAIT(6);
	DO_PLANEMASK(planemask);
	if (rop == GXcopy) {
		GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
		GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		pGlint->FrameBufferReadMode = FastFillEnable;
	} else {
		GLINT_WRITE_REG(pGlint->pprod | pGlint->BlitMode, FBReadMode);
		GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
		GLINT_WRITE_REG(color, PatternRamData0);
		pGlint->FrameBufferReadMode = FastFillEnable | SpanOperation;
	}
	GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
	GLINT_WRITE_REG(1<<16,dY);
}

void TXSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int  y, int  w, int  h)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
	GLINT_WAIT(5);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,StartXDom);
	GLINT_WRITE_REG(y<<16,StartY);
	GLINT_WRITE_REG(h,GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid | pGlint->FrameBufferReadMode,Render);
}

#if 0
void GLINTSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
  int dy = y2 - y1 + 1;
  
  if ((dy < 16) && (grop == GXcopy)) {
	GLINT_WAIT(5);
	GLINT_WRITE_REG((x1+1)<<16, StartXSub);
	GLINT_WRITE_REG(x1<<16,StartXDom);
	GLINT_WRITE_REG(y1<<16,StartY);
	GLINT_WRITE_REG(dy,GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable,Render);
	return;
  }

  GLINT_WAIT(5);
  GLINT_WRITE_REG (gcolor, GLINTColor);
  GLINT_WRITE_REG (x1 << 16, StartXDom);
  GLINT_WRITE_REG (y1 << 16, StartY);
  GLINT_WRITE_REG (dy, GLINTCount);

  GLINT_WRITE_REG (PrimitiveLine, Render);
}

void GLINTSetClippingRectangle(int x1, int y1, int x2, int y2)
{
	GLINT_WAIT(3);
	GLINT_WRITE_REG((y1&0x0FFF)<<16|(x1&0x0FFF), ScissorMinXY);
	GLINT_WRITE_REG((y2&0x0FFF)<<16|(x2&0x0FFF), ScissorMaxXY);
	GLINT_WRITE_REG(1, ScissorMode); /* Enable Scissor Mode */
}

void GLINTSetupForScreenToScreenCopy( int xdir, int  ydir, int  rop,
				    unsigned planemask, int transparency_color)
{
	blitydir = ydir;
	blitxdir = xdir;

	GLINT_WAIT(4);
	DO_PLANEMASK(planemask);
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);

	if ((rop == GXclear) || (rop == GXset)) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else 
	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | blitmode, FBReadMode);
	}

	GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void GLINTSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2,
				     int w, int h)
{
	int srcaddr;
	int dstaddr;

	/* Glint 500TX only allows left to right bitblt's */

	GLINT_WAIT(8);
	if (blitydir < 0) {
		y1 = y1 + h - 1;
		y2 = y2 + h - 1;
		GLINT_WRITE_REG(-1<<16, dY);
	} else {
		GLINT_WRITE_REG(1<<16, dY);
	}

	srcaddr = GLINTWindowBase + y1 * glintInfoRec.displayWidth + x1;
	dstaddr = GLINTWindowBase + y2 * glintInfoRec.displayWidth + x2;

	GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
	GLINT_WRITE_REG(x2<<16, StartXDom);
	GLINT_WRITE_REG((x2+w)<<16, StartXSub);
	GLINT_WRITE_REG(y2<<16, StartY);
	GLINT_WRITE_REG(h, GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | SpanOperation, Render);
}

void GLINTSetupForScanlineScreenToScreenColorExpand(int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{	
  REPLICATE(fg);
  gbg = bg;
  gfg = fg;


  grop = rop;
  GLINT_WAIT(11);
  DO_PLANEMASK(planemask);
  if (rop == GXcopy) {
    GLINT_WRITE_REG(pprod, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
    GLINT_WRITE_REG(fg, FBBlockColor);
    mode = FastFillEnable;
  } else {
    GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
    GLINT_WRITE_REG(fg, PatternRamData0);
    mode = FastFillEnable | SpanOperation;
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(x<<16, StartXDom);
  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(h, GLINTCount);
  GLINT_WRITE_REG(1<<16, dY);
  ScanlineWordCount = (w+31)>>5;
  GLINT_WRITE_REG(PrimitiveTrapezoid | mode | SyncOnBitMask, Render);
}

void GLINTSubsequentScanlineScreenToScreenColorExpand(int srcaddr)
{
	register CARD32 *ptr = (CARD32 *) ScratchBuffer;
	int count = ScanlineWordCount >> 3; 

	while (count--) {
		GLINT_WAIT(8);
		GLINT_WRITE_REG(*(ptr), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+1), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+2), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+3), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+4), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+5), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+6), BitMaskPattern);
		GLINT_WRITE_REG(*(ptr+7), BitMaskPattern);
		ptr+=8;
	}
	count = ScanlineWordCount & 0x07;
	GLINT_WAIT(count);
	while (count--) 
		GLINT_WRITE_REG(*(ptr++), BitMaskPattern);
}

void GLINTSetupFor8x8PatternColorExpand(int patternx, int patterny, 
					   int bg, int fg, int rop,
					   unsigned planemask)
{
  if (bg == -1) mode = -1;
	else    mode = 0;
  REPLICATE(fg);
  REPLICATE(bg);

  gfg = fg;
  gbg = bg;

  GLINT_WAIT(13);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG((patternx & 0x000000FF),       AreaStipplePattern0);
  GLINT_WRITE_REG((patternx & 0x0000FF00) >> 8,  AreaStipplePattern1);
  GLINT_WRITE_REG((patternx & 0x00FF0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG((patterny & 0x000000FF),       AreaStipplePattern4);
  GLINT_WRITE_REG((patterny & 0x0000FF00) >> 8,  AreaStipplePattern5);
  GLINT_WRITE_REG((patterny & 0x00FF0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
  
  grop = rop;

  if (rop == GXcopy) {
      GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
      GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
      GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
      GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }

  GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16, dY);	
}

void 
GLINTSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  int span = 0;
  
  GLINT_WAIT(10);
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(x<<16,     StartXDom);
  GLINT_WRITE_REG(y<<16,     StartY);
  GLINT_WRITE_REG(h,         GLINTCount);

  if (mode != -1) {
	if (grop == GXcopy) {
	  GLINT_WRITE_REG(gbg, FBBlockColor);
	  GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable,Render);
	} else {
  	  GLINT_WRITE_REG(gbg, PatternRamData0);
	  GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12|ASM_InvertPattern |
					UNIT_ENABLE, AreaStippleMode);
	  GLINT_WRITE_REG(AreaStippleEnable | SpanOperation | FastFillEnable | 
						PrimitiveTrapezoid, Render);
	}
  }

  if (grop == GXcopy) {
	GLINT_WRITE_REG(gfg, FBBlockColor);
	span = 0;
  } else {
  	GLINT_WRITE_REG(gfg, PatternRamData0);
	span = SpanOperation;
  }
  GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12|
  						UNIT_ENABLE, AreaStippleMode);
  GLINT_WRITE_REG(AreaStippleEnable | span | FastFillEnable | 
						PrimitiveTrapezoid, Render);
}

void
GLINTSubsequentScanlineScreenToScreenCopy(LineAddr, skipleft, x, y, w)
	int LineAddr, skipleft, x, y, w;
{
	int dstaddr;

	GLINT_WAIT(8);
	GLINT_WRITE_REG(-1<<16, dY);

	dstaddr = GLINTWindowBase + y * glintInfoRec.displayWidth + x;

	GLINT_WRITE_REG((LineAddr/(glintInfoRec.bitsPerPixel/8)) - dstaddr, 									FBSourceOffset);

	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(1, GLINTCount);
	
	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | SpanOperation, 
									Render);
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

void
GLINTStippleCPUToScreen(x, y, w, h, src, srcwidth,
stipplewidth, stippleheight, srcx, srcy)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int stipplewidth, stippleheight;
    int srcx, srcy;
{
    unsigned char *srcp = (srcwidth * srcy) + src;
    int dwords = (w + 31) >> 5; 
    int count, offset; 


    GLINT_WAIT(1);
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | span | 
						SyncOnBitMask, Render);

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

	   while(count--) {
		GLINT_WAIT(1);
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
	   	
	   while(count--) {
	   	shift = stipplewidth - offset;
		scratch = (CARD32*)(srcp + (offset >> 3));
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		    if(scratch2) {
			GLINT_WAIT(1);
		        GLINT_WRITE_REG((*scratch >> scratch2) |
			     (scratch[1] << (32 - scratch2)), BitMaskPattern);
		    } else {
			GLINT_WAIT(1);
			GLINT_WRITE_REG(*scratch, BitMaskPattern); 
		    }
		} else {
			GLINT_WAIT(1);
			GLINT_WRITE_REG((*((CARD32*)srcp) << shift) |
			((*scratch >> scratch2) & ShiftMasks[shift]), BitMaskPattern);
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


void
GLINTFillRectStippled(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int nBoxInit;		/* number of rectangles to fill */
    BoxPtr pBoxInit;		/* Pointer to first rectangle to fill */
{
    int rectWidth, rectHeight;
    BoxPtr pBox;
    int color;
    int nBox;
    int xoffset, yoffset, StippleType;
    PixmapPtr pPixmap = pGC->stipple;

    if(pGC->fillStyle == FillStippled){	/* Transparent Stipples */
	StippleType = 0;
    } else if(pGC->alu == GXcopy)  /* OpaqueEasyStipples */
	StippleType = 2;
    else 				/* OpaqueHardStipples */
	StippleType = 4;

    /* >>>>>>>>>>> Set ROP, Planemask <<<<<<<<<<< */
    GLINT_WAIT(6);
    DO_PLANEMASK(pGC->planemask);
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG(1<<16, dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (pGC->alu == GXcopy) {
	span = 0;
	GLINT_WRITE_REG(pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
    } else {
	span = SpanOperation;
	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }

    for (nBox = nBoxInit, pBox = pBoxInit; nBox > 0; nBox--, pBox++) {
	rectWidth = pBox->x2 - pBox->x1;
	rectHeight = pBox->y2 - pBox->y1;

	if ((rectWidth > 0) && (rectHeight > 0)) {
	    xoffset = (pBox->x1 - (pGC->patOrg.x + pDrawable->x))
	        % pPixmap->drawable.width;
	    if (xoffset < 0)
	        xoffset += pPixmap->drawable.width;
	    yoffset = (pBox->y1 - (pGC->patOrg.y + pDrawable->y))
	        % pPixmap->drawable.height;
	    if (yoffset < 0)
	        yoffset += pPixmap->drawable.height;

	    GLINT_WAIT(4);
  	    GLINT_WRITE_REG(pBox->x1<<16, StartXDom);
  	    GLINT_WRITE_REG(pBox->y1<<16, StartY);
  	    GLINT_WRITE_REG(pBox->x2<<16, StartXSub);
  	    GLINT_WRITE_REG(rectHeight, GLINTCount);

	    switch(StippleType) {
	      case 0:
		color = pGC->fgPixel; REPLICATE(color);
		/* >>>>>>>>>>>>>>>>>  Set fg  <<<<<<<<<<<<<<<<<< */
		GLINT_WAIT(1);
		if (pGC->alu == GXcopy) {
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 2:
		/* >>>>>>>>>>>  Set bg and draw rect <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		GLINT_WAIT(3);
		GLINT_WRITE_REG(color, FBBlockColor);
		GLINT_WRITE_REG(PrimitiveTrapezoid|FastFillEnable,Render);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 4:
    		/* >>>>> Make sure our bitmask is not inverted <<<<<< */
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		GLINT_WAIT(1);
		if (pGC->alu == GXcopy) {
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
	/*	>>>>>>>>>>  Invert Mask <<<<<<<<< */
		GLINT_WAIT(2);
		GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	/*	>>>>>>>>>>  Set bg <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	    }
	}
    }

    GLINT_WAIT(1);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG;
}

void 
GLINTWriteBitmap(x, y, w, h, src, srcwidth, srcx, srcy, 
	bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    unsigned char *srcp;
    unsigned char *srcpntr;
    int dwords, skipleft, height;
    register int count; 
    register CARD32* pattern;
    Bool SecondPass = FALSE;

    srcp = (srcwidth * srcy) + (srcx >> 3) + src; 
    srcx &= 0x07;
    if(skipleft = (long)srcp & 0x03) { 
	skipleft = (skipleft << 3) + srcx;	
	srcp = (unsigned char *)((long)srcp & ~0x03);
    } else
	skipleft = srcx;    

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    /* >>>>> Set rop, planemask, left edge clip skipleft pixels right
	of x (skipleft is sometimes 0 and clipping isn't needed). <<<<<< */
    if (skipleft > 0) 
	GLINTSetClippingRectangle(x+skipleft, y, x+w, y+h);
    GLINT_WAIT(13);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    if (rop == GXcopy) {
	mode = 0;
	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	mode = SpanOperation;
	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
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
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	GLINT_WRITE_REG(PrimitiveTrapezoid |mode|FastFillEnable,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    }

   /* >>>>>>>>> initiate transfer (x,y,w,h).  Skipleft pixels on the
	left edge will be clipped <<<<<< */

SECOND_PASS:
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | mode | SyncOnBitMask, Render);
    
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
	GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
	if (rop == GXcopy) {
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	goto SECOND_PASS;
    }

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(0, ScissorMode);
    SET_SYNC_FLAG;	
}

#define MAX_GLYPHS	1024			/* that's gotta be enough */
static unsigned int* Glyphs[MAX_GLYPHS]; 

void
GLINTImageTextTECPUToScreenColorExpand(pDrawable, pGC, xInit, yInit,
					nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    int w, h, x, y;
    int glyphWidth, count;		       

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
    DO_PLANEMASK(pGC->planemask);
    gfg =  pGC->fgPixel;
    gbg = pGC->bgPixel;
    REPLICATE (gfg);
    REPLICATE (gbg);
 
    GLINT_WRITE_REG((GXcopy<<1)|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((x+w)<<16, StartXSub);
    GLINT_WRITE_REG(x<<16,StartXDom);
    GLINT_WRITE_REG(y<<16,StartY);
    GLINT_WRITE_REG(h,GLINTCount);
    GLINT_WRITE_REG(1<<16,dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(pprod, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
    GLINT_WRITE_REG(gbg, FBBlockColor);
    GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable, Render);
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

    for(count = 0; count < nglyph; count++) 
	Glyphs[count] = (unsigned int*)FONTGLYPHBITS(pglyphBase,*ppci++);	

    /*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
    GLINT_WRITE_REG(gfg, FBBlockColor);
    GLINT_WRITE_REG(PrimitiveTrapezoid |FastFillEnable| SyncOnBitMask, Render);
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
 
    for(count = 0; count < h; count++, y++) {
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)GLINTMMIOBase + GLINT_TAG_ADDR(0x00,0x0d)),
 		Glyphs, count, nglyph, glyphWidth);
    }
}

void
GLINTPolyTextTECPUToScreenColorExpand(pDrawable, pGC, xInit, yInit,
 					nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC *pGC;
    int xInit, yInit;
    int nglyph;
    CharInfoPtr *ppci;		       /* array of character info */
    unsigned char *pglyphBase;	       /* start of array of glyphs */
{
    int w, h, x, y;
    int glyphWidth, count;		       
 
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
 
    /*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
    DO_PLANEMASK(pGC->planemask);
    gfg = pGC->fgPixel;
    REPLICATE (gfg);
 
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((x+w)<<16, StartXSub);
    GLINT_WRITE_REG(x<<16,StartXDom);
    GLINT_WRITE_REG(y<<16,StartY);
    GLINT_WRITE_REG(1<<16,dY);
    GLINT_WRITE_REG(h,GLINTCount);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (pGC->alu == GXcopy) {
 	GLINT_WRITE_REG(pprod, FBReadMode);
     	GLINT_WRITE_REG(UNIT_DISABLE, PatternRamMode);
 	GLINT_WRITE_REG(gfg, FBBlockColor);
 	GLINT_WRITE_REG(PrimitiveTrapezoid|FastFillEnable|SyncOnBitMask,Render);
    } else {
 	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
     	GLINT_WRITE_REG(UNIT_ENABLE, PatternRamMode);
 	GLINT_WRITE_REG(gfg, PatternRamData0);
 	GLINT_WRITE_REG(PrimitiveTrapezoid|FastFillEnable|
				SpanOperation|SyncOnBitMask,Render);
    }
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
 
      
    for(count = 0; count < h; count++, y++) { 
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)GLINTMMIOBase + GLINT_TAG_ADDR(0x00,0x0d)),
 		Glyphs, count, nglyph, glyphWidth);
    }
}

#ifdef GLINT_3D
void glint_accel_dummy( void )
{
	glint_3d_dummy();
	return;
}
#endif
#endif
