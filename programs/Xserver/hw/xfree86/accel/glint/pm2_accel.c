/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/pm2_accel.c,v 1.5 1998/04/26 16:04:40 robin Exp $ */
/*
 * Copyright 1996,1997 by Alan Hourihane, Wigan, England.
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


#include "cfb.h"
#include "xf86.h"
#include "miline.h"
#include "compiler.h"

#include "glint_regs.h"
#include "glint.h"
#include "xf86xaa.h"
#include "xf86expblt.h"
#include "dixfontstr.h"

#include <float.h>

#if DEBUG && !defined(XFree86LOADER)
#include <stdio.h>
#endif

static char	bppand[4] = { 0x03, 0x01, 0x00, 0x00 };
extern int	pprod;
extern int      ScanlineWordCount;
extern int	Bppshift;
extern Bool 	UsePCIRetry;
static int      mode;
static int	span;
static int      blitmode;
static int      gcolor;
static int      grop;
static int      gbg, gfg;
extern          GLINTWindowBase;

extern void     GLINTSync ();
extern void	xf86ImageGlyphBltTE();
extern void	xf86PolyGlyphBltTE();

void            Permedia2SetupForFillRectSolid();
void		Permedia2SubsequentFillRectSolid();
void            Permedia2SetupForFillRectSolid24bpp();
void		Permedia2SubsequentFillRectSolid24bpp();
void 		Permedia2SubsequentScreenToScreenCopy ();
void            Permedia2SetupForScreenToScreenCopy ();
void 		Permedia2SubsequentScreenToScreenCopy2432bpp ();
void            Permedia2SetupForScreenToScreenCopy2432bpp ();
void            Permedia2SubsequentTwoPointLine ();
void            Permedia2SubsequentTwoPointLine24bpp ();
void            Permedia2SetupFor8x8PatternColorExpand ();
void		Permedia2Subsequent8x8PatternColorExpand();
void            Permedia2SetupFor8x8PatternColorExpand24bpp ();
void		Permedia2Subsequent8x8PatternColorExpand24bpp();
void            Permedia2SetupForScanlineScreenToScreenColorExpand ();
void            Permedia2SetupForScanlineScreenToScreenColorExpand24bpp();
void		Permedia2SubsequentScanlineScreenToScreenColorExpand ();
void            Permedia2SetClippingRectangle();
void		Permedia2FillRectStippled();
void		Permedia2WriteBitmap();
void		Permedia2ImageTextTECPUToScreenColorExpand();
void		Permedia2PolyTextTECPUToScreenColorExpand();
void 		Permedia2DoImageWrite8bpp();
void 		Permedia2DoImageWrite16bpp();
void 		Permedia2DoImageWrite32bpp();

void
Permedia2AccelInit ()
{
  xf86AccelInfoRec.Flags = PIXMAP_CACHE |
    COP_FRAMEBUFFER_CONCURRENCY |
    BACKGROUND_OPERATIONS |
    DELAYED_SYNC;
 
  xf86AccelInfoRec.Sync = GLINTSync;

  xf86GCInfoRec.PolyFillRectSolidFlags = 0;
  if (glintInfoRec.bitsPerPixel == 24) {
    xf86AccelInfoRec.SetupForFillRectSolid = 
				Permedia2SetupForFillRectSolid24bpp;
    xf86AccelInfoRec.SubsequentFillRectSolid = 	
				Permedia2SubsequentFillRectSolid24bpp;
    xf86AccelInfoRec.SubsequentTwoPointLine = 
				Permedia2SubsequentTwoPointLine24bpp;
  } else {
    xf86AccelInfoRec.SetupForFillRectSolid = Permedia2SetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = Permedia2SubsequentFillRectSolid;
    xf86AccelInfoRec.SubsequentTwoPointLine = Permedia2SubsequentTwoPointLine;
  }
    
  if (glintInfoRec.bitsPerPixel >= 24) {
    xf86AccelInfoRec.SetupForScreenToScreenCopy = 	
				Permedia2SetupForScreenToScreenCopy2432bpp;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy = 		
				Permedia2SubsequentScreenToScreenCopy2432bpp;
  } else {
    xf86AccelInfoRec.SetupForScreenToScreenCopy = 	
				Permedia2SetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy = 		
				Permedia2SubsequentScreenToScreenCopy;
  }
  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY; 

  xf86AccelInfoRec.SetClippingRectangle = Permedia2SetClippingRectangle;

  xf86AccelInfoRec.ColorExpandFlags =
    VIDEO_SOURCE_GRANULARITY_PIXEL |
    CPU_TRANSFER_PAD_DWORD |
    ONLY_TRANSPARENCY_SUPPORTED |
    BIT_ORDER_IN_BYTE_LSBFIRST;

  xf86AccelInfoRec.ScratchBufferAddr = 1;
  xf86AccelInfoRec.ScratchBufferBase = 
	(void *)((char*)GLINTMMIOBase + OutputFIFO + 4);
  xf86AccelInfoRec.ScratchBufferSize = 255 * sizeof(CARD32);
  xf86AccelInfoRec.PingPongBuffers = 1;

  if (glintInfoRec.bitsPerPixel == 24) {
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    	Permedia2SetupForScanlineScreenToScreenColorExpand24bpp;
  } else {
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    	Permedia2SetupForScanlineScreenToScreenColorExpand;
  }
  xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
    	Permedia2SubsequentScanlineScreenToScreenColorExpand;
  
  xf86AccelInfoRec.PatternFlags =
    				HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    				HARDWARE_PATTERN_PROGRAMMED_BITS |
    				HARDWARE_PATTERN_MONO_TRANSPARENCY |
    				HARDWARE_PATTERN_SCREEN_ORIGIN;

  if (glintInfoRec.bitsPerPixel == 24) {
    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
				Permedia2SetupFor8x8PatternColorExpand24bpp;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
				Permedia2Subsequent8x8PatternColorExpand24bpp;
  } else {
    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
				Permedia2SetupFor8x8PatternColorExpand;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
				Permedia2Subsequent8x8PatternColorExpand;
  }

  if (glintInfoRec.bitsPerPixel != 24) {
  	/* Replace XAA functions */
  	xf86AccelInfoRec.FillRectOpaqueStippled = Permedia2FillRectStippled;
  	xf86AccelInfoRec.FillRectStippled = Permedia2FillRectStippled;
  	xf86GCInfoRec.SecondaryPolyFillRectOpaqueStippledFlags = 0;
  	xf86GCInfoRec.SecondaryPolyFillRectStippledFlags = 0;
  	xf86AccelInfoRec.WriteBitmap = Permedia2WriteBitmap;

  	xf86AccelInfoRec.ImageTextTE=Permedia2ImageTextTECPUToScreenColorExpand;
  	xf86GCInfoRec.ImageGlyphBltTE = xf86ImageGlyphBltTE;
 
  	xf86AccelInfoRec.PolyTextTE = Permedia2PolyTextTECPUToScreenColorExpand;
  	xf86GCInfoRec.PolyGlyphBltTE = xf86PolyGlyphBltTE;
  }

  if (glintInfoRec.bitsPerPixel == 8)
  	xf86AccelInfoRec.DoImageWrite = Permedia2DoImageWrite8bpp;
  else
  if (glintInfoRec.bitsPerPixel == 16)
  	xf86AccelInfoRec.DoImageWrite = Permedia2DoImageWrite16bpp;
  else
  if (glintInfoRec.bitsPerPixel == 32)
  	xf86AccelInfoRec.DoImageWrite = Permedia2DoImageWrite32bpp;

  xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.displayWidth *
        glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel / 8);
  xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;
}

void
Permedia2SetClippingRectangle (int x1, int y1, int x2, int y2)
{
  GLINT_WAIT(3);
  GLINT_WRITE_REG (((y1&0x0fff)<<16)|(x1&0x0fff), ScissorMinXY);
  GLINT_WRITE_REG (((y2&0x0fff)<<16)|(x2&0x0fff), ScissorMaxXY);
  GLINT_WRITE_REG (1, ScissorMode);
}


static CARD32 BlitDir;

void
Permedia2SetupForScreenToScreenCopy2432bpp (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  BlitDir = 0;
  if (xdir == 1) BlitDir |= XPositive;
  if (ydir == 1) BlitDir |= YPositive;
  
  grop = rop;

  GLINT_WAIT(5);
  DO_PLANEMASK(planemask);

  GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
  GLINT_WRITE_REG(0, RasterizerMode);

  if ((rop == GXset) || (rop == GXclear)) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else
  if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	GLINT_WRITE_REG(pprod|FBRM_SrcEnable, FBReadMode);
  } else {
	GLINT_WRITE_REG(pprod|FBRM_SrcEnable|blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void
Permedia2SubsequentScreenToScreenCopy2432bpp (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  int srcaddr = y1 * glintInfoRec.displayWidth + x1;
  int dstaddr = y2 * glintInfoRec.displayWidth + x2;
  GLINT_WAIT(4);
  GLINT_WRITE_REG((y2<<16)|x2, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
  GLINT_WRITE_REG(PrimitiveRectangle | BlitDir, Render);
}

void
Permedia2SetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  BlitDir = 0;
  if (xdir == 1) BlitDir |= XPositive;
  if (ydir == 1) BlitDir |= YPositive;
  
  grop = rop;

  GLINT_WAIT(4);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
  GLINT_WRITE_REG(0, RasterizerMode);

  if ((rop == GXset) || (rop == GXclear)) {
	mode = pprod;
  } else
  if ((rop == GXcopy) || (rop == GXcopyInverted)) {
	mode = pprod | FBRM_SrcEnable;
  } else {
	mode = pprod | FBRM_SrcEnable | blitmode;
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void
Permedia2SubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  int lowerbits = bppand[(glintInfoRec.bitsPerPixel>>3)-1];
  int srcaddr;
  int dstaddr;
  char align;

  /* We can only use GXcopy for Packed modes */
  if (grop != GXcopy) {
  	srcaddr = y1 * glintInfoRec.displayWidth + x1;
  	dstaddr = y2 * glintInfoRec.displayWidth + x2;
	GLINT_WAIT(5);
	GLINT_WRITE_REG(mode, FBReadMode);
  	GLINT_WRITE_REG((y2<<16)|x2, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  } else {
  	srcaddr = y1 * glintInfoRec.displayWidth + (x1 & ~lowerbits);
  	dstaddr = y2 * glintInfoRec.displayWidth + (x2 & ~lowerbits);
  	align = (x2 & lowerbits) - (x1 & lowerbits);
	GLINT_WAIT(6);
	GLINT_WRITE_REG(mode | FBRM_Packed, FBReadMode);
  	GLINT_WRITE_REG((y2<<16)|x2>>Bppshift, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|(w+7)>>Bppshift, RectangleSize);
  	GLINT_WRITE_REG(align<<29|x2<<16|(x2+w), PackedDataLimits);
  }
	

  GLINT_WRITE_REG(srcaddr - dstaddr, FBSourceOffset);
  GLINT_WRITE_REG(PrimitiveRectangle | BlitDir, Render);
}

void Permedia2SetupForFillRectSolid24bpp(int color,int rop,unsigned planemask)
{
  gcolor = color;

  GLINT_WAIT(7);
  DO_PLANEMASK(planemask);
  grop = rop;
  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  GLINT_WRITE_REG(color, ConstantColor);
  if (rop == GXcopy) {
  	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
  	GLINT_WRITE_REG(pprod|blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16,dY);
}

void Permedia2SetupForFillRectSolid(int color, int rop, unsigned planemask)
{
  gcolor = color;
  REPLICATE(color);

  GLINT_WAIT(7);
  DO_PLANEMASK(planemask);
  grop = rop;
  if (rop == GXcopy) {
	mode = pprod;
	GLINT_WRITE_REG(pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(color, FBBlockColor);
  } else {
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
      	GLINT_WRITE_REG(color, ConstantColor);
	/* We can use Packed mode for filling solid non-GXcopy rasters */
	mode = pprod|FBRM_DstEnable;
  }
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16,dY);
}

void Permedia2SubsequentFillRectSolid24bpp(int x, int y, int w, int h)
{
  GLINT_WAIT(3);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);

  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive, Render);
}

void Permedia2SubsequentFillRectSolid(int x, int y, int w, int h)
{
  int speed = 0;

  if (grop == GXcopy) {
	GLINT_WAIT(3);
  	GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  	speed = FastFillEnable;
  } else {
	GLINT_WAIT(5);
	GLINT_WRITE_REG(mode|FBRM_Packed, FBReadMode);
  	GLINT_WRITE_REG((y<<16)|x>>Bppshift, RectangleOrigin);
  	GLINT_WRITE_REG((h<<16)|(w+7)>>Bppshift, RectangleSize);
  	GLINT_WRITE_REG(x<<16|(x+w), PackedDataLimits);
  	speed = 0;
  }
  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | speed, Render);
}

static void MoveBYTE(dest, src, dwords)
   register CARD32* dest;
   register unsigned char* src;
   register int dwords;
{
     while(dwords) {
	*dest = *src;
	src += 1;
	dest += 1;
	dwords -= 1;
     }	
}

static void MoveWORDS(dest, src, dwords)
   register CARD32* dest;
   register unsigned short* src;
   register int dwords;
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

static void MoveDWORDS(dest, src, dwords)
   register CARD32* dest;
   register CARD32* src;
   register int dwords;
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
     switch(dwords) {
	case 0:	return;
	case 1: *dest = *src;
		return;
	case 2: *dest = *src;
		*(dest + 1) = *(src + 1);
		return;
	case 3: *dest = *src;
		*(dest + 1) = *(src + 1);
 		*(dest + 2) = *(src + 2);
		return;
    }
}

static int LineCountDown;

void
Permedia2SetupForScanlineScreenToScreenColorExpand24bpp(int x,int y,int w,int h,
				int bg, int fg, int rop, unsigned planemask)
{
  gfg = fg;
  REPLICATE (gfg);

  GLINT_WAIT(9);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  ScanlineWordCount = (w + 31) >> 5;
  if (rop == GXcopy) {
      GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
      GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  GLINT_WRITE_REG(gfg, ConstantColor);
  GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
							SyncOnBitMask, Render);

   LineCountDown = h;
   GLINT_WAIT(ScanlineWordCount + 1);
   GLINT_WRITE_REG(((ScanlineWordCount - 1) << 16) | 0x0D, OutputFIFO);
}

void
Permedia2SetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  gfg = fg;
  REPLICATE (gfg);

  GLINT_WAIT(9);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  ScanlineWordCount = (w + 31) >> 5;
  if (rop == GXcopy) {
      GLINT_WRITE_REG(0, RasterizerMode);
      GLINT_WRITE_REG(pprod, FBReadMode);
      GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG(gfg, FBBlockColor);
      GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
					SyncOnBitMask | FastFillEnable, Render);
  } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(gfg, ConstantColor);
  	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
							SyncOnBitMask, Render);
  }

   LineCountDown = h;
   GLINT_WAIT(ScanlineWordCount + 1);
   GLINT_WRITE_REG(((ScanlineWordCount - 1) << 16) | 0x0D, OutputFIFO);
}

void
Permedia2SubsequentScanlineScreenToScreenColorExpand(int srcaddr)
{
   if(--LineCountDown) {
      GLINT_WAIT(ScanlineWordCount + 1);
      GLINT_WRITE_REG(((ScanlineWordCount - 1) << 16) | 0x0D, OutputFIFO);
   }
}

void 
Permedia2SetupFor8x8PatternColorExpand24bpp(int patternx, int patterny, 
					   int bg, int fg, int rop,
					   unsigned planemask)
{
  if (bg == -1) mode = -1;
	else    mode = 0;

  gfg = fg;
  gbg = bg;
  REPLICATE(gfg);
  REPLICATE(gbg);
  
  GLINT_WAIT(13);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG((patternx & 0xFF), AreaStipplePattern0);
  GLINT_WRITE_REG((patternx & 0xFF00) >> 8, AreaStipplePattern1);
  GLINT_WRITE_REG((patternx & 0xFF0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG((patterny & 0xFF), AreaStipplePattern4);
  GLINT_WRITE_REG((patterny & 0xFF00) >> 8, AreaStipplePattern5);
  GLINT_WRITE_REG((patterny & 0xFF0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
  
  grop = rop;

  if (rop == GXcopy) {
      GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
      GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
}

void 
Permedia2SetupFor8x8PatternColorExpand(int patternx, int patterny, 
					   int bg, int fg, int rop,
					   unsigned planemask)
{
  if (bg == -1) mode = -1;
	else    mode = 0;

  gfg = fg;
  gbg = bg;
  REPLICATE(gfg);
  REPLICATE(gbg);
  
  GLINT_WAIT(13);
  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG((patternx & 0xFF), AreaStipplePattern0);
  GLINT_WRITE_REG((patternx & 0xFF00) >> 8, AreaStipplePattern1);
  GLINT_WRITE_REG((patternx & 0xFF0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG((patternx & 0xFF000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG((patterny & 0xFF), AreaStipplePattern4);
  GLINT_WRITE_REG((patterny & 0xFF00) >> 8, AreaStipplePattern5);
  GLINT_WRITE_REG((patterny & 0xFF0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG((patterny & 0xFF000000) >> 24, AreaStipplePattern7);
  
  grop = rop;

  if (rop == GXcopy) {
      GLINT_WRITE_REG(pprod, FBReadMode);
      GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
  } else {
      GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
      GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  }
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
}

void 
Permedia2Subsequent8x8PatternColorExpand24bpp(int patternx, int patterny, 
				   int x, int y,
				   int w, int h)
{
  span = 0;
  
  GLINT_WAIT(8);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);

  if (mode != -1) {
      	  GLINT_WRITE_REG(gbg, ConstantColor);
	  GLINT_WRITE_REG(patternx<<7|patterny<<12| ASM_InvertPattern |
			UNIT_ENABLE, AreaStippleMode);
	  GLINT_WRITE_REG(AreaStippleEnable | XPositive | 
				YPositive | PrimitiveRectangle, Render);
  }

  GLINT_WRITE_REG(gfg, ConstantColor);
  GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  GLINT_WRITE_REG(AreaStippleEnable | XPositive | YPositive |
						PrimitiveRectangle, Render);
}

void 
Permedia2Subsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  span = 0;
  
  GLINT_WAIT(8);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);

  if (mode != -1) {
	if (grop == GXcopy) {
      	  GLINT_WRITE_REG(gbg, FBBlockColor);
	  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
					FastFillEnable, Render);
	} else { 
      	  GLINT_WRITE_REG(gbg, ConstantColor);
	  GLINT_WRITE_REG(patternx<<7|patterny<<12| ASM_InvertPattern |
			UNIT_ENABLE, AreaStippleMode);
	  GLINT_WRITE_REG(AreaStippleEnable | XPositive | 
				YPositive | PrimitiveRectangle, Render);
	}
  }

  if (grop == GXcopy) {
	GLINT_WRITE_REG(gfg, FBBlockColor);
	span = FastFillEnable;
  } else {
  	GLINT_WRITE_REG(gfg, ConstantColor);
	span = 0;
  }
  GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  GLINT_WRITE_REG(AreaStippleEnable | span | XPositive | YPositive |
						PrimitiveRectangle, Render);
}

void
Permedia2SubsequentTwoPointLine24bpp(int x1, int y1, int x2, int y2, int bias)
{
  int dy = y2 - y1 + 1;
  
  GLINT_WAIT(5);
  GLINT_WRITE_REG (gcolor, GLINTColor);
  GLINT_WRITE_REG (x1 << 16, StartXDom);
  GLINT_WRITE_REG (y1 << 16, StartY);
  GLINT_WRITE_REG (dy, GLINTCount);

  GLINT_WRITE_REG (PrimitiveLine, Render);
}

void
Permedia2SubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
{
  int dy = y2 - y1 + 1;
  
  if((dy < 16) && (grop == GXcopy)) {
	GLINT_WAIT(3);
  	GLINT_WRITE_REG((y1<<16)|x1, RectangleOrigin);
  	GLINT_WRITE_REG((dy<<16)|1, RectangleSize);
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
		FastFillEnable, Render);
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
Permedia2StippleCPUToScreen(x, y, w, h, src, srcwidth,
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
  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | span | SyncOnBitMask, Render);

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
Permedia2FillRectStippled(pDrawable, pGC, nBoxInit, pBoxInit)
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
    GLINT_WAIT(5);
    DO_PLANEMASK(pGC->planemask);
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    if (pGC->alu == GXcopy) {
	span = FastFillEnable;
 	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	span = 0;
 	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
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

	    GLINT_WAIT(2);
  	    GLINT_WRITE_REG((pBox->y1<<16)|pBox->x1, RectangleOrigin);
  	    GLINT_WRITE_REG((rectHeight<<16)|rectWidth, RectangleSize);

	    switch(StippleType) {
	      case 0:
		color = pGC->fgPixel; REPLICATE(color);
		/* >>>>>>>>>>>>>>>>>  Set fg  <<<<<<<<<<<<<<<<<< */
		GLINT_WAIT(1);
		if (pGC->alu == GXcopy) {
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		Permedia2StippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 2:
		/* >>>>>>>>>>>  Set bg and draw rect <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		GLINT_WAIT(3);
		GLINT_WRITE_REG(0, RasterizerMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		GLINT_WRITE_REG(PrimitiveRectangle|XPositive|YPositive|
							FastFillEnable,Render);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		GLINT_WAIT(2);
		if (pGC->alu == GXcopy) {
		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		Permedia2StippleCPUToScreen(
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
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		Permedia2StippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
	/*	>>>>>>>>>>  Invert Mask <<<<<<<<< */
	/*	>>>>>>>>>>  Set bg <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		GLINT_WAIT(2);
		if (pGC->alu == GXcopy) {
		    GLINT_WRITE_REG(InvertBitMask, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(InvertBitMask|
				BitMaskPackingEachScanline,RasterizerMode);
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		Permedia2StippleCPUToScreen(
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
Permedia2WriteBitmap(x, y, w, h, src, srcwidth, srcx, srcy, 
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
    if (skipleft)
	Permedia2SetClippingRectangle(x+skipleft,y,x+w,y+h);
 
    GLINT_WAIT(10);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    if (rop == GXcopy) {
	mode = FastFillEnable;
    	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	mode = 0;
	GLINT_WRITE_REG(BitMaskPackingEachScanline,RasterizerMode);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
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
    srcpntr = srcp;
    while(height--) {
    	GLINT_WAIT(dwords);
    	/* 0x0D is the TAG value for BitMaskPattern */
    	GLINT_WRITE_REG(((dwords - 1) << 16) | 0x0D, OutputFIFO);
   	MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
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
    SET_SYNC_FLAG;	
}

#define MAX_GLYPHS	1024			/* that's gotta be enough */
static unsigned int* Glyphs[MAX_GLYPHS]; 

void
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
		(unsigned int*)((char*)GLINTMMIOBase + OutputFIFO + 4),
 		Glyphs, count, nglyph, glyphWidth);
    }
#else
    for(count = 0; count < h; count++) {
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)GLINTMMIOBase + BitMaskPattern),
 		Glyphs, count, nglyph, glyphWidth);
    }
#endif
    SET_SYNC_FLAG;	
}


void
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
 	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
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
		(unsigned int*)((char*)GLINTMMIOBase + OutputFIFO + 4),
 		Glyphs, count, nglyph, glyphWidth);
    }
#else
    for(count = 0; count < h; count++) { 
	xf86DrawTextScanlineFixedBase((unsigned int*)(
 		(char*)GLINTMMIOBase + BitMaskPattern),
 		Glyphs, count, nglyph, glyphWidth);
    }
#endif
    SET_SYNC_FLAG;	
}

#define MAX_FIFO_ENTRIES 256

void
Permedia2DoImageWrite8bpp(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords, count;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    unsigned char* srcPntr;			/* index into the image */
    CARD32* srcp;
    unsigned char *srcpbyte;
    int Bpp = glintInfoRec.bitsPerPixel >> 3; 
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool FastTexLoad;

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (alu == GXcopy) {      
	GLINT_WRITE_REG(pprod | FBRM_Packed, FBReadMode);
    } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	FastTexLoad = FALSE;
	dwords = (w + 3) >> 2;
	if((!(x&3)) && (!(w&3))) FastTexLoad = TRUE;	
	if((alu != GXcopy) || (planemask != ~0))
		FastTexLoad = FALSE;

	if (alu == GXcopy) {
	  skipleft = 0;
	} else {
	  if((skipleft = (long)srcPntr & 0x03)) {
	 	if(Bpp == 3)
			skipleft = 4 - skipleft;
	    	else
	    		skipleft /= Bpp;

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	if(Bpp == 3)
	    	   srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    	else   
	    	   srcPntr = (unsigned char*)((long)srcPntr & ~0x03); 
	  }
	}
	
        if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  GLINTSync();	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = ((y * glintInfoRec.displayWidth) + x) >> 2;
	      srcp = (CARD32*)srcPntr;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
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
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
  	   int lowerbits = bppand[(glintInfoRec.bitsPerPixel>>3)-1];
  	   char align = (x & lowerbits);
		
    	   if (skipleft)
		Permedia2SetClippingRectangle(x+skipleft,y,x+w,y+h);

	   if (alu == GXcopy) {
	     GLINT_WAIT(6);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF)>>Bppshift, RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|(w+lowerbits)>>Bppshift, RectangleSize);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(alu<<1|UNIT_ENABLE, LogicalOpMode);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   if (alu == GXcopy) {
	    while(h--) {
	      count = dwords;
	      srcp = (CARD32*)srcPntr;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	    }  
	   } else {
	    while(h--) {
	      count = w;
	      srcpbyte = (unsigned char *)srcPntr;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcpbyte += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveBYTE((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(unsigned char *)srcpbyte, count);
	      }
	      srcPntr += srcwidth;
	    }  
	   }
	}
    }

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG;
}

void
Permedia2DoImageWrite16bpp(pSrc,pDst,alu,prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords, count;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    unsigned char* srcPntr;			/* index into the image */
    CARD32* srcp;
    unsigned short* srcpword;
    int Bpp = glintInfoRec.bitsPerPixel >> 3; 
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool FastTexLoad;

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (alu == GXcopy) {      
	GLINT_WRITE_REG(pprod | FBRM_Packed, FBReadMode);
    } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	FastTexLoad = FALSE;
	dwords = (w + 1) >> 1;
	if((!(x&1)) && (!(w&1))) FastTexLoad = TRUE;
	if((alu != GXcopy) || (planemask != ~0))
		FastTexLoad = FALSE;

	if (alu == GXcopy) {
	  skipleft = 0;
	} else {
	  if((skipleft = (long)srcPntr & 0x03)) {
	   	if(Bpp == 3)
			skipleft = 4 - skipleft;
	    	else
	    		skipleft /= Bpp;

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	if(Bpp == 3)
	    	   srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    	else   
	    	   srcPntr = (unsigned char*)((long)srcPntr & ~0x03); 
	  }
	}
	
        if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  GLINTSync();	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = ((y * glintInfoRec.displayWidth) + x) >> 1;
	      srcp = (CARD32*)srcPntr;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
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
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
  	   int lowerbits = bppand[(glintInfoRec.bitsPerPixel>>3)-1];
  	   char align = (x & lowerbits);
		
    	   if (skipleft)
		Permedia2SetClippingRectangle(x+skipleft,y,x+w,y+h);

	   GLINT_WAIT(6);
	   if (alu == GXcopy) {
	     GLINT_WAIT(6);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF)>>Bppshift, RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|(w+lowerbits)>>Bppshift, RectangleSize);
  	     GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
	   } else {
	     GLINT_WAIT(5);
	     GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	     GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   }
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(alu<<1|UNIT_ENABLE, LogicalOpMode);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   if (alu == GXcopy) {
	    while(h--) {
	      count = dwords;
	      srcp = (CARD32*)srcPntr;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	    }  
	   } else {
	    while(h--) {
	      count = w;
	      srcpword = (unsigned short *)srcPntr;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(unsigned short *)srcpword, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcpword += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(unsigned short *)srcpword, count);
	      }
	      srcPntr += srcwidth;
	    }  
	   }
	}
    }

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG;
}

void
Permedia2DoImageWrite32bpp(pSrc,pDst,alu,prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords, count;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    unsigned char* srcPntr;			/* index into the image */
    CARD32* srcp;
    int Bpp = glintInfoRec.bitsPerPixel >> 3; 
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool FastTexLoad;

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (alu == GXcopy) {      
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	FastTexLoad = TRUE;
	dwords = w;
	if((alu != GXcopy) || (planemask != ~0))
		FastTexLoad = FALSE;
	
	if (!FastTexLoad) {
	  if((skipleft = (long)srcPntr & 0x03)) {
	   	if(Bpp == 3)
			skipleft = 4 - skipleft;
	    	else
	    		skipleft /= Bpp;

	    	x -= skipleft;	     
	    	w += skipleft;
	
	    	if(Bpp == 3)
	    	   srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    	else   
	    	   srcPntr = (unsigned char*)((long)srcPntr & ~0x03); 
	  }
	}
	
        if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  GLINTSync();	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = (y * glintInfoRec.displayWidth) + x;
	      srcp = (CARD32*)srcPntr;
	      GLINT_WRITE_REG(address, TextureDownloadOffset);
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x11 << 4) | 0x0D is the TAG for TextureData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x11 << 4) |
						0x0D, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
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
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	      y++;
	  }
	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	} else {
    	   if (skipleft)
		Permedia2SetClippingRectangle(x+skipleft,y,x+w,y+h);

	   GLINT_WAIT(6);
	   GLINT_WRITE_REG((y<<16)|(x&0xFFFF), RectangleOrigin);
	   GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	   GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	   GLINT_WRITE_REG(alu<<1|UNIT_ENABLE, LogicalOpMode);
  	   GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
				SyncOnHostData, Render);

	   while(h--) {
	      count = dwords;
	      srcp = (CARD32*)srcPntr;
	      while(count >= MAX_FIFO_ENTRIES) {
    	    	GLINT_WAIT(MAX_FIFO_ENTRIES);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((MAX_FIFO_ENTRIES - 2) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, MAX_FIFO_ENTRIES - 1);
		count -= MAX_FIFO_ENTRIES - 1;
		srcp += MAX_FIFO_ENTRIES - 1;
	      }
	      if(count) {
    	    	GLINT_WAIT(count + 1);
		/* (0x15 << 4) | 0x05 is the TAG for FBSourceData */
        	GLINT_WRITE_REG(((count - 1) << 16) | (0x15 << 4) | 
					0x05, OutputFIFO);
		MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	 		(CARD32*)srcp, count);
	      }
	      srcPntr += srcwidth;
	   }  
	}
    }

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, ScissorMode);
    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG;
}
