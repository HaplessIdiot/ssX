/* $XFree86: $ */

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
 *           Helmut Fahrion, <hf@suse.de>
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

#include "glint_regs.h"
#include "glint.h"
#include "xf86xaa.h"

#include <float.h>

#if DEBUG
#include <stdio.h>
#endif

extern int      LogicalOpReadTest[];
extern int      ScanlineWordCount;
extern short    DashPattern;
extern int      DashPatternSize;
extern CARD32   ScratchBuffer[512];
extern int      blitxdir, blitydir;
extern int      mode;
extern int      pprod;
extern char     clip;
extern int      clipmin, clipmax;
extern int      gcolor;
extern int      grop;
extern int      gtransparency;
extern int      gbg, gfg, gstrw;
extern          GLINTWindowBase;

extern void     GLINTSync ();
extern void     GLINTSubsequentFillTrapezoidSolid ();
extern void     GLINTSetupForDashedLine ();
extern void     GLINTSubsequentDashedTwoPointLine ();
extern void     GLINTSetupForScreenToScreenCopy ();
extern void     GLINTSubsequentScreenToScreenCopy ();
extern void     GLINTSetClippingRectangle ();
extern void     GLINTSetupForFill8x8Pattern ();
extern void     GLINTSubsequentFill8x8Pattern ();
extern void     GLINTSubsequent8x8PatternColorExpand ();
extern void     GLINTSubsequentScanlineScreenToScreenColorExpand ();
extern void     GLINTSubsequentFillRectSolid ();

void            PermediaSetupForFillRectSolid ();
void PermediaSubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h);
void
PermediaSetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color);


/* void            PermediaSubsequentFillRectSolid (); */
void            PermediaSubsequentTwoPointLine ();
void            PermediaSetupFor8x8PatternColorExpand ();
void            PermediaSetupForScanlineScreenToScreenColorExpand ();
void            GLINTBresenhamLine (int x1, int y1, int x2, int y2);
void            PermediaSetClippingRectangle (int x1, int y1, int x2, int y2);

void
PermediaAccelInit ()
{
  xf86AccelInfoRec.Flags = PIXMAP_CACHE |
    HORIZONTAL_TWOPOINTLINE |
    HARDWARE_CLIP_LINE |
    USE_TWO_POINT_LINE |
    TWO_POINT_LINE_NOT_LAST |
    BACKGROUND_OPERATIONS |
    COP_FRAMEBUFFER_CONCURRENCY;
    DELAYED_SYNC;  /* not for GL 1000 */

  xf86AccelInfoRec.PatternFlags =
    HARDWARE_PATTERN_NOT_LINEAR |
    HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    HARDWARE_PATTERN_PROGRAMMED_BITS |
    HARDWARE_PATTERN_TRANSPARENCY;

  xf86AccelInfoRec.Sync = GLINTSync;

  xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;

  xf86AccelInfoRec.SetupForFillRectSolid = PermediaSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = GLINTSubsequentFillRectSolid;
  xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;

  xf86AccelInfoRec.SetupForScreenToScreenCopy = PermediaSetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy = PermediaSubsequentScreenToScreenCopy;

  xf86AccelInfoRec.SetClippingRectangle = PermediaSetClippingRectangle;
  xf86AccelInfoRec.SubsequentTwoPointLine = PermediaSubsequentTwoPointLine;

  /* Permedia does not have a Linestripplemode */
  /* xf86AccelInfoRec.SetupForDashedLine = PermediaSetupForDashedLine;
     xf86AccelInfoRec.LinePatternBuffer = (void *) &DashPattern;
     xf86AccelInfoRec.LinePatternMaxLength = 8;
  */

  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | NO_PLANEMASK;

  xf86AccelInfoRec.ColorExpandFlags =
    VIDEO_SOURCE_GRANULARITY_BYTE |
    /* CPU_TRANSFER_PAD_DWORD | */ /* is not correct, and not for GL 1000 */
    ONLY_TRANSPARENCY_SUPPORTED |
    BIT_ORDER_IN_BYTE_LSBFIRST |
    NO_PLANEMASK;

  xf86AccelInfoRec.ScratchBufferAddr = 1;
  xf86AccelInfoRec.ScratchBufferSize = 512;
  xf86AccelInfoRec.ScratchBufferBase = (void *) ScratchBuffer;
  xf86AccelInfoRec.PingPongBuffers = 1;

  xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    PermediaSetupForScanlineScreenToScreenColorExpand;
  xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
    GLINTSubsequentScanlineScreenToScreenColorExpand;
  
  xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
  xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
  xf86AccelInfoRec.SetupFor8x8PatternColorExpand = PermediaSetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand = GLINTSubsequent8x8PatternColorExpand;

  xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;

  xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.virtualY *
    glintInfoRec.displayWidth * glintInfoRec.bitsPerPixel / 8;
}

void
PermediaSetRop (int rop, int fg)
{
  if (rop == GXcopy)
    {
      mode |= FastFillEnable;
      GLINT_WAIT (6);
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | Use_ConstantFBWriteData | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG (fg, FBBlockColor);
    }
  else
    {
      GLINT_WAIT (7);
      GLINT_WRITE_REG (pprod | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
      GLINT_WRITE_REG (fg, FBWriteData);
      /* setup the flatschading value */
      GLINT_WRITE_REG (0xffffffff, ConstantColor);
    }
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);
  clip = 0;
}

void
PermediaSetClippingRectangle (int x1, int y1, int x2, int y2)
{
  clip = 1;
  clipmin = (y1 << 16) | x1;
  clipmax = ((y2 + 1) << 16) | (x2 + 1);

  GLINT_WAIT (5);
  GLINT_WRITE_REG (clipmin, ScissorMinXY);
  GLINT_WRITE_REG (clipmax, ScissorMaxXY);
  GLINT_WRITE_REG (0, WindowOrigin);
  GLINT_WRITE_REG ((glintInfoRec.displayWidth - 1) | 0x07ff0000, ScreenSize);
  GLINT_WRITE_REG (SCI_USERANDSCREEN, ScissorMode);	/* Enable Scissor Mode */
}


void
PermediaSetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  blitydir = ydir;
  blitxdir = xdir;

  REPLICATE (planemask);

  GLINT_WAIT (11);
  GLINT_WRITE_REG (0, RasterizerMode);

 if (rop == GXcopy)
    {
      GLINT_WRITE_REG (pprod | FBRM_SrcEnable, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | Use_ConstantFBWriteData | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
    }
  else
    {
      GLINT_WRITE_REG (pprod | FBRM_SrcEnable | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
    }

  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (0xFFFFFFFF, FBSoftwareWriteMask);

  GLINT_WRITE_REG (UNIT_DISABLE, AreaStippleMode);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);

  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);
}

/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void
PermediaSubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  GLINT_WAIT (9);
  /* Permedia only allows left to right bitblt's */
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);

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

  /* srcaddr - dstaddr */
  GLINT_WRITE_REG ((GLINTWindowBase + y1 * glintInfoRec.displayWidth + x1) -
		   (GLINTWindowBase + y2 * glintInfoRec.displayWidth + x2),
		   FBSourceOffset);
  GLINT_WRITE_REG (0, WaitForCompletion);

  /* care must be taken when the source and destination
     overlap, choose the source scanning direction to the
     overlaping area */
  if (x2 > x1)
    {
      GLINT_WRITE_REG ((long) (x2 + w << 16), StartXDom);
      GLINT_WRITE_REG ((long) (x2 << 16), StartXSub);
    }
  else
    {
      GLINT_WRITE_REG ((long) (x2 << 16), StartXDom);
      GLINT_WRITE_REG ((long) (x2 + w << 16), StartXSub);
    }

  GLINT_WRITE_REG (y2 << 16, StartY);
  GLINT_WRITE_REG (h, GLINTCount);

  GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
}

void
PermediaSetupForFillRectSolid (int color, int rop, unsigned planemask)
{
  REPLICATE (color);
  REPLICATE (planemask);

  gcolor = color;
  mode = PrimitiveTrapezoid;

  PermediaSetRop (rop, color);

  GLINT_WAIT (2);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (1 << 16, dY);
}

void
PermediaSetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  REPLICATE (fg);
  REPLICATE (planemask);

  GLINT_WAIT (2);

  GLINT_WRITE_REG (UNIT_DISABLE, RasterizerMode);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);

  mode = PrimitiveTrapezoid | SyncOnBitMask;

  PermediaSetRop (rop, fg);

  GLINT_WAIT (9);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);
  GLINT_WRITE_REG (x << 16, StartXDom);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG ((x + w) << 16, StartXSub);
  GLINT_WRITE_REG (h, GLINTCount);
  GLINT_WRITE_REG (1 << 16, dY);
  ScanlineWordCount = (w + 31) >> 5;
  GLINT_WRITE_REG (mode, Render);
}

void
PermediaSetupFor8x8PatternColorExpand (int patternx, int patterny,
				       int bg, int fg, int rop,
				       unsigned planemask)
{
  /* first draw color ist the fg also the bg color in this routine! */
  gcolor = fg;
  gbg = bg;

  gtransparency = bg;

  REPLICATE (planemask);
  REPLICATE (gcolor);
  REPLICATE (gbg);

  grop = rop;
  mode = PrimitiveTrapezoid | AreaStippleEnable;

  GLINT_WAIT (8);

  GLINT_WRITE_REG ((patternx & 0x000000ff), AreaStipplePattern0);
  GLINT_WRITE_REG ((patternx & 0x0000ff00) >> 8, AreaStipplePattern1);
  GLINT_WRITE_REG ((patternx & 0x00ff0000) >> 16, AreaStipplePattern2);
  GLINT_WRITE_REG ((patternx & 0xff000000) >> 24, AreaStipplePattern3);
  GLINT_WRITE_REG ((patterny & 0x000000ff), AreaStipplePattern4);
  GLINT_WRITE_REG ((patterny & 0x0000ff00) >> 8, AreaStipplePattern5);
  GLINT_WRITE_REG ((patterny & 0x00ff0000) >> 16, AreaStipplePattern6);
  GLINT_WRITE_REG ((patterny & 0xff000000) >> 24, AreaStipplePattern7);

  if (rop == GXcopy)
    {
      GLINT_WAIT (5);
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | Use_ConstantFBWriteData | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG (gbg, FBBlockColor);
    }
  else
    {
      GLINT_WAIT (6);
      GLINT_WRITE_REG (pprod | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
      GLINT_WRITE_REG (gbg, FBWriteData);
      /* setup the flatschading value */
      GLINT_WRITE_REG (0xffffffff, ConstantColor);
    }

  GLINT_WAIT (4);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (1 << 16, dY);
  GLINT_WRITE_REG (UNIT_DISABLE, RasterizerMode);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);
  /* GLINT_WRITE_REG(UNIT_ENABLE, AreaStippleMode); */
  gstrw = 0;
}

#ifdef GLINT_3D
/* test ohne glx */
extern void     run_demo (void);

#endif


void
PermediaSubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
{
  register int    dx, dy, adx, ady;

  dx = x2 - x1;
  dy = y2 - y1;
  adx = abs (dx);
  ady = abs (dy);

  GLINT_WAIT (8);

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

  if (adx && ady && ((adx + ady) < 14))
    {
      GLINTBresenhamLine (x1, y1, x2, y2);
      return;
    }

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

  GLINT_WAIT (2);
  GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
  GLINT_WRITE_REG (gcolor, GLINTColor);

  if (dx > dy)
    {
      a = dy + dy;
      c = a - dx;
      b = c - dx;
      for (i = 0; i <= dx; i++)
	{
	  GLINT_WAIT (3);
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
	  GLINT_WAIT (3);
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
