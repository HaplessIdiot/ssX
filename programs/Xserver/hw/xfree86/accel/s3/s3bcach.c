/* $XConsortium: s3bcach.c,v 1.1 94/03/28 21:14:19 dpw Exp $ */
/* $XFree86$ */
/*
 * Copyright 1993 by Jon Tombs. Oxford University
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Jon Tombs or Oxford University shall
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. The authors  make no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * JON TOMBS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 */

/*
 * Id: s3bcach.c,v 2.3 1993/07/24 13:16:56 jon Exp
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
#define XCONFIG_FLAGS_ONLY
#include        "xf86_Config.h"



static bitMapRowPtr headBitRow;  /* top of linked list of cache rows */
extern CacheFont8Ptr s3HeadFont; /* top of linked list of fonts in cache */
extern int s3FC_MAX_HEIGHT;
extern Bool xf86Verbose;

#ifdef DEBUG_FCACHE
static void showcache();
#endif

#define PIXMAP_WIDTH 64

/*
 * TERMS:
 * A bitmap row is a rectangular section of the offscreen memory, which may
 * contain one or more font blocks.
 *
 * A font block is the bitmap images of 32 characters from a font. Characters
 * are spaced regularly with spacing equal to the width of the widest character
 * in the font.
 */

void
s3BitCache8Init(w, y)
     int   w, y;
{
   int   i;
   bitMapRowPtr bptr;
   static Bool reEntry = FALSE;
   int s3CacheLen;
   
   if (reEntry) {
      CacheFont8Ptr CFptr;
/*
 * The screen has been destroyed so we must go through all cached font blocks
 * and return them to the cache
 */
      for (CFptr = s3HeadFont; CFptr != NULL; CFptr=CFptr->next) {
	 if (CFptr->font) {
	    for (i = 0; i < 8; i++) {
	       if (CFptr->fblock[i] != NULL)
		  s3CReturnBlock(CFptr->fblock[i]);
	    }
	 }
      }
      return;
   }

   /* y now includes the cursor space */
   s3FC_MAX_HEIGHT = s3ScissB + 1 - y;
   

   /* Currently no pixmap cache when s3DisplayWidth < 1024 */

   if ((s3FC_MAX_HEIGHT < PIXMAP_WIDTH) || 
       (s3DisplayWidth < 1024)) { /* no pixmap cache */
      s3CacheLen = s3DisplayWidth;
      ErrorF("%s %s: No pixmap expanding area available\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   } else {
      s3CacheLen = 768;
      if (!reEntry) {
	 s3InitFrect(768, y, PIXMAP_WIDTH);
      }
      ErrorF("%s %s: Using a single %dx%d area for expanding pixmaps\n",
	     XCONFIG_PROBED, s3InfoRec.name, PIXMAP_WIDTH, PIXMAP_WIDTH);
   }
      
   reEntry = TRUE;

   if (s3FC_MAX_HEIGHT >= 8) {
      s3HeadFont = (CacheFont8Ptr) Xcalloc(sizeof(CacheFont8Rec));
      headBitRow = (bitMapRowPtr) Xcalloc(sizeof(bitMapRowRec));
      bptr = headBitRow;

      /* There are 8 initial cache rows, one per bitmap plane. All of have
       * total height of the off screen bitmap minus 1 for the cursor
       */
      for (i = 0; i < 8; i++) {	
	 bptr->h = s3FC_MAX_HEIGHT;
	 bptr->freew = s3CacheLen;	/* rest for pixmaps */
	 bptr->y = y;
	 bptr->mask = 1 << i;
	 bptr->next = (bitMapRowPtr) Xcalloc(sizeof(bitMapRowRec));
	 bptr = bptr->next;
      }
   } else
      s3FC_MAX_HEIGHT = 0; /* give up the idea of a cache */

   if (xf86Verbose) {
      if (s3FC_MAX_HEIGHT) {
	 ErrorF("%s %s: Using 8 pages of %dx%d for font caching\n",
	   XCONFIG_PROBED, s3InfoRec.name, s3CacheLen, s3FC_MAX_HEIGHT);
      } else {
	 ErrorF("%s %s: Font caching is disabled due to lack of video ram\n",
           XCONFIG_PROBED, s3InfoRec.name);
      }
   }
   /* make sure one font can't fill the cache */
   s3FC_MAX_HEIGHT /= 2; 
}

/*
 * Go through the linked list of rows looking for a row with height big
 * enough for the font block to be cached.
 * If the height is OK then we check that the free length is also big enough
 * and if so add the block to a linked list in blocks in that row.
 *
 * We also look for any space that is currently in use that would have been
 * big enough. If none of the rows have room, one of these is removed from
 * the cache (subject to its lru value), and the function recurses, knowing
 * this time it will meet success.
 * If we didn't even find a block in use big enough we return NULL and the
 * font code must throw out some other blocks to make room.
 */
bitMapBlockPtr
s3CGetBlock(w, h)
     int   w, h;
{
   bitMapRowPtr bptr = headBitRow;
   bitMapBlockPtr canidate = NULL;
   int oldest=0;   
   
   do {
      if (bptr->h >= h) {
	 if (bptr->blocks == NULL) { /* first use of this row, we know it
				      * must be wide enough as the font
				      * code has already checked
				      */
	    bptr->blocks = (bitMapBlockPtr) Xcalloc(sizeof(bitMapBlockRec));
	    bptr->blocks->x = 0;
	    bptr->blocks->y = bptr->y;
	    bptr->blocks->h = h;
	    bptr->blocks->w = w;
	    bptr->blocks->daddy = bptr; /* so we can trace a block back to its
	                                 * parent row.  
		                         */
	    if (bptr->h > h) {	/* split the row. We create a new row with
	                         * the unused height of the first.
				 */
	       bitMapRowPtr nbptr = (bitMapRowPtr) Xcalloc(sizeof(bitMapRowRec));

	       nbptr->h = bptr->h - h;
	       nbptr->freew = bptr->freew;
	       nbptr->y = bptr->y + h;
	       nbptr->mask = bptr->mask;
	       nbptr->next = bptr->next;
	       nbptr->next->prev = nbptr;
	       bptr->next = nbptr;
	       nbptr->prev = bptr;
	       bptr->h = h;
	    }
	    bptr->freew -= w;   /* reduce the free space of this row */
	    bptr->blocks->mask = bptr->mask;
	    SHOWCACHE();
	    return bptr->blocks;
	 } else { /* This row already contains a block */
	    if (bptr->freew >= w) { /* is the space left big enough? */
	       bitMapBlockPtr bbptr = bptr->blocks;

	       while (bbptr->next != NULL) /* find the last block */
		  bbptr = bbptr->next; 

	       /* and add this block onto the end */
	       bbptr->next = (bitMapBlockPtr) Xcalloc(sizeof(bitMapBlockRec));
	       bbptr->next->x = bbptr->x + bbptr->w;
	       bbptr->next->y = bptr->y;
	       bbptr->next->h = h;
	       bbptr->next->w = w;
	       bbptr->next->daddy = bptr;
	       bbptr->next->mask = bptr->mask;
	       bptr->freew -= w;
	       SHOWCACHE();
	       return bbptr->next;
	    } else { /* see if any slot is big enough anyway */
	       bitMapBlockPtr bbptr = bptr->blocks;
	       while (bbptr->next != NULL)  {
		  if (bbptr->w > w && bbptr->lru > oldest) {
		     oldest = bbptr->lru;
		     canidate = bbptr; /* our prime candidate to remove
				        * If no space is found
					*/
		  }
		  bbptr = bbptr->next;
	       }
	    }
		     
	 }
      }
   } while ((bptr = bptr->next) != NULL);

   /* If we get here then there are no slots left
    * We throw out the least used block if we found one big enough.
    * else we return null and let the calling code do something about it.
    */

   ERROR_F(("forced font return\n"));
   if (canidate != NULL) {
      s3CReturnBlock(canidate);
      return s3CGetBlock(w, h);
   } else
      return NULL;	/* shouldn't happen unless you try very hard */
   
}

/*
 * Return a block to the parent row:
 * Several cases can arise.
 * - We are the last block of the linked list, in which case just add our
 *   length to the free length.
 * - We are the only block in a row. The row is now empty, so if another
 *   empty row exists below or above we merge.
 * - we a a block in the middle of the list of blocks. This is nasty.
 *   we shuffle the blocks that follow by shifting them to the left our length.
 *   This keeps the free space at the right hand end.
 */

void
s3CReturnBlock(block)
     bitMapBlockPtr block;
{
   bitMapBlockPtr line, tmpb;
   bitMapRowPtr bptr;

   bptr = block->daddy;
   line = bptr->blocks;

   ERROR_F(("free font: (%dx%d):\n", bptr->h, block->w));
   SHOWCACHE();
   bptr->freew += block->w;	/* this much we can be sure of */

   if (block->next == NULL) {	/* we are the last of a row */

      if (block == line) {	/* we are the row */
	 if ((bptr->prev != NULL) && (bptr->mask == bptr->prev->mask)
	     && (bptr->prev->blocks == NULL)) {
	  /* we are not first in plane so cut ourselves out upward */
	    bptr->prev->h += bptr->h;
	    bptr->prev->next = bptr->next;
	    if (bptr->next)  /* don't go of the end of the last row */
	       bptr->next->prev = bptr->prev;
	    Xfree(bptr);
	    ERROR_F(("returning row to above\n"));
	 } else if ((bptr->next != NULL) && (bptr->mask == bptr->next->mask)
		    && (bptr->next->blocks == NULL)) {
	  /* we are not last in plane and the row below is empty, so cut
	   * ourselves out merging with the row below.
	   */
	    bptr->next->h += bptr->h;
	    bptr->next->prev = bptr->prev;
	    bptr->next->y = bptr->y;
	    if (bptr->prev) { /* we are not the head row */
	       bptr->prev->next = bptr->next;
	    } else {  /* promote the row below to new head row */
	       headBitRow = bptr->next; 
	    }
	    Xfree(bptr);
       	    ERROR_F(("returning row to below\n"));
	 } else {  /* just zero our length */
	    ERROR_F(("left zero length line\n"));
	    bptr->blocks = NULL;
	 }
      } else {
	 /* simply loose the end of the line */
	 tmpb = line;
	 
	 while (tmpb->next != block)
	    tmpb = tmpb->next;

	 tmpb->next = NULL;
	 ERROR_F(("returning end of line\n"));
      }
   } else { /* we are not the last of the row */
      int len;
      len = 0;

      tmpb = block->next;

      /* find the length of blocks behind and adjust there x co-ordinate
       * by our width
       */
      while (tmpb != NULL) {
         len += tmpb->w;	 
	 tmpb->x -= block->w;
	 tmpb = tmpb->next;
      }

      if (xf86VTSema)
      {  /* now shift the following block using a plane copy */
         ERROR_F(("Shifting row\n"));
	 BLOCK_CURSOR;
         WaitQueue(8);
         S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
         S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
         S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth - 1));
         S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
         S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX);  
         S3_OUTW(FRGD_MIX, FSS_BITBLT | MIX_SRC);
         S3_OUTW(BKGD_MIX, BSS_BITBLT | MIX_SRC);
         S3_OUTW(WRT_MASK, block->mask);

         WaitQueue(8);		/* now shift the cache */
         S3_OUTW(RD_MASK, block->mask);
         S3_OUTW(CUR_Y, block->y);
         S3_OUTW(CUR_X, block->x + block->w);
         S3_OUTW(DESTX_DIASTP, block->x);
         S3_OUTW(DESTY_AXSTP, block->y);
         S3_OUTW(MAJ_AXIS_PCNT, len - 1);
         S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (bptr->h - 1));
         S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y |
	       DRAW | PLANAR | WRTDATA);

         WaitQueue(4);		/* sanity returns */
         S3_OUTW(RD_MASK, 0xff);
         S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
         S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
         S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
	 UNBLOCK_CURSOR;
      }

      /* reconnect the new list of blocks */
      if (block == line)
	 bptr->blocks = block->next;
      else {
	 tmpb = line;
	 while (tmpb->next != block)
	    tmpb = tmpb->next;

	 tmpb->next = block->next;
      }
   }
   ERROR_F(("----------To---------------\n"));
   SHOWCACHE();
   /* This allows us to remove the reference to this block even if we don't
    * know where that is. This is used for when we need to free a block in
    * order to store a new one.
    */
   *(block->reference)=NULL; 
   Xfree(block);

}

#ifdef DEBUG_FCACHE
/*
 * debuging print out, this give a nice picture of the current structure
 * of linked lists - believe it or not.
 */
static void
showcache()
{
   bitMapBlockPtr tmpb;
   bitMapRowPtr brptr;

   for (brptr = headBitRow; brptr->next != NULL; brptr = brptr->next) {
      ErrorF("mask %d freew %d h %d: ", brptr->mask, brptr->freew, brptr->h);
      for (tmpb = brptr->blocks; tmpb != NULL; tmpb = tmpb->next)
	 ErrorF("block x=%d w=%d ", tmpb->x, tmpb->w);
      ErrorF(":\n");
   }
}
#endif
