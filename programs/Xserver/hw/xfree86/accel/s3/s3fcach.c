/* $XConsortium: s3fcach.c,v 1.1 94/03/28 21:17:12 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3fcach.c,v 3.2 1994/08/03 13:27:53 dawes Exp $ */
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
 * Modified by Amancio Hasty and Jon Tombs
 * 
 * Id: s3fcach.c,v 2.5 1993/08/09 06:17:57 jon Exp jon
 */


#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
#include        "xf86.h"
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"s3.h"
#include	"regs3.h"
#include        "xf86bcache.h"
#include	"xf86fcache.h"
#include	"xf86text.h"

#define XCONFIG_FLAGS_ONLY
#include        "xf86_Config.h"

static unsigned long s3FontAge;
#define NEXT_FONT_AGE  ++s3FontAge

extern Bool xf86Verbose;

#define ALIGNMENT 8
#define PIXMAP_WIDTH 64

void
s3FontCache8Init()
{
   static int first = TRUE;
   int x, y, w, h;
   int BitPlane;
   CachePool FontPool;

   /* y now includes the cursor space */
   x = 0;
   y = s3CursorStartY + s3CursorLines;
   h = s3ScissB + 1 - y;

   /* Currently no pixmap cache when s3DisplayWidth < 1024 */

   if ((h <  PIXMAP_WIDTH) || (s3DisplayWidth < 1024)) { /* no pixmap cache */
      w = s3DisplayWidth;
      ErrorF("%s %s: No pixmap expanding area available\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   } else {
      w = 768;
      if (h >= PIXMAP_WIDTH) {
	 if (first) {
	    s3InitFrect(768, y, PIXMAP_WIDTH);
	 }
         ErrorF("%s %s: Using a single %dx%d area for expanding pixmaps\n",
	        XCONFIG_PROBED, s3InfoRec.name, PIXMAP_WIDTH, PIXMAP_WIDTH);
      }
   }
      
   /*
    * Don't allow a font cache if we don't have room for at least
    * 2 complete 6x13 fonts.
    */
   if (w >= 6*32 && h >= 2*13) {
      if( first ) {
         FontPool = xf86CreateCachePool(ALIGNMENT);
         for( BitPlane = s3InfoRec.bitsPerPixel-1; BitPlane >= 0; BitPlane-- ) {
            xf86AddToCachePool(FontPool, x, y, w, h, BitPlane );
	 }

         xf86InitFontCache(FontPool, w, h, s3ImageOpStipple, s3alu );
         xf86InitText(s3GlyphWrite, s3NoCPolyText, s3NoCImageText );
         ErrorF("%s %s: Using %d planes of %dx%d at (%d,%d) aligned %d as font cache\n",
                XCONFIG_PROBED, s3InfoRec.name,
                s3InfoRec.bitsPerPixel, w, h, x, y, ALIGNMENT );
      }
      else {
        xf86ReleaseFontCache();
      }
   } else if (first) {

      /*
       * Crash and burn if the cached glyph write function gets called.
       */
      xf86InitText( NULL, s3NoCPolyText, s3NoCImageText );
      ErrorF( "%s %s: No font cache available\n",
              XCONFIG_PROBED, s3InfoRec.name );
   }
   first = 0;
   return;
}

static __inline__ void
Dos3CPolyText8(x, y, count, chars, fentry, pGC, pBox)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
     BoxPtr pBox;
{
   int   gHeight;
   int   w = fentry->w;
   int blocki = 255;
   unsigned short height = 0;
   unsigned short width = 0;
   Pixel pmsk = 0;

   BLOCK_CURSOR;
   for (;count > 0; count--, chars++) {
      CharInfoPtr pci;
      short xoff;

      pci = fentry->pci[(int)*chars];

      if (pci != NULL) {

	 gHeight = GLYPHHEIGHTPIXELS(pci);
	 if (gHeight) {

	    if ((int) (*chars / 32) != blocki) {
	       bitMapBlockPtr block;
	       
	       blocki = (int) (*chars / 32);
	       block = fentry->fblock[blocki];
	       if (block == NULL) {
		  /*
		   * Reset the GE context to a known state before calling
		   * the xf86loadfontblock function.
		   */
		  WaitQueue(8);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
		  S3_OUTW(RD_MASK, 0xFFFF);
#ifdef S3_32BPP
		  if (s3InfoRec.bitsPerPixel == 32)
		     S3_OUTW(RD_MASK, 0xFFFF);
#endif

		  xf86loadFontBlock(fentry, blocki);
		  block = fentry->fblock[blocki];		  

		  /*
		   * Restore the GE context.
		   */
		  WaitQueue(4);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | (short)pBox->x1);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | (short)pBox->y1);
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (short)(pBox->x2 - 1));
		  S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | (short)(pBox->y2 - 1));
		  WaitQueue(5);
		  S3_OUTW(FRGD_COLOR, (short)pGC->fgPixel);
#ifdef S3_32BPP
		  if (s3InfoRec.bitsPerPixel == 32)
		     S3_OUTW(FRGD_COLOR, (short)(pGC->fgPixel)>>16));
#endif
		  S3_OUTW(MULTIFUNC_CNTL, 
			PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
		  S3_OUTW(FRGD_MIX, FSS_FRGDCOL | s3alu[pGC->alu]);
		  S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
		  S3_OUTW(WRT_MASK, pGC->planemask);
#ifdef S3_32BPP
		  if (s3InfoRec.bitsPerPixel == 32)
		     S3_OUTW(WRT_MASK, (short)(pGC->planemask>>16));
#endif
		  height = width = pmsk = 0;
	       }
	       WaitQueue(2);
	       S3_OUTW(CUR_Y, block->y);	       

	       /*
		* Is thre readmask altered
		*/
	       if (!pmsk || pmsk != (1 << block->id)) {
		  pmsk = 1 << block->id;
	       	  S3_OUTW(RD_MASK, (unsigned short)pmsk);		  	       
#ifdef S3_32BPP
		  if (s3InfoRec.bitsPerPixel == 32)
		     S3_OUTW(RD_MASK, (unsigned short)(pmsk>>16));
#endif
	       }
	       xoff = block->x;
	       block->lru = NEXT_FONT_AGE;
   	    }

	    WaitQueue(6);

	    S3_OUTW(CUR_X, (short) (xoff + (*chars & 0x1f) * w));
	    S3_OUTW(DESTX_DIASTP,
		  (short)(x + pci->metrics.leftSideBearing));
	    S3_OUTW(DESTY_AXSTP, (short)(y - pci->metrics.ascent));
	    if (!width || (short)(GLYPHWIDTHPIXELS(pci) - 1) != width) {
	       width = (short)(GLYPHWIDTHPIXELS(pci) - 1);
	       S3_OUTW(MAJ_AXIS_PCNT, width);
	    }
	    if (!height || (short)(gHeight - 1) != height) {
	       height = (short)(gHeight - 1);
	       S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | height);
	    }
	    S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
	 }
	 x += pci->metrics.characterWidth;
      }
   }
   UNBLOCK_CURSOR;

   return;
}

/*
 * Set the hardware scissors to match the clipping rectables and 
 * call the glyph output routine.
 */
void 
s3GlyphWrite(x, y, count, chars, fentry, pGC, pBox, numRects)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
     BoxPtr pBox;
{
   BLOCK_CURSOR;
   WaitQueue(5);
   S3_OUTW(FRGD_COLOR, (short)pGC->fgPixel);
#ifdef S3_32BPP
   if (s3InfoRec.bitsPerPixel == 32)
      S3_OUTW(FRGD_COLOR, (short)(pGC->fgPixel)>>16));
#endif        
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | s3alu[pGC->alu]);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
   S3_OUTW(WRT_MASK, pGC->planemask);
#ifdef S3_32BPP 
   if (s3InfoRec.bitsPerPixel == 32)
      S3_OUTW(WRT_MASK, (short)(pGC->planemask>>16));
#endif

   for (; --numRects >= 0; ++pBox) {
      WaitQueue(4);
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | (short)pBox->x1);
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | (short)pBox->y1);
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (short)(pBox->x2 - 1));
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | (short)(pBox->y2 - 1));

      Dos3CPolyText8(x, y, count, chars, fentry, pGC, pBox);
   }

   WaitQueue(8);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
   S3_OUTW(RD_MASK, 0xFFFF);
#ifdef S3_32BPP
   if (s3InfoRec.bitsPerPixel == 32)
      S3_OUTW(RD_MASK, 0xFFFF);
#endif
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;

   return;
}
