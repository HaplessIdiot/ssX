/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3_virge/s3im.c,v 3.0 1996/09/22 13:25:43 dawes Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Modified by Amancio Hasty and extensively by Jon Tombs & Phil Richards.
 *
 */
/* $XConsortium: s3im.c /main/10 1995/12/29 15:55:19 kaleb $ */


#include "misc.h"
#include "xf86.h"
#include "s3.h"
#include "regs3.h"
#include "s3im.h"
#include "scrnintstr.h"
#include "cfbmskbits.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

#define	reorder(a,b)	b = \
	(a & 0x80) >> 7 | \
	(a & 0x40) >> 5 | \
	(a & 0x20) >> 3 | \
	(a & 0x10) >> 1 | \
	(a & 0x08) << 1 | \
	(a & 0x04) << 3 | \
	(a & 0x02) << 5 | \
	(a & 0x01) << 7;

extern unsigned char s3SwapBits[256];
extern int s3ScreenMode;
extern int   s3DisplayWidth;
extern int   s3BankSize;
extern unsigned char s3Port51;
extern unsigned char s3Port40;
extern unsigned char s3Port54;
extern int xf86Verbose;
extern Bool s3LinearAperture;

static void s3ImageRead (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, unsigned long
#endif
);
static void s3ImageWrite (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, unsigned long
#endif
);
static void s3ImageFill (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, int, int, unsigned long
#endif
);
static void s3ImageReadNoMem (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, unsigned long
#endif
);
void s3ImageWriteNoMem (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, unsigned long
#endif
);
static void s3ImageFillNoMem (
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, int, int, unsigned long
#endif
);
extern ScrnInfoRec s3InfoRec;
int s3Bpp;
int s3BppDisplayWidth;
Pixel s3BppPMask;

void
s3ImageInit ()
{
   int   i;
   static Bool reEntry = FALSE;

   if (reEntry) {
      return;
   }

   reEntry = TRUE;
   s3Bpp = s3InfoRec.bitsPerPixel / 8;

   s3BppDisplayWidth = s3Bpp * s3DisplayWidth;
   s3BppPMask = (1UL << s3InfoRec.bitsPerPixel) - 1;

   for (i = 0; i < 256; i++) {
      reorder (i, s3SwapBits[i]);
   }

   s3ImageReadFunc  = s3ImageRead;
   s3ImageWriteFunc = s3ImageWrite;
   s3ImageFillFunc  = s3ImageFill;

   if (xf86Verbose)
      ErrorF ("%s %s: Using a banksize of %dk, line width of %d\n",
           XCONFIG_PROBED, s3InfoRec.name, s3BankSize/1024, s3DisplayWidth);
}

/* fast ImageWrite(), ImageRead(), and ImageFill() routines */
/* there are two cases; (i) when the bank switch can occur in the */
/* middle of raster line, and (ii) when it is guaranteed not possible. */
/* In theory, s3InfoRec.virtualX should contain the number of bytes */
/* on the raster line; however, this is not necessarily true, and for */
/* many situations, the S3 card will always have 1024. */
/* Phil Richards <pgr@prg.ox.ac.uk> */
/* 26th November 1992 */
/* Bug fixed by Jon Tombs */
/* 30/7/94 began 16,32 bit support */


static void
#if NeedFunctionPrototypes
s3ImageWrite (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   px,
     int   py,
     int   alu,
     unsigned long planemask)
#else
s3ImageWrite (x, y, w, h, psrc, pwidth, px, py, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     int   alu;
     unsigned long planemask;
#endif
{
   int   j, offset;
   char *videobuffer;

   if (alu == ROP_D)
      return;

   if ((alu != ROP_S) || ((planemask & s3BppPMask) != s3BppPMask)) {
      s3ImageWriteNoMem(x, y, w, h, psrc, pwidth, px, py, alu, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;

   if (w == 0 || h == 0)
      return;

   BLOCK_CURSOR;
#if 0
   WaitQueue16_32(2,3);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw32(WRT_MASK, planemask);
#endif

   psrc += pwidth * py + px * s3Bpp;
   offset = (y * s3BppDisplayWidth) + x *s3Bpp;

   w *= s3Bpp;
   WaitIdleEmpty();

   for (j = 0; j < h; j++, psrc += pwidth, offset += s3BppDisplayWidth) {
      MemToBus (&videobuffer[offset], psrc, w);
   }

   UNBLOCK_CURSOR;
}

static void
#if NeedFunctionPrototypes
s3ImageRead (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   px,
     int   py,
     unsigned long planemask)
#else
s3ImageRead (x, y, w, h, psrc, pwidth, px, py, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     unsigned long planemask;
#endif
{
   int   j;
   int   offset;
   char *videobuffer;


   if (w == 0 || h == 0)
      return;

   if ((planemask & s3BppPMask) != s3BppPMask) {
      s3ImageReadNoMem(x, y, w, h, psrc, pwidth, px, py, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;


#if 0
   outw (FRGD_MIX, FSS_PCDATA | ROP_S);
#endif

   psrc += pwidth * py + px * s3Bpp;
   offset = (y * s3BppDisplayWidth) + x * s3Bpp;
   w *= s3Bpp;

   BLOCK_CURSOR;
   WaitIdleEmpty ();

   for (j = 0; j < h; j++, psrc += pwidth, offset += s3BppDisplayWidth) {
      BusToMem (psrc, &videobuffer[offset], w);
   }

#if 0
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | ROP_S);
#endif
   UNBLOCK_CURSOR;
}

static void
#if NeedFunctionPrototypes
s3ImageFill (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   pw,
     int   ph,
     int   pox,
     int   poy,
     int   alu,
     unsigned long planemask)
#else
s3ImageFill (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   pw;
     int   ph;
     int   pox;
     int   poy;
     int   alu;
     unsigned long planemask;
#endif
{
   int   j;
   char *pline;
   int   ypix, xpix, offset0;
   int   cxpix;
   char *videobuffer;

   if (alu == ROP_D)
      return;

   if ((alu != ROP_S) || ((planemask & s3BppPMask) != s3BppPMask)) {
     s3ImageFillNoMem(x, y, w, h, psrc, pwidth,
                    pw, ph, pox, poy, alu, planemask);
     return;
   }

   videobuffer = (char *) s3VideoMem;

   if (w == 0 || h == 0)
      return;


#if 0
   WaitQueue16_32(2,3);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw32(WRT_MASK, planemask);
#endif

   w  *= s3Bpp;
   pw *= s3Bpp;

   modulus ((x - pox) * s3Bpp, pw, xpix);
   cxpix = pw - xpix ;

   modulus (y - poy, ph, ypix);
   pline = psrc + pwidth * ypix;

   offset0 = (y * s3BppDisplayWidth) + x * s3Bpp;

   BLOCK_CURSOR;
   WaitIdleEmpty();

   for (j = 0; j < h; j++, offset0 += s3BppDisplayWidth) {
      if (w <= cxpix) {
	 MemToBus (&videobuffer[offset0], pline + xpix, w);
      } else {
	 int   width, offset;

	 MemToBus (&videobuffer[offset0], pline + xpix, cxpix);

	 offset = offset0 + cxpix;
	 for (width = w - cxpix; width >= pw; width -= pw, offset += pw)
	    MemToBus (&videobuffer[offset], pline, pw);

       /* at this point: 0 <= width < pw */
	 if (width > 0)
	    MemToBus (&videobuffer[offset], pline, width);
      }

      if ((++ypix) == ph) {
	 ypix = 0;
	 pline = psrc;
      } else
	 pline += pwidth;
   }

   UNBLOCK_CURSOR;
}

void
#if NeedFunctionPrototypes
s3ImageWriteNoMem (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   px,
     int   py,
     int   alu,
     unsigned long planemask)
#else
s3ImageWriteNoMem (x, y, w, h, psrc, pwidth, px, py, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     int   alu;
     unsigned long planemask;
#endif
{
   if (0) ErrorF("s3ImageWriteNoMem called\n");
#ifdef OLD_S3
   int   i, j;

   if (alu == ROP_D)
      return;

   if (w == 0 || h == 0)
      return;

   BLOCK_CURSOR;
   WaitQueue16_32(3,4);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw32 (WRT_MASK, planemask);
   outw (MULTIFUNC_CNTL, PIX_CNTL | 0);

   WaitQueue(5);
   outw (CUR_X, x);
   outw (CUR_Y, y);
   outw (MAJ_AXIS_PCNT, w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   WaitIdle();
   outw (CMD, CMD_RECT | BYTSEQ | _16BIT | INC_Y | INC_X | DRAW | PCDATA
	  | WRTDATA);

   w *= s3Bpp;
   psrc += pwidth * py;

   for (j = 0; j < h; j++) {
      /* This assumes we can cast odd addresses to short! */
      short *psrcs = (short *)&psrc[px*s3Bpp];
      for (i = 0; i < w; ) {
	 if (s3InfoRec.bitsPerPixel == 32) {
	    outl (PIX_TRANS, *((int*)(psrcs)));
	    psrcs+=2;
	    i += 4;
	 }
	 else {
	    outw (PIX_TRANS, *psrcs++);
	    i += 2;
	 }
      }
      psrc += pwidth;
   }
#if 0
   WaitQueue16_32(2,3);
   outw (FRGD_MIX, FSS_FRGDCOL | ROP_S);
   outw32(WRT_MASK, ~0);
#endif
   UNBLOCK_CURSOR;
#endif
}


static void
#if NeedFunctionPrototypes
s3ImageReadNoMem (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   px,
     int   py,
     unsigned long planemask)
#else
s3ImageReadNoMem (x, y, w, h, psrc, pwidth, px, py, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     unsigned long planemask;
#endif
{
   if (0) ErrorF("s3ImageReadNoMem called\n");
#ifdef OLD_S3
   int   i, j;

   if (w == 0 || h == 0)
      return;

   BLOCK_CURSOR;
   WaitIdleEmpty ();
   WaitQueue(7);
   outw (MULTIFUNC_CNTL, PIX_CNTL);
   outw (FRGD_MIX, FSS_PCDATA | ROP_S);
   outw (CUR_X, x);
   outw (CUR_Y, y);
   outw (MAJ_AXIS_PCNT, w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   outw (CMD, CMD_RECT  | BYTSEQ | _16BIT | INC_Y | INC_X | DRAW |
	  PCDATA);

   WaitQueue16_32(1,2);

   outw32(RD_MASK, planemask);

   WaitQueue(8);

   w *= s3Bpp;
   psrc += pwidth * py;

   for (j = 0; j < h; j++) {
      short *psrcs = (short *)&psrc[px*s3Bpp];
      for (i = 0; i < w; ) {
	 if (s3InfoRec.bitsPerPixel == 32) {
	    *((long*)(psrcs)) = inl(PIX_TRANS);
	    psrcs += 2;
	    i += 4;
	 } else {
	    *psrcs++ = inw(PIX_TRANS);
	    i += 2;
	 }
      }
      psrc += pwidth;
   }
   WaitQueue16_32(2,3);
   outw32(RD_MASK, ~0);
   outw (FRGD_MIX, FSS_FRGDCOL | ROP_S);
   UNBLOCK_CURSOR;
#endif
}

static void
#if NeedFunctionPrototypes
s3ImageFillNoMem (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   pw,
     int   ph,
     int   pox,
     int   poy,
     int   alu,
     unsigned long planemask)
#else
s3ImageFillNoMem (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   pw;
     int   ph;
     int   pox;
     int   poy;
     int   alu;
     unsigned long planemask;
#endif
{
   if (0) ErrorF("s3ImageFillNoMem called\n");
#ifdef OLD_S3
   int   i, j;
   char *pline;
   int   mod;

   if (alu == ROP_D)
      return;

   if (w == 0 || h == 0)
      return;

   BLOCK_CURSOR;
   WaitQueue16_32(7,8);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw32 (WRT_MASK, planemask);
   outw (CUR_X, x);
   outw (CUR_Y, y);
   outw (MAJ_AXIS_PCNT, w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   WaitIdle();
   outw (CMD, CMD_RECT | BYTSEQ|_16BIT | INC_Y | INC_X | DRAW |
	 PCDATA | WRTDATA);

   for (j = 0; j < h; j++) {
      unsigned short wrapped;
      unsigned short *pend;
      unsigned short *plines;

      modulus (y + j - poy, ph, mod);
      pline = psrc + pwidth * mod;
      pend = (unsigned short *)&pline[pw*s3Bpp];
      wrapped = (pline[0] << 8) + (pline[pw-1] << 0); /* only for 8bpp */

      modulus (x - pox, pw, mod);

      plines = (unsigned short *) &pline[mod*s3Bpp];

      for (i = 0; i < w * s3Bpp;)  {

         /* arrg in 8BPP we need to check for wrap round */
         if (plines + 1 > pend) {
	    outw (PIX_TRANS, wrapped);
            plines = (unsigned short *)&pline[1]; i += 2;
	 } else {
	    if (s3InfoRec.bitsPerPixel == 32) {
	       outl (PIX_TRANS, *((long*)(plines)));
	       plines += 2;
	       i += 4;
	    }
	    else {
	       outw (PIX_TRANS, *plines++);
	       i += 2;
	    }
	 }
	 if (plines == pend)
	    plines = (unsigned short *)pline;

      }
   }

   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | ROP_S);
   UNBLOCK_CURSOR;
#endif
}

static int _internal_s3_mskbits[33] =
{
   0x0,
   0x1, 0x3, 0x7, 0xf,
   0x1f, 0x3f, 0x7f, 0xff,
   0x1ff, 0x3ff, 0x7ff, 0xfff,
   0x1fff, 0x3fff, 0x7fff, 0xffff,
   0x1ffff, 0x3ffff, 0x7ffff, 0xfffff,
   0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
   0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
   0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

#define MSKBIT(n) (_internal_s3_mskbits[(n)])


static void
s3RealImageStipple(x, y, w, h, psrc, pwidth, pw, ph, pox, poy,
		   fgPixel, bgPixel, alu, planemask, opaque)
    int			x;
    int			y;
    int			w;
    int			h;
    unsigned char	*psrc;
    int			pw, ph, pox, poy;
    int			pwidth;
    Pixel		fgPixel;
    Pixel		bgPixel;
    int			alu;
    Pixel		planemask;
    int			opaque;
{
    int			srcx, srch, dstw;
    CARD32		*ptmp;
    int 		origwidth;


    if (alu == ROP_D || w == 0 || h == 0)
	return;

    BLOCK_CURSOR;

    origwidth = w;
    w = s3CheckLSPN(w);
    if (w != origwidth) {
       WaitQueue(1+6 + (opaque ? 1 : 0));
       SETB_CLIP_L_R(x, x + origwidth-1);
    }
    else {
       WaitQueue(1+4 + (opaque ? 1 : 0));
    }

    ;SET_WRT_MASK(planemask);
    ;SET_FRGD_MIX(FSS_FRGDCOL | alu);
    ;SET_PIX_CNTL(MIXSEL_EXPPC | COLCMPOP_F);

    SETB_SRC_FG_CLR(fgPixel);
    if (opaque) {
       SETB_SRC_BG_CLR(bgPixel);
    }
    else {
       alu |= MIX_MONO_TRANSP;
    }

    SETB_RWIDTH_HEIGHT(w - 1, h);
    SETB_RSRC_XY(x, y);
    SETB_RDEST_XY(x, y);
    WaitIdle();
#if 0  /* CMD_RECT broken :-( */
      SETB_CMD_SET(s3_gcmd | CMD_RECT | INC_X | INC_Y | alu
		   | CMD_ITA_DWORD | MIX_CPUDATA | MIX_MONO_SRC );
#else
      SETB_CMD_SET(s3_gcmd | CMD_BITBLT | INC_X | INC_Y | alu
		   | CMD_ITA_DWORD | MIX_CPUDATA | MIX_MONO_SRC );
#endif

    modulus(x - pox, pw, x);
    modulus(y - poy, ph, y);
    /*
     * When the source bitmap is properly aligned, max 32 pixels wide
     * and nonrepeating, use this faster loop instead.
     */
    if( (x & 7) == 0 && w <= 32 && x+w <= pw && y+h <= ph ) {
        CARD32 pix;
        unsigned char *pnt;

	pnt = (unsigned char *)(psrc + pwidth * y + (x >> 3));
	while( h-- > 0 ) {
	   volatile unsigned long tmp;
	    pix = (CARD32)(ldl_u((CARD32 *)(pnt)));

	   tmp =    s3SwapBits[   pix         & 0xff ]
			 | (s3SwapBits[ ( pix >>  8 ) & 0xff ] <<  8)
			 | (s3SwapBits[ ( pix >> 16 ) & 0xff ] << 16)
			 | (s3SwapBits[ ( pix >> 24 ) & 0xff ] << 24);
	    *IMG_TRANS = tmp;

	    pnt += pwidth;
	}
    }
    else {
	while( h > 0 ) {
	    srch = ( y+h > ph ? ph - y : h );
	    while( srch > 0 ) {
		dstw = w;
		srcx = x;
		ptmp = (CARD32 *)(psrc + pwidth * y);
		while( dstw > 0 ) {
		    volatile unsigned long tmp;
		    int np, x2;
		    CARD32 *pnt, pix;
		    /*
		     * Assemble 32 bits and feed them to the draw engine.
		     */
		    np = pw - srcx;		/* No. pixels left in bitmap.*/
		    pnt =(CARD32 *)
				       ((unsigned char *)(ptmp) + (srcx >> 3));
		    x2 = srcx & 7;		/* Offset within byte. */
		    if( np >= 32 ) {
#ifdef __alpha__
		      pix =   (CARD32)(ldq_u((unsigned long *)(pnt)) >> x2);
#else
		      pix =   (CARD32)(ldl_u((unsigned int *)(pnt)) >> x2);
		      if (x2>0) 
			 pix |= (CARD32)(ldl_u((unsigned int *)(pnt+1)) << (32-x2));
#endif
		    }
		    else if( pw >= 32 ) {
#ifdef __alpha__
		      pix = (CARD32)((ldq_u((unsigned long *)(pnt)) >> x2)
					     & MSKBIT(np)) | (*ptmp << np);
#else
		      pix =   (CARD32)(  ldl_u((unsigned int *)(pnt)) >> x2 );
		      if (x2>0) 
			 pix |= (CARD32)(ldl_u((unsigned int *)(pnt+1)) << (32-x2));
		      pix = (pix  & MSKBIT(np)) | (*ptmp << np);
#endif
		    }
		    else if( pw >= 16 ) {
		       pix = (((ldl_u(pnt)) >> x2) & MSKBIT(np)) | (*ptmp << np);
		    }
		    else if( pw >= 8 ) {
			pix = ((ldl_u(pnt) >> x2) & MSKBIT(np)) | (ldl_u(ptmp) << np)
						      | (ldl_u(pnt) << (np+pw));
		    }
		    else {
			pix = (*ptmp >> x2) & MSKBIT(np);
			while( np < 32 && np < dstw ) {
			    pix |= *ptmp << np;
			    np += pw;
			}
		    }
		    tmp =     s3SwapBits[   pix         & 0xff ]
		                 | (s3SwapBits[ ( pix >>  8 ) & 0xff ] <<  8)
				 | (s3SwapBits[ ( pix >> 16 ) & 0xff ] << 16)
				 | (s3SwapBits[ ( pix >> 24 ) & 0xff ] << 24);
		    *IMG_TRANS = tmp;
		    srcx += 32;
		    if( srcx >= pw )
			srcx -= pw;
		    dstw -= 32;
		}
		y++;
		h--;
		srch--;
	    }
	    y = 0;
	}
     }

    if (w != origwidth) {
       SETB_CLIP_L_R(0, s3DisplayWidth-1);
    }  

    UNBLOCK_CURSOR;
}

void
#if NeedFunctionPrototypes
s3ImageStipple (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   pw,
     int   ph,
     int   pox,
     int   poy,
     Pixel fgPixel,
     int   alu,
     unsigned long planemask)
#else
s3ImageStipple (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, fgPixel, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   pw;
     int   ph;
     int   pox;
     int   poy;
     Pixel fgPixel;
     int   alu;
     unsigned long planemask;
#endif
{

    s3RealImageStipple(x, y, w, h, psrc, pwidth, pw, ph, pox, poy,
		       fgPixel, 0, alu, planemask, 0);
}

#if NeedFunctionPrototypes
void
s3ImageOpStipple (
     int   x,
     int   y,
     int   w,
     int   h,
     char *psrc,
     int   pwidth,
     int   pw,
     int   ph,
     int   pox,
     int   poy,
     Pixel fgPixel,
     Pixel bgPixel,
     int   alu,
     unsigned long planemask)
#else
void
s3ImageOpStipple (x, y, w, h, psrc, pwidth, pw,
		  ph, pox, poy, fgPixel, bgPixel, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   pw, ph, pox, poy;
     Pixel fgPixel;
     Pixel bgPixel;
     int   alu;
     unsigned long planemask;
#endif
{

    s3RealImageStipple(x, y, w, h, psrc, pwidth, pw, ph, pox, poy,
		       fgPixel, bgPixel, alu, planemask, 1);
}
