/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_accel.c,v 1.7 1997/09/25 07:31:12 hohndel Exp $ */
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
static int ScanlineWordCount;
int BitMaskOffset;
int gcolor;
int notline = 0;
int mode;
int ropsave, xcoord, ycoord, width, height;
int gbg, gfg;
int mode2;
short DashPattern;
int DashPatternSize;
CARD32 ScratchBuffer[512];
static int blitxdir, blitydir;

void GLINTSync();
void PermediaSetupForFillRectSolid();
void PermediaSubsequentFillRectSolid();
void GLINTSetupForFillRectSolid();
void GLINTSubsequentFillRectSolid();
void GLINTSubsequentFillTrapezoidSolid();
void PermediaSubsequentFillTrapezoidSolid();
void GLINTSubsequentTwoPointLine();
void GLINTSetupForDashedLine();
void GLINTSubsequentDashedTwoPointLine();
void PermediaSetupForScreenToScreenCopy();
void PermediaSubsequentScreenToScreenCopy();
void GLINTSetupForScreenToScreenCopy();
void GLINTSubsequentScreenToScreenCopy();
void GLINTSetClippingRectangle();
void GLINTSetupForFill8x8Pattern();
void GLINTSubsequentFill8x8Pattern();
void GLINTSetupFor8x8PatternColorExpand();
void GLINTSubsequent8x8PatternColorExpand();
void GLINTSetupForScanlineScreenToScreenColorExpand();
void GLINTSubsequentScanlineScreenToScreenColorExpand();
	

#define REPLICATE(r)								\
{														\
	if (glintInfoRec.bitsPerPixel < 32)		\
	{													\
		r |= r << 16;								\
		if (glintInfoRec.bitsPerPixel < 16)	\
		{												\
			r |= r << 8;							\
		}												\
	}													\
}


void GLINTAccelInit() {
    if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	xf86AccelInfoRec.Flags = GXCOPY_ONLY;
#if 0
	    BACKGROUND_OPERATIONS | 
	    COP_FRAMEBUFFER_CONCURRENCY |  
	    DELAYED_SYNC; 
#endif
	xf86AccelInfoRec.PatternFlags = 0;
    } else if( IS_3DLABS_TX_MX_CLASS(coprotype)) {
	xf86AccelInfoRec.Flags = PIXMAP_CACHE |
	    ONLY_LEFT_TO_RIGHT_BITBLT |
	    COP_FRAMEBUFFER_CONCURRENCY |
#if 0
	    HORIZONTAL_TWOPOINTLINE |
	    HARDWARE_CLIP_LINE |
	    USE_TWO_POINT_LINE |
	    TWO_POINT_LINE_NOT_LAST |
#endif
	    BACKGROUND_OPERATIONS |
	    NO_CAP_NOT_LAST |
	    DELAYED_SYNC;
  
#if 0
	xf86AccelInfoRec.PatternFlags =
	    HARDWARE_PATTERN_NOT_LINEAR |
	    HARDWARE_PATTERN_SCREEN_ORIGIN |
	    HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
	    HARDWARE_PATTERN_MONO_TRANSPARENCY | 
	    HARDWARE_PATTERN_TRANSPARENCY |
	    HARDWARE_PATTERN_PROGRAMMED_BITS;
#endif
    }
    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
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
  
	xf86AccelInfoRec.ColorExpandFlags = VIDEO_SOURCE_GRANULARITY_PIXEL |
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
	xf86AccelInfoRec.ScratchBufferBase = (void*)ScratchBuffer;
	xf86AccelInfoRec.PingPongBuffers = 1;

#if 0 /* Pattern upload is not yet working.... */
	xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
	xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
#endif
  
#if 0
	xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
	    GLINTSetupFor8x8PatternColorExpand;
	xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
	    GLINTSubsequent8x8PatternColorExpand;
#endif
  
    } else if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	xf86AccelInfoRec.Sync = GLINTSync;

	xf86GCInfoRec.PolyFillRectSolidFlags     = NO_PLANEMASK;
	xf86AccelInfoRec.SetupForFillRectSolid   = PermediaSetupForFillRectSolid;
	xf86AccelInfoRec.SubsequentFillRectSolid = PermediaSubsequentFillRectSolid;
	xf86AccelInfoRec.SubsequentFillTrapezoidSolid = PermediaSubsequentFillTrapezoidSolid;
	
	xf86AccelInfoRec.SetupForScreenToScreenCopy = PermediaSetupForScreenToScreenCopy;
	xf86AccelInfoRec.SubsequentScreenToScreenCopy =  PermediaSubsequentScreenToScreenCopy;


	xf86AccelInfoRec.SetClippingRectangle = GLINTSetClippingRectangle;

	xf86AccelInfoRec.SetupForDashedLine = GLINTSetupForDashedLine;
	xf86AccelInfoRec.SubsequentDashedTwoPointLine = GLINTSubsequentDashedTwoPointLine;
	xf86AccelInfoRec.LinePatternBuffer = (void *) &DashPattern;
	xf86AccelInfoRec.LinePatternMaxLength = 16;
  
	/* xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | NO_PLANEMASK; */
	/*  
	xf86AccelInfoRec.ColorExpandFlags = VIDEO_SOURCE_GRANULARITY_PIXEL |
	    CPU_TRANSFER_PAD_DWORD |
	    ONLY_TRANSPARENCY_SUPPORTED | 
	    BIT_ORDER_IN_BYTE_LSBFIRST |
	    NO_PLANEMASK;
	    */
	/*
	xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
	    GLINTSetupForScanlineScreenToScreenColorExpand;
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
	    GLINTSubsequentScanlineScreenToScreenColorExpand;
	    */

	xf86AccelInfoRec.ScratchBufferAddr = 1;
	xf86AccelInfoRec.ScratchBufferSize = 512;
	xf86AccelInfoRec.ScratchBufferBase = (void*)ScratchBuffer;
	xf86AccelInfoRec.PingPongBuffers = 1;
  
#if 0 /* Pattern upload is not yet working.... */
	xf86AccelInfoRec.SetupForFill8x8Pattern = GLINTSetupForFill8x8Pattern;
	xf86AccelInfoRec.SubsequentFill8x8Pattern = GLINTSubsequentFill8x8Pattern;
#endif
	
	xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
	    GLINTSetupFor8x8PatternColorExpand;
	xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
	    GLINTSubsequent8x8PatternColorExpand;
	    
    }

    xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;

    xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.virtualY *
	glintInfoRec.displayWidth * glintInfoRec.bitsPerPixel / 8;

    xf86AccelInfoRec.PixmapCacheMemoryEnd = 
	glintInfoRec.videoRam * 1024 - 1024 - 16384;
}


#define F_BIAS			127
#define F_SIGN_BIT		31
#define F_EXPONENT_BITS		23
#define F_FRACTION_BITS		0


long IntToFixedPoint16 ( long i )
{
	return i << 16;
}

/*

 Converts 32 bit floatP to 9.15 fixedP used for color parameters.The input
 range is assumed to be 0.0 - 1.0

*/
long FloatToColor( float fi )
{
	long 				f= *((long *) &fi);
	long 				sign;
	unsigned char 	exponent;
	
	sign = ( f >> F_SIGN_BIT );
	exponent = (unsigned char)(f >> F_EXPONENT_BITS);
	if (exponent < (F_BIAS-15) )
		return (0);
	if (exponent > (F_BIAS+8) )
	{
		f = ((unsigned long)((f | 0x00800000) << 8 )
					>> ((F_BIAS+16) - exponent));
		if (sign < 0)
			f = -f;
		return (f);
	}
	return ( 0x007fffff ^ sign);
}



/*

 Converts 32 bit floatP to 16.16 fixedP used for the rasterizer parameters

*/
long FloatToCoord (float fi)
{
	long 				f= *((long *) &fi);
	long 				sign;
	unsigned char 	exponent;
	long				res;

	sign = f >> F_SIGN_BIT;
	exponent = (unsigned char) (f >> F_EXPONENT_BITS);
	if (exponent < (F_BIAS-16) )
		return(0);
	if (exponent < (F_BIAS+15) )
	{
		res = ((unsigned long)((f | 0x00800000) << 8)
						>> ( (F_BIAS+15) - exponent) );
		if (sign < 0)
			res = -res;
		return (res);
	}
	return (0x7FFFFFFF ^ sign);
}



/*

 Converts 32 bit floatP to 24.16 fixedP as used by Z values.This assumes
 24 bit Z buffer.

*/
void FloatToDepth ( float fi, long *zi, long *zf)
{

	long 				f= *((long *) &fi);
	long 				sign;
	unsigned char 	exponent;
	long				resh;
	long				resl;

	sign = f >> F_SIGN_BIT;
	exponent = (unsigned char) (f >> F_EXPONENT_BITS);
	if (exponent < (F_BIAS-16) )
	{
		*zi = 0;
		*zf = 0;
		return;
	}
	if (exponent < (F_BIAS+23) )
	{
		f = ((f | 0x00800000) << 8);
		if (exponent < (F_BIAS+0))
		{
			resh = 0;
			resl = ((unsigned long) f >> ((F_BIAS-1) - exponent));
		}
		else
		{
			unsigned char shift;
			
			shift = ((F_BIAS+31) - exponent);
			resh = ((unsigned long) f >> shift);
			resl = (f << (31 - shift));
			resl <<= 1;
		}
		if (sign < 0)
		{
			unsigned long old_resl;

			resl = ~resl;
			resh = ~resh;
			old_resl = resl;
			resl += 0x00010000;
			if ( resl < old_resl )
				++resh;
		}
	}
	else
	{
		resh = (0x007fffff ^ sign);
		resl = (0xffff0000 ^ sign);
	}
	resl &= 0xffff0000;
	*zi = resh;
	*zf = resl;
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

void PermediaSetupForFillRectSolid(int color, int rop, unsigned planemask)
{
	GLINT_WAIT(8);

	REPLICATE(color);

	gcolor = color;
	
	GLINT_WRITE_REG(pprod, FBReadMode);
	GLINT_WRITE_REG(color, FBBlockColor);
	GLINT_WRITE_REG(0, LogicalOpMode);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);
	GLINT_WRITE_REG(0xFFFFFFFF, FBSoftwareWriteMask);
	mode = FastFillEnable;

	GLINT_WRITE_REG(0, FBPixelOffset);
	GLINT_WRITE_REG(0, AreaStippleMode);
	GLINT_WRITE_REG(1<<16,dY);
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 * We need checks here that x <= 2047 and y <= 1023 (these seem to be the
 * drawing limits for the Permedia
 */
void PermediaSubsequentFillRectSolid(int x, int  y, int  w, int  h)
{
	GLINT_WAIT(7);

	/* not in PermediaSetupForFillRectSolid !
	   is the same setup for RectSolid and Trapezoid */
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);

	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(x<<16,     StartXDom);
	GLINT_WRITE_REG(y<<16,     StartY);
	GLINT_WRITE_REG(h,GLINTCount);

	GLINT_WRITE_REG(PrimitiveTrapezoid | mode, Render);
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

	gcolor = color;
	
	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		mode = FastFillEnable;
	} else {
		GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(color, PatternRamData0);
		mode = FastFillEnable | SpanOperation;
	}
	REPLICATE(planemask);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);

	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	/* GLINT_WRITE_REG(0,dXDom); */
	GLINT_WRITE_REG(0,RasterizerMode);
	/* GLINT_WRITE_REG(0,dXSub); */
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
	GLINT_WRITE_REG(h,GLINTCount);
	/* not in GlintSetupForFillRectSolid !
	   is the same setup for RectSolid and Trapezoid */
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);

	GLINT_WRITE_REG(PrimitiveTrapezoid | mode,Render);
}

void GLINTSetupForDashedLine(fg, bg, rop, planemask, size)
{
	REPLICATE(fg);
	REPLICATE(bg);

	GLINT_WRITE_REG(0, RasterizerMode);

	gfg = fg;
	gbg = bg;

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
	}
	REPLICATE(planemask);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);

	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	GLINT_WRITE_REG(0,dXDom);
	GLINT_WRITE_REG(0,dXSub);
	GLINT_WRITE_REG(1<<16,dY);

	DashPatternSize = size >> 4;

}

void GLINTSubsequentDashedTwoPointLine(int x1, int y1, int x2, int y2,
				       int bias, int offset)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	GLINT_WAIT(7);

	GLINT_WRITE_REG(0, UpdateLineStippleCounters);
	GLINT_WRITE_REG(1 | DashPattern << 10 | DashPatternSize << 1, LineStippleMode);

	GLINT_WRITE_REG(x1<<16, StartXDom);
	GLINT_WRITE_REG(y1<<16, StartY);

#if 0
	if ((dx == 0) && (dy == 0)) { /* This is a point */
		GLINT_WRITE_REG(POINT, Render);
		return;
	}
#endif

	if (abs(dx) >= abs(dy)) {	/* XMajor axis (i.e. Horizontal) */
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
			GLINT_WRITE_REG((dx<<16)/dy, dXDom); 
		}
		GLINT_WRITE_REG(abs(dy+1), GLINTCount);
	}

	GLINT_WRITE_REG(PrimitiveLine | LineStippleEnable | FastFillEnable | SpanOperation | SyncOnHostData, Render);
	GLINT_WRITE_REG(gfg, GLINTColor);

	GLINT_WRITE_REG(0, UpdateLineStippleCounters);
	GLINT_WRITE_REG(1 | ~DashPattern << 10 | DashPatternSize << 1, LineStippleMode);

	GLINT_WRITE_REG(PrimitiveLine | LineStippleEnable | FastFillEnable | SpanOperation | SyncOnHostData, Render);
	GLINT_WRITE_REG(gbg, GLINTColor);
	GLINT_WRITE_REG(0, ScissorMode);
}

void GLINTSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	GLINT_WAIT(6);
	GLINT_WRITE_REG(gcolor, GLINTColor);
	GLINT_WRITE_REG(x1<<16, StartXDom);
	GLINT_WRITE_REG(y1<<16, StartY);

#if 0
	if ((dx == 0) && (dy == 0)) { /* This is a point */
		GLINT_WRITE_REG(POINT, Render);
		return;
	}
#endif

	if (abs(dx) >= abs(dy)) {	/* XMajor axis (i.e. Horizontal) */
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
			GLINT_WRITE_REG((dx<<16)/dy, dXDom); 
		}
		GLINT_WRITE_REG(abs(dy+1), GLINTCount);
	}

	GLINT_WRITE_REG(PrimitiveLine, Render);
	GLINT_WRITE_REG(0, ScissorMode);
}

void GLINTSetClippingRectangle(int x1, int y1, int x2, int y2)
{
	GLINT_WAIT(4);
	GLINT_WRITE_REG(y1<<16|x1, ScissorMinXY);
	GLINT_WRITE_REG(y2<<16|x2, ScissorMaxXY);
	GLINT_WRITE_REG((glintInfoRec.displayWidth-1) | 0x7FFF0000, ScreenSize);

	GLINT_WRITE_REG(3, ScissorMode); /* Enable Scissor Mode */
}

void PermediaSetupForScreenToScreenCopy( int xdir, int  ydir, int  rop,
				    unsigned planemask, int transparency_color)
{
	blitydir = ydir;
	blitxdir = xdir;

	notline = 0;

	GLINT_WAIT(8);
	GLINT_WRITE_REG(0, RasterizerMode);
	if (glintInfoRec.bitsPerPixel < 32)
	{
		planemask |= planemask << 16;
		if (glintInfoRec.bitsPerPixel < 16)
		{
			planemask |= planemask << 8;
		}
	}

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
	} else {
	        GLINT_WRITE_REG(pprod | FBRM_SrcEnable | FBRM_DstEnable, FBReadMode);
	}

	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);
	GLINT_WRITE_REG(0xFFFFFFFF, FBSoftwareWriteMask);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode); 
	GLINT_WRITE_REG(0, AreaStippleMode);

	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
}
/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void PermediaSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2,
				     int w, int h)
{
	GLINT_WAIT(7);
	/* Permedia only allows left to right bitblt's */

	if (blitydir < 0) {
		y1 = y1 + h - 1;
		y2 = y2 + h - 1;
		GLINT_WRITE_REG(-1<<16, dY);
	} else {
		GLINT_WRITE_REG(1<<16, dY);
	}

	/* srcaddr - dstaddr */
	GLINT_WRITE_REG((GLINTWindowBase + y1 * glintInfoRec.displayWidth + x1) - 
			(GLINTWindowBase + y2 * glintInfoRec.displayWidth + x2), 
			FBSourceOffset);
	/* this is said to be needed after writing to FBSourceOffset 
	   GLINT_WRITE_REG(0,WaitForCompletion); */

	/* care must be taken when the source and destination
	   overlap, choose the source scanning direction to the
	   overlaping area */
	if (x2 > x1) {
	  GLINT_WRITE_REG((long)(x2+w << 16), StartXDom);
	  GLINT_WRITE_REG((long)(x2<<16), StartXSub);
	}
	else {  
	  GLINT_WRITE_REG((long)(x2 << 16), StartXDom);
	  GLINT_WRITE_REG((long)(x2+w <<16), StartXSub);
	}

	GLINT_WRITE_REG(y2<<16, StartY);
	GLINT_WRITE_REG(h, GLINTCount);

	GLINT_WRITE_REG(PrimitiveTrapezoid, Render);
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

	GLINT_WAIT(7);
	GLINT_WRITE_REG(0, PatternRamMode);
	GLINT_WRITE_REG(0, RasterizerMode);

	if ((rop == GXcopy) || (rop == GXcopyInverted)) {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | FBRM_SrcEnable | FBRM_DstEnable, FBReadMode);
	}
	REPLICATE(planemask);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);

	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);

	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
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
	GLINT_WRITE_REG(h, GLINTCount);

	GLINT_WRITE_REG(PrimitiveTrapezoid | FastFillEnable | SpanOperation, Render);
}

void GLINTSetupForScanlineScreenToScreenColorExpand(int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
	GLINT_WAIT(13);

	REPLICATE(fg);

	if (bg != -1) { ErrorF("BG IS %d %d\n",bg,rop); }
	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
		GLINT_WRITE_REG(fg, FBBlockColor);
		GLINT_WRITE_REG(0, PatternRamMode);
		GLINT_WRITE_REG(0, LogicalOpMode);
		mode = FastFillEnable;
	} else {
		GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
		GLINT_WRITE_REG(1, PatternRamMode);
		GLINT_WRITE_REG(fg, PatternRamData0);
		mode = FastFillEnable | SpanOperation;
		GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	}
	REPLICATE(planemask);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);

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
	  GLINT_WAIT(1);
		GLINT_WRITE_REG(*(ptr++), BitMaskPattern);
	}
}

void GLINTSetupForFill8x8Pattern(int patternx, int patterny, int rop,
				 unsigned planemask, int transparency_color)
{
	GLINT_WAIT(16);

	/* Is this the right way to upload a to patternram for an 8x8 ? */
	GLINT_WRITE_REG(0, dXDom);
	GLINT_WRITE_REG(0, dXSub);
	GLINT_WRITE_REG((patternx)<<16, StartXDom);
	GLINT_WRITE_REG(((patternx)+8)<<16, StartXSub);
	GLINT_WRITE_REG(patterny<<16, StartY);
	GLINT_WRITE_REG(8, GLINTCount);
	GLINT_WRITE_REG(1<<16, dY);
	GLINT_WRITE_REG(0, LogicalOpMode);
#if 0
	GLINT_WRITE_REG(1 | (0x0F << 1) | (1 << 6) | (1 << 9), PatternRamMode);
#else
	GLINT_WRITE_REG(1, PatternRamMode);
#endif
	GLINT_WRITE_REG(pprod | FBRM_SrcEnable, FBReadMode); /* Get pattern */
	GLINT_WRITE_REG(PrimitiveTrapezoid /*| FastFillEnable | SpanOperation*/, Render);

	if (rop == GXcopy) {
		GLINT_WRITE_REG(pprod, FBReadMode);
	} else {
		GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
	}
	REPLICATE(planemask);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);
	GLINT_WRITE_REG(0, RasterizerMode);
#if 1
	GLINT_WRITE_REG(1 | (0x0F << 1) | (1 << 6) | (1 << 9), PatternRamMode);
#else
	GLINT_WRITE_REG(1, PatternRamMode);
#endif
}

void GLINTSubsequentFill8x8Pattern(int patternx, int patterny, int x, int y,
				   int w, int h)
{
	GLINT_WAIT(5);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(h, GLINTCount);

	GLINT_WRITE_REG(PrimitiveTrapezoid | SpanOperation | FastFillEnable, Render);
}

void GLINTSetupFor8x8PatternColorExpand(int patternx, int patterny, 
					int bg, int fg, int rop,
					unsigned planemask)
{
	GLINT_WAIT(15);
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
		GLINT_WRITE_REG(pprod | FBRM_DstEnable, FBReadMode);
	}

	REPLICATE(planemask);
	GLINT_WRITE_REG(planemask, FBHardwareWriteMask);
	GLINT_WRITE_REG(rop<<1|1, LogicalOpMode);
	REPLICATE(fg);
	REPLICATE(bg);

	gfg = fg;
	gbg = bg;

	GLINT_WRITE_REG(1<<16, dY);
	GLINT_WRITE_REG(0, RasterizerMode);
}

void GLINTSubsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  GLINT_WAIT(7);
	GLINT_WRITE_REG(x<<16, StartXDom);
	GLINT_WRITE_REG((x+w)<<16, StartXSub);
	GLINT_WRITE_REG(y<<16, StartY);
	GLINT_WRITE_REG(h, GLINTCount);
	GLINT_WRITE_REG(gfg, FBBlockColor);
	GLINT_WRITE_REG(1 | 2 << 1 | 2 << 4 | patternx << 7 | 
		patterny << 12 | 0 << 17, AreaStippleMode);

	GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid | FastFillEnable, Render);

	if (gbg != -1) {
	        GLINT_WAIT(3);
		GLINT_WRITE_REG(gbg, FBBlockColor);
		GLINT_WRITE_REG(1 | 2 << 1 | 2 << 4 | patternx << 7 | 
			patterny << 12 | 1 << 17, AreaStippleMode);

		GLINT_WRITE_REG(AreaStippleEnable | PrimitiveTrapezoid | FastFillEnable, Render);
	}
}

void
GLINTSubsequentFillTrapezoidSolid(y, h, left, dxl, dyl, el, right, dxr, dyr, er)
int y, h, left, dxl, dyl, el, right, dxr, dyr, er;
{ 
  /* I have tested with xclock, i must add 2 to pass lines and Trapezoids */
  right += 2;
  GLINT_WAIT(7);
  /* GLINTSetClippingRectangle(left,y,right,y+h); */
  GLINT_WRITE_REG(left<<16, StartXDom);
  GLINT_WRITE_REG(right<<16, StartXSub);
  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG(h, GLINTCount);
  GLINT_WRITE_REG((dxl<<16)/dyl,dXDom);
  GLINT_WRITE_REG((dxr<<16)/dyr,dXSub);
  
  GLINT_WRITE_REG(PrimitiveTrapezoid | mode, Render);
  /* GLINT_WRITE_REG(0, ScissorMode); */
}

void
PermediaSubsequentFillTrapezoidSolid(int y, int h, int left, int dxl, int dyl, int el, 
				                   int right, int dxr, int dyr, int er)
{
  right += 2;

  GLINT_WAIT(7);

  GLINT_WRITE_REG(left<<16, StartXDom);
  GLINT_WRITE_REG(right<<16, StartXSub);
  GLINT_WRITE_REG((dxl<<16)/dyl, dXDom); 
  GLINT_WRITE_REG((dxr<<16)/dyr, dXSub);

  GLINT_WRITE_REG(y<<16, StartY);
  GLINT_WRITE_REG(h, GLINTCount);

  GLINT_WRITE_REG(PrimitiveTrapezoid | SubPixelCorrectionEnable | mode, Render);
}


/* End of glint_accel.c  *****************************************************/

