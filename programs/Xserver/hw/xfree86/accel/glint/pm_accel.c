/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/pm_accel.c,v 1.4 1997/12/05 22:01:31 hohndel Exp $ */

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
 * Permedia accelerated options.
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
static int      blitxdir, blitydir;
static int      mode;
static int	span;
static int      blitmode;
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
extern void     GLINTSetupForFill8x8Pattern ();
extern void     GLINTSubsequentFill8x8Pattern ();
extern void     GLINTSubsequentScanlineScreenToScreenColorExpand ();

void            PermediaSubsequentScanlineScreenToScreenCopy ();
void            PermediaSetupForFillRectSolid();
void		PermediaSubsequentFillRectSolid();
void 		PermediaSubsequentScreenToScreenCopy ();
void            PermediaSetupForScreenToScreenCopy ();
void            PermediaSubsequentTwoPointLine ();
void            PermediaSetupFor8x8PatternColorExpand ();
void		PermediaSubsequent8x8PatternColorExpand();
void            PermediaSetupForScanlineScreenToScreenColorExpand ();
void            GLINTBresenhamLine (int x1, int y1, int x2, int y2);
void            PermediaSetClippingRectangle (int x1, int y1, int x2, int y2);
void		PermediaWriteBitmap();
void		PermediaFillRectStippled();

void
PermediaAccelInit ()
{
  xf86AccelInfoRec.Flags = PIXMAP_CACHE |
#if 0
    HARDWARE_CLIP_LINE |
    HORIZONTAL_TWOPOINTLINE |
    USE_TWO_POINT_LINE |
    TWO_POINT_LINE_NOT_LAST |
#endif
    BACKGROUND_OPERATIONS |
    ONLY_LEFT_TO_RIGHT_BITBLT |
#if 0
    DO_NOT_BLIT_STIPPLES |
#endif
    COP_FRAMEBUFFER_CONCURRENCY |
    DELAYED_SYNC;

#if 1
  xf86AccelInfoRec.PatternFlags =
    HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    HARDWARE_PATTERN_PROGRAMMED_BITS |
    HARDWARE_PATTERN_SCREEN_ORIGIN;
#endif

  xf86AccelInfoRec.Sync = GLINTSync;

  xf86GCInfoRec.PolyFillRectSolidFlags = 0;
  xf86AccelInfoRec.SetupForFillRectSolid = PermediaSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = PermediaSubsequentFillRectSolid;
#if 0
  xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;
#endif

  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY; 

  xf86AccelInfoRec.SetupForScreenToScreenCopy = PermediaSetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy = PermediaSubsequentScreenToScreenCopy;

  xf86AccelInfoRec.SetClippingRectangle = PermediaSetClippingRectangle;
#if 0
  xf86AccelInfoRec.SubsequentTwoPointLine = PermediaSubsequentTwoPointLine;
#endif

  xf86AccelInfoRec.ColorExpandFlags =
    VIDEO_SOURCE_GRANULARITY_PIXEL |
    NO_SYNC_AFTER_CPU_COLOR_EXPAND |
    CPU_TRANSFER_PAD_DWORD |
    ONLY_TRANSPARENCY_SUPPORTED |
    BIT_ORDER_IN_BYTE_LSBFIRST;

  xf86AccelInfoRec.ScratchBufferAddr = 1;
  xf86AccelInfoRec.ScratchBufferSize = 512;
  xf86AccelInfoRec.ScratchBufferBase = (void *) ScratchBuffer;
  xf86AccelInfoRec.PingPongBuffers = 1;

  xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    PermediaSetupForScanlineScreenToScreenColorExpand;
  xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
    GLINTSubsequentScanlineScreenToScreenColorExpand;
  
#if 0
  xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
  xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
#endif

#if 1
  xf86AccelInfoRec.SetupFor8x8PatternColorExpand = PermediaSetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand = PermediaSubsequent8x8PatternColorExpand;
#endif

#if 0
  xf86AccelInfoRec.WriteBitmap = PermediaWriteBitmap;
  xf86AccelInfoRec.FillRectStippled = PermediaFillRectStippled;
  xf86AccelInfoRec.FillRectOpaqueStippled = PermediaFillRectStippled;
  xf86GCInfoRec.SecondaryPolyFillRectStippledFlags = 0;
  xf86GCInfoRec.SecondaryPolyFillRectOpaqueStippledFlags = 0;
#endif
 
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
	    PermediaSubsequentScanlineScreenToScreenCopy;
  }
}

void
PermediaSetClippingRectangle (int x1, int y1, int x2, int y2)
{
  GLINT_WRITE_REG ((y1 << 16) | x1, ScissorMinXY);
  GLINT_WRITE_REG ((y2 << 16) | x2, ScissorMaxXY);
  GLINT_WRITE_REG (1, ScissorMode);
}


void
PermediaSetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  blitydir = ydir;
  blitxdir = xdir;
  grop = rop;

  DO_PLANEMASK(planemask);

  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);

#if 1
  if ((rop == GXclear) || (rop == GXset)) {
	GLINT_WRITE_REG(pprod, FBReadMode);
	mode = 0;
  } else {
  	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
 	} else {
      		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | blitmode, FBReadMode);
  	}
	mode = 1;
  }
#endif

  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);
}

void
PermediaSubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  int srcaddr = y1 * glintInfoRec.displayWidth + x1;
  int dstaddr = y2 * glintInfoRec.displayWidth + x2;
  int align = (dstaddr - srcaddr) & 7;
  /* Permedia only allows left to right bitblt's */

  if (blitydir < 0)
    {
      y1 = y1 + h - 1;
      y2 = y2 + h - 1;
      GLINT_WRITE_REG (-1 << 16, dY);
    }
  else
    {
      GLINT_WRITE_REG (1 << 16, dY);
    }

#if 0
  if (mode == 1) {
	if ((w % 4) == 0) {
		w >>= 2;
  		if ((grop == GXcopy) || (grop == GXcopyInverted)) {
			GLINT_WRITE_REG(1<<19 | pprod | FBRM_SrcEnable, FBReadMode);
 		} else {
      			GLINT_WRITE_REG(1<<19 | pprod | FBRM_SrcEnable | blitmode, FBReadMode);
  		}
	        GLINT_WRITE_REG(0, PackedDataLimits);
	} else {
  	if ((grop == GXcopy) || (grop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
 	} else {
      		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | blitmode, FBReadMode);
  	}
	}
  }
#endif

  GLINT_WRITE_REG (srcaddr - dstaddr, FBSourceOffset);
  GLINT_WRITE_REG (x2<<16, StartXDom);
  GLINT_WRITE_REG ((x2+w)<<16, StartXSub);
  GLINT_WRITE_REG (y2 << 16, StartY);
  GLINT_WRITE_REG (h, GLINTCount);

  GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
  GLINT_WRITE_REG(0, FBSourceOffset);
}

void PermediaSetupForFillRectSolid(int color, int rop, unsigned planemask)
{
  REPLICATE(color);
  gcolor = color;

  GLINT_WRITE_REG(0, RasterizerMode);
  DO_PLANEMASK(planemask);
  if (rop == GXcopy) {
	mode = FastFillEnable;
  	GLINT_WRITE_REG(pprod, FBReadMode);
  	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(color, FBBlockColor);
  } else {
	mode = 0;
      	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
      	GLINT_WRITE_REG(color, ConstantColor);
      	GLINT_WRITE_REG(CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16, dY);
  GLINT_WRITE_REG(0, dXDom);
  GLINT_WRITE_REG(0, dXSub);
  GLINT_WRITE_REG(0, StartXSub);
}

void PermediaSubsequentFillRectSolid(int x, int y, int w, int h)
{
  GLINT_WRITE_REG(x<<16, StartXDom);
  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG(h, GLINTCount);
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(PrimitiveTrapezoid | mode, Render);
}

void
PermediaSetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  REPLICATE (fg);
  REPLICATE (bg);
  DO_PLANEMASK(planemask);

  if (rop == GXcopy) {
	mode = FastFillEnable;
  	GLINT_WRITE_REG(0, RasterizerMode);
  	GLINT_WRITE_REG(fg, FBBlockColor);
	GLINT_WRITE_REG(pprod, FBReadMode);
  	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
  } else {
	mode = 0;
	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(fg, GLINTColor);
  }
  GLINT_WRITE_REG((rop<<1)|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(0, dXDom);
  GLINT_WRITE_REG(0, dXSub);
  GLINT_WRITE_REG(x << 16, StartXDom);
  GLINT_WRITE_REG(y << 16, StartY);
  GLINT_WRITE_REG((x + w) << 16, StartXSub);
  GLINT_WRITE_REG(h, GLINTCount);
  GLINT_WRITE_REG(1 << 16, dY);
  ScanlineWordCount = (w + 31) >> 5;
  GLINT_WRITE_REG(PrimitiveTrapezoid | SyncOnBitMask | mode, Render);
}

void
PermediaSetupFor8x8PatternColorExpand (int patternx, int patterny,
				       int bg, int fg, int rop,
				       unsigned planemask)
{
  DO_PLANEMASK(planemask);
  gfg = fg;
  gbg = bg;
  REPLICATE(gfg);
  REPLICATE(gbg);
  grop = rop;

  GLINT_WRITE_REG ((patternx & 0x000000ff), AreaStipplePattern0);
  GLINT_WRITE_REG ((patternx & 0x0000ff00) >> 8, AreaStipplePattern1);
  GLINT_WRITE_REG ((patternx & 0x00ff0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG ((patternx & 0xff000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG ((patterny & 0x000000ff), AreaStipplePattern4);
  GLINT_WRITE_REG ((patterny & 0x0000ff00) >> 8, AreaStipplePattern5);
  GLINT_WRITE_REG ((patterny & 0x00ff0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG ((patterny & 0xff000000) >> 24, AreaStipplePattern7);

  GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
  if (rop == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
	GLINT_WRITE_REG(gfg, ConstantColor);
	GLINT_WRITE_REG(0, PMTextureReadMode);
	GLINT_WRITE_REG(gbg, Texel0);
  }
  GLINT_WRITE_REG((rop << 1) | UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1 << 16, dY);
}

void 
PermediaSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(x<<16,     StartXDom);
  GLINT_WRITE_REG(y<<16,     StartY);
  GLINT_WRITE_REG(h,         GLINTCount);

  if (grop == GXcopy) {
   GLINT_WRITE_REG(gbg, FBBlockColor);
   GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable, Render);
   GLINT_WRITE_REG(patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
   GLINT_WRITE_REG(gfg, ConstantColor);
  } else {
   GLINT_WRITE_REG(1<<20|patternx<<7|patterny<<12|UNIT_ENABLE, AreaStippleMode);
  }
  GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid, Render);
}

void
PermediaSubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
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
  GLINT_WRITE_REG ((mode & ~PrimitiveTrapezoid) | PrimitiveLine, Render);
}



void
GLINTBresenhamLine (int x1, int y1, int x2, int y2)
{
  /* This code is from bresenham.c Mesa 3-D graphics library */
  /* by Brain Paul */
  register int    dx, dy, xf, yf, a, b, c, i;

  if (x2 > x1)
    {
      dx = x2 - x1;
      xf = 1;
    }
  else
    {
      dx = x1 - x2;
      xf = -1;
    }

  if (y2 > y1)
    {
      dy = y2 - y1;
      yf = 1;
    }
  else
    {
      dy = y1 - y2;
      yf = -1;
    }

  GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
  GLINT_WRITE_REG (gcolor, GLINTColor);

  if (dx > dy)
    {
      a = dy + dy;
      c = a - dx;
      b = c - dx;
      for (i = 0; i <= dx; i++)
	{
	  GLINT_WRITE_REG (x1 << 16, StartXDom);
	  GLINT_WRITE_REG (y1 << 16, StartY);
	  GLINT_WRITE_REG ((mode & ~PrimitiveTrapezoid) | PrimitivePoint, Render);
	  x1 += xf;
	  if (c < 0)
	    {
	      c += a;
	    }
	  else
	    {
	      c += b;
	      y1 += yf;
	    }
	}
      return;
    }
  else
    {
      a = dx + dx;
      c = a - dy;
      b = c - dy;
      for (i = 0; i <= dy; i++)
	{
	  GLINT_WRITE_REG (x1 << 16, StartXDom);
	  GLINT_WRITE_REG (y1 << 16, StartY);
	  GLINT_WRITE_REG ((mode & ~PrimitiveTrapezoid) | PrimitivePoint, Render);
	  y1 += yf;
	  if (c < 0)
	    {
	      c += a;
	    }
	  else
	    {
	      c += b;
	      x1 += xf;
	    }
	}
      return;
    }
}

void
PermediaSubsequentScanlineScreenToScreenCopy (LineAddr, skipleft, x, y, w)
     int             LineAddr, skipleft, x, y, w;
{
  int dstaddr = y * glintInfoRec.displayWidth + x;

  GLINT_WRITE_REG ((LineAddr / (glintInfoRec.bitsPerPixel / 8)) - dstaddr, 
		   FBSourceOffset);

  GLINT_WRITE_REG (-1 << 16, dY);
  GLINT_WRITE_REG ((x + w) << 16, StartXDom);
  GLINT_WRITE_REG (x << 16, StartXSub);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG (1, GLINTCount);

  GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
  GLINT_WRITE_REG (0, FBSourceOffset);
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

PermediaStippleCPUToScreen(x, y, w, h, src, srcwidth,
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

#ifdef QWORD_PADDING
    Bool PlusOne = ((dwords * h) & 0x01);
#endif 

   /*  >>>>>>  Initiate the transfer (x,y,w,h) <<<<<<<< */
  GLINT_WRITE_REG(0, dXDom);
  GLINT_WRITE_REG(0, dXSub);
  GLINT_WRITE_REG(x<<16, StartXDom);
  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(h, GLINTCount);
  GLINT_WRITE_REG(1<<16, dY);
  GLINT_WRITE_REG(PrimitiveTrapezoid | span | SyncOnBitMask, Render);

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

    GLINT_WRITE_REG(0, WaitForCompletion);
#ifdef QWORD_PADDING
    if(PlusOne) {
	GLINT_WRITE_REG(0, BitMaskPattern); 
    }
#endif 
}

void
PermediaFillRectStippled(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int nBoxInit;		/* number of rectangles to fill */
    BoxPtr pBoxInit;		/* Pointer to first rectangle to fill */
{
    int rectWidth, rectHeight;
    BoxPtr pBox;
    int i;
    int color;
    int nBox;
    int xoffset, yoffset, StippleType;
    PixmapPtr pPixmap = pGC->stipple;

    if(pGC->fillStyle == FillStippled){	/* Transparent Stipples */
	StippleType = 0;
	color = pGC->fgPixel; REPLICATE(color);
	/* >>>>>>>>>>>>>>>>>  Set fg  <<<<<<<<<<<<<<<<<< */
	if (pGC->alu == GXcopy) {
		span = FastFillEnable;
    		GLINT_WRITE_REG(0, RasterizerMode);
		GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
		GLINT_WRITE_REG(color, FBBlockColor);
	} else {
		span = 0;
    		GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
		GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		GLINT_WRITE_REG(color, ConstantColor);
	}
    } else if(pGC->alu == GXcopy)  /* OpaqueEasyStipples */
	StippleType = 2;
    else 				/* OpaqueHardStipples */
	StippleType = 4;

    /* >>>>>>>>>>> Set ROP, Planemask <<<<<<<<<<< */
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    DO_PLANEMASK(pGC->planemask);
    if (pGC->alu == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
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

	    switch(StippleType) {
	      case 0:
		PermediaStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 2:
		/* >>>>>>>>>>>  Set bg and draw rect <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
    		GLINT_WRITE_REG(0, RasterizerMode);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		GLINT_WRITE_REG(PrimitiveTrapezoid |span,Render);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
    		    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		PermediaStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 4:
    		/* >>>>> Make sure our bitmask is not inverted <<<<<< */
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		    GLINT_WRITE_REG(color, ConstantColor);
    		    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
		}
		PermediaStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
	/*	>>>>>>>>>>  Invert Mask <<<<<<<<< */
	/*	>>>>>>>>>>  Set bg <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
    		    GLINT_WRITE_REG(2, RasterizerMode);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		    GLINT_WRITE_REG(color, ConstantColor);
    		    GLINT_WRITE_REG(2|BitMaskPackingEachScanline, RasterizerMode);
		}
		PermediaStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	     default:
			/* Should never get here */
			ErrorF("Can't handle stipple\n");
		break;
	    }
	}
    }

    GLINT_WRITE_REG(0, RasterizerMode);
    SET_SYNC_FLAG;
}

void 
PermediaWriteBitmap(x, y, w, h, src, srcwidth, srcx, srcy, 
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

#if QWORD_PADDED
    Bool PlusOne;
#endif

    srcp = (srcwidth * srcy) + (srcx >> 3) + src; 
    srcx &= 0x07;
    if(skipleft = (int)srcp & 0x03) { 
	skipleft = (skipleft << 3) + srcx;	
	srcp = (unsigned char *)((int)srcp & ~0x03);
    } else
	skipleft = srcx;    

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

#if QWORD_PADDED
     PlusOne = ((dwords * h) & 0x01);
#endif

    /* >>>>> Set rop, planemask, left edge clip skipleft pixels right
	of x (skipleft is sometimes 0 and clipping isn't needed). <<<<<< */
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    if (rop == GXcopy) {
    	GLINT_WRITE_REG(0, RasterizerMode);
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
    	GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }
    GLINT_WRITE_REG(x<<16, StartXDom);
    GLINT_WRITE_REG(y<<16, StartY);
    GLINT_WRITE_REG((x+w)<<16, StartXSub);
    GLINT_WRITE_REG(h, GLINTCount);
    GLINT_WRITE_REG(0, dXDom);
    GLINT_WRITE_REG(0, dXSub);
    GLINT_WRITE_REG(1<<16, dY);
    if (skipleft > 0) 
	PermediaSetClippingRectangle(x+skipleft, y, x+w, y+h);

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	GLINT_WRITE_REG(PrimitiveTrapezoid |mode,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    }

   /* >>>>>>>>> initiate transfer (x,y,w,h).  Skipleft pixels on the
	left edge will be clipped <<<<<< */

SECOND_PASS:
    GLINT_WRITE_REG(PrimitiveTrapezoid | mode | SyncOnBitMask, Render);
    
    height = h;
    srcpntr = srcp;
    while(height--) {
	count = dwords;
	pattern = (CARD32*)srcpntr;
	while(count--) {
		GLINT_WRITE_REG(*(pattern++), BitMaskPattern);
	}
	srcpntr += srcwidth;
    }    

    GLINT_WRITE_REG(0, WaitForCompletion);
#if QWORD_PADDED
    if(PlusOne) GLINT_WRITE_REG(0, BitMaskPattern);
#endif

    if(SecondPass) {
	SecondPass = FALSE;
	/* >>>>>> invert bitmask and set bg <<<<<<<< */
	REPLICATE(bg);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(2, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(2|BitMaskPackingEachScanline, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	goto SECOND_PASS;
    }

    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(0, ScissorMode);
    SET_SYNC_FLAG;	
}
