/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/agx/agxFCach.c,v 3.5 1994/08/12 13:56:36 dawes Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
 * Copyright 1994 by Henry A. Worth, Sunnyvale, California.
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
 * KEVIN E. MARTIN AND HENRY A. WORTH DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, 
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING 
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Modified by Amancio Hasty and Jon Tombs
 * Rewritten for the AGX by Henry A. Worth (haw30@eng.amdahl.com
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
#include	"agx.h"
#include	"regagx.h"
#include        "agxBCach.h"

extern CacheFont8Ptr agxHeadFont;
static unsigned long agxFontAge;
#define NEXT_FONT_AGE  ++agxFontAge

static void agxloadFontBlock();
static __inline__ void DoagxCPolyText8();
extern CacheFont8Ptr agxFontCache;

Bool geBlockMove = FALSE;

void
agxUnCacheFont8(font)
     FontPtr font;
{
   int   i;
   CacheFont8Ptr ptr, last;

   ERROR_F(("UnCach 0x%x\n", font));
   last = agxHeadFont;
   for (ptr = agxHeadFont; ptr != NULL; ptr = ptr->next) {      
      if (ptr->font == font) {
	 for (i = 0; i < BLOCKS_PER_FONT; i++) 
	    if (ptr->fblock[i] != NULL) {
	        agxCReturnBlock(ptr->fblock[i]);
	    }

	 if (ptr != agxHeadFont) {
	    last->next = ptr->next;
	    Xfree(ptr);
	 }
         else {
	    if (ptr->next != NULL) { /* move the head down */
	       agxHeadFont=ptr->next;
	       Xfree(ptr);		  
	    }
            else { /* one and only entry */
	       agxHeadFont->font = NULL;
	    }
         }
#ifdef DEBUG_FCACHE
         for (ptr = agxHeadFont; ptr != NULL; ptr = ptr->next)
	    ErrorF("fonts 0x%x\n", ptr->font);
#endif	       
      	 return;	 
      }
      last=ptr;
   }
}

CacheFont8Ptr
agxCacheFont8(font)
     FontPtr font;
{
   int   c;
   unsigned long n;
   unsigned char chr;
   int   width, height;
   int   bWidth;
   int   blockSize;
   int   gper;
   CharInfoPtr pci;

   CacheFont8Ptr last, ret = agxHeadFont;

   while (ret != NULL) {
      if (ret->font == font)
	 return ret;
      last = ret;	 
      ret = ret->next;
   } 

   width = FONTMAXBOUNDS(font, rightSideBearing) 
            - FONTMINBOUNDS(font, leftSideBearing);
   height = FONTMAXBOUNDS(font, ascent) 
             + FONTMAXBOUNDS(font, descent);

   bWidth = PixmapBytePad(width,1); 
   gper = CACHE_LINE_WIDTH_BYTES / bWidth;
   blockSize =  (((BLOCK_NUM_CHAR - 1) / gper) + 1) * height;

   if ( (width > CACHE_LINE_WIDTH_PIXELS) 
        || (FONTFIRSTROW(font) != 0) 
        || (FONTLASTROW(font) != 0)
        || (FONTLASTCOL(font) > 255)  
        || blockSize > ROW_NUM_LINES )
       return NULL;

   if (agxHeadFont->font == NULL)
       ret = agxHeadFont;
   else
       ret = (CacheFont8Ptr) Xcalloc(sizeof(CacheFont8Rec));
   if (ret == NULL)
      return NULL;

   ret->wPix = width;
   ret->hPix = height;
   ret->wBytes = bWidth;
   ret->gper = gper;
   ret->blockSize = blockSize;
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
      }
      else {
	 ret->pci[c] = pci;
      }
   }
   if (ret != agxHeadFont)
      last->next = ret;

   return ret;
}

static void
agxloadFontBlock(fentry, block)
     CacheFont8Ptr fentry;
     int   block;
{
   unsigned int   i, j, c;
   unsigned char  chr;
   unsigned int   nbyWidth;
   unsigned char *pb, *pbits;
   unsigned char *pglyph;
   unsigned int   gWidth, gHeight, gSize;
   unsigned int   nbyGlyphWidth;
   unsigned int   nbyPadGlyph;

   ERROR_F(("loading 0x%x (0x%x) 0x%x\n", 
             fentry->font, block, fentry->fblock[block]));


   nbyWidth = fentry->wBytes;  		/* glyph width in bytes */
   gSize = nbyWidth * fentry->hPix;  	 /*  font cache glyph size in bytes */

   pbits = (unsigned char *)ALLOCATE_LOCAL(gSize);   /* buffer for copy */

   /* 
    * We have to worry about modifing a block that is
    * currently being used by the graphics engine.
    * For now, we assume the worse, in the long-run
    * it may be useful to keep track of the in-use
    * block and only wait when actually needed.
    */
   GE_WAIT_IDLE_SHORT();

   if ( pbits != NULL 
        && (fentry->fblock[block]
             = agxCGetBlock(fentry->blockSize)) != NULL ) {

      unsigned int first = block << BLOCK_NUM_SHIFT;  /* first char in block */
      unsigned int last  = first + BLOCK_NUM_CHAR;    /* last+1 char in block */

      fentry->fblock[block]->reference = (pointer *) &(fentry->fblock[block]); 

      /* for each character in the block */
      for ( c = first; c < last; c++ ) {

	 if (fentry->pci[c] != NULL) {

	    pglyph = FONTGLYPHBITS(pglyphBase, fentry->pci[c]);
	    gWidth = GLYPHWIDTHPIXELS(fentry->pci[c]);
	    gHeight = GLYPHHEIGHTPIXELS(fentry->pci[c]);
	    if (gWidth && gHeight) {
	       nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(fentry->pci[c]);
               nbyPadGlyph = PixmapBytePad(gWidth, 1);

	       if ( nbyWidth == nbyPadGlyph
#if GLYPHPADBYTES != 4
		    && (((int)pglyph) & 3) == 0
#endif
		  ) {
                  /*
                   * glyph is in acceptable format, no need to copy 
                   */
		  pb = pglyph;
               }
	       else
               {
                  unsigned char *pg;
                  /*
                   * copy glyph into pbits, padded to font w 
                   * This could be optimized to use word and dword moves.
                   * Also should optimize to build a list of pointers 
                   * to the glyphs and use those to eliminate the 
                   * buffer and allow benchmarking different load
                   * sequences.
                   */
		  pb = pbits;
		  for ( i = 0; i < gHeight; i++ ) {
                     pb = pbits + (i * nbyWidth);
                     pg = pglyph + (i * nbyGlyphWidth);
		     for (j = 0; 
                          j < nbyGlyphWidth && j < nbyWidth;
                          j++)
			*pb++ = *pg++;
                     while ( j++ < nbyWidth )
                        *pb++ = 0x00;
                  }
		  pb = pbits;
	       }
               /* 
                * Copy into appropriate position in the cache block.
                * Note: unused glyph rows in the glyph will be 
                *       garbage in the cache. 
                */
               {  
                  unsigned int  vidBase;
                  unsigned int  offset;
                  unsigned int  idx;
                  unsigned int  linePos;
                  unsigned int  lineOff;
                  unsigned char bank;
                  pointer  base;

                  idx     = (c & BLOCK_IDX_MASK); 
                  linePos = idx % fentry->gper;
                  lineOff = (idx / fentry->gper) * fentry->hPix; 
                  vidBase = fentry->fblock[block]->daddy->offset 
                            + ( (fentry->fblock[block]->line + lineOff)
                                * CACHE_LINE_WIDTH_BYTES )
                            + ( linePos * nbyWidth );
                  
                  bank = (vidBase >> 16) & 0x3F ;
                  base = (pointer)((unsigned char *)agxVideoMem +
				   (vidBase & 0xFFFF));

                  /* cache is laid out so that blocks don't cross banks */ 
                  outb( agxApIdxReg, bank );
                  for( i=0, offset = 0; i < gHeight; i++ ) {
                     MemToBus( (void *)((unsigned char *)base+offset), 
                               pb,
			       nbyWidth );
                     offset += CACHE_LINE_WIDTH_BYTES;
                     pb+=nbyWidth;
                  }
               }
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

      if (pbits) DEALLOCATE_LOCAL(pbits);

      for (fptr = agxHeadFont; fptr == NULL; fptr= fptr->next)
	 if (fptr != fentry) {
	    for (i = 0; i < BLOCKS_PER_FONT; i++)
	       if (fptr->fblock[i] != NULL) {
	         agxCReturnBlock(fptr->fblock[i]);
		 found = TRUE;
	       }
	    if (found)
	       break;
	 }

      /* getting real desperate - this doesn't work with pre-loading */
      if (!found) { 
         ERROR_F(("Flushing Current Font!\n"));
	 for (i = 0; i < BLOCKS_PER_FONT; i++)
	    if (fentry->fblock[i] != NULL) 
	       agxCReturnBlock(fentry->fblock[i]);	    
      }
      agxloadFontBlock(fentry, block);
      return;
   }
   for (i = 0; i < BLOCKS_PER_FONT; i++)
      ERROR_F(("got 0x%x(0x%x) 0x%x\n", fentry->font, i, fentry->fblock[i]));
}

int
agxCPolyText8(pDraw, pGC, x, y, count, chars, fentry)
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
   char  toload[BLOCKS_PER_FONT];

   for (i = 0; i < BLOCKS_PER_FONT; i++)
      toload[i] = 0;

   /*
    * If miPolyText8() is to be believed, the returned new X value is
    * completely independent of what happens during rendering.
    */

   /*
    * Too maximize concurrency we shouldn't preload for the AGX, 
    * which won't use the GE to load fonts. I'll remove this once
    * I get this stuff going and checks are added to prevent
    * modification of in-use blocks. Pre-loading also aggravates
    * thrashing with a small cache.
    */
   ret_x = x;
   for (i = 0; i < count; i++) {
      toload[chars[i] & FONT_BLOCK_MASK] = 1;
      ret_x += fentry->pci[(int)chars[i]] ? 
	          fentry->pci[(int)chars[i]]->metrics.characterWidth : 0; 
   }

   for (i = 0; i < BLOCKS_PER_FONT; i++) {
      if (toload[i]) {
#if 0
	 if ((fentry->fblock[i]) == NULL) {
	    agxloadFontBlock(fentry, i);
	 }
#else
	 if ((fentry->fblock[i]) != NULL) 
#endif
	 fentry->fblock[i]->lru++;
      }
   }

   x += pDraw->x;
   y += pDraw->y;
   maxAscent = FONTMAXBOUNDS(pfont, ascent);
   maxDescent = FONTMAXBOUNDS(pfont, descent);
   minLeftBearing = FONTMINBOUNDS(pfont, leftSideBearing);
   pRegion = ((cfbPrivGC *) 
                 (pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip;

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

   for (; --numRects >= 0; ++pBox) {
      /* mask off clipped areas of the destination */
      GE_WAIT_IDLE();
      GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MASK_MAP );
      GE_OUT_D( GE_MASK_MAP_X, (long) pBox->x1 );
      GE_OUT_D( GE_MASK_MAP_Y, (long) pBox->y1 );
#ifndef NO_MULTI_IO 
      GE_OUT_D( GE_PIXEL_MAP_WIDTH, (pBox->y2 - pBox->y1)-1 << 16
                                     | (pBox->x2 - pBox->x1)-1    );
#else
      GE_OUT_W( GE_PIXEL_MAP_WIDTH, (short) (pBox->x2 - pBox->x1) - 1 );
      GE_OUT_W( GE_PIXEL_MAP_HEIGHT, (short) (pBox->y2 - pBox->y1) - 1 );
#endif
      DoagxCPolyText8(x, y, count, chars, fentry, pGC);
   }

   return ret_x;
}

int
agxCImageText8(pDraw, pGC, x, y, count, chars, fentry)
     DrawablePtr pDraw;
     GCPtr pGC;
     int   x;
     int   y;
     int   count;
     unsigned char *chars;
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
   (void)agxCPolyText8(pDraw, pGC, x, y, count, chars, fentry);

 /* put all the toys away when done playing */
   gcvals[0] = oldAlu;
   gcvals[1] = oldFG;
   gcvals[2] = oldFS;
   DoChangeGC(pGC, GCFunction | GCForeground | GCFillStyle, gcvals, 0);
   return 0;
}

static __inline__ void
DoagxCPolyText8(x, y, count, chars, fentry, pGC)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
{
   CharInfoPtr pci;
   unsigned short h = fentry->hPix;
   unsigned short w = fentry->wBytes<<3;
   unsigned short xStart, yStart;
   unsigned int blocki = 255;
   bitMapBlockPtr block;
   unsigned short gHeight; 
   unsigned short gWidth; 
   unsigned short idx;  
   unsigned short line;  
   unsigned short linePos;  
   unsigned int   blockBase;
   unsigned int   oldBlockBase = 0;

   GE_WAIT_IDLE();
   MAP_SET_DST( GE_MS_MAP_A );
#ifndef NO_MULTI_IO
   GE_OUT_W(GE_FRGD_MIX, MIX_DST << 8 | pGC->alu );
#else
   GE_OUT_B(GE_FRGD_MIX, pGC->alu );
   GE_OUT_B(GE_BKGD_MIX, MIX_DST );
#endif
   GE_OUT_D(GE_PIXEL_BIT_MASK, pGC->planemask);
   GE_OUT_D(GE_FRGD_CLR, pGC->fgPixel);

   for (;count > 0; count--, chars++) {

      pci = fentry->pci[(int)*chars];

      if (pci != NULL) {

	 gHeight = GLYPHHEIGHTPIXELS(pci);
	 if (gHeight) {

	    if ((int) (*chars >> BLOCK_NUM_SHIFT) != blocki) {
	       
	       blocki = (int) (*chars >> BLOCK_NUM_SHIFT);
	       block = fentry->fblock[blocki];
	       if (block == NULL) {
                  geBlockMove = FALSE;
		  agxloadFontBlock(fentry, blocki);
		  block = fentry->fblock[blocki];
                  if( geBlockMove ) {
                     GE_WAIT_IDLE();
                     MAP_SET_DST( GE_MS_MAP_A );
#ifndef NO_MULTI_IO
                     GE_OUT_W(GE_FRGD_MIX, MIX_DST << 8 | pGC->alu );
#else
                     GE_OUT_B(GE_FRGD_MIX, pGC->alu );
                     GE_OUT_B(GE_BKGD_MIX, MIX_DST );
#endif
                     GE_OUT_D(GE_PIXEL_BIT_MASK, pGC->planemask);
                     GE_OUT_D(GE_FRGD_CLR, pGC->fgPixel);
                  }
	       }
               block->lru = NEXT_FONT_AGE;
               blockBase = agxMemBase + block->daddy->offset;
               if( blockBase != oldBlockBase ) {
                  GE_WAIT_IDLE_SHORT();
                  GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_C );
  
                  GE_OUT_D( GE_PIXEL_MAP_BASE, blockBase );
#ifndef NO_MULTI_IO 
                  GE_OUT_D( GE_PIXEL_MAP_WIDTH,  ROW_NUM_LINES-1 << 16
                                                 | CACHE_LINE_WIDTH_PIXELS-1 );
#else
                  GE_OUT_W( GE_PIXEL_MAP_WIDTH,  CACHE_LINE_WIDTH_PIXELS-1 );
                  GE_OUT_W( GE_PIXEL_MAP_HEIGHT, ROW_NUM_LINES-1 );
#endif
                  GE_OUT_B( GE_PIXEL_MAP_FORMAT, GE_MF_1BPP );
                  oldBlockBase = blockBase;
               }
   	    }
            idx = (*chars) & BLOCK_IDX_MASK;
            line = block->line + ((idx / fentry->gper) * h);
            linePos = w * (idx % fentry->gper);
            xStart = x + pci->metrics.leftSideBearing;
            yStart = y - pci->metrics.ascent;
            gHeight--;
            gWidth = GLYPHWIDTHPIXELS(pci) - 1;

            GE_WAIT_IDLE_SHORT();

#ifndef NO_MULTI_IO
            GE_OUT_D( GE_DEST_MAP_X, yStart << 16 | xStart );
            GE_OUT_D( GE_PAT_MAP_X,  line << 16 | linePos ); 
            GE_OUT_D( GE_OP_DIM_WIDTH,  gHeight << 16 | gWidth );
#else
            GE_OUT_W( GE_DEST_MAP_X, xStart );
            GE_OUT_W( GE_DEST_MAP_Y, yStart );
            GE_OUT_W( GE_PAT_MAP_X,  linePos ); 
            GE_OUT_W( GE_PAT_MAP_Y,  line );
            GE_OUT_W( GE_OP_DIM_WIDTH,  gWidth );
            GE_OUT_W( GE_OP_DIM_HEIGHT, gHeight ); 
#endif
            GE_START_CMD( GE_OP_BITBLT
                          | GE_OP_FRGD_SRC_CLR 
                          | GE_OP_DEST_MAP_A
                          | GE_OP_PAT_MAP_C
                          | GE_OP_MASK_BOUNDARY
                          | GE_OP_INC_X
                          | GE_OP_INC_Y );
	 }
         x += pci->metrics.characterWidth;
      }
   }

   GE_WAIT_IDLE_EXIT();
   return;
}
