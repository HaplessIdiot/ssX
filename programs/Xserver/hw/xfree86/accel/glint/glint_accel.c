/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_accel.c,v 1.16 1997/11/08 16:24:25 hohndel Exp $ */
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
 *	     Stefan Dirsch, <sndirsch@suse.de>
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
#include "mispans.h"

#include "glint_regs.h"
#include "glint.h"
#include "xf86xaa.h"

#include <float.h>

#if DEBUG
#include <stdio.h>
#endif

unsigned char byte_reversed[256] =
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

extern int pprod;
extern int coprotype;
extern GLINTWindowBase;
int ScanlineWordCount;
static int BitMaskOffset;
static int gcolor;
static int blitmode;
static int grop;
static int gtransparency;
static int mode;
static int span;
static int gbg, gfg;
static int DashPatternSize;
static short DashPattern;
CARD32 ScratchBuffer[512];
static int blitxdir, blitydir;

void GLINTSync();
void GLINTSetupForFillRectSolid();
void GLINTSubsequentFillRectSolid();
void GLINTSubsequentFillTrapezoidSolid();
void GLINTSubsequentTwoPointLine();
void GLINTSetupForDashedLine();
void GLINTSubsequentDashedTwoPointLine();
void GLINTSetupForScreenToScreenCopy();
void GLINTSubsequentScreenToScreenCopy();
void GLINTSetClippingRectangle();
void GLINTSetupForFill8x8Pattern();
void GLINTSubsequentFill8x8Pattern();
void GLINTSetupFor8x8PatternColorExpand();
void GLINTSubsequent8x8PatternColorExpand();
void GLINTSetupForScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenCopy();
void GLINTFillRectStippled();
void GLINTWriteBitmap();

void GLINTAccelInit() 
{
    xf86AccelInfoRec.Flags = 	PIXMAP_CACHE |
      				ONLY_LEFT_TO_RIGHT_BITBLT |
 				COP_FRAMEBUFFER_CONCURRENCY |
#if 0
      				HARDWARE_CLIP_LINE |
				TWO_POINT_LINE_NOT_LAST |
      				USE_TWO_POINT_LINE |
				LINE_PATTERN_MSBFIRST_LSBJUSTIFIED |
#endif
      				BACKGROUND_OPERATIONS |
				DELAYED_SYNC;

    /* With PCI Retry its faster to blit directly and ignore the cache */
    if (OFLG_ISSET(OPTION_PCI_RETRY, &glintInfoRec.options)) 
	xf86AccelInfoRec.Flags |= DO_NOT_BLIT_STIPPLES;

    xf86AccelInfoRec.PatternFlags =	HARDWARE_PATTERN_PROGRAMMED_BITS |
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
					HARDWARE_PATTERN_SCREEN_ORIGIN |
					HARDWARE_PATTERN_MONO_TRANSPARENCY;

    xf86AccelInfoRec.Sync = GLINTSync;
  
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;
  
    xf86AccelInfoRec.SetupForFillRectSolid = GLINTSetupForFillRectSolid;
#if 0
    xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;
#endif
    xf86AccelInfoRec.SubsequentFillRectSolid = GLINTSubsequentFillRectSolid;

#if 0
    xf86AccelInfoRec.SubsequentTwoPointLine = GLINTSubsequentTwoPointLine;
#endif
    xf86AccelInfoRec.SetClippingRectangle = GLINTSetClippingRectangle;

#if 0
    xf86AccelInfoRec.SetupForDashedLine = GLINTSetupForDashedLine;
    xf86AccelInfoRec.SubsequentDashedTwoPointLine = GLINTSubsequentDashedTwoPointLine;
    xf86AccelInfoRec.LinePatternBuffer = (void *)&DashPattern;
    xf86AccelInfoRec.LinePatternMaxLength = 16;
#endif
  
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
      GLINTSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
      GLINTSubsequentScreenToScreenCopy;
  
    xf86AccelInfoRec.ColorExpandFlags = VIDEO_SOURCE_GRANULARITY_PIXEL |
      					NO_SYNC_AFTER_CPU_COLOR_EXPAND |
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

#if 0
    xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
#endif

    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
      					GLINTSetupFor8x8PatternColorExpand;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
      					GLINTSubsequent8x8PatternColorExpand;

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
}

void GLINTSync() {
	while (GLINT_READ_REG(DMACount) != 0);
	GLINT_WRITE_REG(0x400, FilterMode);
	GLINT_WRITE_REG(0, GlintSync);
	do {
    		while(GLINT_READ_REG(OutFIFOWords) == 0);
#define Sync_tag 0x188
	} while (GLINT_READ_REG(OutputFIFO) != Sync_tag);
}

void GLINTSetupForFillRectSolid(int color, int rop, unsigned planemask)
{
	REPLICATE(color);
	gcolor = color;
	
	DO_PLANEMASK(planemask);

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		mode = FastFillEnable;
	} else {
		GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(color, PatternRamData0);
		mode = FastFillEnable | SpanOperation;
	}
	GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);

	GLINT_WRITE_REG(1<<16,dY);
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);
	GLINT_WRITE_REG(0,StartXSub);
}

void GLINTSubsequentFillRectSolid(int x, int  y, int  w, int  h)
{
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,StartXDom);
	GLINT_WRITE_REG(y<<16,StartY);
	GLINT_WRITE_REG(h,GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid | mode,Render);
}

void GLINTSetupForDashedLine(fg, bg, rop, planemask, size)
{
	REPLICATE(fg);
	REPLICATE(bg);

	gfg = fg;
	gbg = bg;

	DO_PLANEMASK(planemask);

	grop = rop;

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
	}

	GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
	GLINT_WRITE_REG(0, StartXSub);
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);
	GLINT_WRITE_REG(1<<16,dY);

	DashPatternSize = size;
}

void GLINTSubsequentDashedTwoPointLine(int x1, int y1, int x2, int y2,
				       int bias, int offset)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;


	GLINT_WRITE_REG(x1<<16, StartXDom);
	GLINT_WRITE_REG(y1<<16, StartY);

	if (abs(dx) > abs(dy)) {	/* XMajor axis (i.e. Horizontal) */
		if (dx < 0) {
			GLINT_WRITE_REG(-1<<16, dXDom);
			dx = -dx;
		} else {
			GLINT_WRITE_REG(1<<16, dXDom);
		}
		if (dx == 0) { 
			/* dx = 1; */
			GLINT_WRITE_REG(1<<16, dY); 
		} else {
			GLINT_WRITE_REG((dy<<16)/dx, dY);
		}
		if (!(bias & 0x100)) dx += 1;
		GLINT_WRITE_REG(abs(dx), GLINTCount);
	} else {			/* YMajor axis  (i.e. Vertical) */
		if (dy < 0) {
			GLINT_WRITE_REG(-1<<16, dY);
			dy = -dy;
		} else {
			GLINT_WRITE_REG(1<<16, dY);
		}
		if (dy == 0) { 
			/* dy = 1; */
			GLINT_WRITE_REG(0, dXDom); 
		} else { 
			GLINT_WRITE_REG(((dx<<16)/dy), dXDom); 
		}
		if (!(bias & 0x100)) dy += 1;
		GLINT_WRITE_REG(abs(dy), GLINTCount);
 	}

	if (gbg != -1) {
		if (miSpansEasyRop(grop)) {
		  if (grop == GXcopy) {
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(gbg, FBBlockColor);
			mode = FastFillEnable;
		  } else {
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(gbg, PatternRamData0);
			mode = FastFillEnable | SpanOperation;
		  }
	  	  GLINT_WRITE_REG(PrimitiveTrapezoid | mode,Render);
		} else {
		  GLINT_WRITE_REG(0, UpdateLineStippleCounters);
		  GLINT_WRITE_REG(DashPattern << 10 |  
				1<<17 | UNIT_ENABLE, LineStippleMode);
		  GLINT_WRITE_REG(gbg, GLINTColor);
		  GLINT_WRITE_REG(PrimitiveLine | LineStippleEnable, Render);
		}
	}

	GLINT_WRITE_REG(0, UpdateLineStippleCounters);
	GLINT_WRITE_REG(DashPattern << 10 | UNIT_ENABLE, LineStippleMode);
	GLINT_WRITE_REG(gfg, GLINTColor);
	GLINT_WRITE_REG(PrimitiveLine | LineStippleEnable, Render);

	GLINT_WRITE_REG(0, ScissorMode);
}

void GLINTSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	GLINT_WRITE_REG(gcolor, GLINTColor);
	GLINT_WRITE_REG(x1<<16, StartXDom);
	GLINT_WRITE_REG(y1<<16, StartY);

	if ((dx == 0) && (dy == 0)) { /* This is a point */
		GLINT_WRITE_REG(PrimitivePoint, Render);
		return;
	}

	if (abs(dx) > abs(dy)) {	/* XMajor axis (i.e. Horizontal) */
		if (dx < 0) {
			GLINT_WRITE_REG(-1<<16, dXDom);
			dx = -dx;
		} else {
			GLINT_WRITE_REG(1<<16, dXDom);
		}
		if (dx == 0) { 
			/* dx = 1; */
			GLINT_WRITE_REG(0, dY); 
		} else {
			GLINT_WRITE_REG(((dy<<16)/dx), dY);
		}
		if (!(bias & 0x100)) dx += 1;
		GLINT_WRITE_REG(abs(dx), GLINTCount);
	} else {			/* YMajor axis  (i.e. Vertical) */
		if (dy < 0) {
			GLINT_WRITE_REG(-1<<16, dY);
			dy = -dy;
		} else {
			GLINT_WRITE_REG(1<<16, dY);
		}
		if (dy == 0) { 
			/* dy = 1; */
			GLINT_WRITE_REG(0, dXDom); 
		} else { 
			GLINT_WRITE_REG(((dx<<16)/dy), dXDom); 
		}
		if (!(bias & 0x100)) dy += 1;
		GLINT_WRITE_REG(abs(dy), GLINTCount);
	} 

	GLINT_WRITE_REG(PrimitiveLine, Render);
	GLINT_WRITE_REG(0, ScissorMode);
}

void GLINTSetClippingRectangle(int x1, int y1, int x2, int y2)
{
	GLINT_WRITE_REG(y1<<16|x1, ScissorMinXY);
	GLINT_WRITE_REG(y2<<16|x2, ScissorMaxXY);
	GLINT_WRITE_REG(1, ScissorMode); /* Enable Scissor Mode */
}

void GLINTSetupForScreenToScreenCopy( int xdir, int  ydir, int  rop,
				    unsigned planemask, int transparency_color)
{
	blitydir = ydir;
	blitxdir = xdir;

	DO_PLANEMASK(planemask);

	GLINT_WRITE_REG(0, PatternRamMode);

	if ((rop == GXclear) || (rop == GXset)) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else 
	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | blitmode, FBReadMode);
	}

	GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
}

void GLINTSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2,
				     int w, int h)
{
	int srcaddr;
	int dstaddr;

	/* Glint 500TX only allows left to right bitblt's */

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
	GLINT_WRITE_REG(0, FBSourceOffset);
}

void GLINTSetupForScanlineScreenToScreenColorExpand(int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{	
  REPLICATE(fg);
  gbg = bg;
  gfg = fg;

  DO_PLANEMASK(planemask);

  grop = rop;
  if (rop == GXcopy) {
    GLINT_WRITE_REG(pprod, FBReadMode);
    GLINT_WRITE_REG(fg, FBBlockColor);
    GLINT_WRITE_REG(0, PatternRamMode);
    mode = FastFillEnable;
  } else {
    GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    GLINT_WRITE_REG(1, PatternRamMode);
    GLINT_WRITE_REG(fg, PatternRamData0);
    mode = FastFillEnable | SpanOperation;
  }
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(0, dXDom);
  GLINT_WRITE_REG(0, dXSub);
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
  register CARD32 *ptr = (CARD32 *)ScratchBuffer;
  int count = ScanlineWordCount;

  while (count--){
    GLINT_WRITE_REG(*(ptr++), BitMaskPattern);
  }
}

void
GLINTSetupForFill8x8Pattern(patternx, patterny, rop, planemask,transparency_color)
	int patternx, patterny, rop, planemask, transparency_color;
{
  GLINT_WRITE_REG(0,         dXDom);
  GLINT_WRITE_REG(0,         dXSub);
  GLINT_WRITE_REG(1<<16,     dY);
  GLINT_WRITE_REG((patternx+8)<<16, StartXSub);
  GLINT_WRITE_REG(patternx<<16,     StartXDom);
  GLINT_WRITE_REG(patterny<<16,     StartY);
  GLINT_WRITE_REG(8,         GLINTCount);
  GLINT_WRITE_REG((1<<9)|(1<<6)|(0x0F<<1)|1, PatternRamMode);
  GLINT_WRITE_REG(0, LogicalOpMode);
  GLINT_WRITE_REG(0, FBWriteMode);
  GLINT_WRITE_REG(0xFFFFFFFF, FBHardwareWriteMask);
  GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
  GLINT_WRITE_REG(PrimitiveTrapezoid, Render);

  GLINT_WRITE_REG(UNIT_ENABLE, FBWriteMode);
	
  DO_PLANEMASK(planemask);

  if (rop == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
  } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
  }
  GLINT_WRITE_REG((2<<9)|(2<<6)|(0x0F<<1)|1, PatternRamMode);
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
}

void GLINTSubsequentFill8x8Pattern(int patternx, int patterny, int x, int y,
				   int w, int h)
{
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,     StartXDom);
	GLINT_WRITE_REG(y<<16,     StartY);
	GLINT_WRITE_REG(h,         GLINTCount);
	GLINT_WRITE_REG(PrimitiveTrapezoid|FastFillEnable|SpanOperation,Render);
}

void GLINTSetupFor8x8PatternColorExpand(int patternx, int patterny, 
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

  GLINT_WRITE_REG((rop<<1) | UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16, dY);	
  GLINT_WRITE_REG(0, dXDom);
  GLINT_WRITE_REG(0, dXSub);
}

void 
GLINTSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  int span = 0;
  
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(x<<16,     StartXDom);
  GLINT_WRITE_REG(y<<16,     StartY);
  GLINT_WRITE_REG(h,         GLINTCount);

  if (mode != -1) {
	if (miSpansEasyRop(grop)) {
  	  if (grop == GXcopy) {
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(gbg, FBBlockColor);
		mode = FastFillEnable;
	  } else {
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(gbg, PatternRamData0);
		mode = FastFillEnable | SpanOperation;
	  }
	  GLINT_WRITE_REG(PrimitiveTrapezoid | mode,Render);
	} else {
  	  	if (grop == GXcopy) {
			GLINT_WRITE_REG(gbg, FBBlockColor);
			GLINT_WRITE_REG(0, PatternRamMode);
			span = 0;
		} else {
			GLINT_WRITE_REG(1, PatternRamMode);
  			GLINT_WRITE_REG(gbg, PatternRamData0);
			span = SpanOperation;
		}
		GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12| ASM_InvertPattern |
			UNIT_ENABLE, AreaStippleMode);
		GLINT_WRITE_REG(AreaStippleEnable | span | FastFillEnable | 
						PrimitiveTrapezoid, Render);
	}
  }

  if (grop == GXcopy) {
	GLINT_WRITE_REG(gfg, FBBlockColor);
	GLINT_WRITE_REG(0, PatternRamMode);
	span = 0;
  } else {
	GLINT_WRITE_REG(1, PatternRamMode);
  	GLINT_WRITE_REG(gfg, PatternRamData0);
	span = SpanOperation;
  }
  GLINT_WRITE_REG(2<<1|2<<4|patternx<<7|patterny<<12|
  						UNIT_ENABLE, AreaStippleMode);
  GLINT_WRITE_REG(AreaStippleEnable | span | FastFillEnable | 
						PrimitiveTrapezoid, Render);
}

void
GLINTSubsequentFillTrapezoidSolid(int y, int h, int left, int dxl, int dyl, int el, 
				  int right, int dxr, int dyr, int er)
{

  GLINT_WRITE_REG(left<<16, StartXDom);
  GLINT_WRITE_REG((right+1)<<16, StartXSub);
  GLINT_WRITE_REG((dxl<<16)/dyl, dXDom); 
  GLINT_WRITE_REG((dxr<<16)/dyr, dXSub);

  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG(h, GLINTCount);

  GLINT_WRITE_REG(PrimitiveTrapezoid | mode, Render);
}

void
GLINTSubsequentScanlineScreenToScreenCopy(LineAddr, skipleft, x, y, w)
	int LineAddr, skipleft, x, y, w;
{
	int dstaddr;


	GLINT_WRITE_REG(-1<<16, dY);

	dstaddr = GLINTWindowBase + y * glintInfoRec.displayWidth + x;

	GLINT_WRITE_REG((LineAddr/(glintInfoRec.bitsPerPixel/8)) - dstaddr, FBSourceOffset);

	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(1, GLINTCount);
	
	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | SpanOperation, Render);
	GLINT_WRITE_REG(0, FBSourceOffset);
}

int stip_width, stip_height;

void
GLINTStipple32x32Pattern(x, y, w, h, srcx, srcy)
	int x, y, w, h, srcx, srcy;
{
  int width, height;
  GLINT_WRITE_REG((x+w)<<16, StartXSub);
  GLINT_WRITE_REG(x<<16,     StartXDom);
  GLINT_WRITE_REG(y<<16,     StartY);
  GLINT_WRITE_REG(h,         GLINTCount);

  if (stip_width <= 32) width = 4;
  if (stip_width <= 16) width = 3;
  if (stip_width <= 8) width = 2;
  if (stip_width <= 4) width = 1;
  if (stip_width <= 2) width = 0;
  if (stip_height <= 32) height = 4;
  if (stip_height <= 16) height = 3;
  if (stip_height <= 8) height = 2;
  if (stip_height <= 4) height = 1;
  if (stip_height <= 2) height = 0;
width = 4; height = 4;

  GLINT_WRITE_REG(width<<1|height<<4|srcx<<7|srcy<<12|mode<<17|UNIT_ENABLE, AreaStippleMode);

  GLINT_WRITE_REG(AreaStippleEnable | span | FastFillEnable | 
						PrimitiveTrapezoid, Render);
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
  GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | span | SyncOnBitMask, Render);

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

#ifdef QWORD_PADDING
    if(PlusOne) {
	GLINT_WRITE_REG(0, BitMaskPattern); 
    }
#endif 
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
    int i;
    int color;
    int nBox;
    int xoffset, yoffset, StippleType;
    PixmapPtr pPixmap = pGC->stipple;

    /* 6 types of stipples. TransparentStipples, OpaqueEasyStipples and
	OpaqueHardStipples.  Each of these in both HardwarePattern
	and CPUToScreen versions */
 
    /* But for now, we only use CPUToScreen as Hardware pattern can
       only be in power of 2 */

    if(pGC->fillStyle == FillStippled){	/* Transparent Stipples */
	StippleType = 0;
	color = pGC->fgPixel; REPLICATE(color);
	/* >>>>>>>>>>>>>>>>>  Set fg  <<<<<<<<<<<<<<<<<< */
	if (pGC->alu == GXcopy) {
		span = 0;
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(color, FBBlockColor);
	} else {
		span = SpanOperation;
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(color, PatternRamData0);
	}
    } else if(miSpansEasyRop(pGC->alu))  /* OpaqueEasyStipples */
	StippleType = 2;
    else 				/* OpaqueHardStipples */
	StippleType = 4;

    stip_width = pPixmap->drawable.width;
    stip_height = pPixmap->drawable.height;

#if 0
    if((stip_width <= 32) && (stip_height <= 32) /* && !(stip_width & (stip_width -1)) && !(stip_height & (stip_height - 1)) */) {
  	register CARD32 *ptr = (CARD32 *)pPixmap->devPrivate.ptr;
	StippleType++;   
	/* >>>>>>>>>>>>>>>>  Load pattern <<<<<<<<<<<<<<<<<<<<<  */
	/* Size is pPixmap->drawable.width by pPixmap->drawable.height */
	/* starting at pPixmap->devPrivate.ptr in memory with a */
	/* pPixmap->devKind byte scanline pitch. */
	for (i=0;i<16;i++) {
		GLINT_WRITE_REG(*(ptr++), GLINT_TAG_ADDR(0x04,i));
	}
	if (stip_height > 16) {
		for (i=0;i<(stip_height-16);i++) {
			GLINT_WRITE_REG(*(ptr++), GLINT_TAG_ADDR(0x05,i));
		}
	}
    }
#endif

    /* >>>>>>>>>>> Set ROP, Planemask <<<<<<<<<<< */
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    DO_PLANEMASK(pGC->planemask);
    if (pGC->alu == GXcopy) {
	span = 0;
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	span = SpanOperation;
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }

    if(StippleType < 4) {
    	/* >>>>> Make sure our bitmask is not inverted <<<<<< */
	GLINT_WRITE_REG(0, RasterizerMode);
	mode = 0;
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
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 1: 
		GLINTStipple32x32Pattern(pBox->x1, pBox->y1, 
			rectWidth, rectHeight, xoffset, yoffset);
		break;
	      case 2:
		/* >>>>>>>>>>>  Set bg and draw rect <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
		    span = 0;
		    GLINT_WRITE_REG(0, PatternRamMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = SpanOperation;
		    GLINT_WRITE_REG(1, PatternRamMode);
		    GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINT_WRITE_REG(PrimitiveTrapezoid |span|FastFillEnable,Render);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      case 3:
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
		    span = 0;
		    GLINT_WRITE_REG(0, PatternRamMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = SpanOperation;
		    GLINT_WRITE_REG(1, PatternRamMode);
		    GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINT_WRITE_REG(PrimitiveTrapezoid |span|FastFillEnable,Render);
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStipple32x32Pattern(pBox->x1, pBox->y1, 
			rectWidth, rectHeight, xoffset, yoffset);
		break;
	      case 4:
    		/* >>>>> Make sure our bitmask is not inverted <<<<<< */
		GLINT_WRITE_REG(0, RasterizerMode);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
	/*	>>>>>>>>>>  Invert Mask <<<<<<<<< */
		GLINT_WRITE_REG(2, RasterizerMode);
	/*	>>>>>>>>>>  Set bg <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStippleCPUToScreen(
	        	pBox->x1, pBox->y1, rectWidth, rectHeight,
	       	 	pPixmap->devPrivate.ptr, pPixmap->devKind,
	        	pPixmap->drawable.width, pPixmap->drawable.height,
	        	xoffset, yoffset);
		break;
	      default:
    		/* >>>>> Make sure our bitmask is not inverted <<<<<< */
		mode = 0;
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStipple32x32Pattern(pBox->x1, pBox->y1, 
			rectWidth, rectHeight, xoffset, yoffset);
	/*	>>>>>>>>>>  Invert Mask Area Stipple Mask <<<<<<<<< */
		mode = 1;
	/*	>>>>>>>>>>  Set bg <<<<<<<<<<<<<< */
		color = pGC->bgPixel; REPLICATE(color);
		if (pGC->alu == GXcopy) {
			span = 0;
			GLINT_WRITE_REG(0, PatternRamMode);
			GLINT_WRITE_REG(color, FBBlockColor);
		} else {
			span = SpanOperation;
			GLINT_WRITE_REG(1, PatternRamMode);
			GLINT_WRITE_REG(color, PatternRamData0);
		}
		GLINTStipple32x32Pattern(pBox->x1, pBox->y1, 
			rectWidth, rectHeight, xoffset, yoffset);
		break;
	    }
	}
    }

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
    GLINT_WRITE_REG(0, RasterizerMode);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    if (rop == GXcopy) {
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
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
	GLINTSetClippingRectangle(x+skipleft, y, x+w, y+h);

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = 0;
	    GLINT_WRITE_REG(0, PatternRamMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = SpanOperation;
	    GLINT_WRITE_REG(1, PatternRamMode);
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
	if (rop == GXcopy) {
	    mode = 0;
	    GLINT_WRITE_REG(0, PatternRamMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    mode = SpanOperation;
	    GLINT_WRITE_REG(1, PatternRamMode);
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	GLINT_WRITE_REG(PrimitiveTrapezoid |mode|FastFillEnable,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = 0;
	    GLINT_WRITE_REG(0, PatternRamMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = SpanOperation;
	    GLINT_WRITE_REG(1, PatternRamMode);
	    GLINT_WRITE_REG(fg, PatternRamData0);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
	if (rop == GXcopy) {
	    mode = 0;
	    GLINT_WRITE_REG(0, PatternRamMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = SpanOperation;
	    GLINT_WRITE_REG(1, PatternRamMode);
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
	count = dwords;
	pattern = (CARD32*)srcpntr;
	while(count--) {
		GLINT_WRITE_REG(*(pattern++), BitMaskPattern);
	}
	srcpntr += srcwidth;
    }    

#if QWORD_PADDED
    if(PlusOne) GLINT_SLOW_WRITE_REG(0);
#endif

    if(SecondPass) {
	SecondPass = FALSE;
	/* >>>>>> invert bitmask and set bg <<<<<<<< */
	REPLICATE(bg);
	GLINT_WRITE_REG(2, RasterizerMode);
	if (rop == GXcopy) {
	    mode = 0;
	    GLINT_WRITE_REG(0, PatternRamMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    mode = SpanOperation;
	    GLINT_WRITE_REG(1, PatternRamMode);
	    GLINT_WRITE_REG(bg, PatternRamData0);
	}
	goto SECOND_PASS;
    }

    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(0, ScissorMode);
    SET_SYNC_FLAG;	
}
