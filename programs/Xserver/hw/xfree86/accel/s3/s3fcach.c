/* $XConsortium: s3fcach.c,v 1.1 94/03/28 21:17:12 dpw Exp $ */
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
#include        "s3bcach.h"

#define   FC_MAX_WIDTH  24
extern int   s3FC_MAX_HEIGHT;

extern CacheFont8Ptr s3HeadFont;
static unsigned long s3FontAge;
#define NEXT_FONT_AGE  ++s3FontAge

static void s3loadFontBlock();
static __inline__ void Dos3CPolyText8();
extern CacheFont8Ptr s3FontCache;

void
s3UnCacheFont8(font)
     FontPtr font;
{
   int   i;
   CacheFont8Ptr ptr, last;

   ERROR_F(("UnCach %d\n", font));
   last = s3HeadFont;
   for (ptr = s3HeadFont; ptr != NULL; ptr = ptr->next) {      
      if (ptr->font == font) {
	 for (i = 0; i < 8; i++) 
	    if (ptr->fblock[i] != NULL) {
	        s3CReturnBlock(ptr->fblock[i]);
	    }

	    if (ptr != s3HeadFont) {
	       last->next = ptr->next;
	       Xfree(ptr);
	    } else {
	       if (ptr->next != NULL) { /* move the head down */
		  s3HeadFont=ptr->next;
		  Xfree(ptr);		  
	       } else { /* one and only entry */
		  s3HeadFont->font = NULL;
	       }
	    }
#ifdef DEBUG_FCACHE
            for (ptr = s3HeadFont; ptr != NULL; ptr = ptr->next)
	       ErrorF("fonts %d\n", ptr->font);
#endif	       
      	    return;	 
      }
      last=ptr;
   }
}

CacheFont8Ptr
s3CacheFont8(font)
     FontPtr font;
{
   int   c;
   unsigned long n;
   unsigned char chr;
   int   width, height;
   CharInfoPtr pci;

   CacheFont8Ptr last, ret = s3HeadFont;

   while (ret != NULL) {
      if (ret->font == font)
	 return ret;
      last = ret;	 
      ret = ret->next;
   } 

   width = FONTMAXBOUNDS(font, rightSideBearing) -
      FONTMINBOUNDS(font, leftSideBearing);
   height = FONTMAXBOUNDS(font, ascent) + FONTMAXBOUNDS(font, descent);

   if ((width > FC_MAX_WIDTH) || (height > s3FC_MAX_HEIGHT) ||
       (FONTFIRSTROW(font) != 0) || (FONTLASTROW(font) != 0) ||
       (FONTLASTCOL(font) > 255))
       return NULL;

       if (s3HeadFont->font == NULL)
	 ret = s3HeadFont;
       else
	 ret = (CacheFont8Ptr) Xcalloc(sizeof(CacheFont8Rec));

      if (ret == NULL)
	 return NULL;
      ret->w = width;
      ret->h = height;
      ret->font = font;

    /*
     * We load all the font infos now, and the fonts themselves are demand
     * loaded into the cache as 32 font bitmaps. This way we can load alot
     * more things into cache at the expense of the cache management.
     */

      for (c = 0; c < 256; c++) {
	 chr = (unsigned char)c;
	 GetGlyphs(font, 1, &chr, Linear8Bit, &n, &pci);
	 if (n == 0) {
	    ret->pci[c] = NULL;
	 } else {
	    ret->pci[c] = pci;
	 }
      }
      if (ret != s3HeadFont)
	 last->next = ret;

      return ret;
}

static void
s3loadFontBlock(fentry, block)
     CacheFont8Ptr fentry;
     int   block;
{
   int   i, j, c;
   unsigned char chr;
   int   nbyLine;
   unsigned char *pb, *pbits;
   unsigned char *pglyph;
   int   gWidth, gHeight;
   int   nbyGlyphWidth;
   int   nbyPadGlyph;

   nbyLine = PixmapBytePad(fentry->w, 1);
   ERROR_F(("loading %d (%d) %d\n", fentry->font, block, fentry->fblock[block]));

   pbits = (unsigned char *)ALLOCATE_LOCAL(nbyLine * fentry->h);
   if (pbits != NULL &&
    (fentry->fblock[block] = s3CGetBlock(32 * fentry->w, fentry->h)) != NULL) {

      fentry->fblock[block]->reference = (pointer *) &(fentry->fblock[block]); 

      for (c = block * 32; c < (block * 32) + 32; c++) {

	 if (fentry->pci[c] != NULL) {
	    chr = (unsigned char)c;

	    pglyph = FONTGLYPHBITS(pglyphBase, fentry->pci[c]);
	    gWidth = GLYPHWIDTHPIXELS(fentry->pci[c]);
	    gHeight = GLYPHHEIGHTPIXELS(fentry->pci[c]);
	    if (gWidth && gHeight) {
	       nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(fentry->pci[c]);
	       nbyPadGlyph = PixmapBytePad(gWidth, 1);

	       if (nbyGlyphWidth == nbyPadGlyph
#if GLYPHPADBYTES != 4
		   && (((int)pglyph) & 3) == 0
#endif
		  )
		  pb = pglyph;
	       else {
		  for (i = 0, pb = pbits;
		       i < gHeight;
		       i++, pb = pbits + (i * nbyPadGlyph))
		     for (j = 0; j < nbyGlyphWidth; j++)
			*pb++ = *pglyph++;
		  pb = pbits;
	       }

	       s3ImageOpStipple(fentry->fblock[block]->x +
				(c % 32) * fentry->w,
				fentry->fblock[block]->y,
				gWidth, gHeight,
				(char *)pb, nbyGlyphWidth, gWidth, gHeight,
				fentry->fblock[block]->x +
				(c % 32) * fentry->w,
				fentry->fblock[block]->y,
				0xff, 0, s3alu[GXcopy],
				fentry->fblock[block]->daddy->mask);

	    }
	 }
      }
      DEALLOCATE_LOCAL(pbits);
   } else {
      CacheFont8Ptr fptr;
      Bool found = FALSE;
   /*
    * If we get here we are in deep trouble, half way through printing a
    * string we have been unable to load a font block into the cache, the
    * get Block function found no block of the right size, this is probably
    * impossible but just to stop potential core dumps we shall do something
    * stupid about it anyway we just throw away the font blocks of another
    * font. Or even ourselves in desperate times!
    * Unfortunatly this doesn't work if we use the preload code so the
    * demand load makes more sense.
    */
      ERROR_F(("Time to write new font cache management\n"));

      for (fptr = s3HeadFont; fptr == NULL; fptr= fptr->next)
	 if (fptr != fentry) {
	    for (i = 0; i < 8; i++)
	       if (fptr->fblock[i] != NULL) {
	         s3CReturnBlock(fptr->fblock[i]);
		 found = TRUE;
	       }
	    if (found)
	       break;
	 }

      /* getting real desperate - this doesn't work with pre-loading */
      if (!found) { 
	 for (i = 0; i < 8; i++)
	    if (fentry->fblock[i] != NULL) 
	       s3CReturnBlock(fentry->fblock[i]);	    
      }
      s3loadFontBlock(fentry, block);
      return;
   }
   for (i = 0; i < 8; i++)
   ERROR_F(("got %d (%d) %d\n", fentry->font, i, fentry->fblock[i]));
}

int
s3CPolyText8(pDraw, pGC, x, y, count, chars, fentry)
     DrawablePtr pDraw;
     GCPtr pGC;
     int   x;
     int   y;
     int   count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
{
   int   i;
   BoxPtr pBox;
   int   numRects;
   RegionPtr pRegion;
   int   yBand;
   int   maxAscent, maxDescent;
   int   minLeftBearing;
   FontPtr pfont = pGC->font;
   int ret_x;

#if 1
   {
   char  toload[8];

   for (i = 0; i < 8; i++)
      toload[i] = 0;

   /*
    * If miPolyText8() is to be believed, the returned new X value is
    * completely independent of what happens during rendering.
    */
   ret_x = x;
   for (i = 0; i < count; i++) {
      toload[chars[i] / 32] = 1;
      ret_x += fentry->pci[(int)chars[i]] ? 
	       fentry->pci[(int)chars[i]]->metrics.characterWidth : 0; 
   }

   for (i = 0; i < 8; i++)
      if (toload[i]) {
	 if ((fentry->fblock[i]) == NULL) {
	    s3loadFontBlock(fentry, i);
	 }
	 fentry->fblock[i]->lru++;
      }
   }
#else
   /*
    * If miPolyText8() is to be believed, the returned new X value is
    * completely independent of what happens during rendering.
    */
   ret_x = x;
   for (i = 0; i < count; i++) {
      ret_x += fentry->pci[(int)chars[i]] ? 
	       fentry->pci[(int)chars[i]]->metrics.characterWidth : 0; 
   }
#endif

   x += pDraw->x;
   y += pDraw->y;
   maxAscent = FONTMAXBOUNDS(pfont, ascent);
   maxDescent = FONTMAXBOUNDS(pfont, descent);
   minLeftBearing = FONTMINBOUNDS(pfont, leftSideBearing);
   pRegion = ((cfbPrivGC *) (pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip;

   pBox = REGION_RECTS(pRegion);
   numRects = REGION_NUM_RECTS(pRegion);
   while (numRects && pBox->y2 <= y - maxAscent) {
      ++pBox;
      --numRects;
   }
   if (!numRects || pBox->y1 >= y + maxDescent)
      return ret_x;
   yBand = pBox->y1;
   while (numRects && pBox->y1 == yBand && pBox->x2 <= x + minLeftBearing) {
      ++pBox;
      --numRects;
   }
   if (!numRects)
      return ret_x;

   BLOCK_CURSOR;
   WaitQueue(5);
   S3_OUTW(FRGD_COLOR, (short)pGC->fgPixel);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | s3alu[pGC->alu]);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
   S3_OUTW(WRT_MASK, (short)pGC->planemask);

   for (; --numRects >= 0; ++pBox) {
      WaitQueue(4);
      S3_OUTW (MULTIFUNC_CNTL, SCISSORS_L | (short)pBox->x1);
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | (short)pBox->y1);
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (short)(pBox->x2 - 1));
      S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | (short)(pBox->y2 - 1));

      Dos3CPolyText8(x, y, count, chars, fentry, pGC);
   }

   WaitQueue(8);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
   S3_OUTW(RD_MASK, 0xff);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;

   return ret_x;
}

int
s3CImageText8(pDraw, pGC, x, y, count, chars, fentry)
     DrawablePtr pDraw;
     GCPtr pGC;
     int   x;
     int   y;
     int   count;
     char *chars;
     CacheFont8Ptr fentry;
{
   ExtentInfoRec info;		/* used by QueryGlyphExtents() */
   XID   gcvals[3];
   int   oldAlu, oldFS;
   unsigned long oldFG;
   xRectangle backrect;
   CharInfoPtr *ppci;
   unsigned long n;

   if (!(ppci = (CharInfoPtr *) ALLOCATE_LOCAL(count * sizeof(CharInfoPtr))))
      return 0;

   GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)chars,
	     Linear8Bit, &n, ppci);

   QueryGlyphExtents(pGC->font, ppci, n, &info);

   DEALLOCATE_LOCAL(ppci);

   if (info.overallWidth >= 0) {
      backrect.x = x;
      backrect.width = info.overallWidth;
   } else {
      backrect.x = x + info.overallWidth;
      backrect.width = -info.overallWidth;
   }
   backrect.y = y - FONTASCENT(pGC->font);
   backrect.height = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);

   oldAlu = pGC->alu;
   oldFG = pGC->fgPixel;
   oldFS = pGC->fillStyle;

 /* fill in the background */
   gcvals[0] = GXcopy;
   gcvals[1] = pGC->bgPixel;
   gcvals[2] = FillSolid;
   DoChangeGC(pGC, GCFunction | GCForeground | GCFillStyle, gcvals, 0);
   ValidateGC(pDraw, pGC);
   (*pGC->ops->PolyFillRect) (pDraw, pGC, 1, &backrect);

 /* put down the glyphs */
   gcvals[0] = oldFG;
   DoChangeGC(pGC, GCForeground, gcvals, 0);
   ValidateGC(pDraw, pGC);
   (void)s3CPolyText8(pDraw, pGC, x, y, count, (unsigned char *)chars, fentry);

 /* put all the toys away when done playing */
   gcvals[0] = oldAlu;
   gcvals[1] = oldFG;
   gcvals[2] = oldFS;
   DoChangeGC(pGC, GCFunction | GCForeground | GCFillStyle, gcvals, 0);
   return 0;
}

static __inline__ void
Dos3CPolyText8(x, y, count, chars, fentry, pGC)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
{
   int   gHeight;
   int   w = fentry->w;
   int blocki = 255;

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
#if 1	       
	       if (block == NULL) {
		  s3loadFontBlock(fentry, blocki);
		  block = fentry->fblock[blocki];		  
		  WaitQueue(5);
		  S3_OUTW(FRGD_COLOR, (short)pGC->fgPixel);
		  S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
		  S3_OUTW(FRGD_MIX, FSS_FRGDCOL | s3alu[pGC->alu]);
		  S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
		  S3_OUTW(WRT_MASK, (short)pGC->planemask);
	       }
#endif	       
	       WaitQueue(2);
	       S3_OUTW(CUR_Y, block->y);	       
	       S3_OUTW(RD_MASK, (short)block->mask);		  	       
	       xoff = block->x;
	       block->lru = NEXT_FONT_AGE;
   	    }

	    WaitQueue(6);

	    S3_OUTW(CUR_X, (short) (xoff + (*chars & 0x1f) * w));
	    S3_OUTW(DESTX_DIASTP,
		  (short)(x + pci->metrics.leftSideBearing));
	    S3_OUTW(DESTY_AXSTP, (short)(y - pci->metrics.ascent));
	    S3_OUTW(MAJ_AXIS_PCNT, (short)(GLYPHWIDTHPIXELS(pci) - 1));
	    S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT |
		  (short)(gHeight - 1));

	    S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y |
		  DRAW | PLANAR | WRTDATA);

	 }
	 x += pci->metrics.characterWidth;
      }
   }

   return;
}
