/* $XConsortium: s3im.c,v 1.1 94/03/28 21:15:39 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3im.c,v 3.1 1994/06/18 16:24:46 dawes Exp $ */
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
 * Id: s3im.c,v 2.7 1993/08/10 15:20:03 jon Exp jon
 */


#include "misc.h"
#include "xf86.h"
#include "s3.h"
#include "regs3.h"
#include "s3im.h"
#include "scrnintstr.h"
#include "cfbmskbits.h"
#include "s3linear.h"
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
extern Bool xf86Verbose;
extern Bool s3LinearAperture;
extern void  (*s3ImageReadFunc) (), (*s3ImageWriteFunc) ();
extern void  (*s3ImageFillFunc) ();

static void s3ImageRead ();
static void s3ImageWrite ();
static void s3ImageFill ();
static void s3ImageReadNoMem ();
void s3ImageWriteNoMem ();
static void s3ImageFillNoMem ();
static void s3ImageReadBanked ();
static void s3ImageWriteBanked ();
static void s3ImageFillBanked ();
static char old_bank = -1;
extern char s3Mbanks;
extern ScrnInfoRec s3InfoRec;


void
s3ImageInit ()
{
   int   i;
   static Bool reEntry = FALSE;
      
   if (reEntry) {
      old_bank=-1;
      if (s3InfoRec.videoRam > 1024)
	 s3Mbanks = -1;
      return;
   }
   
   reEntry = TRUE;
   for (i = 0; i < 256; i++) {
      reorder (i, s3SwapBits[i]);
   }
   
   if (OFLG_ISSET(OPTION_NO_MEM_ACCESS, &s3InfoRec.options)) {
      s3ImageReadFunc = s3ImageReadNoMem;
      s3ImageWriteFunc = s3ImageWriteNoMem;
      s3ImageFillFunc = s3ImageFillNoMem;
      ErrorF ("%s %s: Direct memory accessing has been disabled\n",
	      OFLG_ISSET(XCONFIG_NOMEMACCESS, &s3InfoRec.xconfigFlag) ?
		XCONFIG_PROBED : XCONFIG_GIVEN, s3InfoRec.name);
      return;
   }


  if (s3LinearAperture &&
      !OFLG_ISSET(OPTION_NOLINEAR_MODE, &s3InfoRec.options)) {
	 s3ImageReadFunc = s3ImageRead;
	 s3ImageWriteFunc = s3ImageWrite; 
	 s3ImageFillFunc = s3ImageFill;            
   } else {
      if (!(s3BankSize % s3DisplayWidth)) {
	 s3ImageReadFunc = s3ImageRead;
	 s3ImageWriteFunc = s3ImageWrite; 
	 s3ImageFillFunc = s3ImageFill;
      } else {
	 s3ImageReadFunc = s3ImageReadBanked;
	 s3ImageWriteFunc = s3ImageWriteBanked;
	 s3ImageFillFunc = s3ImageFillBanked;
      }
   }


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


static void
s3ImageWriteBanked (x, y, w, h, psrc, pwidth, px, py, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short alu;
     short planemask;
{
   int   j, offset;
   char  bank;
   char *videobuffer;

   if (alu == MIX_DST)
      return;

   if ((alu != MIX_SRC) || ((planemask & PMSK) != PMSK)) {
      s3ImageWriteNoMem(x, y, w, h, psrc, pwidth, px, py, alu, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;
            
   if (w == 0 || h == 0)
      return;

   BLOCK_CURSOR;
   WaitQueue (2);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);

   WaitQueue (8);

   psrc += pwidth * py + px;
   offset = (y * s3DisplayWidth) + x;
   bank = offset / s3BankSize;
   offset %= s3BankSize;

   s3EnableLinear();
 /*
  * if we do a bank switch here, is _not_ possible to do one in the loop
  * before some data has been copied; for that situation to occur it would be
  * necessary that offset == s3BankSize; and by the above initialisation of
  * offset, we know offset < s3BankSize
  */
   outb (vgaCRIndex, 0x35);
   if (old_bank != bank) {
      s3BankSelect(bank);
   }

   for (j = 0; j < h; j++, psrc += pwidth, offset += s3DisplayWidth) {
      if (offset + w > s3BankSize) {
	 int   partwidth;

       /* do the copy in two parts with a bank switch inbetween */
	 partwidth = s3BankSize - offset;
	 if (partwidth > 0)
	    MemToBus (&videobuffer[offset], psrc, partwidth);

       /* bank switch to the next bank */
	 bank++;
	s3BankSelect(bank);

       /* adjust the offset by 1 banks worth */
	 offset -= s3BankSize;

       /* for a partial copy, copy the bit that was left over only */
	 if (partwidth > 0) {
	    MemToBus (videobuffer, psrc + partwidth, w - partwidth);
	    continue;
	 }
       /* drop through to the `normal' copy */
      }
      MemToBus (&videobuffer[offset], psrc, w);
   }
   old_bank = bank;

   s3DisableLinear();
   
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageReadBanked (x, y, w, h, psrc, pwidth, px, py, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     unsigned char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short planemask;
{
   int   j;
   int   offset;
   int   bank;
   char *videobuffer;

   if (w == 0 || h == 0)
      return;

   if ((planemask & PMSK) != PMSK) {
      s3ImageReadNoMem(x, y, w, h, psrc, pwidth, px, py, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;

   BLOCK_CURSOR;
   WaitIdleEmpty ();
   outw (FRGD_MIX, FSS_PCDATA | MIX_SRC);

   s3EnableLinear();
   psrc += pwidth * py + px;
   offset = (y * s3DisplayWidth) + x;
   bank = offset / s3BankSize;
   offset %= s3BankSize;

   outb (vgaCRIndex, 0x35);
   if (old_bank != bank) {
      s3BankSelect(bank);
   }

   for (j = 0; j < h; j++, psrc += pwidth, offset += s3DisplayWidth) {
      if (offset + w > s3BankSize) {
	 int   partwidth;

       /* do the copy in two parts with a bank switch inbetween */
	 partwidth = s3BankSize - offset;
	 if (partwidth > 0)
	    BusToMem (psrc, &videobuffer[offset], partwidth);

       /* bank switch to the next bank */
	 bank++;
         s3BankSelect(bank);

       /* adjust the offset by 1 banks worth */
	 offset -= s3BankSize;

	 if (partwidth > 0) {
	    BusToMem (psrc + partwidth, videobuffer, w - partwidth);
	    continue;
	 }
       /* drop through to the `normal' copy */
      }
      BusToMem (psrc, &videobuffer[offset], w);
   }
   old_bank = bank;
   s3DisableLinear();
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageFillBanked (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     int   pw, ph, pox, poy;
     unsigned char *psrc;
     int   pwidth;
     short alu;
     short planemask;
{
   int   j;
   unsigned char *pline;
   int   ypix, xpix0, offset0;
   int   cxpix;
   char *videobuffer;
   char  bank;

   if (alu == MIX_DST)
      return;

   if ((alu != MIX_SRC) || ((planemask & PMSK) != PMSK)) {
      s3ImageFillNoMem(x, y, w, h, psrc, pwidth,
                     pw, ph, pox, poy, alu, planemask);
      return;
   }
   videobuffer = (char *) s3VideoMem;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitQueue (2);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);

   WaitQueue (8);

   modulus (x - pox, pw, xpix0);
   cxpix = pw - xpix0;

   modulus (y - poy, ph, ypix);
   pline = psrc + pwidth * ypix;

   offset0 = (y * s3DisplayWidth) + x;
   bank = offset0 / s3BankSize;
   offset0 %= s3BankSize;

   s3EnableLinear();
   outb (vgaCRIndex, 0x35);
   if (bank != old_bank) {
      s3BankSelect(bank);
   }

   for (j = 0; j < h; j++, offset0 += s3DisplayWidth) {
      int   offset, width, xpix;

      width = (w <= cxpix) ? w : cxpix;
      xpix = xpix0;
      offset = offset0;

      if (offset + width >= s3BankSize) {
	 int   partwidth;

	 partwidth = s3BankSize - offset;
	 offset0 -= s3BankSize;

	 if (partwidth > 0) {
	    MemToBus (&videobuffer[offset], pline + xpix, partwidth);
	    width -= partwidth;
	    xpix += partwidth;
	    offset = 0;
	 } else
	    offset = offset0;

	 bank++;
         s3BankSelect(bank);      
      }
      MemToBus (&videobuffer[offset], pline + xpix, width);

      offset += width;
      for (width = w - cxpix; width >= pw; width -= pw, offset += pw) {
       /* identical to ImageWrite() */
	 if (offset + pw > s3BankSize) {
	    int   partwidth;

	    partwidth = s3BankSize - offset;
	    if (partwidth > 0)
	       MemToBus (&videobuffer[offset], pline, partwidth);

	    bank++;
	    s3BankSelect(bank);     
	    offset -= s3BankSize;
	    offset0 -= s3BankSize;	     

	    if (partwidth > 0) {
	       MemToBus (videobuffer, pline + partwidth, pw - partwidth);
	       continue;
	    }
	 }
	 MemToBus (&videobuffer[offset], pline, pw);
      }

    /* at this point: 0 <= width < pw */
      xpix = 0;    
      if (width > 0) {
	 if (offset + width > s3BankSize) {
	    int   partwidth;

	    partwidth = s3BankSize - offset;

	    if (partwidth > 0) {
	       MemToBus (&videobuffer[offset], pline, partwidth);
	       width -= partwidth;
	       xpix = partwidth;
	       offset = 0;
	    } else {
	       offset -= s3BankSize;
	    }

	    offset0 -= s3BankSize;
	    bank++;
	    s3BankSelect(bank);
	 }
	 MemToBus (&videobuffer[offset], pline + xpix, width);
      }
      if ((++ypix) == ph) {
	 ypix = 0;
	 pline = psrc;
      } else
	 pline += pwidth;
   }
   old_bank = bank;

   s3DisableLinear();
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageWrite (x, y, w, h, psrc, pwidth, px, py, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short alu;
     short planemask;
{
   int   j, offset;
   char  bank;
   char *videobuffer;

   if (alu == MIX_DST)
      return;

   if ((alu != MIX_SRC) || ((planemask & PMSK) != PMSK)) {
      s3ImageWriteNoMem(x, y, w, h, psrc, pwidth, px, py, alu, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitQueue (2);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);

   s3EnableLinear();
   psrc += pwidth * py + px;
   offset = (y * s3DisplayWidth) + x;
   bank = offset / s3BankSize;
   offset %= s3BankSize;

   
 /*
  * if we do a bank switch here, is _not_ possible to do one in the loop
  * before some data has been copied; for that situation to occur it would be
  * necessary that offset == s3BankSize; and by the above initialisation of
  * offset, we know offset < s3BankSize
  */
   outb (vgaCRIndex, 0x35);
   if (old_bank != bank) {
      s3BankSelect(bank);
   }
   
   for (j = 0; j < h; j++, psrc += pwidth, offset += s3DisplayWidth) {
      if (offset >= s3BankSize) {
       /* bank switch to the next bank */
	 bank++;
	 s3BankSelect(bank);

       /* adjust the offset by 1 banks worth */
	 offset -= s3BankSize;
      }
      MemToBus (&videobuffer[offset], psrc, w);
   }
   old_bank = bank;
   s3DisableLinear();
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageRead (x, y, w, h, psrc, pwidth, px, py, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     unsigned char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short planemask;
{
   int   j;
   int   offset;
   int   bank;
   char *videobuffer;


   if (w == 0 || h == 0)
      return;

   if ((planemask & PMSK) != PMSK) {
      s3ImageReadNoMem(x, y, w, h, psrc, pwidth, px, py, planemask);
      return;
   }

   videobuffer = (char *) s3VideoMem;
      
   BLOCK_CURSOR;
   WaitIdleEmpty ();
   outw (FRGD_MIX, FSS_PCDATA | MIX_SRC);
   s3EnableLinear();
   psrc += pwidth * py + px;
   offset = (y * s3DisplayWidth) + x;
   bank = offset / s3BankSize;
   offset %= s3BankSize;

   outb (vgaCRIndex, 0x35);
   if (old_bank != bank) {
      s3BankSelect(bank);
   }

   for (j = 0; j < h; j++, psrc += pwidth, offset += s3DisplayWidth) {
      if (offset >= s3BankSize) {
       /* bank switch to the next bank */
	 bank++;
	 s3BankSelect(bank);

       /* adjust the offset by 1 banks worth */
	 offset -= s3BankSize;
      }
      BusToMem (psrc, &videobuffer[offset], w);
   }
   old_bank = bank;

   s3DisableLinear();
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageFill (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     int   pw, ph, pox, poy;
     unsigned char *psrc;
     int   pwidth;
     short alu;
     short planemask;
{
   int   j;
   unsigned char *pline;
   int   ypix, xpix, offset0;
   int   cxpix;
   char *videobuffer;
   char  bank;

   if (alu == MIX_DST)
      return;

   if ((alu != MIX_SRC) || ((planemask & PMSK) != PMSK)) {
     s3ImageFillNoMem(x, y, w, h, psrc, pwidth,
                    pw, ph, pox, poy, alu, planemask);
     return;
   }

   videobuffer = (char *) s3VideoMem;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitQueue (2);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);

   s3EnableLinear();
   modulus (x - pox, pw, xpix);
   cxpix = pw - xpix;

   modulus (y - poy, ph, ypix);
   pline = psrc + pwidth * ypix;

   offset0 = (y * s3DisplayWidth) + x;
   bank = offset0 / s3BankSize;
   offset0 %= s3BankSize;

   outb (vgaCRIndex, 0x35);
   if (bank != old_bank) {
      s3BankSelect(bank);
   }

   for (j = 0; j < h; j++, offset0 += s3DisplayWidth) {
      if (offset0 >= s3BankSize) {
	 bank++;
	 s3BankSelect(bank);
	 offset0 -= s3BankSize;
      }
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
   old_bank = bank;
   s3DisableLinear();
   WaitQueue(1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

void
s3ImageWriteNoMem (x, y, w, h, psrc, pwidth, px, py, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short alu;
     short planemask;
{
   int   i, j;

   if (alu == MIX_DST)
      return;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitQueue (3);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);
   outw (MULTIFUNC_CNTL, 0xa000);

   WaitQueue (5);
   outw (CUR_X, (short) x);
   outw (CUR_Y, (short) y);
   outw (MAJ_AXIS_PCNT, (short) w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   outw (CMD, CMD_RECT | BYTSEQ | _16BIT | INC_Y | INC_X | DRAW | PCDATA
	  | WRTDATA);

   WaitQueue (8);

   w += px;
   psrc += pwidth * py;

   for (j = 0; j < h; j++) {
      for (i = px; i < w; i += 2)
	 outw (PIX_TRANS, (short) ((psrc[i + 1] << 8) | psrc[i]));
      psrc += pwidth;
   }

   WaitQueue (2);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   outw (WRT_MASK, 0xffff);
   UNBLOCK_CURSOR;
}

static void
s3ImageReadNoMem (x, y, w, h, psrc, pwidth, px, py, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     unsigned char *psrc;
     int   pwidth;
     int   px;
     int   py;
     short planemask;
{
   int   i, j;
   short pix;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitIdleEmpty ();
   WaitQueue (7);
   outw (MULTIFUNC_CNTL, 0xa000);
   outw (FRGD_MIX, FSS_PCDATA | MIX_SRC);
   outw (CUR_X, (short) x);
   outw (CUR_Y, (short) y);
   outw (MAJ_AXIS_PCNT, (short) w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   outw (CMD, CMD_RECT | BYTSEQ | _16BIT | INC_Y | INC_X | DRAW |
	  PCDATA);

   WaitQueue (8);

 /* wait for data to be ready */
   if (!S3_x64_SERIES(s3ChipId)) {
     while ((inw (GP_STAT) & 0x100) == 0) ;
   }
   else {
     /* x64: GP_STAT bit 8 is reserved for 864/964 */
   }

   w += px;
   psrc += pwidth * py;

   for (j = 0; j < h; j++) {
      for (i = px; i < w; i++) {
	 pix = inw (PIX_TRANS);
	 psrc[i++] = (char) (pix & planemask);
	 if (i < w)
	    psrc[i] = (char) ((pix >> 8) & planemask);
      }
      psrc += pwidth;
   }
   WaitQueue (1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

static void
s3ImageFillNoMem (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu,
		  planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     unsigned char *psrc;
     int   pwidth;
     int   pw, ph, pox, poy;
     short alu;
     short planemask;
{
   int   i, j;
   unsigned char *pline;
   int   mod;

   if (alu == MIX_DST)
      return;

   if (w == 0 || h == 0)
      return;
      
   BLOCK_CURSOR;
   WaitQueue (7);
   outw (FRGD_MIX, FSS_PCDATA | alu);
   outw (WRT_MASK, planemask);
   outw (CUR_X, (short) x);
   outw (CUR_Y, (short) y);
   outw (MAJ_AXIS_PCNT, (short) w - 1);
   outw (MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   outw (CMD, CMD_RECT | INC_Y | INC_X | DRAW | PCDATA | WRTDATA);

   for (j = 0; j < h; j++) {
      modulus (y + j - poy, ph, mod);
      pline = psrc + pwidth * mod;
      for (i = 0; i < w; i++) {
	 modulus (x + i - pox, pw, mod);
	 outw (PIX_TRANS, (short) pline[mod]);
      }
   }

   WaitQueue (1);
   outw (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);   
   UNBLOCK_CURSOR;
}

static int _internal_s3_mskbits[9] =
{
   0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

/* #define MSKBITS(n) ((n)? ((1<<(n))-1): 0) */
#define MSKBIT(n) (_internal_s3_mskbits[(n)])
#define SWPBIT(s) (s3SwapBits[pline[(s)]])

void
s3ImageStipple (x, y, w, h, psrc, pwidth, pw,
		ph, pox, poy, fgPixel, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     char *psrc;
     int   pw, ph, pox, poy;
     int   pwidth;
     int   fgPixel;
     short alu;
     short planemask;
{
   int   x1, x2, y1, y2, width;
   unsigned char *newsrc = NULL;

   if (alu == MIX_DST)
      return;

   if (w == 0 || h == 0)
      return;
      
   x1 = x ;
   x2 = (x + w);
   y1 = y;
   y2 = y + h;

   width = w;

   if (pw <= 8 && pw < w) {
      newsrc = (unsigned char *) ALLOCATE_LOCAL (2 * ph * sizeof (char));

      if (!newsrc) {
	 return;
      }
      while (pw <= 8) {
	 unsigned char *newline, *pline;
	 int   i;

	 pline = (unsigned char *)psrc;
	 newline = newsrc;
	 for (i = 0; i < ph; i++) {
	    newline[0] = (pline[0] & (0xff >> (8 - pw))) | pline[0] << pw;
	    if (pw > 4)
	       newline[1] = pline[0] >> (8 - pw);

	    pline += pwidth;
	    newline += 2;
	 }
	 pw *= 2;
	 pwidth = 2;
	 psrc = (char *)newsrc;
      }
   }
   BLOCK_CURSOR;

   WaitQueue (6);
   S3_OUTW (WRT_MASK, planemask);   
   S3_OUTW (FRGD_MIX, FSS_FRGDCOL | alu);
   S3_OUTW (BKGD_MIX, BSS_BKGDCOL | MIX_DST);
   S3_OUTW (FRGD_COLOR, (short) fgPixel);
   S3_OUTW (MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPPC | COLCMPOP_F);
   S3_OUTW (MAJ_AXIS_PCNT, (short) (width - 1));

   WaitQueue (4);
   S3_OUTW (CUR_X, (short) x1);
   S3_OUTW (CUR_Y, (short) y1);
   S3_OUTW (MULTIFUNC_CNTL, MIN_AXIS_PCNT | h-1);   
   S3_OUTW (CMD, CMD_RECT | PCDATA | _16BIT | INC_Y | INC_X |
	     DRAW | PLANAR | WRTDATA);

   {
      int   xpix, ypix, j;
      int   clobits, lobits, chibits, hibits, pw8;
      unsigned char *pline;

      modulus (x - pox, pw, xpix);
      clobits = xpix % 8;
      lobits = 8 - clobits;
      xpix /= 8;

      hibits = pw % 8;
      chibits = 8 - hibits;
      pw8 = pw / 8;

      modulus (y1 - poy, ph, ypix);
      pline = (unsigned char *)psrc + (pwidth * ypix);
      for (j = y1; j < y2; j++) {
	 unsigned long getbuf;
	 int   i, bitlft, pix;


         if (pw8 == xpix) {
            bitlft = hibits - clobits;
            getbuf = (SWPBIT (xpix) & ~MSKBIT (chibits)) >> chibits;
            pix = 0;
	 } else {
            bitlft = lobits;
            getbuf = SWPBIT (xpix) & MSKBIT (lobits);
            pix = xpix + 1;
	 }

	 for (i = 0; i < width; i += 16) {
	    while (bitlft < 16) {
	       if (pix >= pw8) {
		  if (hibits > 0) {
		     getbuf = (getbuf << hibits)
			| ((SWPBIT (pix) & ~MSKBIT (chibits)) >> chibits);
		     bitlft += hibits;
		  }
		  pix = 0;
	       }
	       getbuf = (getbuf << 8) | SWPBIT (pix++);
	       bitlft += 8;
	    }
	    bitlft -= 16;
	    S3_OUTW (PIX_TRANS, (getbuf >> bitlft) & 0xffff);
	 }

	 if ((++ypix) == ph) {
	    ypix = 0;
	    pline = (unsigned char *)psrc;
	 } else
	    pline += pwidth;
      }
   }

   WaitQueue (3);
   S3_OUTW (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW (BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   S3_OUTW (MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   UNBLOCK_CURSOR;

   if (newsrc)
      DEALLOCATE_LOCAL (newsrc);
}

#if NeedFunctionPrototypes
void
s3ImageOpStipple (int x, int y, int w, int h, unsigned char *psrc, int pwidth,
		  int pw, int ph, int pox, int poy, int fgPixel, int bgPixel,
		  short alu, short planemask)
#else
void
s3ImageOpStipple (x, y, w, h, psrc, pwidth, pw, ph, pox, poy, fgPixel,
		  bgPixel, alu, planemask)
     int   x;
     int   y;
     int   w;
     int   h;
     unsigned char  *psrc;
     int   pwidth;
     int   pw, ph, pox, poy;
     int   fgPixel;
     int   bgPixel;
     short alu;
     short planemask;
#endif
{
   int   x1, x2, y1, y2, width;
   unsigned char *newsrc = NULL;

   if (alu == MIX_DST)
      return;

   if (w == 0 || h == 0)
      return;

   x1 = x;
   x2 = (x + w);
   y1 = y;
   y2 = y + h;

   width = w;

   if (pw <= 8 && pw < w) {
      newsrc = (unsigned char *) ALLOCATE_LOCAL (2 * ph * sizeof (char));

      if (!newsrc) {
	 return;
      }
      while (pw <= 8) {
	 unsigned char *newline, *pline;
	 int   i;

	 pline = psrc;
	 newline = newsrc;
	 for (i = 0; i < ph; i++) {
	    newline[0] = (pline[0] & (0xff >> (8 - pw))) | pline[0] << pw;
	    if (pw > 4)
	       newline[1] = pline[0] >> (8 - pw);

	    pline += pwidth;
	    newline += 2;
	 }
	 pw *= 2;
	 pwidth = 2;
	 psrc = newsrc;
      }
   }
   BLOCK_CURSOR;

#if 0
WaitQueue(4);
S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | y);
S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | x);
S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (x+w)-1);
S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | (y+h)-1);
#endif

   WaitQueue (7);
   S3_OUTW (WRT_MASK, planemask);   
   S3_OUTW (FRGD_MIX, FSS_FRGDCOL | alu);
   S3_OUTW (BKGD_MIX, BSS_BKGDCOL | alu);
   S3_OUTW (FRGD_COLOR, (short) fgPixel);
   S3_OUTW (BKGD_COLOR, (short) bgPixel);
   S3_OUTW (MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPPC | COLCMPOP_F);
   S3_OUTW (MAJ_AXIS_PCNT, (short) (width - 1));

   WaitQueue (4);
   S3_OUTW (CUR_X, (short) x1);
   S3_OUTW (CUR_Y, (short) y1);
   S3_OUTW (MULTIFUNC_CNTL, MIN_AXIS_PCNT | h-1);   
   S3_OUTW (CMD, CMD_RECT | PCDATA | _16BIT | INC_Y | INC_X |
	     DRAW | PLANAR | WRTDATA);


   {
      int   xpix, ypix, j;
      int   clobits, lobits, chibits, hibits, pw8;
      unsigned char *pline;

      modulus (x - pox, pw, xpix);
      clobits = xpix % 8;
      lobits = 8 - clobits;
      xpix /= 8;

      hibits = pw % 8;
      chibits = 8 - hibits;
      pw8 = pw / 8;

      modulus (y1 - poy, ph, ypix);
      pline = (unsigned char *)psrc + (pwidth * ypix);

      for (j = y1; j < y2; j++) {
	 unsigned long getbuf;
	 int   i, bitlft, pix;

	 WaitQueue (3);
         if (pw8 == xpix) {
            bitlft = hibits - clobits;
            getbuf = (SWPBIT (xpix) & ~MSKBIT (chibits)) >> chibits;
            pix = 0;
	 } else {
            bitlft = lobits;
            getbuf = SWPBIT (xpix) & MSKBIT (lobits);
            pix = xpix + 1;
	 }
	 
	 for (i = 0; i < width; i += 16) {
	    while (bitlft < 16) {
	       if (pix >= pw8) {
		  if (hibits > 0) {
		     getbuf = (getbuf << hibits)
			| ((SWPBIT (pix) & ~MSKBIT (chibits)) >> chibits);
		     bitlft += hibits;
		  }
		  pix = 0;
	       }
	       getbuf = (getbuf << 8) | SWPBIT (pix++);
	       bitlft += 8;
	    }
	    bitlft -= 16;
	    S3_OUTW (PIX_TRANS, (getbuf >> bitlft) & 0xffff);
	 }

	 if ((++ypix) == ph) {
	    ypix = 0;
	    pline = (unsigned char *)psrc;
	 } else
	    pline += pwidth;
      }
   }

   WaitQueue (3);
   S3_OUTW (FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW (BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   S3_OUTW (MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   UNBLOCK_CURSOR;

   if (newsrc)
      DEALLOCATE_LOCAL (newsrc);
}
