/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/pm2_accel.c,v 1.1 1997/12/06 07:52:49 hohndel Exp $ */
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
#include "mispans.h"

#include "glint_regs.h"
#include "glint.h"
#include "xf86xaa.h"

#include <float.h>

#if DEBUG
#include <stdio.h>
#endif

extern int	pprod;
extern int      ScanlineWordCount;
static short    DashPattern;
static int      DashPatternSize;
extern CARD32   ScratchBuffer[512];
static CARD32	ImageWriteBuffer[8192];
static int      blitxdir, blitydir;
static int      mode;
static int	span;
static int      blitmode;
static int	blitmodec;
static char     clip;
static int      clipmin, clipmax;
static int      gcolor;
static int      grop;
static int      gtransparency;
static int      gbg, gfg, gstrw;
extern          GLINTWindowBase;

extern void     GLINTSync ();
extern void     GLINTSubsequentFillTrapezoidSolid ();
extern void     GLINTSetupForDashedLine ();
extern void     GLINTSubsequentDashedTwoPointLine ();
extern void     GLINTSetClippingRectangle ();
extern void	GLINTBresenhamLine();

void            Permedia2SubsequentScanlineScreenToScreenCopy ();
void            Permedia2SetupForFillRectSolid();
void		Permedia2SubsequentFillRectSolid();
void 		Permedia2SubsequentScreenToScreenCopy ();
void            Permedia2SetupForScreenToScreenCopy ();
void            Permedia2SubsequentTwoPointLine ();
void		Permedia2SetupForFill8x8Pattern ();
void     	Permedia2SubsequentFill8x8Pattern ();
void            Permedia2SetupFor8x8PatternColorExpand ();
void		Permedia2Subsequent8x8PatternColorExpand();
void            Permedia2SetupForScanlineScreenToScreenColorExpand ();
void		Permedia2SubsequentScanlineScreenToScreenColorExpand ();
void            Permedia2SetClippingRectangle (int x1, int y1, int x2, int y2);
void		Permedia2SetupForImageWrite();
void		Permedia2SubsequentImageWrite();

void
Permedia2AccelInit ()
{
  xf86AccelInfoRec.Flags = PIXMAP_CACHE |
#if 0
    HARDWARE_CLIP_LINE |
    HORIZONTAL_TWOPOINTLINE |
    USE_TWO_POINT_LINE |
    TWO_POINT_LINE_NOT_LAST |
#endif
    BACKGROUND_OPERATIONS |
    COP_FRAMEBUFFER_CONCURRENCY |
    DELAYED_SYNC;

  xf86AccelInfoRec.PatternFlags =
    HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    HARDWARE_PATTERN_PROGRAMMED_BITS |
    HARDWARE_PATTERN_MONO_TRANSPARENCY |
    HARDWARE_PATTERN_SCREEN_ORIGIN;

  xf86AccelInfoRec.Sync = GLINTSync;

  xf86GCInfoRec.PolyFillRectSolidFlags = 0;
  xf86AccelInfoRec.SetupForFillRectSolid = Permedia2SetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = Permedia2SubsequentFillRectSolid;
#if 0
  xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;
#endif

  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY; 

  xf86AccelInfoRec.SetupForScreenToScreenCopy = Permedia2SetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy = Permedia2SubsequentScreenToScreenCopy;

  xf86AccelInfoRec.SetClippingRectangle = Permedia2SetClippingRectangle;
#if 0
  xf86AccelInfoRec.SubsequentTwoPointLine = Permedia2SubsequentTwoPointLine;
#endif

  xf86AccelInfoRec.ColorExpandFlags =
    VIDEO_SOURCE_GRANULARITY_PIXEL |
    NO_SYNC_AFTER_CPU_COLOR_EXPAND |
    CPU_TRANSFER_PAD_DWORD |
    BIT_ORDER_IN_BYTE_LSBFIRST;

  xf86AccelInfoRec.ScratchBufferAddr = 1;
  xf86AccelInfoRec.ScratchBufferSize = 512;
  xf86AccelInfoRec.ScratchBufferBase = (void *) ScratchBuffer;
  xf86AccelInfoRec.PingPongBuffers = 1;

  xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    Permedia2SetupForScanlineScreenToScreenColorExpand;
  xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
    Permedia2SubsequentScanlineScreenToScreenColorExpand;
  
#if 0
  xf86AccelInfoRec.SetupForFill8x8Pattern = Permedia2SetupForFill8x8Pattern;
  xf86AccelInfoRec.SubsequentFill8x8Pattern = Permedia2SubsequentFill8x8Pattern;
#endif

  xf86AccelInfoRec.SetupFor8x8PatternColorExpand = Permedia2SetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand = Permedia2Subsequent8x8PatternColorExpand;

#if 0
  xf86AccelInfoRec.ImageWriteBase = (void *) ImageWriteBuffer;
  xf86AccelInfoRec.ImageWriteRange = glintInfoRec.displayWidth * 4;
  xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY;
  xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.displayWidth *
	glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel / 8);
  xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;
  xf86AccelInfoRec.SetupForImageWrite = Permedia2SetupForImageWrite;
  xf86AccelInfoRec.SubsequentImageWrite = Permedia2SubsequentImageWrite;
#else
  xf86AccelInfoRec.ImageWriteOffset = glintInfoRec.displayWidth *
	glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel / 8);

  /* 4 scanlines should be enough */
  xf86AccelInfoRec.ImageWriteRange = glintInfoRec.displayWidth * 4 *
	(glintInfoRec.bitsPerPixel / 8);

  xf86AccelInfoRec.PixmapCacheMemoryStart =
	xf86AccelInfoRec.ImageWriteOffset + xf86AccelInfoRec.ImageWriteRange;

  xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;

  /* Check Pixmap cache and if enough room, enable ImageWrites */
  if ((xf86AccelInfoRec.PixmapCacheMemoryEnd -
	   xf86AccelInfoRec.ImageWriteOffset) >
		  xf86AccelInfoRec.ImageWriteRange)
  {
	  xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY;
	  xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy =
	    Permedia2SubsequentScanlineScreenToScreenCopy;
  }
#endif
}

void
Permedia2SetClippingRectangle (int x1, int y1, int x2, int y2)
{
  GLINT_WRITE_REG ((y1<<16)|x1, ScissorMinXY);
  GLINT_WRITE_REG ((y2<<16)|x2, ScissorMaxXY);
  GLINT_WRITE_REG (1, ScissorMode);
}


void
Permedia2SetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  blitxdir = xdir;
  blitydir = ydir;
  
  grop = rop;

  DO_PLANEMASK(planemask);

  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);

  if ((rop == GXclear) || (rop == GXset)) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
  	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
 	} else {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | blitmode, FBReadMode);
	}
	GLINT_WRITE_REG(0, WaitForCompletion);
  }
  GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
}

void
Permedia2SubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  int srcaddr = y1 * glintInfoRec.displayWidth + x1;
  int dstaddr = y2 * glintInfoRec.displayWidth + x2;

  GLINT_WRITE_REG (srcaddr - dstaddr, FBSourceOffset);

  GLINT_WRITE_REG((y2<<16)|x2, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  mode = 0;
  if (blitxdir >= 0) mode |= XPositive;
  if (blitydir >= 0) mode |= YPositive;
  GLINT_WRITE_REG (PrimitiveRectangle | mode, Render);

  GLINT_WRITE_REG(0, FBSourceOffset);
}

void Permedia2SetupForFillRectSolid(int color, int rop, unsigned planemask)
{
  REPLICATE(color);
  gcolor = color;

  DO_PLANEMASK(planemask);
  GLINT_WRITE_REG(0, RasterizerMode);
  grop = rop;
  if (rop == GXcopy) {
	mode = FastFillEnable;
	GLINT_WRITE_REG(pprod, FBReadMode);
	GLINT_WRITE_REG(color, FBBlockColor);
  } else {
	mode = 0;
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
      	GLINT_WRITE_REG(color, ConstantColor);
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void Permedia2SubsequentFillRectSolid(int x, int y, int w, int h)
{
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | mode, Render);
}

void
Permedia2SetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  DO_PLANEMASK(planemask);
  gfg = fg;
  gbg = bg;
  REPLICATE (gfg);
  REPLICATE (gbg);
  grop = rop;

  GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);
  ScanlineWordCount = (w + 31) >> 5;
  if (rop == GXcopy) {
      GLINT_WRITE_REG(0, RasterizerMode);
      GLINT_WRITE_REG(pprod, FBReadMode);
      if (bg != -1) {
    	GLINT_WRITE_REG(gbg, FBBlockColor);
        GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
						FastFillEnable, Render);
      }
      GLINT_WRITE_REG(gfg, FBBlockColor);
      GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
					SyncOnBitMask | FastFillEnable, Render);
  } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	GLINT_WRITE_REG(gfg, ConstantColor);
	if (bg != -1) {
  		GLINT_WRITE_REG(BitMaskPackingEachScanline|
					ForceBackgroundColor, RasterizerMode);
		GLINT_WRITE_REG(gbg, Texel0);
	} else {
  		GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	}
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |
							SyncOnBitMask, Render);
  }
}

void
Permedia2SubsequentScanlineScreenToScreenColorExpand(int srcaddr)
{
	register CARD32 *ptr = (CARD32 *) ScratchBuffer;
	int count = ScanlineWordCount; 

	while (count--) {
		GLINT_WRITE_REG(*(ptr++), BitMaskPattern);
	}
}

void
Permedia2SetupForFill8x8Pattern(patternx, patterny, rop, planemask,transparency_color)
	int patternx, patterny, rop, planemask, transparency_color;
{
  GLINT_WRITE_REG(0,         dXDom);
  GLINT_WRITE_REG(0,         dXSub);
  GLINT_WRITE_REG(1<<16,     dY);
  GLINT_WRITE_REG(patternx, StartXDom);
  GLINT_WRITE_REG(patterny, StartY);
  GLINT_WRITE_REG((patternx+8), StartXSub);
  GLINT_WRITE_REG(8, GLINTCount);
	
  DO_PLANEMASK(planemask);

  if (rop == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void 
Permedia2SubsequentFill8x8Pattern(int patternx, int patterny, int x, int y,
				   int w, int h)
{
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,     StartXDom);
	GLINT_WRITE_REG(y<<16,     StartY);
	GLINT_WRITE_REG(h,         GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid|TextureEnable,Render);
}

void 
Permedia2SetupFor8x8PatternColorExpand(int patternx, int patterny, 
					   int bg, int fg, int rop,
					   unsigned planemask)
{
  unsigned long a,b,c,d,e,f,g,h;

  DO_PLANEMASK(planemask);
  if (bg == -1) mode = -1;
	else    mode = 0;
  REPLICATE(fg);
  REPLICATE(bg);

  gfg = fg;
  gbg = bg;
  

  a = (patternx & 0xFF); 		a = a | a << 8 | a << 16 | a << 24;
  b = (patternx & 0xFF00) >> 8;		b = b | b << 8 | b << 16 | b << 24;
  c = (patternx & 0xFF0000) >> 16;	c = c | c << 8 | c << 16 | c << 24;
  d = (patternx & 0xFF000000) >> 24;	d = d | d << 8 | d << 16 | d << 24;
  e = (patterny & 0xFF); 		e = e | e << 8 | e << 16 | e << 24;
  f = (patterny & 0xFF00) >> 8;		f = f | f << 8 | f << 16 | f << 24;
  g = (patterny & 0xFF0000) >> 16;	g = g | g << 8 | g << 16 | g << 24;
  h = (patterny & 0xFF000000) >> 24;	h = h | h << 8 | h << 16 | h << 24;

  GLINT_WRITE_REG(a, AreaStipplePattern0);
  GLINT_WRITE_REG(b, AreaStipplePattern1);
  GLINT_WRITE_REG(c, AreaStipplePattern2);
  GLINT_WRITE_REG(d, AreaStipplePattern3);
  GLINT_WRITE_REG(e, AreaStipplePattern4);
  GLINT_WRITE_REG(f, AreaStipplePattern5);
  GLINT_WRITE_REG(g, AreaStipplePattern6);
  GLINT_WRITE_REG(h, AreaStipplePattern7);
  
  grop = rop;

  if (rop == GXcopy) {
      GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
      GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
}

void 
Permedia2Subsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  int span = 0;
  
  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((h<<16)|w, RectangleSize);

  if (mode != -1) {
	if (miSpansEasyRop(grop)) {
  	  if (grop == GXcopy) {
		GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
		GLINT_WRITE_REG(gbg, FBBlockColor);
		mode = FastFillEnable;
	  } else {
		GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		GLINT_WRITE_REG(gbg, ConstantColor);
		mode = 0;
	  }
	  GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | 
								mode,Render);
	} else {
  	  	if (grop == GXcopy) {
			GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
			GLINT_WRITE_REG(gbg, FBBlockColor);
			span = FastFillEnable;
		} else {
			GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  			GLINT_WRITE_REG(gbg, ConstantColor);
			span = 0;
		}
		GLINT_WRITE_REG(patternx<<7|patterny<<12| ASM_InvertPattern |
			UNIT_ENABLE, AreaStippleMode);
		GLINT_WRITE_REG(AreaStippleEnable | span | XPositive | 
					YPositive | PrimitiveRectangle, Render);
	}
  }

  if (grop == GXcopy) {
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(gfg, FBBlockColor);
	span = FastFillEnable;
  } else {
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  	GLINT_WRITE_REG(gfg, ColorDDAMode);
	span = 0;
  }
  GLINT_WRITE_REG(patternx<<7|patterny<<12|
  						UNIT_ENABLE, AreaStippleMode);
  GLINT_WRITE_REG(AreaStippleEnable | span | XPositive | YPositive |
						PrimitiveRectangle, Render);
}

void
Permedia2SubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
{
  register int    dx, dy, adx, ady;

  dx = x2 - x1;
  dy = y2 - y1;
  adx = abs (dx);
  ady = abs (dy);


  /* same setup as trapezoid ! */
  GLINT_WRITE_REG (gcolor, GLINTColor);
  GLINT_WRITE_REG (x1 << 16, StartXDom);
  GLINT_WRITE_REG (y1 << 16, StartY);

  if (!dx && !dy)
    {
#ifdef GLINT_3D
      run_demo ();
      GLINT_WRITE_REG (gcolor, GLINTColor);
      GLINT_WRITE_REG (x1 << 16, StartXDom);
      GLINT_WRITE_REG (y1 << 16, StartY);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
#endif
      /* This is a point */
      GLINT_WRITE_REG ((mode & ~PrimitiveTrapezoid) | PrimitivePoint, Render);
      return;
    }
  /*
  if (adx && ady && ((adx + ady) < 14))
    {
      GLINTBresenhamLine (x1, y1, x2, y2);
      return;
    }
    */
  if (adx >= ady)
    {				/* XMajor axis (i.e. Horizontal) */
      if (dx < 0)
	{
	  GLINT_WRITE_REG (-1 << 16, dXDom);
	  dx = adx;
	}
      else
	GLINT_WRITE_REG (1 << 16, dXDom);

      if (!dx)
	{
	  adx = dx = 1;
	  GLINT_WRITE_REG (0, dY);
	}
      else
	GLINT_WRITE_REG ((dy << 16) / dx, dY);

      GLINT_WRITE_REG (adx, GLINTCount);
    }
  else
    {				/* YMajor axis  (i.e. Vertical) */
      if (dy < 0)
	{
	  GLINT_WRITE_REG (-1 << 16, dY);
	  dy = ady;
	}
      else
	GLINT_WRITE_REG (1 << 16, dY);

      GLINT_WRITE_REG ((!dy ? 0 : (dx << 16) / dy), dXDom);
      GLINT_WRITE_REG (ady + 1, GLINTCount);
    }

  /* can't overwrite the mode !
     have the same setup funcion as Trapezoid ! */
  GLINT_WRITE_REG (FastFillEnable | PrimitiveLine, Render);
}

void
Permedia2SubsequentScanlineScreenToScreenCopy (LineAddr, skipleft, x, y, w)
     int             LineAddr, skipleft, x, y, w;
{
  int dstaddr = y * glintInfoRec.displayWidth + x;

  GLINT_WRITE_REG ((LineAddr / (glintInfoRec.bitsPerPixel / 8)) - dstaddr, 
		   FBSourceOffset);

  GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
  GLINT_WRITE_REG((1<<16)|w, RectangleSize);
  GLINT_WRITE_REG(PrimitiveRectangle | XPositive, Render);
  GLINT_WRITE_REG(0, FBSourceOffset);
}

void
Permedia2SetupForImageWrite(rop, planemask, transparency_color)
{
	DO_PLANEMASK(planemask);
	
	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
	}

	GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void
Permedia2SubsequentImageWrite(x, y, w, h, skipleft)
{
	register CARD32 *ptr = (CARD32 *) ImageWriteBuffer;
	int count = ((w * h) + 31) >> 5;
	GLINT_WRITE_REG(y * glintInfoRec.displayWidth + x, TextureDownloadOffset);
#if 0
#if 1
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,     StartXDom);
	GLINT_WRITE_REG(y<<16,     StartY);
	GLINT_WRITE_REG(h,         GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid | TextureEnable, Render);
#else
	GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
	GLINT_WRITE_REG((h<<16)|w, RectangleSize);
	GLINT_WRITE_REG(PrimitiveRectangle | SyncOnHostData, Render);
#endif
#endif
	while (count--) {
		GLINT_WRITE_REG(*(ptr++), TextureData);
	}
}
