/* $XConsortium: cir_im.c,v 1.1 94/03/28 21:49:31 dpw Exp $ */
/*
 *
 * Copyright 1993 by Bill Reynolds, Santa Fe, New Mexico
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Bill Reynolds not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Bill Reynolds makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * BILL REYNOLDS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL BILL REYNOLDS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Bill Reynolds, bill@goshawk.lanl.gov
 *
 * Reworked by: Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 * Modifications: Harm Hanemaayer <hhanemaa@cs.ruu.nl>
 *
 * Id: cir_im.c,v 0.7 1993/09/16 01:07:25 scooper Exp
 */


/*
 * Image read/write BLT functions.
 * WARNING: The scanline/total size byte padding for BitBLT engine functions
 * that involve system memory is EXTREMELY tricky. There are many different
 * cases. The databook can be misleading in places.
 */


#include "misc.h"
#include "xf86.h"
#include "X.h"
#include "Xos.h"
#include "Xmd.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "cfb8bit.h"

#include "mergerop.h"
#include "vgaBank.h"
#include "compiler.h"
#include "vga.h"	/* For vgaBase. */

#include "cir_driver.h"
#include "cir_blitter.h"


/* #if __GNUC__ > 1 */
#if 0	/* Broken. */

/*
 * This another piece of critical code which is very difficult to do
 * efficiently in C. Transfers 16-bit words, reversing the per-byte
 * bit order. 
 */

static __inline__ transferwords( unsigned char *base, unsigned char *srcp,
int count ) {
	int dummy, temp;
	asm(
	"cmpl $4,%3\n\t"
	"jb 2f\n\t"

	"1:\n\t"
	"movzbl 2(%1),%2\n\t"
	"movb _byte_reversed(%2),%%al\n\t"
	"movzbl 3(%1),%2\n\t"
	"movb _byte_reversed(%2),%%ah\n\t"
	"shll $16,%%eax\n\t"
	"movzbl (%1),%2\n\t"
	"movb _byte_reversed(%2),%%al\n\t"
	"movzbl 1(%1),%2\n\t"
	"movb _byte_reversed(%2),%%ah\n\t"
	/* We now have per-byte bit order reversed word of data in eax. */
	"movl %%eax,(%0)\n\t"

	/* Now do the second word. */
	"movzbl 6(%1),%2\n\t"
	"movb _byte_reversed(%2),%%al\n\t"
	"movzbl 7(%1),%2\n\t"
	"movb _byte_reversed(%2),%%ah\n\t"
	"shll $16,%%eax\n\t"
	"movzbl 4(%1),%2\n\t"
	"movb _byte_reversed(%2),%%al\n\t"
	"movzbl 5(%1),%2\n\t"
	"movb _byte_reversed(%2),%%ah\n\t"
	"movl %%eax,(%0)\n\t"

	"addl $8,%1\n\t"
	"subl $4,%3\n\t"
	"cmpl $4,%3\n\t"
	"jae 1b\n\t"

	"2:\n\t"
	"andl %3,%3\n\t"
	"jz 4f\n\t"

	"3:\n\t"
	"movzbl (%1),%2\n\t"
	"movb _byte_reversed(%2),%%al\n\t"
	"movzbl 1(%1),%2\n\t"
	"movb _byte_reversed(%2),%%ah\n\t"
	"movw %%ax,(%0)\n\t"
	"addl $2,%1\n\t"
	"decl %3\n\t"
	"jnz 3b\n\t"

	"4:\n\t"
	:
	: "r" (base), "r" (srcp), "r" (temp), "r" (count), "ax" (dummy)
	: "ax", "1", "2", "3"
	);
}

#else

static __inline__ transferwords( unsigned char *base, unsigned char *srcp, int count ) {
	while (count > 4) {
		unsigned long bits;
		unsigned short data;
		bits = *(unsigned short *)srcp;
		data = byte_reversed[(unsigned char)bits] +
			(byte_reversed[(unsigned char)(bits >> 8)] << 8);
		*(unsigned short *)base = data;
		bits = *(unsigned short *)(srcp + 2);
		data = byte_reversed[(unsigned char)bits] +
			(byte_reversed[(unsigned char)(bits >> 8)] << 8);
		*(unsigned short *)base = data;
		bits = *(unsigned short *)(srcp + 4);
		data = byte_reversed[(unsigned char)bits] +
			(byte_reversed[(unsigned char)(bits >> 8)] << 8);
		*(unsigned short *)base = data;
		bits = *(unsigned short *)(srcp + 6);
		data = byte_reversed[(unsigned char)bits] +
			(byte_reversed[(unsigned char)(bits >> 8)] << 8);
		*(unsigned short *)base = data;
		srcp += 8;
		count -= 4;
	}
	while (count > 0) {
		unsigned long bits;
		unsigned short data;
		bits = *(unsigned short *)srcp;
		data = byte_reversed[(unsigned char)bits] +
			(byte_reversed[(unsigned char)(bits >> 8)] << 8);
		*(unsigned short *)base = data;
		srcp += 2;
		count--;
	}
}

#endif



/*
 * This should replace the assembler CirrusImageReadTransfer routine,
 * which may make wrong assumptions.
 *
 * We need to read a multiple of 4 bytes in total. We must not pad scanlines.
 * We may only read words.
 *
 * According to the 542x databook, this is the way to go. However, it
 * doesn't work; we must pad scanlines to multiples of 4.
 * The 543x book says something similar, but on that chip it appears the
 * scanline MUST be padded (at least the function that worked on the 5426
 * doesn't work on the 543x apparently).
 *
 */

static void CirrusImageReadTransfer543x( int w, int h, void *destp,
int destpitch, void *base ) {
	unsigned char *buffer, *bufferp;
	int i;
	int size;
	int nbytes;
	size = w * h;
	/* Number of bytes, rounded up to multiple of 4. */
	nbytes = (size + 3) & ~3;
	buffer = ALLOCATE_LOCAL(nbytes);

	/* Read raw data into buffer. */
	bufferp = buffer;
	while (nbytes > 0) {
		int chunk;
		chunk = min(65536, nbytes);
		memcpy(bufferp, vgaBase, chunk);
		bufferp += chunk;
		nbytes -= chunk;
	}

	/* Copy raw data to destination pixmap. */
	bufferp = buffer;
	for (i = 0; i < h; i++) {
		memcpy((unsigned char *)destp + i * destpitch, bufferp, w);
		bufferp += w;
	}
	DEALLOCATE_LOCAL(buffer);
}


void
CirrusImageWrite (pdstBase, psrcBase, widthSrc, widthDst, x, y,
		  x1, y1, w, h, xdir, ydir, alu, planemask)
     pointer pdstBase, psrcBase;	/* start of src bitmap */
     int widthSrc, widthDst;
     int x, y, x1, y1, w, h;	/* Src x,y; Dst x1,y1; Dst (w)idth,(h)eight */
     int xdir, ydir;
     int alu;
     unsigned long planemask;
{
  pointer psrc;
  unsigned int dstAddr;

  if ((planemask & 0xFF) == 0xFF)
    {
      int width, height;

      if (!HAVE543X() && h > 1024) {
          /* Split into two. */
          CirrusImageWrite(pdstBase, psrcBase, widthSrc, widthDst, x, y,
              x1, y1, w, 1024, xdir, ydir, alu, planemask);
          CirrusImageWrite(pdstBase, psrcBase, widthSrc, widthDst, x, y +
              1024, x1, y1 + 1024, w, h - 1024, xdir, ydir, alu, planemask);
          return;
      }

      psrc = (unsigned char *)psrcBase + (y * widthSrc) + x;
      dstAddr = (y1 * widthDst) + x1;

      SETDESTADDR(dstAddr);
      SETDESTPITCH(widthDst);
      SETWIDTH(w);
      SETHEIGHT(h);
      /* Set the direction and source (System Memory) */
      SETBLTMODE(SYSTEMSRC);
      SETROP(cirrus_rop[alu]);
      STARTBLT();

      /*
       * We must transfer 4 bytes per blit line.  This is cautious code and I
       * do not read from outside of the pixmap... The 386/486 allows
       * unaligned memory acceses, and has little endian word ordering.  This
       * is used to our advantage when dealing with the 3 byte remainder.
       * Don't try this on your Sparc :-)
       */

      CirrusImageWriteTransfer(w, h, psrc, widthSrc, vgaBase);

      WAITUNTILFINISHED();
    }
  else
    {
      vgaImageWrite(pdstBase, psrcBase, widthSrc, widthDst, x, y,
		    x1, y1, w, h, xdir, ydir, alu, planemask);
    }
  
}


void
CirrusImageRead (pdstBase, psrcBase, widthSrc, widthDst, x, y,
		 x1, y1, w, h, xdir, ydir, alu, planemask)
     pointer pdstBase, psrcBase;	/* start of src bitmap */
     int widthSrc, widthDst;
     int x, y, x1, y1, w, h;	/* Src x,y; Dst x1,y1; Dst (w)idth,(h)eight */
     int xdir, ydir;
     int alu;
     unsigned long planemask;
{
  pointer pdst;
  unsigned int srcAddr;

  if ((planemask & 0xFF) == 0xFF)
    {
      int width, height;

      if (!HAVE543X() && h > 1024) {
          /* Split into two. */
          CirrusImageRead(pdstBase, psrcBase, widthSrc, widthDst, x, y,
              x1, y1, w, 1024, xdir, ydir, alu, planemask);
          CirrusImageRead(pdstBase, psrcBase, widthSrc, widthDst, x, y +
              1024, x1, y1 + 1024, w, h - 1024, xdir, ydir, alu, planemask);
          return;
      }

      pdst = (unsigned char *)pdstBase + (y1 * widthDst) + x1;
      srcAddr = (y * widthSrc) + x;

      SETSRCADDR(srcAddr);
      SETSRCPITCH(widthSrc);
      SETWIDTH(w);
      SETHEIGHT(h);
      /* Set the direction and destination (System Memory) */
      SETBLTMODE(SYSTEMDEST);
      SETROP(cirrus_rop[alu]);
      STARTBLT();

      /*
       * We must transfer 4 bytes per blit line.  This is cautious code and I
       * do not read from outside of the pixmap... The 386/486 allows
       * unaligned memory acceses, and has little endian word ordering.  This
       * is used to our advantage when dealing with the 3 byte remainder.
       * Don't try this on your Sparc :-)
       */
       
      /*
       * I doubt whether the reading of a multiple of 4 bytes *per
       * blit line* is right; I think we just need to read a multiple of
       * 4 bytes in total. We must not pad scanlines.
       * See 542x databook D8-6.
       *
       * The databook appears to have been wrong. The 543x databook says
       * the same thing, but it may be right. Sigh.
       *
       */

      if (HAVE543X())
          CirrusImageReadTransfer543x(w, h, pdst, widthDst, vgaBase);
      else
	  CirrusImageReadTransfer(w, h, pdst, widthDst, vgaBase);

      WAITUNTILFINISHED();
    }
  else
    {
      vgaImageRead(pdstBase, psrcBase, widthSrc, widthDst, x, y,
		   x1, y1, w, h, xdir, ydir, alu, planemask);
    }
}


#if 0

/*
 * Cirrus two-color bitmap write function (used by CopyPlane).
 * We use the BitBLT engine with color expansion system memory source,
 * reversing the per-byte bit order as we go.
 *
 * Currently doesn't handle origin offset and tiling (i.e. box and boy should
 * correspond to top-left corner of area, and bw == w, bh == h).
 *
 * bwidth is the 'pitch' of the bitmap in bytes.
 */

#ifdef __STDC__
void CirrusWriteBitmap( int x, int y, int w, int h,
unsigned char *srcp, int bwidth, int bw, int bh, int box, int boy, int bg,
int fg, int destpitch, int alu )
#else
void CirrusWriteBitmap( x, y, w, h, srcp, bwidth, bw, bh, box, boy, bg,
fg, destpitch, alu )
	int x, y, w, h;
	unsigned char *srcp;
	int bwidth, bw, bh, box, boy, bg, fg, destpitch, alu;
#endif
{
	int destaddr;
	int i;
	int bytewidth;	/* Area width in bytes. */

#if 0
	ErrorF("CirrusWriteBitmap size (%d, %d), pixwidth = %d, bw = %d\n",
		w, h, bwidth, bw);
#endif		

	if (w != bw || h != bh) {
		ErrorF("Bitmap not same size as area.\n");
		return;
	}

	if (!HAVE543X() && h > 1024) {
		/* Split into two. */
		CirrusWriteBitmap(x, y, w, 1024, srcp, bwidth, bw, 1024, box,
			boy, bg, fg, destpitch, alu);
		CirrusWriteBitmap(x, y + 1024, w, h - 1024, srcp + bwidth *
			1024, bwidth, bw, h - 1024, box, boy, bg, fg,
			destpitch, alu);
		return;
	}

	/* Bug: need to handle origin. */

	destaddr = y * destpitch + x;

	/* Number of bytes in area line. */
	bytewidth = (w + 7) / 8;

	SETDESTADDR(destaddr);
	SETDESTPITCH(destpitch);
	SETSRCADDR(0);
	SETSRCPITCH(0);
	SETWIDTH(w);
	SETHEIGHT(h);
	SETBLTMODE(SYSTEMSRC | COLOREXPAND);
	SETROP(cirrus_rop[alu]);

	SETBACKGROUNDCOLOR(bg);
	SETFOREGROUNDCOLOR(fg);

	STARTBLT();

	/* We must be very cautious here. For each bitmap line, we must pad
	 * to a byte boundary, but we may not transfer bytes; and for the
	 * total operation, we must transfer a multiple of 4 bytes. Failure
	 * to do this locks the machine.
	 */

	{
		unsigned char *buffer;
		int size;
		size = bytewidth * h;
		buffer = (unsigned char *)ALLOCATE_LOCAL(size + 3);
		for (i = 0; i < h; i++) {
			memcpy(buffer + i * bytewidth, srcp + i * bwidth,
				bytewidth);
		}
		/* Make sure we transfer a multiple of four bytes. */
		switch (size & 3)  {
		case 0 :
			/* Transfer (size / 2) 16-bit words. */
			transferwords(vgaBase, buffer, size / 2);
			break;
		case 1 :
			transferwords(vgaBase, buffer, (size + 3) / 2);
			break;
		case 2 :
			transferwords(vgaBase, buffer, (size + 2) / 2);
			break;
		case 3 :
			transferwords(vgaBase, buffer, (size + 1) / 2);
			break;
		}
		DEALLOCATE_LOCAL(buffer);
	}

	WAITUNTILFINISHED();

	SETBACKGROUNDCOLOR(0x0f);
	SETFOREGROUNDCOLOR(0);
}

#endif
