/* $XConsortium: cir_blitter.c,v 1.1 94/03/28 21:48:10 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_blitter.c,v 3.3 1994/08/20 07:36:15 dawes Exp $ */
/*
 *
 * Copyright 1994 by H. Hanemaayer, Utrecht, The Netherlands
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of H. Hanemaayer not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  H. Hanemaayer makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * H. HANEMAAYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL H. HANEMAAYER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  H. Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 */


/*
 * This file contains all low-level functions that use the BitBLT engine
 * on the 5426/5428/5429/5434 (except for those that involve system memory,
 * they are in cir_im.c, and the text functions in cir_teblt8.c).
 */
 
/*
 * This file is compiled twice, once with CIRRUS_MMIO defined.
 */


#include "xf86.h"	/* For vgaBase. */
#include "vga.h"	/* For vgaBase. */
#include "compiler.h"

#include "cir_driver.h"
#ifdef CIRRUS_MMIO
#include "cir_blitmm.h"
#else
#include "cir_blitter.h"
#endif
#include "cir_inline.h"


#ifndef CIRRUS_MMIO	/* Define these only once. */

/* Cirrus raster operations. */

int cirrus_rop[16] = {
	CROP_0,			/* GXclear */
	CROP_AND,		/* GXand */
	CROP_SRC_AND_NOT_DST,	/* GXandReverse */
	CROP_SRC,		/* GXcopy */
	CROP_NOT_SRC_AND_DST,	/* GXandInverted */
	CROP_DST,		/* GXnoop */
	CROP_XOR,		/* GXxor */
	CROP_OR,		/* GXor */
	CROP_NOR,		/* GXnor */
	CROP_XNOR,		/* GXequiv */
	CROP_NOT_DST,		/* GXinvert */
	CROP_SRC_OR_NOT_DST,	/* GXorReverse */
	CROP_NOT_SRC,		/* GXcopyInverted */
	CROP_NOT_SRC_OR_DST,	/* GXorInverted */
	CROP_NAND,		/* GXnand */
	CROP_1			/* GXset */
};

/*
 * The BLT functions defined in this file support exiting without waiting
 * for the BLT to finish. The calling function must set
 * cirrusDoBackgroundBLT, call any BLT functions defined in this file,
 * and when the operations are done check whether a BLT is still in
 * progress (cirrusBLTisBusy) and if so call WAITUNTILFINISHED().
 * If the function does any other video memory access or BLT functions
 * not defined in this file, it must make sure no BLT is in progress
 * before drawing.
 *
 */

int cirrusBLTisBusy = FALSE;
int cirrusDoBackgroundBLT = FALSE;

int cirrusBLTPatternAddress;	/* Address for BitBLT filling pattern. */


/*
 * CirrusInvalidateShadowVariables
 * Invalidate values of shadow variables for I/O registers.
 * This helps very slightly on a VLB system. May be unoptimal on the fast
 * host bus interface the 543x (but it can use MMIO).
 * Should be a win for ISA bus.
 */

int cirrusWriteModeShadow,
    cirrusPixelMaskShadow,
    cirrusModeExtensionsShadow,
    cirrusBltSrcAddrShadow,
    cirrusBltDestPitchShadow,
    cirrusBltSrcPitchShadow,
    cirrusBltHeightShadow,
    cirrusBltModeShadow,
    cirrusBltRopShadow;
unsigned int
    cirrusForegroundColorShadow,
    cirrusBackgroundColorShadow;

void CirrusInvalidateShadowVariables() {
    cirrusWriteModeShadow = -1;
    cirrusPixelMaskShadow = -1;
    cirrusModeExtensionsShadow = -1;
    cirrusBltSrcAddrShadow = -1;
    cirrusBltDestPitchShadow = -1;
    cirrusBltSrcPitchShadow = -1;
    cirrusBltHeightShadow = -1;
    cirrusBltModeShadow = -1;
    cirrusBltRopShadow = -1;
    /* For the lower byte of the 32-bit color registers, there is no safe
     * invalid value. We just set them to a specific value (making sure
     * we don't write to non-existant color registers).
     * 
    cirrusBackgroundColorShadow = 0xffffffff;	/* Defeat the macros. */
    cirrusForegroundColorShadow = 0xffffffff;
    if (cirrusChip >= CLGD5422 && cirrusChip <= CLGD5430) {
	SETBACKGROUNDCOLOR16(0x0000);
	SETFOREGROUNDCOLOR16(0x0000);
    }
    else
    if (cirrusChip == CLGD5434) {
    	SETBACKGROUNDCOLOR32(0x00000000);
    	SETFOREGROUNDCOLOR32(0x00000000);
    }
    else {
    	SETBACKGROUNDCOLOR(0x00);
    	SETFOREGROUNDCOLOR(0x00);
    }
 }

#endif	/* !defined(CIRRUS_MMIO) */


/*
 * 8x8 pattern fill with color expanded pattern.
 *
 * patternword1 contains the first 32 pixels (first pixel in LSByte, MSBit),
 * patternword2 the second. 
 */

#ifdef CIRRUS_MMIO
#define _CirrusBLTColorExpand8x8PatternFill CirrusMMIOBLTColorExpand8x8PatternFill
#else
#define _CirrusBLTColorExpand8x8PatternFill CirrusBLTColorExpand8x8PatternFill
#endif
 
void _CirrusBLTColorExpand8x8PatternFill(dstAddr, fgcolor, bgcolor,
fillWidth, fillHeight, dstPitch, rop, patternword1, patternword2)
     unsigned int dstAddr;
     int fgcolor, bgcolor;
     int fillHeight, fillWidth, dstPitch;
     int rop;
     unsigned long patternword1, patternword2;
{
  unsigned int srcAddr;
  int i;
  pointer pDst;

  if (!HAVE543X() && fillHeight > 1024) {
      /* Split into two for 5426, 5428 & 5429. */
      _CirrusBLTColorExpand8x8PatternFill(dstAddr, fgcolor, bgcolor,
          fillWidth, 1024, dstPitch, rop, patternword1, patternword2);
      _CirrusBLTColorExpand8x8PatternFill(dstAddr + dstPitch * 1024,
          fgcolor, bgcolor, fillWidth, fillHeight - 1024, dstPitch, rop,
          patternword1, patternword2);
      return;
  }

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  /* NOTE:
   * I tried System memory source pattern fill, it's not explicitly
   * disallowed (nor documented) in the databook, but it doesn't work. 
   */

  /* Pattern fill with color expanded video memory source. */

  /* Write 8 byte monochrome pattern. */
  /* Because of 16K bank granularity and 32K window, we don't have to */
  /* check for bank boundaries. */
  srcAddr = cirrusBLTPatternAddress;
  CIRRUSSETWRITE(srcAddr);
  *(unsigned long *)(CIRRUSWRITEBASE() + srcAddr) = patternword1;
  *(unsigned long *)(CIRRUSWRITEBASE() + srcAddr + 4) = patternword2;

  SETDESTADDR(dstAddr);
  SETSRCADDR(cirrusBLTPatternAddress);
  SETDESTPITCH(dstPitch);
  SETWIDTH(fillWidth);
  SETHEIGHT(fillHeight);

  /* 8x8 Pattern Copy, screen to screen blt, forwards, color expand. */
  SETBLTMODE(PATTERNCOPY | COLOREXPAND);
  SETROP(rop);
  SETFOREGROUNDCOLOR(fgcolor);
  SETBACKGROUNDCOLOR(bgcolor);

  /* Do it. */
  STARTBLT();

  if (cirrusDoBackgroundBLT)
    cirrusBLTisBusy = TRUE;
  else {
    WAITUNTILFINISHED();
    SETFOREGROUNDCOLOR(0);
  }

  /*
   * Serious problem: leaving the foreground color register at non-zero is
   * not a good idea for normal framebuffer access (Set/Reset enable).
   * Because of this, a calling function that uses background BLTs must
   * set it to zero when it is finished.
   */

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}


/*
 * 8x8 pattern fill.
 *
 * Regular 8x8 byte-per-pixel pattern.
 * 
 */

#ifdef CIRRUS_MMIO
#define _CirrusBLT8x8PatternFill CirrusMMIOBLT8x8PatternFill
#else
#define _CirrusBLT8x8PatternFill CirrusBLT8x8PatternFill
#endif

void _CirrusBLT8x8PatternFill(dstAddr, w, h, pattern, destpitch, rop)
     unsigned int dstAddr;
     int w, h, destpitch;
     int rop;
     void *pattern;
{
  unsigned int srcAddr;
  int i;
  pointer pDst;

  if (!HAVE543X() && h > 1024) {
      /* Split into two for 5426, 5428, & 5429. */
      _CirrusBLT8x8PatternFill(dstAddr, w, 1024, pattern, destpitch, rop);
      _CirrusBLT8x8PatternFill(dstAddr + destpitch * 1024, w, h - 1024,
          pattern, destpitch, rop);
      return;
  }

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  /* Pattern fill with video memory source. */

  /* Write 64 byte pattern. */
  /* Because of 16K bank granularity and 32K window, we don't have to */
  /* check for bank boundaries. */
  srcAddr = cirrusBLTPatternAddress;
  CIRRUSSETWRITE(srcAddr);
  memcpy(CIRRUSWRITEBASE() + srcAddr, pattern, 64);

  SETDESTADDR(dstAddr);
  SETSRCADDR(cirrusBLTPatternAddress);
  SETDESTPITCH(destpitch);
  SETWIDTH(w);
  SETHEIGHT(h);

  /* 8x8 Pattern Copy, screen to screen blt, forwards. */
  SETBLTMODE(PATTERNCOPY);
  SETROP(rop);

  /* Do it. */
  STARTBLT();

  if (cirrusDoBackgroundBLT)
    cirrusBLTisBusy = TRUE;
  else
    WAITUNTILFINISHED();

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}


/*
 * 16x16 pattern fill.
 *
 * Uses two 16x8 byte-per-pixel pattern fills (meant for 8x8 16-bit pixel
 * fill), vertically interleaved.
 * 
 */

#ifdef CIRRUS_MMIO
#define _CirrusBLT16x16PatternFill CirrusMMIOBLT16x16PatternFill
#else
#define _CirrusBLT16x16PatternFill CirrusBLT16x16PatternFill
#endif

void _CirrusBLT16x16PatternFill(dstAddr, w, h, pattern, destpitch, rop)
     unsigned int dstAddr;
     int w, h, destpitch;
     int rop;
     unsigned char *pattern;
{
  unsigned int srcAddr;
  int i;
  pointer pDst;
  int blith, blitpitch;

  if (!HAVE543X() && h > 1024) {
      /* Split into two for 5426, 5428 & 5429. */
      _CirrusBLT16x16PatternFill(dstAddr, w, 1024, pattern, destpitch, rop);
      _CirrusBLT16x16PatternFill(dstAddr + destpitch * 1024, w, h - 1024,
          pattern, destpitch, rop);
      return;
  }

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  /* Pattern fill with video memory source. */

  /* Write 128 byte pattern (even lines). */
  /* Because of 16K bank granularity and 32K window, we don't have to */
  /* check for bank boundaries. */
  srcAddr = cirrusBLTPatternAddress;
  CIRRUSSETWRITE(srcAddr);
  for (i = 0; i < 8; i++)
      memcpy(CIRRUSWRITEBASE() + srcAddr + i * 16,
          pattern + i * 32, 16);

  blitpitch = destpitch * 2;
  blith = (h / 2) + (h & 1);

  SETDESTADDR(dstAddr);
  SETSRCADDR(cirrusBLTPatternAddress);
  SETDESTPITCH(blitpitch);
  SETWIDTH(w);
  SETHEIGHT(blith);

  /* 8x8 Pattern Copy, 16-bit pixels, screen to screen blt, forwards. */
  SETBLTMODE(PATTERNCOPY | PIXELWIDTH16);
  SETROP(rop);

  /* Do it. */
  STARTBLT();

  WAITUNTILFINISHED();

  /* Now do uneven lines. */
  /* Write pattern. */
  for (i = 0; i < 8; i++)
	  memcpy(CIRRUSWRITEBASE() + srcAddr + i * 16,
	  	pattern + i * 32 + 16, 16);

  dstAddr += destpitch;
/*  blitpitch = destpitch * 2; */
  blith = (h / 2);

  SETDESTADDR(dstAddr);
  SETSRCADDR(cirrusBLTPatternAddress);
/* SETDESTPITCH(blitpitch); */
  SETWIDTH(w);
  SETHEIGHT(blith);

  /* 8x8 Pattern Copy, 16-bit pixels, screen to screen blt, forwards. */
/*  SETBLTMODE(PATTERNCOPY | PIXELWIDTH); */
/*  SETROP(rop); */

  STARTBLT();

  if (cirrusDoBackgroundBLT)
    cirrusBLTisBusy = TRUE;
  else
    WAITUNTILFINISHED();

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}

/*
 * 32x32 pattern fill for 5434.
 *
 * Uses four 32x8 byte-per-pixel pattern fills (meant for 8x8 32-bit pixel
 * fill), vertically interleaved.
 * This is untested.
 *
 * Something like this would also work on the 5426/28 (by filling vertical
 * bands 16 pixels wide, 4-way interleaved) if the virtual screen width is
 * less than 1024 (i.e. rarely).
 */

#ifdef CIRRUS_MMIO
#define _CirrusBLT32x32PatternFill CirrusMMIOBLT32x32PatternFill
#else
#define _CirrusBLT32x32PatternFill CirrusBLT32x32PatternFill
#endif

void _CirrusBLT32x32PatternFill(dstAddr, w, h, pattern, destpitch, rop)
     unsigned int dstAddr;
     int w, h, destpitch;
     int rop;
     void *pattern;
{
  unsigned int srcAddr;
  int i, k;
  int blith, blitpitch;

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  /* No need to split into two for 5434 (handles heights up to 2048). */

  /* Pattern fill with video memory source. */

  /* Set up write bank for writing pattern. */
  srcAddr = cirrusBLTPatternAddress;
  CIRRUSSETWRITE(srcAddr);

  /* Set up BLT parameters that remain constant. */
  blitpitch = destpitch * 4;	/* Four-way interleaving. */
  SETDESTPITCH(blitpitch);
  /* 8x8 Pattern Copy, 32-bit pixels, screen to screen blt, forwards. */
  SETBLTMODE(PATTERNCOPY | PIXELWIDTH32);
  SETROP(rop);

  for (k = 0; k < 4; k++) {
      /* Do the lines for which [index % 4 == k]. */
      /* Write 32x8 pattern. */
      for (i = 0; i < 8; i++)
          memcpy(CIRRUSWRITEBASE() + srcAddr + i * 32,
	  	(unsigned char *)pattern + i * 32 * 4 + 32 * k, 32);
      blith = h / 4;
      if (h & 3 > k)
  	  blith++;
      SETDESTADDR(dstAddr);
      SETSRCADDR(cirrusBLTPatternAddress);
      SETWIDTH(w);
      SETHEIGHT(blith);
      STARTBLT();
      if (k != 3 || !cirrusDoBackgroundBLT)
          WAITUNTILFINISHED();
      else
          cirrusBLTisBusy = TRUE;

      dstAddr += destpitch;
  }

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}


/*
 * BitBLT. Direction can be forwards or backwards. Can be modified to
 * support special rops.
 */

#ifdef CIRRUS_MMIO
#define _CirrusBLTBitBlt CirrusMMIOBLTBitBlt
#else
#define _CirrusBLTBitBlt CirrusBLTBitBlt
#endif

void _CirrusBLTBitBlt(dstAddr, srcAddr, dstPitch, srcPitch, w, h, dir)
     unsigned int dstAddr, srcAddr;
     int dstPitch, srcPitch;
     int w, h;
     int dir;			/* >0, increase adrresses, <0, decrease */
{
  if ((!HAVE543X()) && h > 1024) {
     /* Split into two. */
     if (dir > 0) {
         _CirrusBLTBitBlt(dstAddr, srcAddr, dstPitch, srcPitch, w, 1024, dir);
         _CirrusBLTBitBlt(dstAddr + dstPitch * 1024, srcAddr + srcPitch * 1024,
             dstPitch, srcPitch, w, h - 1024, dir);
     }
     else {
         _CirrusBLTBitBlt(dstAddr, srcAddr, dstPitch, srcPitch, w, 1024, dir);
         _CirrusBLTBitBlt(dstAddr - dstPitch * 1024, srcAddr - srcPitch * 1024,
             dstPitch, srcPitch, w, h - 1024, dir);
     }
     return;
  }

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  SETSRCADDR(srcAddr);
  SETSRCPITCH(srcPitch);
  SETDESTADDR(dstAddr);
  SETDESTPITCH(dstPitch);
  SETWIDTH(w);
  SETHEIGHT(h);

  if (dir > 0) {
      SETBLTMODE(FORWARDS);
  }
  else {
      SETBLTMODE(BACKWARDS);
  }

  SETROP(CROP_SRC);

  STARTBLT();
  if (cirrusDoBackgroundBLT)
    cirrusBLTisBusy = TRUE;
  else
    WAITUNTILFINISHED();

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}


#if 0	/* Experimental, not used. */

void CirrusBLTColorExpandImageWriteFill(dstAddr, fgcolor, fillWidth,
fillHeight, dstPitch)
     unsigned int dstAddr;
     int fgcolor;
     int fillHeight, fillWidth, dstPitch;
{
  int size, i;

  if (!HAVE543X() && fillHeight > 1024) {
      /* Split into two for 5426, 5428 & 5429. */
      CirrusBLTColorExpandImageWriteFill(dstAddr, fgcolor, fillWidth, 1024,
          dstPitch);
      CirrusBLTColorExpandImageWriteFill(dstAddr + dstPitch * 1024, fgcolor,
          fillWidth, fillHeight - 1024, dstPitch);
      return;
  }

  if (cirrusBLTisBusy)
    WAITUNTILFINISHED();

  /* System-to-video memory blit with color expansion. */

  SETDESTADDR(dstAddr);
  SETSRCADDR(0);
  SETDESTPITCH(dstPitch);
  SETWIDTH(fillWidth);
  SETHEIGHT(fillHeight);

  SETBLTMODE(SYSTEMSRC | COLOREXPAND);
  SETROP(CROP_SRC);
  SETFOREGROUNDCOLOR(fgcolor);

  /* Do it. */
  STARTBLT();

  /* Calculate number of dwords to transfer. */
  size = ((((fillWidth + 7) / 8) * fillHeight) + 3) / 4;

  CirrusWordTransfer(size, 0xffffffff, CIRRUSBASE());

  WAITUNTILFINISHED();

  SETFOREGROUNDCOLOR(0);

#ifdef CIRRUS_MMIO
  cirrusMMIOFlag = TRUE;
#endif
}

#endif

#ifdef CIRRUS_MMIO
#define _CirrusBLTWaitUntilFinished CirrusMMIOBLTWaitUntilFinished
#else
#define _CirrusBLTWaitUntilFinished CirrusBLTWaitUntilFinished
#endif

void _CirrusBLTWaitUntilFinished() {
  int count, timeout;
#ifndef CIRRUS_MMIO  
  if (cirrusUseMMIO) {
  	/* If we have MMIO, better use it. */
  	CirrusMMIOBLTWaitUntilFinished();
  	return;
  }
#endif
  count = 0;
  timeout = 0;
  cirrusBLTisBusy = FALSE;	/* That will be the case on exit. */
  for (;;) {
    int busy;
    BLTBUSY(busy);
    if (!busy)
    	return;
    count++;
    if (count == 10000000) {
    	ErrorF("Cirrus: BitBLT engine time-out.\n");
    	*(unsigned long *)CIRRUSBASE() = 0;
    	count = 9990000;
    	timeout++;
    	if (timeout == 8) {
    		/* Reset BitBLT engine. */
    		BLTRESET();
    		return;
    	}
    }
  }
}
