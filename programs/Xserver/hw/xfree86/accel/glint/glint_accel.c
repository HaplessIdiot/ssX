/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_accel.c,v 1.13 1997/11/01 15:04:31 hohndel Exp $ */
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
 * GLINT accelerated options.
 */

#define ACCEL_DEBUG

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


unsigned char   byte_reversed[256] =
{
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

int             LogicalOpReadTest[] =
{
  0,				/* 00 */
  FBRM_DstEnable,		/* 01 */
  FBRM_DstEnable,		/* 02 */
  0,				/* 03 */
  FBRM_DstEnable,		/* 04 */
  FBRM_DstEnable,		/* 05 */
  FBRM_DstEnable,		/* 06 */
  FBRM_DstEnable,		/* 07 */
  FBRM_DstEnable,		/* 08 */
  FBRM_DstEnable,		/* 09 */
  FBRM_DstEnable,		/* 10 */
  FBRM_DstEnable,		/* 11 */
  0,				/* 12 */
  FBRM_DstEnable,		/* 13 */
  FBRM_DstEnable,		/* 14 */
  0,				/* 15 */
};

extern int      pprod;
extern int      coprotype;
extern          GLINTWindowBase;
int             ScanlineWordCount;
char            clip = 0;
int             clipmin, clipmax;
int             BitMaskOffset;
int             gcolor;
int             grop;
int             gtransparency;
int             notline = 0;
int             mode;
int             ropsave, xcoord, ycoord, width, height;
int             pattern_pos_x, pattern_pos_y;
int             gbg, gfg, gstrw;
int             mode2;
short           DashPattern;
int             DashPatternSize;
CARD32          ScratchBuffer[512];
int             blitxdir, blitydir;


void            GLINTSync ();
void            GLINTSetupForFillRectSolid ();
void            GLINTSubsequentFillRectSolid ();
void            GLINTSubsequentFillTrapezoidSolid ();
void            GLINTSubsequentTwoPointLine ();
void            GLINTSubsequentTwoPointLine ();
void            GLINTSetupForDashedLine ();
void            GLINTSubsequentDashedTwoPointLine ();
void            GLINTSetupForScreenToScreenCopy ();
void            GLINTSubsequentScreenToScreenCopy ();
void            GLINTSetClippingRectangle ();
void            GLINTSetupForFill8x8Pattern ();
void            GLINTSubsequentFill8x8Pattern ();
void            GLINTSetupFor8x8PatternColorExpand ();
void            GLINTSubsequent8x8PatternColorExpand ();
void            GLINTSetupForScanlineScreenToScreenColorExpand ();
void            GLINTSubsequentScanlineScreenToScreenColorExpand ();


void
GLINTAccelInit ()
{
  int             vramsave;

  if (IS_3DLABS_TX_MX_CLASS (coprotype))
    {

      xf86AccelInfoRec.Flags =
	PIXMAP_CACHE |
	ONLY_LEFT_TO_RIGHT_BITBLT |
	COP_FRAMEBUFFER_CONCURRENCY |
	HORIZONTAL_TWOPOINTLINE |
	HARDWARE_CLIP_LINE |
	USE_TWO_POINT_LINE |
	TWO_POINT_LINE_NOT_LAST |
	BACKGROUND_OPERATIONS |	/* NO_CAP_NOT_LAST | */
	DELAYED_SYNC;

      xf86AccelInfoRec.PatternFlags =
	HARDWARE_PATTERN_NOT_LINEAR |
	HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
	HARDWARE_PATTERN_PROGRAMMED_BITS |
	HARDWARE_PATTERN_TRANSPARENCY;

      xf86AccelInfoRec.Sync = GLINTSync;

      xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;

      xf86AccelInfoRec.SetupForFillRectSolid = GLINTSetupForFillRectSolid;
      xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;
      xf86AccelInfoRec.SubsequentFillRectSolid = GLINTSubsequentFillRectSolid;
      xf86AccelInfoRec.SubsequentTwoPointLine = GLINTSubsequentTwoPointLine;

      xf86AccelInfoRec.SetClippingRectangle = GLINTSetClippingRectangle;

      xf86AccelInfoRec.SetupForDashedLine = GLINTSetupForDashedLine;
      xf86AccelInfoRec.SubsequentDashedTwoPointLine = GLINTSubsequentDashedTwoPointLine;
      xf86AccelInfoRec.LinePatternBuffer = (void *) &DashPattern;
      xf86AccelInfoRec.LinePatternMaxLength = 16;

      xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | NO_PLANEMASK;

      xf86AccelInfoRec.SetupForScreenToScreenCopy =
	GLINTSetupForScreenToScreenCopy;
      xf86AccelInfoRec.SubsequentScreenToScreenCopy =
	GLINTSubsequentScreenToScreenCopy;

      xf86AccelInfoRec.ColorExpandFlags =
	VIDEO_SOURCE_GRANULARITY_BYTE |
	CPU_TRANSFER_PAD_DWORD |
	ONLY_TRANSPARENCY_SUPPORTED |
	BIT_ORDER_IN_BYTE_LSBFIRST |
	NO_PLANEMASK;

      xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
	GLINTSetupForScanlineScreenToScreenColorExpand;
      xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
	GLINTSubsequentScanlineScreenToScreenColorExpand;

      xf86AccelInfoRec.ScratchBufferAddr = 1;
      xf86AccelInfoRec.ScratchBufferSize = 512;
      xf86AccelInfoRec.ScratchBufferBase = (void *) ScratchBuffer;
      xf86AccelInfoRec.PingPongBuffers = 1;

      xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
      xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;

      xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
	GLINTSetupFor8x8PatternColorExpand;
      xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
	GLINTSubsequent8x8PatternColorExpand;
      xf86AccelInfoRec.ImageWriteOffset = glintInfoRec.displayWidth *
                        glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel /
8);
      /* 4 scanlines should be enough */
      xf86AccelInfoRec.ImageWriteRange = glintInfoRec.displayWidth * 
                        (glintInfoRec.bitsPerPixel / 8) * 4;
 
      xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;
 
      xf86AccelInfoRec.PixmapCacheMemoryStart =
        xf86AccelInfoRec.ImageWriteOffset + xf86AccelInfoRec.ImageWriteRange;
   
      xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;
 
      /* Check Pixmap cache and if enough room, enable ImageWrites */
      if ( (xf86AccelInfoRec.PixmapCacheMemoryEnd - 
                xf86AccelInfoRec.ImageWriteOffset) >
                        xf86AccelInfoRec.ImageWriteRange) {
        xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY | NO_PLANEMASK;
        xf86AccelInfoRec.SubsequentScanlineScreenToScreenCopy =
                        GLINTSubsequentScanlineScreenToScreenCopy;
      }
    }
  else if (IS_3DLABS_PERMEDIA_CLASS (coprotype))
    PermediaAccelInit ();

  xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;

  xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.virtualY *
    glintInfoRec.displayWidth * glintInfoRec.bitsPerPixel / 8;

#ifndef GLINT_3D
  xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;
#else
  xf86AccelInfoRec.PixmapCacheMemoryEnd = (glintInfoRec.videoRam * 1024) -
    xf86AccelInfoRec.PixmapCacheMemoryStart;
#endif
}


/*
 * This is the implementation of the Sync() function.
 */
void
GLINTSync ()
{
  while (GLINT_READ_REG (DMACount) != 0);
  GLINT_WAIT (2);
  GLINT_WRITE_REG (0xc00, FilterMode);
  GLINT_WRITE_REG (0, GlintSync);
  do
    {
      while (GLINT_READ_REG (OutFIFOWords) == 0);
#define Sync_tag 0x188
    }
  while (GLINT_READ_REG (OutputFIFO) != Sync_tag);
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */

void
GLINTSetRop (int rop, int fg)
{
  if (rop == GXcopy)
    {
      mode |= FastFillEnable;
      GLINT_WAIT (6);
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | Use_ConstantFBWriteData | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG (0, PatternRamMode);
      GLINT_WRITE_REG (fg, FBBlockColor);
    }
  else
    {
      /* not fastfill with log. op */
      mode |= SpanOperation;
      GLINT_WAIT (9);
      GLINT_WRITE_REG (pprod | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
      GLINT_WRITE_REG (fg, FBWriteData);
      GLINT_WRITE_REG (1, PatternRamMode);
      GLINT_WRITE_REG (fg, PatternRamData0);
      /* setup the flatschading value */
      GLINT_WRITE_REG (0xffffffff, ConstantColor);
    }
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);
  clip = 0;
}


/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */

void
GLINTSetupForFillRectSolid (int color, int rop, unsigned planemask)
{

  REPLICATE (color);
  REPLICATE (planemask);

  gcolor = color;
  mode = PrimitiveTrapezoid;

  GLINTSetRop (rop, color);

  GLINT_WAIT (2);
  /* GLINT_WRITE_REG (0, ScissorMode); */
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (1 << 16, dY);
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void
GLINTSubsequentFillRectSolid (int x, int y, int w, int h)
{
  GLINT_WAIT (7);
  GLINT_WRITE_REG ((x + w) << 16, StartXSub);
  GLINT_WRITE_REG (x << 16, StartXDom);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG (h, GLINTCount);
  /* not in GlintSetupForFillRectSolid !
     is the same setup for RectSolid and Trapezoid */
  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);

  GLINT_WRITE_REG (mode, Render);
}

void
GLINTSetupForDashedLine (fg, bg, rop, planemask, size)
{
  gtransparency = bg;

  REPLICATE (fg);
  REPLICATE (bg);
  REPLICATE (planemask);

  mode = PrimitiveLine | LineStippleEnable /* | SyncOnHostData */ ;

  gfg = fg;
  gbg = bg;

  if (rop == GXcopy)
    {
      GLINT_WAIT (6);
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG (UNIT_DISABLE, PatternRamMode);
      GLINT_WRITE_REG (gfg, FBBlockColor);
    }
  else
    {
      mode |= SpanOperation;
      GLINT_WAIT (8);
      GLINT_WRITE_REG (pprod | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
      GLINT_WRITE_REG (1, PatternRamMode);
      GLINT_WRITE_REG (gfg, PatternRamData0);
      GLINT_WRITE_REG (UNIT_DISABLE, PatternRamMode);
      GLINT_WRITE_REG (0xffffffff, ConstantColor);
    }

  GLINT_WAIT (8);

  GLINT_WRITE_REG (gfg, GLINTColor);

  GLINT_WRITE_REG (UNIT_DISABLE, RasterizerMode);
  GLINT_WRITE_REG (UNIT_DISABLE, AreaStippleMode);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);

  GLINT_WRITE_REG (0, dXSub);

  DashPatternSize = (size >> 4);
}

void
GLINTSubsequentDashedTwoPointLine (int x1, int y1, int x2, int y2,
				   int bias, int offset)
{
  int             dx, dy, adx, ady, z1 = 0;

  dx = x2 - x1;
  dy = y2 - y1;
  adx = abs (dx);
  ady = abs (dy);

  GLINT_WAIT (11);

  /* GLINT_WRITE_REG (0, LoadLineStippleCounters); */
  GLINT_WRITE_REG (0, UpdateLineStippleCounters);

  GLINT_WRITE_REG ((DashPattern << 10) | (DashPatternSize << 1) |
		   UNIT_ENABLE, LineStippleMode);
  /* GLINT_WRITE_REG (0, WaitForCompletion); */

  GLINT_WRITE_REG (x1 << 16, StartXDom);
  GLINT_WRITE_REG (y1 << 16, StartY);

  if (adx >= ady)
    {
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
    {
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

  if (gtransparency != -1)
    {
      GLINT_WAIT (4);
      GLINT_WRITE_REG (mode, Render);
      GLINT_WRITE_REG (gbg, GLINTColor);
      GLINT_WRITE_REG (mode, Render);
      GLINT_WRITE_REG (gfg, GLINTColor);
      return;
    }

  GLINT_WRITE_REG (mode, Render);
}

void
GLINTSetClippingRectangle (int x1, int y1, int x2, int y2)
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
GLINTSetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  blitydir = ydir;
  blitxdir = xdir;

  notline = 0;

  REPLICATE (planemask);

  GLINT_WAIT (10);
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
GLINTSubsequentScreenToScreenCopy (int x1, int y1, int x2, int y2,
				   int w, int h)
{
  GLINT_WAIT (8);
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
GLINTSetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  REPLICATE (fg);
  GLINT_WAIT (15);
  GLINT_WRITE_REG (UNIT_DISABLE, RasterizerMode);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);

  if (rop == GXcopy)
    {
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (fg, FBBlockColor);
      GLINT_WRITE_REG (0, PatternRamMode);
      GLINT_WRITE_REG (UNIT_DISABLE, LogicalOpMode);
      mode = FastFillEnable;
    }
  else
    {
      GLINT_WRITE_REG (pprod | FBRM_DstEnable, FBReadMode);
      GLINT_WRITE_REG (1, PatternRamMode);
      GLINT_WRITE_REG (fg, PatternRamData0);
      mode = SpanOperation;
      GLINT_WRITE_REG (rop << 1 | UNIT_ENABLE, LogicalOpMode);
    }
  REPLICATE (planemask);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);

  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);
  GLINT_WRITE_REG (x << 16, StartXDom);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG ((x + w) << 16, StartXSub);
  GLINT_WRITE_REG (h, GLINTCount);
  GLINT_WRITE_REG (1 << 16, dY);
  ScanlineWordCount = (w + 31) >> 5;
  GLINT_WRITE_REG (PrimitiveTrapezoid | mode | SyncOnBitMask, Render);
}

void
GLINTSubsequentScanlineScreenToScreenColorExpand (int srcaddr)
{
  register CARD32 *ptr = (CARD32 *) ScratchBuffer;
  int             count = ScanlineWordCount;

  while (count--)
    {
      GLINT_SLOW_WRITE_REG (*(ptr++), BitMaskPattern);
    }
}

void
GLINTSetupForFill8x8Pattern (int patternx, int patterny, int rop,
			     unsigned planemask, int transparency_color)
{
  pattern_pos_x = patternx;
  pattern_pos_y = patterny;

  REPLICATE (planemask);

  GLINT_WAIT (10);
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

void
GLINTSubsequentFill8x8Pattern (int patternx, int patterny, int x, int y,
			       int w, int h)
{
  int             anzx, restx, anzy, resty, a, b, sx, sy, ppx, ppy;

  ppx = pattern_pos_x + patternx;
  ppy = pattern_pos_y + patterny;

  GLINT_WAIT (2);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);
  GLINT_WRITE_REG (1 << 16, dY);

  if (w < 8 && h < 8)
    {
      GLINT_WAIT (7);
      /* srcaddr - dstaddr */
      GLINT_WRITE_REG ((GLINTWindowBase + patterny * glintInfoRec.displayWidth +
			patternx) -
		       (GLINTWindowBase + y * glintInfoRec.displayWidth + x),
		       FBSourceOffset);
      GLINT_WRITE_REG (0, WaitForCompletion);

      GLINT_WRITE_REG ((long) (x + w << 16), StartXDom);
      GLINT_WRITE_REG ((long) (x << 16), StartXSub);

      GLINT_WRITE_REG (y << 16, StartY);
      GLINT_WRITE_REG (h, GLINTCount);

      GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
      return;
    }

  anzx = w / 8;
  anzy = h / 8;

  restx = w - anzx * 8;
  resty = h - anzy * 8;

  w = 8;
  h = 8;

  for (b = 0; b <= anzy; b++)
    {
      sy = y + b * 8;


      if (b < anzy)
	{
	  GLINT_WAIT (2);
	  GLINT_WRITE_REG (sy << 16, StartY);
	  GLINT_WRITE_REG (h, GLINTCount);

	  for (a = 0; a <= anzx; a++)
	    {
	      GLINT_WAIT (5);
	      sx = x + a * 8;

	      if (a < anzx)
		GLINT_WRITE_REG ((long) (sx + w << 16), StartXDom);
	      else
		GLINT_WRITE_REG ((long) (sx + restx << 16), StartXDom);

	      GLINT_WRITE_REG ((GLINTWindowBase + ppy * glintInfoRec.displayWidth +
				ppx) -
		    (GLINTWindowBase + sy * glintInfoRec.displayWidth + sx),
			       FBSourceOffset);
	      GLINT_WRITE_REG (0, WaitForCompletion);
	      GLINT_WRITE_REG ((long) (sx << 16), StartXSub);
	      GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
	    }
	}
      else
	{
	  GLINT_WAIT (2);
	  GLINT_WRITE_REG (sy << 16, StartY);
	  GLINT_WRITE_REG (resty, GLINTCount);

	  for (a = 0; a <= anzx; a++)
	    {
	      GLINT_WAIT (5);
	      sx = x + a * 8;

	      if (a < anzx)
		GLINT_WRITE_REG ((long) (sx + w << 16), StartXDom);
	      else
		GLINT_WRITE_REG ((long) (sx + restx << 16), StartXDom);

	      GLINT_WRITE_REG ((GLINTWindowBase + ppy * glintInfoRec.displayWidth +
				ppx) -
		    (GLINTWindowBase + sy * glintInfoRec.displayWidth + sx),
			       FBSourceOffset);
	      GLINT_WRITE_REG (0, WaitForCompletion);

	      GLINT_WRITE_REG ((long) (sx << 16), StartXSub);
	      GLINT_WRITE_REG (PrimitiveTrapezoid, Render);
	    }
	}
    }
}


void
GLINTSetupFor8x8PatternColorExpand (int patternx, int patterny,
				    int bg, int fg, int rop,
				    unsigned planemask)
{
/* #define   selxy1 0 */
/* #define   selxy2 1 */
#define   selxy3 2
/* #define   selxy4 3 */
/* #define   selxy5 4 */
/* #define   sel1bit  (selxy1 << 4) | (selxy1 << 1) */
/* #define   sel2bit  (selxy2 << 4) | (selxy2 << 1) */
#define   sel3bit  (selxy3 << 4) | (selxy3 << 1)
/* #define   sel4bit  (selxy4 << 4) | (selxy4 << 1) */
/* #define   sel5bit  (selxy5 << 4) | (selxy5 << 1) */

/*   int p, x, a, b, c, d, e, f, g, h; */

  /* first draw color ist the fg also the bg color in this routine! */
  gcolor = fg;
  gbg = bg;

  gtransparency = bg;

  REPLICATE (planemask);
  REPLICATE (gcolor);
  REPLICATE (gbg);

  grop = rop;
  mode = PrimitiveTrapezoid | AreaStippleEnable;

  gstrw = sel3bit;		/* 2bit only */

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
      GLINT_WAIT (6);
      GLINT_WRITE_REG (pprod, FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (UNIT_DISABLE, ColorDDAMode);
      GLINT_WRITE_REG (UNIT_DISABLE, PatternRamMode);
      GLINT_WRITE_REG (gbg, FBBlockColor);
    }
  else
    {
      mode |= SpanOperation;
      GLINT_WAIT (8);
      GLINT_WRITE_REG (pprod | LogicalOpReadTest[rop], FBReadMode);
      GLINT_WRITE_REG (0, WaitForCompletion);
      GLINT_WRITE_REG ((rop << 1) | UNIT_ENABLE, LogicalOpMode);
      GLINT_WRITE_REG (CDDA_FlatShading | UNIT_ENABLE, ColorDDAMode);
      /* GLINT_WRITE_REG(gbg, FBWriteData); *//* to test */
      GLINT_WRITE_REG (1, PatternRamMode);
      GLINT_WRITE_REG (gbg, PatternRamData0);
      GLINT_WRITE_REG (0xffffffff, ConstantColor);
    }

  GLINT_WAIT (6);
  GLINT_WRITE_REG (planemask, FBHardwareWriteMask);
  GLINT_WRITE_REG (1 << 16, dY);

  GLINT_WRITE_REG (UNIT_DISABLE, RasterizerMode);
  GLINT_WRITE_REG (UNIT_DISABLE, ScissorMode);
  GLINT_WRITE_REG (UNIT_DISABLE, LineStippleMode);
}

void
GLINTSubsequent8x8PatternColorExpand (int patternx, int patterny, int x, int y,
				      int w, int h)
{
  char            a;

  GLINT_WAIT (10);
  GLINT_WRITE_REG (0, dXDom);
  GLINT_WRITE_REG (0, dXSub);
  GLINT_WRITE_REG ((x + w) << 16, StartXSub);
  GLINT_WRITE_REG (x << 16, StartXDom);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG (h, GLINTCount);

  for (a = 0; a < 2; a++)
    {
      /* not GXcopy then bg and fg fill pattern */
      /* it's magic, you must set first bg then fg */
      if (!a)
	{
	  GLINT_WRITE_REG (gbg, GLINTColor);
	  GLINT_WRITE_REG (UNIT_ENABLE | ASM_InvertPattern | gstrw, AreaStippleMode);
	}
      else
	{
	  GLINT_WRITE_REG (gcolor, GLINTColor);
	  GLINT_WRITE_REG (UNIT_ENABLE | gstrw, AreaStippleMode);
	}

      GLINT_WRITE_REG (mode, Render);

      /* transparency bg, cancel second pass */
      if (gtransparency == -1)
	return;
    }
}

void
GLINTSubsequentFillTrapezoidSolid (int y, int h, int left, int dxl, int dyl, int el,
				   int right, int dxr, int dyr, int er)
{
  /* I have tested with xclock, i must add 2 to pass lines and Trapezoids */
  /* is this a bug in xaa ? */
  right++;
  GLINT_WAIT (7);
  GLINT_WRITE_REG (left << 16, StartXDom);
  GLINT_WRITE_REG (right << 16, StartXSub);
  GLINT_WRITE_REG (y << 16, StartY);
  GLINT_WRITE_REG (h, GLINTCount);
  GLINT_WRITE_REG ((dxl << 16) / dyl, dXDom);
  GLINT_WRITE_REG ((dxr << 16) / dyr, dXSub);

  GLINT_WRITE_REG (mode, Render);
}

#ifdef GLINT_3D
/* test ohne glx */
extern void     run_demo (void);

#endif

void
GLINTSubsequentTwoPointLine (int x1, int y1, int x2, int y2, int bias)
{
  register int    dx, dy, adx, ady;

  dx = x2 - x1;
  dy = y2 - y1;
  adx = abs (dx);
  ady = abs (dy);

  GLINT_WAIT (7);

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
     GLINTBresenhamLine(x1, y1, x2, y2);
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
  GLINT_WRITE_REG ((mode & ~PrimitiveTrapezoid) | PrimitiveLine | FastFillEnable, Render);
}


/* End of glint_accel.c  **************************************************** */
