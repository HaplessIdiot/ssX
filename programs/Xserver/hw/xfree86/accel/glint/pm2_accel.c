/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/pm2_accel.c,v 1.2 1997/12/20 14:20:52 hohndel Exp $ */
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

#if DEBUG
#include <stdio.h>
#endif

static char	bppand[4] = { 0x03, 0x01, 0x00, 0x00 };
extern int	pprod;
extern int      ScanlineWordCount;
extern int	Bppshift;
extern Bool 	UsePCIRetry;
extern CARD32   ScratchBuffer[512];
static int      blitxdir, blitydir;
static int      mode;
static int	span;
static int      blitmode;
static int      gcolor;
static int      grop;
static int      gbg, gfg;
extern          GLINTWindowBase;

extern void     GLINTSync ();
extern void     GLINTSubsequentFillTrapezoidSolid ();
extern void     GLINTSetupForDashedLine ();
extern void     GLINTSubsequentDashedTwoPointLine ();
extern void     GLINTSetClippingRectangle ();
extern void	GLINTBresenhamLine();
extern void	xf86ImageGlyphBltTE();
extern void	xf86PolyGlyphBltTE();

void            Permedia2SetupForFillRectSolid();
void		Permedia2SubsequentFillRectSolid();
void 		Permedia2SubsequentScreenToScreenCopy ();
void            Permedia2SetupForScreenToScreenCopy ();
void            Permedia2SubsequentTwoPointLine ();
void            Permedia2SetupFor8x8PatternColorExpand ();
void		Permedia2Subsequent8x8PatternColorExpand();
void            Permedia2SetupForScanlineScreenToScreenColorExpand ();
void		Permedia2SubsequentScanlineScreenToScreenColorExpand ();
void            Permedia2SetClippingRectangle (int x1, int y1, int x2, int y2);
void		Permedia2FillRectStippled();
void		Permedia2WriteBitmap();
void		Permedia2ImageTextTECPUToScreenColorExpand();
void		Permedia2PolyTextTECPUToScreenColorExpand();
void 		Permedia2DoImageWrite();

void
Permedia2AccelInit ()
{
  xf86AccelInfoRec.Flags = PIXMAP_CACHE |
    COP_FRAMEBUFFER_CONCURRENCY |
    BACKGROUND_OPERATIONS |
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

  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY; 

  xf86AccelInfoRec.SetupForScreenToScreenCopy = 	
	Permedia2SetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy = 		
	Permedia2SubsequentScreenToScreenCopy;

  xf86AccelInfoRec.SetClippingRectangle = Permedia2SetClippingRectangle;
  xf86AccelInfoRec.SubsequentTwoPointLine = Permedia2SubsequentTwoPointLine;

  xf86AccelInfoRec.ColorExpandFlags =
    VIDEO_SOURCE_GRANULARITY_PIXEL |
    CPU_TRANSFER_PAD_DWORD |
    ONLY_TRANSPARENCY_SUPPORTED |
    BIT_ORDER_IN_BYTE_LSBFIRST;

  xf86AccelInfoRec.ScratchBufferAddr = 1;
#if 0
  /* this is for the scratch buffer method */
  xf86AccelInfoRec.ScratchBufferSize = 512 * sizeof(CARD32);
  xf86AccelInfoRec.ScratchBufferBase = (void *) ScratchBuffer;
#else
  /* this is for the direct method */
  xf86AccelInfoRec.ScratchBufferBase = 
	(void *)((char*)GLINTMMIOBase + OutputFIFO + 4);
  xf86AccelInfoRec.ScratchBufferSize = 255 * sizeof(CARD32);
#endif
  xf86AccelInfoRec.PingPongBuffers = 1;


  xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
    	Permedia2SetupForScanlineScreenToScreenColorExpand;
  xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
    	Permedia2SubsequentScanlineScreenToScreenColorExpand;
  
  xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
	Permedia2SetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
	Permedia2Subsequent8x8PatternColorExpand;

  /* Replace XAA functions */
  xf86AccelInfoRec.FillRectOpaqueStippled = Permedia2FillRectStippled;
  xf86AccelInfoRec.FillRectStippled = Permedia2FillRectStippled;
  xf86GCInfoRec.SecondaryPolyFillRectOpaqueStippledFlags = 0;
  xf86GCInfoRec.SecondaryPolyFillRectStippledFlags = 0;
  xf86AccelInfoRec.WriteBitmap = Permedia2WriteBitmap;

  xf86AccelInfoRec.ImageTextTE = Permedia2ImageTextTECPUToScreenColorExpand;
  xf86GCInfoRec.ImageGlyphBltTE = xf86ImageGlyphBltTE;
 
  xf86AccelInfoRec.PolyTextTE = Permedia2PolyTextTECPUToScreenColorExpand;
  xf86GCInfoRec.PolyGlyphBltTE = xf86PolyGlyphBltTE;

#if 0 /* GXcopy only works for now */
  xf86AccelInfoRec.DoImageWrite = Permedia2DoImageWrite;
#endif

  xf86AccelInfoRec.PixmapCacheMemoryStart = glintInfoRec.displayWidth *
        glintInfoRec.virtualY * (glintInfoRec.bitsPerPixel / 8);
  xf86AccelInfoRec.PixmapCacheMemoryEnd = glintInfoRec.videoRam * 1024;


}

void
Permedia2SetClippingRectangle (int x1, int y1, int x2, int y2)
{
  GLINT_WAIT(3);
  GLINT_WRITE_REG ((y1<<16)|x1, ScissorMinXY);
  GLINT_WRITE_REG ((y2<<16)|x2, ScissorMaxXY);
  GLINT_WRITE_REG (1, ScissorMode);
}


static CARD32 BlitDir;

void
Permedia2SetupForScreenToScreenCopy (int xdir, int ydir, int rop,
				 unsigned planemask, int transparency_color)
{
  BlitDir = 0;
  if (xdir == 1) BlitDir |= XPositive;
  if (ydir == 1) BlitDir |= YPositive;
  
  grop = rop;

  DO_PLANEMASK(planemask);

  GLINT_WAIT(3);
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

void Permedia2SetupForFillRectSolid(int color, int rop, unsigned planemask)
{
  gcolor = color;
  REPLICATE(color);

  DO_PLANEMASK(planemask);
  grop = rop;
  if (rop == GXcopy) {
	mode = pprod;
  	GLINT_WAIT(6);
	GLINT_WRITE_REG(pprod, FBReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(color, FBBlockColor);
  } else {
	GLINT_WAIT(5);
	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
      	GLINT_WRITE_REG(color, ConstantColor);
	/* We can use Packed mode for filling solid non-GXcopy rasters */
	mode = pprod|FBRM_DstEnable;
  }
  GLINT_WRITE_REG(0, RasterizerMode);
  GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
  GLINT_WRITE_REG(1<<16,dY);
}

void Permedia2SubsequentFillRectSolid(int x, int y, int w, int h)
{
  int lowerbits = bppand[(glintInfoRec.bitsPerPixel>>3)-1];
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

#if 0

/* this is the old version which takes the data from the scratch
buffer and dumps it directly to the fifo */

void
Permedia2SetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  DO_PLANEMASK(planemask);
  gfg = fg;
  REPLICATE (gfg);

  GLINT_WAIT(8);
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
}


void
Permedia2SubsequentScanlineScreenToScreenColorExpand(int srcaddr)
{
    GLINT_WAIT(ScanlineWordCount + 1);
    /* 0x0D is the TAG value for BitMaskPattern */
    GLINT_WRITE_REG(((ScanlineWordCount - 1) << 16) | 0x0D, OutputFIFO);
    MoveDWORDS((CARD32*)((char*)GLINTMMIOBase + OutputFIFO + 4),
	(CARD32 *)ScratchBuffer, ScanlineWordCount);
}

#else

/* this is the new version which has XAA render directly to the fifo. 
	We can do this only because we know that with 256 entries we'll
	never overrun the fifo.  You'd probably have to stick with
	The previous method for smaller fifos. */

static int LineCountDown;

void
Permedia2SetupForScanlineScreenToScreenColorExpand (int x, int y, int w, int h,
				int bg, int fg, int rop, unsigned planemask)
{
  DO_PLANEMASK(planemask);
  gfg = fg;
  REPLICATE (gfg);

  GLINT_WAIT(8);
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

#endif

void 
Permedia2SetupFor8x8PatternColorExpand(int patternx, int patterny, 
					   int bg, int fg, int rop,
					   unsigned planemask)
{
  DO_PLANEMASK(planemask);
  if (bg == -1) mode = -1;
	else    mode = 0;

  gfg = fg;
  gbg = bg;
  REPLICATE(gfg);
  REPLICATE(gbg);
  
  GLINT_WAIT(12);
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
Permedia2Subsequent8x8PatternColorExpand(int patternx, int patterny, int x, int y,
				   int w, int h)
{
  int span = 0;
  
  GLINT_WAIT(7);
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
	color = pGC->fgPixel; REPLICATE(color);
	/* >>>>>>>>>>>>>>>>>  Set fg  <<<<<<<<<<<<<<<<<< */
	if (pGC->alu == GXcopy) {
		GLINT_WAIT(3);
 		GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
		GLINT_WRITE_REG(color, FBBlockColor);
		GLINT_WRITE_REG(0, RasterizerMode);
	} else {
		GLINT_WAIT(3);
 		GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
		GLINT_WRITE_REG(color, ConstantColor);
		GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	}
    } else if(pGC->alu == GXcopy)  /* OpaqueEasyStipples */
	StippleType = 2;
    else 				/* OpaqueHardStipples */
	StippleType = 4;

    /* >>>>>>>>>>> Set ROP, Planemask <<<<<<<<<<< */
    DO_PLANEMASK(pGC->planemask);
    GLINT_WAIT(3);
    GLINT_WRITE_REG((pGC->alu<<1)|UNIT_ENABLE, LogicalOpMode);
    if (pGC->alu == GXcopy) {
	span = FastFillEnable;
 	GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	span = 0;
 	GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
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
		if (pGC->alu == GXcopy) {
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    GLINT_WRITE_REG(color, ConstantColor);
		}
		GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive | span,Render);
		/* >>>>>>>>>>  Set fg <<<<<<<<<<<<<< */
		color = pGC->fgPixel; REPLICATE(color);
		GLINT_WAIT(2);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
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
		GLINT_WAIT(2);
		if (pGC->alu == GXcopy) {
		    span = FastFillEnable;
		    GLINT_WRITE_REG(0, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
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
		    span = FastFillEnable;
		    GLINT_WRITE_REG(2, RasterizerMode);
		    GLINT_WRITE_REG(color, FBBlockColor);
		} else {
		    span = 0;
		    GLINT_WRITE_REG(2|BitMaskPackingEachScanline, RasterizerMode);
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
    register int count; 
    Bool SecondPass = FALSE;

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

    /* >>>>> Set rop, planemask, left edge clip skipleft pixels right
	of x (skipleft is sometimes 0 and clipping isn't needed). <<<<<< */
    DO_PLANEMASK(planemask);
    GLINT_WAIT(5);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(rop<<1|UNIT_ENABLE, LogicalOpMode);
    GLINT_WRITE_REG((y<<16)|x, RectangleOrigin);
    GLINT_WRITE_REG((h<<16)|w, RectangleSize);
    if (rop == GXcopy) {
    	GLINT_WRITE_REG(pprod, FBReadMode);
    } else {
	GLINT_WRITE_REG(pprod | blitmode, FBReadMode);
    }
    if (skipleft > 0) 
	Permedia2SetClippingRectangle(x+skipleft, y, x+w, y+h);

    if(bg == -1) {
	/* >>>>> set fg <<<<<<<< */
	REPLICATE(fg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
    	    GLINT_WRITE_REG(0, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
    	    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else if(rop == GXcopy) {
	/* >>>>> set bg <<<<<<< */
 	/* >>>>> draw rect (x,y,w,h) */
	REPLICATE(bg);
	GLINT_WAIT(4);
    	GLINT_WRITE_REG(0, RasterizerMode);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, FBBlockColor);
	} else {
	    mode = 0;
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(bg, ConstantColor);
	}
	GLINT_WRITE_REG(PrimitiveRectangle | XPositive | YPositive |mode,Render);
	/* >>>>>> set fg <<<<<< */
	REPLICATE(fg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
    	    GLINT_WRITE_REG(0, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
    	    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    } else {
	SecondPass = TRUE;
	/* >>>>> set fg <<<<<<< */
	REPLICATE(fg);
	GLINT_WAIT(3);
	if (rop == GXcopy) {
	    mode = FastFillEnable;
    	    GLINT_WRITE_REG(0, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, FBBlockColor);
	} else {
	    mode = 0;
    	    GLINT_WRITE_REG(BitMaskPackingEachScanline, RasterizerMode);
	    GLINT_WRITE_REG(UNIT_ENABLE, ColorDDAMode);
	    GLINT_WRITE_REG(fg, ConstantColor);
	}
    }

   /* >>>>>>>>> initiate transfer (x,y,w,h).  Skipleft pixels on the
	left edge will be clipped <<<<<< */

SECOND_PASS:
    GLINT_WAIT(1);
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
	GLINT_WAIT(3);
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

    GLINT_WAIT(2);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(0, ScissorMode);
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
    DO_PLANEMASK(pGC->planemask);
    gfg =  pGC->fgPixel;
    gbg = pGC->bgPixel;
    REPLICATE (gfg);
    REPLICATE (gbg);
 
    GLINT_WAIT(7);
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
    DO_PLANEMASK(pGC->planemask);
    gfg = pGC->fgPixel;
    REPLICATE (gfg);
 
    GLINT_WAIT(8);
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
Permedia2DoImageWrite(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords, count, offset;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    unsigned char* srcPntr;			/* index into the image */
    CARD32* srcp;
    int Bpp = glintInfoRec.bitsPerPixel >> 3; 
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    int Shift;
    Bool FastTexLoad, AlreadyInitialized = FALSE;

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    GLINT_WAIT(3);
    DO_PLANEMASK(planemask);
    GLINT_WRITE_REG(0, RasterizerMode);
    if (alu == GXcopy) {      
	GLINT_WRITE_REG(pprod | FBRM_Packed, FBReadMode);
    } else {
	GLINT_WRITE_REG(pprod | FBRM_Packed | FBRM_DstEnable, FBReadMode);
    }

    switch(Bpp) {
	case 1:		Shift = 2; break;
	case 2:		Shift = 1; break;
	default:	Shift = 0; break;
    }

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

#if 0
	if((skipleft = (int)srcPntr & 0x03)) {
	    if(!(xf86AccelInfoRec.ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }

	    if(Bpp == 3)
		skipleft = 4 - skipleft;
	    else
	    	skipleft /= Bpp;

	    if((x < skipleft) && !(xf86AccelInfoRec.ImageWriteFlags &
				 LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }

	    x -= skipleft;	     
	    w += skipleft;
	
	    if(Bpp == 3)
	    	srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    else   
	    	srcPntr = (unsigned char*)((int)srcPntr & ~0x03);     
	}
#else
   	skipleft = 0;
#endif
	
BAD_ALIGNMENT:
	
	FastTexLoad = FALSE;
	switch(Bpp) {
	   case 1:	dwords = (w + 3) >> 2;
			if((!(x&3)) && (!(w&3))) FastTexLoad = TRUE;	
			break;
	   case 2:	dwords = (w + 1) >> 1;
			if((!(x&1)) && (!(w&1))) FastTexLoad = TRUE;
			break;
	   default:	dwords = w;
			FastTexLoad = TRUE;
			break;
	}
	if((alu != GXcopy) || skipleft || (planemask != ~0))
		FastTexLoad = FALSE;

       if(FastTexLoad) {
	  int address;

	  GLINT_WAIT(1);
	  GLINT_WRITE_REG(UNIT_DISABLE, FBWriteMode);
	  GLINTSync();	/* we are not using the rasterizer */
	  while(h--) {
	      count = dwords;
	      address = ((y * glintInfoRec.displayWidth) + x) >> Shift;
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
	   GLINT_WAIT(6);
	   GLINT_WRITE_REG((y<<16)|x>>Bppshift, RectangleOrigin);
	   GLINT_WRITE_REG((h<<16)|(w+lowerbits)>>Bppshift, RectangleSize);
  	   GLINT_WRITE_REG(align<<29|x<<16|(x+w), PackedDataLimits);
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

    SET_SYNC_FLAG;
}
