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
 *	     Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 * 
 * GLINT accelerated options.
 */
#include "cfb.h"
#include "xf86.h"
#include "miline.h"
#include "compiler.h"

#include "glint_regs.h"
#include "glint.h"
#include "xf86xaa.h"

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
extern GLINTWindowBase;
int gcolor;
int notline = 0;
int mode;
int mode2;
int scanlinedstaddr;

void GLINTSync();
void GLINTSetupForFillRectSolid();
void GLINTSubsequentFillRectSolid();
void GLINTSubsequentFillTrapezoidSolid();
void GLINTSubsequentTwoPointLine();
void GLINTSetupForScreenToScreenCopy();
void GLINTSubsequentScreenToScreenCopy();
void GLINTSetClippingRectangle();
void GLINTSetupForFill8x8Pattern();
void GLINTSubsequentFill8x8Pattern();
void GLINTSetupFor8x8PatternColorExpand();
void GLINTSubsequent8x8PatternColorExpand();
void GLINTSetupForScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenColorExpand();
	
/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void GLINTAccelInit() {

    xf86AccelInfoRec.Flags = PIXMAP_CACHE |
			     ONLY_LEFT_TO_RIGHT_BITBLT |
			     COP_FRAMEBUFFER_CONCURRENCY |
#if 0
			     HORIZONTAL_TWOPOINTLINE |
#endif
#if 0
			     HARDWARE_CLIP_LINE |
#endif
			     BACKGROUND_OPERATIONS |
			     DELAYED_SYNC;

#if 0
    xf86AccelInfoRec.PatternFlags =
			     HARDWARE_PATTERN_SCREEN_ORIGIN |
			     HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
			     HARDWARE_PATTERN_MONO_TRANSPARENCY | 
			     HARDWARE_PATTERN_PROGRAMMED_BITS;
#endif
				
    xf86AccelInfoRec.Sync = GLINTSync;

    xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;

    xf86AccelInfoRec.SetupForFillRectSolid = GLINTSetupForFillRectSolid;
#if 0
	xf86AccelInfoRec.SubsequentFillTrapezoidSolid = GLINTSubsequentFillTrapezoidSolid;
#endif
    xf86AccelInfoRec.SubsequentFillRectSolid = GLINTSubsequentFillRectSolid;
#if 0
    xf86AccelInfoRec.SubsequentTwoPointLine = GLINTSubsequentTwoPointLine;
#endif
#if 0
    xf86AccelInfoRec.SetClippingRectangle = GLINTSetClippingRectangle;
#endif
    xf86GCInfoRec.CopyAreaFlags = NO_PLANEMASK|NO_TRANSPARENCY;

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
       		GLINTSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
	       	GLINTSubsequentScreenToScreenCopy;

#if 0
    xf86AccelInfoRec.ColorExpandFlags = NO_PLANEMASK;
#endif
#if 0
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
				GLINTSetupForScanlineScreenToScreenColorExpand;
    xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
				GLINTSubsequentScanlineScreenToScreenColorExpand;
    xf86AccelInfoRec.ScratchBufferSize = 16384;
    xf86AccelInfoRec.ScratchBufferAddr = glintInfoRec.videoRam * 1024 - 16384 - 1024;
#endif

#if 0
    xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
#endif

#if 0
    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
				GLINTSetupFor8x8PatternColorExpand;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
				GLINTSubsequent8x8PatternColorExpand;
#endif

    xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;

    xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.virtualY *
		glintInfoRec.displayWidth * glintInfoRec.bitsPerPixel / 8;

    xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024
						- 1024 - 16384;
}

/*
 * This is the implementation of the Sync() function.
 */
void GLINTSync() {
	while (GLINT_READ_REG(DMACount) != 0);
    	GLINT_WAIT(2);
	GLINT_WRITE_REG(0xc00, FilterMode);
	GLINT_WRITE_REG(0, GlintSync);
	do {
    		while(GLINT_READ_REG(OutFIFOWords) == 0);
#define Sync_tag 0x188
	} while (GLINT_READ_REG(OutputFIFO) != Sync_tag);
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */

void GLINTSetupForFillRectSolid(int color, int rop, unsigned planemask)
{
	GLINT_WAIT(11);

	if (glintInfoRec.bitsPerPixel < 32)
	{
		color |= color << 16;
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			color |= color << 8;
			planemask |= planemask << 8;
		}
	}

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		GLINT_WRITE_REG(0, LogicalOpMode);
		mode = FASTFILL;
	} else {
		GLINT_WRITE_REG(pprod | ReadDestination, FBReadMode);
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(color, PatternRamData0);
		GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
		mode = 0;
	}

	GLINT_WRITE_REG(color, Color);
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);
	GLINT_WRITE_REG(0, FBPixelOffset);
	GLINT_WRITE_REG(0, ScissorMode);
	GLINT_WRITE_REG(0, AreaStippleMode);
	GLINT_WRITE_REG(WriteEnable, FBWriteMode);
	GLINT_WRITE_REG(1<<16,dY);
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void GLINTSubsequentFillRectSolid(int x, int  y, int  w, int  h)
{
	GLINT_WAIT(5);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,StartXDom);
	GLINT_WRITE_REG(y<<16,StartY);
	GLINT_WRITE_REG(h,Count);

	GLINT_WRITE_REG(TRAPEZOID | mode,Render);
}

void GLINTSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	GLINT_WAIT(6);
	GLINT_WRITE_REG(x1<<16, StartXDom);
	GLINT_WRITE_REG(y1<<16, StartY);

#if 0
	if ((dx == 0) && (dy == 0)) { /* This is a point */
		GLINT_WRITE_REG(POINT, Render);
		return;
	}
#endif

	if (abs(dx) > abs(dy)) {	/* XMajor axis (i.e. Horizontal) */
		if (dx < 0) {
			GLINT_WRITE_REG(-1<<16, dXDom);
			dx = -dx;
		} else {
			GLINT_WRITE_REG(1<<16, dXDom);
		}
		if (dx == 0) { 
			dx = 1;
			GLINT_WRITE_REG(0, dY); 
		} else {
			GLINT_WRITE_REG((dy<<16)/dx, dY);
		}
		GLINT_WRITE_REG(abs(dx), Count);
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
			GLINT_WRITE_REG((dx<<16)/dy, dXDom); 
		}
		GLINT_WRITE_REG(abs(dy+1), Count);
	}

	GLINT_WRITE_REG(LINE, Render);
}

void GLINTSetClippingRectangle(int x1, int y1, int x2, int y2)
{
	GLINT_WAIT(4);
	GLINT_WRITE_REG(y1<<16|x1, ScissorMaxXY);
	GLINT_WRITE_REG(y2<<16|x2, ScissorMinXY);

	GLINT_WRITE_REG(glintInfoRec.displayWidth-1 | (glintInfoRec.virtualY-1)<<16,
						ScreenSize);
	GLINT_WRITE_REG(0x03, ScissorMode); /* Enable Scissor Mode */
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch for solid
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */
static int blitxdir, blitydir;
 
void GLINTSetupForScreenToScreenCopy( int xdir, int  ydir, int  rop,
				    unsigned planemask, int transparency_color)
{
	blitydir = ydir;
	blitxdir = xdir;

	notline = 0;

	GLINT_WAIT(11);
	if (glintInfoRec.bitsPerPixel < 32)
	{
		transparency_color |= transparency_color << 16;
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			transparency_color |= transparency_color << 8;
			planemask |= planemask << 8;
		}
	}

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod | ReadSource, FBReadMode);
	} else {  
		GLINT_WRITE_REG(pprod | ReadSource | ReadDestination, FBReadMode);
	}

	GLINT_WRITE_REG(0, AreaStippleMode);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);

#if 0
	if (transparency_color != -1) {
		GLINT_WRITE_REG(transparency_color, FBBlockColor);
	}
#endif

	GLINT_WRITE_REG(0, ScissorMode);
	GLINT_WRITE_REG(0, PatternRamMode);
	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
	GLINT_WRITE_REG(WriteEnable, FBWriteMode);
}
/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void GLINTSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2,
				     int w, int h)
{
	int srcaddr;
	int dstaddr;

	GLINT_WAIT(7);
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
	GLINT_WRITE_REG(h, Count);

	GLINT_WRITE_REG(TRAPEZOID | FASTFILL | SPANOPERATION, Render);
}

void GLINTSetupForScanlineScreenToScreenColorExpand(int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
	GLINT_WAIT(14);

	if (glintInfoRec.bitsPerPixel < 32)
	{
		fg |= fg << 16;
		bg |= bg << 16;
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			bg |= bg << 8;
			fg |= fg << 8;
			planemask |= planemask << 8;
		}
	}

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod | ReadSource, FBReadMode);
	} else {  
		GLINT_WRITE_REG(pprod | ReadDestination | ReadSource, FBReadMode);
	}

	GLINT_WRITE_REG(fg, ConstantColor);
	GLINT_WRITE_REG(bg, Color);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	GLINT_WRITE_REG(0, ScissorMode);
	GLINT_WRITE_REG(0, PatternRamMode);
	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(h, Count);
	GLINT_WRITE_REG(1<<16, dY);
	scanlinedstaddr = y * glintInfoRec.displayWidth + x;
}

void GLINTSubsequentScanlineScreenToScreenColorExpand(int srcaddr)
{
	GLINT_WAIT(2);
	GLINT_WRITE_REG((srcaddr/8) - scanlinedstaddr, FBSourceOffset);

	GLINT_WRITE_REG(SPANOPERATION | TRAPEZOID | FASTFILL, Render);
}

void GLINTSetupForFill8x8Pattern(int patternx, int patterny, int rop,
				 unsigned planemask, int transparency_color)
{
	GLINT_WAIT(15);
#if 1
	GLINT_WRITE_REG((patternx/8)<<16, StartXDom);
	GLINT_WRITE_REG(((patternx/8)+4)<<16, StartXSub);
	GLINT_WRITE_REG(patterny<<16, StartY);
	GLINT_WRITE_REG(8, Count);
	GLINT_WRITE_REG(1<<16, dY);
	GLINT_WRITE_REG(0, LogicalOpMode);
	GLINT_WRITE_REG(1 | (0x0F << 1) | (1 << 6) | (1 << 9), PatternRamMode);
	GLINT_WRITE_REG(pprod | ReadSource, FBReadMode); /* Get pattern */
	GLINT_WRITE_REG(8, FBWriteMode);
	GLINT_WRITE_REG(TRAPEZOID | FASTFILL, Render);
#endif

	if (glintInfoRec.bitsPerPixel < 32)
	{
		transparency_color |= transparency_color << 16;
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			transparency_color |= transparency_color << 8;
			planemask |= planemask << 8;
		}
	}

#if 0
	if (transparency_color != -1) {
		GLINT_WRITE_REG(transparency_color, FBBlockColor);
	}
#endif

	GLINT_WRITE_REG(WriteEnable, FBWriteMode);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | ReadDestination, FBReadMode);
	}
}

void GLINTSubsequentFill8x8Pattern(int patternx, int patterny, int x, int y,
				   int w, int h)
{
	GLINT_WAIT(6);
	GLINT_WRITE_REG(1<<16, dY);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(h, Count);

	GLINT_WRITE_REG(TRAPEZOID | FASTFILL, Render);
}

int gbg, gfg;

void GLINTSetupFor8x8PatternColorExpand(int patternx, int patterny, 
					int bg, int fg, int rop,
					unsigned planemask)
{
	GLINT_WAIT(16);
	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
	GLINT_WRITE_REG((patternx&0xff), AreaStipplePattern0);
	GLINT_WRITE_REG((patternx&0xff00)>>8, AreaStipplePattern1);
	GLINT_WRITE_REG((patternx&0xff0000)>>16, AreaStipplePattern2);
	GLINT_WRITE_REG((patternx&0xff000000)>>24, AreaStipplePattern3);
	GLINT_WRITE_REG((patterny&0xff), AreaStipplePattern4);
	GLINT_WRITE_REG((patterny&0xff00)>>8, AreaStipplePattern5);
	GLINT_WRITE_REG((patterny&0xff0000)>>16, AreaStipplePattern6);
	GLINT_WRITE_REG((patterny&0xff000000)>>24, AreaStipplePattern7);

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | ReadDestination, FBReadMode);
	}
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);

	if (glintInfoRec.bitsPerPixel < 32)
	{
		fg |= fg << 16;
		bg |= bg << 16;
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			fg |= fg << 8;
			bg |= bg << 8;
			planemask |= planemask << 8;
		}
	}

	GLINT_WRITE_REG(0, ScissorMode);
	gfg = fg;
	gbg = bg;

	GLINT_WRITE_REG(0, PatternRamMode);
	GLINT_WRITE_REG(1<<16, dY);
}

void GLINTSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
	GLINT_WAIT(7);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(h, Count);
	GLINT_WRITE_REG(gfg, FBBlockColor);
	GLINT_WRITE_REG(1 | 2 << 1 | 2 << 4 | patternx << 7 | 
			patterny << 12 | 0 << 17, AreaStippleMode);

	GLINT_WRITE_REG(AREASTIPPLE | TRAPEZOID | FASTFILL, Render);

	if (gbg != -1) {
		GLINT_WAIT(3);
		GLINT_WRITE_REG(gbg, FBBlockColor);
		GLINT_WRITE_REG(1 | 2 << 1 | 2 << 4 | patternx << 7 | 
			patterny << 12 | 1 << 17, AreaStippleMode);
		GLINT_WRITE_REG(AREASTIPPLE | TRAPEZOID | FASTFILL, Render);
	}
}

void
GLINTSubsequentFillTrapezoidSolid(y, h, left, dxl, dyl, el, right, dxr, dyr, er)
int y, h, left, dxl, dyl, el, right, dxr, dyr, er;
{
	GLINT_WAIT(7);
	GLINT_WRITE_REG(left<<16, StartXDom);
	GLINT_WRITE_REG(right<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(h, Count);
	GLINT_WRITE_REG((dxl<<16)/dyl,dXDom);
	GLINT_WRITE_REG((dxr<<16)/dyr,dXSub);

	GLINT_WRITE_REG(TRAPEZOID | FASTFILL | mode,Render);
}
