/* $XConsortium: s3pcach.c,v 1.1 94/03/28 21:16:23 dpw Exp $ */
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
 * Offscreen memory organization:
 * 
 * |<-                      768                 ->|<- 256 or 512->|
 * -----------------------------------------------+-------+--------....
 * |                                              |       |       |
 * |                                              |       |       |
 * |                  FONTS			  |       |       |
 * |                                              |       |       | 
 * |                                              +---+---+---+---+....
 * |                                              |   |   |   |   |
 * |                                              |   |   |   |   |
 * |                                              +-+-+-+-+-+-+-+-+....
 * |                                              | | | | | | | | |
 * +---+---+---+---+------------------------------+-+-+-+-+-+-+-+-+....
 * 
 *                                                \_______________/
 *                                                  Pixmap Cache
 *
 * sizes cache may be more or less depending on off screen ram size.
 * If the space is less than 32 pixel high or if DisplayWidth < 1024,
 * all is used for the fonts.
 *
 * If there is space to the right of the display, it is tiled (depending on
 * width w as: (| or - is 32 pixels)
 *
 *    32<=w<64     64<=w<96    96<=w<128      w>=128
 *      +-+          +-+-+       +-+-+-+     +-+-+-+-+
 *      | |          |   |       |   | |     |       |
 *      +-+          +   +       +   +-+     +       +
 *      | |          |   |       |   | |     |       |
 *      +-+          +-+-+       +-+-+-+     +       +
 *      | |          |   |       |   | |     |       |
 *      +-+          +   +       +   +-+     +       +
 *      | |          |   |       |   | |     |       |
 *      +-+          +-+-+       +-+-+-+     +-+-+-+-+
 *      | |          |   |       |   | |     |   |   |
 *      +-+          +   +       +   +-+     +   +   +
 *      | |          |   |       |   | |     |   |   |
 *      +-+          +-+-+       +-+-+-+     +-+-+-+-+
 *      | |          | | |       |   | |     | | | | |
 *      +-+          +-+-+       +   +-+     +-+-+-+-+
 *      | |          | | |       |   | |     | | | | |
 *      +-+          +-+-+       +-+-+-+     +-+-+-+-+
 */

/*
 * Modified by Amancio Hasty and Jon Tombs.
 *
 * Cache storage and allocation scheme by Jon.
 * 
 * Id: s3pcach.c,v 2.4 1993/08/09 06:17:57 jon Exp jon
 */


#ifdef PIXPRIV
#include "misc.h"
#include "xf86.h"
#include "s3.h"
#include "regs3.h"
#include "s3im.h"
#include "pixmapstr.h"
#include "s3pcach.h"
#include "micache.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

#define NEXT_CACHE_ID	((++CacheIDNum != -1) ? CacheIDNum : ++CacheIDNum)
#define NEXT_CACHE_AGE  (++CacheAge ? CacheAge : s3PcachOverflow())
extern int   s3MAX_SLOTS;

static int   FIRST_128_SLOT = 0;
static int   FIRST_64_SLOT = 0;
static int   FIRST_32_SLOT = 0;		/* fill in the last row */


static CacheInfo *cInfo;
static int CacheIDNum = 1;
static unsigned int CacheAge = 0;
static void  DoCacheExpandPixmap();
static void  DoCacheTile();
static void  DoCacheOpStipple();
static void  DoCacheImageFill();
void  s3CacheFreeSlot();
extern Bool xf86Verbose;

/*
 * This code is pretty ugly. It could be made a bit more readable, but
 * tiling two arbitrary sized rectangles with fixed size squares in a
 * sensible manner is not the easiest thing to code cleanly. Jon.
 */
 
void
s3CacheInit(w, h)
     int   w;
     int   h;
{
   int   i, num;
   int   slots, lines;
   static Bool reEntry = FALSE;
   int  firstLeft64, firstLeft32, maxLeft;
   int leftWidth;
   
   if (reEntry) {
      for (i = 0; i < s3MAX_SLOTS; i++) {
	 cInfo[i].id = -1;
	 cInfo[i].lru = 1;
      }
      CacheAge = 0;   
      return;
   }
   reEntry = TRUE;


   /* h now includes the cursor space */
   lines = s3ScissB + 1 - h;

   slots = lines / 64;
   if (slots > 16)
      slots = 16;

   switch (slots) {
     int tmpslots;
     
     case 0:
     case 1:
     case 2:
	s3MAX_SLOTS = slots * 4;
	break;
     case 3:
     case 4:
     case 5:
     case 6:
        FIRST_64_SLOT = ((slots + 1) / 3) * 2;
	s3MAX_SLOTS = (slots - FIRST_64_SLOT) * 4 + FIRST_64_SLOT;
	break;
     default:
        FIRST_128_SLOT = ((slots + 1) / 8);
	tmpslots = slots - (FIRST_128_SLOT * 4);
	FIRST_64_SLOT = ((tmpslots  + 1) / 3) * 2;
	s3MAX_SLOTS = (tmpslots - FIRST_64_SLOT) * 4 + FIRST_64_SLOT;
	FIRST_64_SLOT += FIRST_128_SLOT;
	s3MAX_SLOTS += FIRST_128_SLOT;
	break;
   }

   FIRST_32_SLOT = s3MAX_SLOTS;
   if ((lines - (slots * 64)) > 32)
      s3MAX_SLOTS += 8;

   if (s3DisplayWidth == 1280) {   
      s3MAX_SLOTS *= 2;
      FIRST_32_SLOT *= 2;
      FIRST_64_SLOT *= 2;
      FIRST_128_SLOT *= 2;
      num = 4;
   } else if (s3DisplayWidth == 1600) {
      s3MAX_SLOTS *= 3;
      FIRST_32_SLOT *= 3;
      FIRST_64_SLOT *= 3;
      FIRST_128_SLOT *= 3;
      num = 6;
   } else if (s3DisplayWidth == 2048) {
      s3MAX_SLOTS *= 5;
      FIRST_32_SLOT *= 5;
      FIRST_64_SLOT *= 5;
      FIRST_128_SLOT *= 5;
      num = 10;
   } else
      num = 2;

   /* For s3DisplayWidth < 1024, no pixmap cache in below-screen area */
   if (s3DisplayWidth < 1024) {
      s3MAX_SLOTS = 0;
      FIRST_32_SLOT = 0;
      FIRST_64_SLOT = 0;
      FIRST_128_SLOT = 0;
   }
     
   /* Now look to the left of the display */
   leftWidth = s3DisplayWidth - s3InfoRec.virtualX;
   slots = s3InfoRec.virtualY / 32;

   leftWidth /=32;

   switch (leftWidth) {
      case 0:
	firstLeft64 = 0;
	firstLeft32 = 0;
	maxLeft = 0;
	break;
      case 1:
	firstLeft64 = 0;
	firstLeft32 = 0;
	maxLeft = slots;
	break;
      case 2:
	firstLeft64 = 0;
	firstLeft32 = (slots - 4)/ 2;
	maxLeft = firstLeft32 + 8;
	break;
      case 3:
	firstLeft64 = 0;
	firstLeft32 = slots / 2;
	maxLeft = firstLeft32 + slots;
	break;
      default: /* case 4: in all sane configurations */
        firstLeft64 = 2;
	firstLeft32 = ((slots - 10)&~1) + 2;
	maxLeft = firstLeft32 + 8;
	break;
   }

   cInfo = (CacheInfoPtr) Xcalloc((s3MAX_SLOTS + maxLeft)
				  * sizeof(CacheInfo));

   for (i = 0; i < FIRST_128_SLOT; i++) {
      cInfo[i].size = 256;
      cInfo[i].x = 768 + (i % (num/2)) * 256;
      cInfo[i].y = h + (i / (num/2)) * 256;
   }
   h += (FIRST_128_SLOT/(num/2)) * 256;

   for (i = FIRST_128_SLOT; i < FIRST_64_SLOT; i++) {
      cInfo[i].size = 128;
      cInfo[i].x = 768 + ((i - FIRST_128_SLOT) % num) * 128;
      cInfo[i].y = h + ((i - FIRST_128_SLOT) / num) * 128;
   }

   /* left hand 128s */
   for (i = FIRST_64_SLOT; i < (FIRST_64_SLOT+firstLeft64); i++) {
      cInfo[i].size = 128;
      cInfo[i].x = s3InfoRec.virtualX;
      cInfo[i].y = (i - FIRST_64_SLOT) * 128;
   }
   h += ((FIRST_64_SLOT - FIRST_128_SLOT) / num) * 128;
   
   for (i = FIRST_64_SLOT+firstLeft64; i < FIRST_32_SLOT+firstLeft64; i++) {
      cInfo[i].size = 64;
      cInfo[i].x = 768 + ((i - (FIRST_64_SLOT+firstLeft64)) % (2 * num)) * 64;
      cInfo[i].y = h + ((i - (FIRST_64_SLOT+firstLeft64)) / (2 * num)) * 64;
   }

   h += ((FIRST_32_SLOT - FIRST_64_SLOT) / (num*2)) * 64;

   /* left hand 64s */
   for (i = FIRST_32_SLOT+firstLeft64; i < FIRST_32_SLOT+firstLeft32; i++) {
      cInfo[i].size = 64;
      switch (leftWidth) {
	 case 2:
	 case 3:
	    cInfo[i].x = 0;
	    cInfo[i].y = (i - (FIRST_32_SLOT+firstLeft64)) * 64;
	    break;
	 default:
	    cInfo[i].x = 64 * ((i - (FIRST_32_SLOT+firstLeft64)) % 2);
	    cInfo[i].y =  128 * firstLeft64 +
		  ((i - (FIRST_32_SLOT+firstLeft64))/ 2) * 64;
	    break;
      }
      cInfo[i].x += s3InfoRec.virtualX;	    
   }
   
   
   for (i = FIRST_32_SLOT+firstLeft32; i < s3MAX_SLOTS+firstLeft32; i++) {
      cInfo[i].size = 32;   
      cInfo[i].x = 768 + ((i - (FIRST_32_SLOT+firstLeft32)) % (4 * num)) * 32;
      cInfo[i].y = h;
   }

   /* left had 32s */
   for (i = s3MAX_SLOTS+firstLeft32; i < s3MAX_SLOTS+maxLeft; i++) {
      cInfo[i].size = 32;
      switch (leftWidth) {
	 case 1:
	   cInfo[i].x = 0;
	   cInfo[i].y = (i - (s3MAX_SLOTS+firstLeft32)) * 32;
	   break;
	 case 2:
	   cInfo[i].x = 32 * ((i - (s3MAX_SLOTS+firstLeft32)) % 2);
	   cInfo[i].y = (firstLeft32 * 64) +
			  ((i - (s3MAX_SLOTS+firstLeft32))/2) * 32;
	   break;
	 case 3:
	   cInfo[i].x = 64;
	   cInfo[i].y = (i - (s3MAX_SLOTS+firstLeft32)) * 32;
	   break;
	 default:
	   cInfo[i].x = ((i - (s3MAX_SLOTS+firstLeft32)) % 4) * 32;
	   cInfo[i].y = firstLeft64 * 128 +
		  ((firstLeft32-firstLeft64)/2) * 64 +
		  ((i - (s3MAX_SLOTS+firstLeft32))/4) * 32;
	   break;
      }
     cInfo[i].x += s3InfoRec.virtualX;	 
   }

   s3MAX_SLOTS += maxLeft;
   FIRST_32_SLOT += firstLeft32;
   FIRST_64_SLOT += firstLeft64;
   
   if (s3MAX_SLOTS > 0)
       miCacheFreeSlot = s3CacheFreeSlot;

    for (i = 0; i < s3MAX_SLOTS; i++) {
      cInfo[i].id = -1;
      cInfo[i].lru = CacheAge;
    }
    
    if (xf86Verbose) {
      if (s3MAX_SLOTS == 0) {
	ErrorF("%s %s: Pixmap cache disabled due to insufficient ",
	       XCONFIG_PROBED, s3InfoRec.name);
        ErrorF("off-screen ram\n");
      } else {
	 ErrorF("%s %s: Pixmap cache:\n", XCONFIG_PROBED, s3InfoRec.name);
	 ErrorF("%s %s: Using ", XCONFIG_PROBED,s3InfoRec.name);
	 if (FIRST_128_SLOT)
	    ErrorF("%d 256-pixel ", FIRST_128_SLOT);
	 if (FIRST_64_SLOT)
	    ErrorF("%d 128-pixel ", FIRST_64_SLOT - FIRST_128_SLOT);
	 if (FIRST_32_SLOT)
	    ErrorF("%d 64-pixel ", FIRST_32_SLOT - FIRST_64_SLOT);
	 ErrorF("and %d 32-pixel slots\n", s3MAX_SLOTS - FIRST_32_SLOT);
      }
    }

	 
#ifdef DEBUG_PCACHE
show_pcach();
  ErrorF("%d %d %d %d\n", FIRST_32_SLOT, FIRST_64_SLOT, FIRST_128_SLOT, s3MAX_SLOTS);
#endif
}

#ifdef DEBUG_PCACHE
crash()
{
   while (1)
      ErrorF("crash!\n");
}

show_pcach()
{
  int i;

   ErrorF("---------------------------------------------------\n");
   for (i = 0; i < s3MAX_SLOTS; i++)
      ErrorF("[%d %d] %d %d %d (%dx%d, %d %d)\n",      cInfo[i].id ,cInfo[i].lru,
        cInfo[i].x, cInfo[i].y, cInfo[i].size,
	     cInfo[i].w, cInfo[i].h,
	     cInfo[i].nx, cInfo[i].ny) ;
}

#endif
#ifdef  WATCHTHETHRASHING
#define SHOWPCACHE(x) show_pcach x
#else
#define SHOWPCACHE(x) /**/
#endif


void
s3CacheFreeSlot(pix)
     PixmapPtr pix;
{
   if (s3MAX_SLOTS > 0 && pix->slot >= 0 && pix->slot < s3MAX_SLOTS &&
       cInfo[pix->slot].id == pix->cacheId) {
      cInfo[pix->slot].id = -1;
      pix->slot = -1;
   }

}

/*
 * Plan: DoCachePixmapBig() called for tiling, filling tries to find the
 * biggest available hole.
 * DoCachePixmapSmall() called for small area tilings etc, tries the small
 * slots first.
 */
 
static int
DoCachePixmapBig(pix, cacher)
     PixmapPtr pix;
     void  (*cacher) ();
{

   int  i, next;
   int Max;

   if (s3MAX_SLOTS == 0) {   
      ErrorF("called DoCachePixmap with s3MAX_SLOTS == 0\n");
      return 0;
   }
   
   if (pix->drawable.width > cInfo[0].size ||
       pix->drawable.height > cInfo[0].size)
     return 0;
			
   if (pix->slot >= 0 && pix->slot < s3MAX_SLOTS &&
       cInfo[pix->slot].id == pix->cacheId)
      return 1;

   Max =  (pix->drawable.width > pix->drawable.height) ?
	 pix->drawable.width : pix->drawable.height;

   if (Max > 64)
      Max = (Max > 128) ? FIRST_128_SLOT : FIRST_64_SLOT;
   else
      Max = (Max > 32) ? FIRST_32_SLOT : s3MAX_SLOTS;

   next = 0;
   for (i = 0; i < Max; i++) {
      if (cInfo[i].id == -1) {
	 pix->slot = i;
	    (*cacher) (pix, cInfo[i].size);
	    return 1;
      } else if (cInfo[i].lru < cInfo[next].lru) {
	    next = i;
      }
   }

   pix->slot = next;
    
   (*cacher) (pix, cInfo[next].size);
   
   return 1;
}

static int
DoCachePixmapSmall(pix, cacher)
     PixmapPtr pix;
     void  (*cacher) ();
{

   int  i, next;
   int Max;

   if (s3MAX_SLOTS == 0 || pix->drawable.width > cInfo[0].size ||
			 pix->drawable.height > cInfo[0].size)
      return 0;
			
   if (pix->slot >= 0 && pix->slot < s3MAX_SLOTS &&
       cInfo[pix->slot].id == pix->cacheId)
      return 1;

   Max =  (pix->drawable.width > pix->drawable.height) ?
	 pix->drawable.width : pix->drawable.height;

   if (Max > 64)
      Max = (Max > 128) ? FIRST_128_SLOT : FIRST_64_SLOT;
   else
      Max = (Max > 32) ? FIRST_32_SLOT : s3MAX_SLOTS;

   next = --Max;
   for (i = Max ; i >= 0; i--) {
      if (cInfo[i].id == -1) {
	pix->slot = i;
	(*cacher) (pix, cInfo[i].size);
	return 1;
      } else if (cInfo[i].lru < cInfo[next].lru) {
	 next = i;
      }
   }


   pix->slot = next;
    
   (*cacher) (pix, cInfo[next].size);
   
   return 1;
}

int
s3CacheTile(pix, big)
     PixmapPtr pix;
     int big;
{
   int ret;
   SHOWPCACHE(());
   if (big < 8)
     ret = DoCachePixmapSmall(pix, DoCacheTile);
   else
     ret = DoCachePixmapBig(pix, DoCacheTile);
   SHOWPCACHE(());
   return ret;
}

int
s3CacheStipple(pix, big)
     PixmapPtr pix;
     int big;
{
   int ret;
   SHOWPCACHE(());
   if (big < 8)
      ret = DoCachePixmapSmall(pix, DoCacheOpStipple);
   else
      ret = DoCachePixmapBig(pix, DoCacheOpStipple);
   SHOWPCACHE(());
   return ret;
}

int
s3CacheOpStipple(pix, big)
     PixmapPtr pix;
     int big;
{
   int ret;
   SHOWPCACHE(());
   if (big < 8)
      ret = DoCachePixmapSmall(pix, DoCacheOpStipple);
   else
      ret = DoCachePixmapBig(pix, DoCacheOpStipple);
   SHOWPCACHE(());
   return ret;
}

static void
DoCacheTile(pix, size)
     PixmapPtr pix;
     int   size;
{
   CacheInfoPtr pci = &cInfo[pix->slot];


   pci->pix_w = pix->drawable.width;
   pci->pix_h = pix->drawable.height;
   pci->nx = size / pix->drawable.width;
   pci->ny = size / pix->drawable.height;
   pci->w = pci->nx * pci->pix_w;
   pci->h = pci->ny * pci->pix_h;
   pci->lru = NEXT_CACHE_AGE;
/*   if (pix->cacheId)
      pci->id = pix->cacheId;
   else */
      pci->id = pix->cacheId = NEXT_CACHE_ID;

   (s3ImageWriteFunc) (pci->x, pci->y, pci->pix_w, pci->pix_h,
		       pix->devPrivate.ptr, pix->devKind, 0, 0,
		       MIX_SRC, 0xffff);

   DoCacheExpandPixmap(pci);
   WaitIdleEmpty(); /* Make sure that all commands have finished */
}

static void
DoCacheOpStipple(pix, size)
     PixmapPtr pix;
     int   size;
{
   CacheInfoPtr pci = &cInfo[pix->slot];

   pci->pix_w = pix->drawable.width;
   pci->pix_h = pix->drawable.height;
   pci->nx = size / pix->drawable.width;
   pci->ny = size / pix->drawable.height;
   pci->w = pci->nx * pci->pix_w;
   pci->h = pci->ny * pci->pix_h;
   pci->lru = NEXT_CACHE_AGE;
/*   if (pix->cacheId)
      pci->id = pix->cacheId;
   else */
      pci->id = pix->cacheId = NEXT_CACHE_ID;

   s3ImageOpStipple(pci->x, pci->y, pci->pix_w, pci->pix_h,
		    pix->devPrivate.ptr, pix->devKind,
		    pci->pix_w, pci->pix_h, pci->x, pci->y,
		    255, 0, MIX_SRC, 0xffff);

   DoCacheExpandPixmap(pci);
   WaitIdleEmpty(); /* Make sure that all commands have finished */   
}

static void
DoCacheExpandPixmap(pci)
     CacheInfoPtr pci;
{
   int   cur_w = pci->pix_w;
   int   cur_h = pci->pix_h;

   BLOCK_CURSOR;
   WaitQueue(7);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
   S3_OUTW(FRGD_MIX, FSS_BITBLT | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   S3_OUTW(WRT_MASK, 0xffff);

 /* Expand in the x direction */

   while (cur_w * 2 <= pci->w) {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)(pci->x + cur_w));
      S3_OUTW(DESTY_AXSTP, (short)pci->y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(cur_w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      cur_w *= 2;
   }

   if (cur_w != pci->w) {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)(pci->x + cur_w));
      S3_OUTW(DESTY_AXSTP, (short)pci->y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - cur_w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      cur_w = pci->w;
   }
 /* Expand in the y direction */
   while (cur_h * 2 <= pci->h) {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)pci->x);
      S3_OUTW(DESTY_AXSTP, (short)(pci->y + cur_h));
      S3_OUTW(MAJ_AXIS_PCNT, (short)(cur_w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      cur_h *= 2;
   }

   if (cur_h != pci->h) {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)pci->x);
      S3_OUTW(DESTY_AXSTP, (short)(pci->y + cur_h));
      S3_OUTW(MAJ_AXIS_PCNT, (short)(cur_w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - cur_h - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
   }
   WaitQueue(2);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

void
s3CImageFill(slot, x, y, w, h, pox, poy, alu, planemask)
     int   slot;
     int   x;
     int   y;
     int   w;
     int   h;
     int   pox;
     int   poy;
     short alu;
     short planemask;
{

   DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, MIX_SRC, FSS_BITBLT, BSS_BITBLT, planemask);

}

void
s3CImageStipple(slot, x, y, w, h, pox, poy, fg, alu, planemask)
     int   slot;
     int   x;
     int   y;
     int   w;
     int   h;
     int   pox;
     int   poy;
     int   fg;
     short alu;
     short planemask;
{
   BLOCK_CURSOR;
   WaitQueue(3);
   S3_OUTW(FRGD_COLOR, fg);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
   S3_OUTW(RD_MASK, 0x01);

   DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, MIX_DST, FSS_FRGDCOL, BSS_BKGDCOL, planemask);

   WaitQueue(2);
   S3_OUTW(RD_MASK, 0xff);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   UNBLOCK_CURSOR;
}

void
s3CImageOpStipple(slot, x, y, w, h, pox, poy, fg, bg, alu, planemask)
     int   slot;
     int   x;
     int   y;
     int   w;
     int   h;
     int   pox;
     int   poy;
     int   fg;
     int   bg;
     short alu;
     short planemask;
{
   BLOCK_CURSOR;
   WaitQueue(4);
   S3_OUTW(FRGD_COLOR, fg);
   S3_OUTW(BKGD_COLOR, bg);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
   S3_OUTW(RD_MASK, 0x01);

   DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, alu, FSS_FRGDCOL, BSS_BKGDCOL, planemask);

   WaitQueue(2);
   S3_OUTW(RD_MASK, 0xff);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   UNBLOCK_CURSOR;
}

static void
DoCacheImageFill(slot, x, y, w, h, pox, poy, fgalu, bgalu, fgmix, bgmix, planemask)
     int   slot;
     int   x;
     int   y;
     int   w;
     int   h;
     int   pox;
     int   poy;
     short fgalu;
     short bgalu;
     short fgmix;
     short bgmix;
     short planemask;
{
   CacheInfoPtr pci = &cInfo[slot];
   int   xwmid, ywmid, orig_xwmid;
   int   startx, starty, endx, endy;
   int   orig_x = x;

   if (w == 0 || h == 0)
      return;

   modulus(x - pox, pci->w, startx);
   modulus(y - poy, pci->h, starty);
   modulus(x - pox + w - 1, pci->w, endx);
   modulus(y - poy + h - 1, pci->h, endy);

   orig_xwmid = xwmid = w - (pci->w - startx + endx + 1);
   ywmid = h - (pci->h - starty + endy + 1);

   BLOCK_CURSOR;
   WaitQueue(7);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
   S3_OUTW(FRGD_MIX, fgmix | fgalu);
   S3_OUTW(BKGD_MIX, bgmix | bgalu);
   S3_OUTW(WRT_MASK, planemask);

   if (starty + h - 1 < pci->h) {
      if (startx + w - 1 < pci->w) {
	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)(pci->x + startx));
	 S3_OUTW(CUR_Y, (short)(pci->y + starty));
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(w - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
      } else {
	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)(pci->x + startx));
	 S3_OUTW(CUR_Y, (short)(pci->y + starty));
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 x += pci->w - startx;

	 while (xwmid > 0) {
	    WaitQueue(7);
	    S3_OUTW(CUR_X, (short)pci->x);
	    S3_OUTW(CUR_Y, (short)(pci->y + starty));
	    S3_OUTW(DESTX_DIASTP, (short)x);
	    S3_OUTW(DESTY_AXSTP, (short)y);
	    S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	    S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h - 1));
	    S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR |
		  WRTDATA);
	    x += pci->w;
	    xwmid -= pci->w;
	 }

	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)pci->x);
	 S3_OUTW(CUR_Y, (short)(pci->y + starty));
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)endx);
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
      }
   } else if (startx + w - 1 < pci->w) {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)(pci->x + startx));
      S3_OUTW(CUR_Y, (short)(pci->y + starty));
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      y += pci->h - starty;

      while (ywmid > 0) {
	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)(pci->x + startx));
	 S3_OUTW(CUR_Y, (short)pci->y);
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(w - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 y += pci->h;
	 ywmid -= pci->h;
      }

      WaitQueue(7);
      S3_OUTW(CUR_X, (short)(pci->x + startx));
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(w - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
   } else {
      WaitQueue(7);
      S3_OUTW(CUR_X, (short)(pci->x + startx));
      S3_OUTW(CUR_Y, (short)(pci->y + starty));
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      x += pci->w - startx;

      while (xwmid > 0) {
	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)pci->x);
	 S3_OUTW(CUR_Y, (short)(pci->y + starty));
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT |
	       (short)(pci->h - starty - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 x += pci->w;
	 xwmid -= pci->w;
      }

      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)(pci->y + starty));
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)endx);
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty - 1));
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      y += pci->h - starty;

      while (ywmid > 0) {
	 x = orig_x;
	 xwmid = orig_xwmid;

	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)(pci->x + startx));
	 S3_OUTW(CUR_Y, (short)pci->y);
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 x += pci->w - startx;

	 while (xwmid > 0) {
	    WaitQueue(7);
	    S3_OUTW(CUR_X, (short)pci->x);
	    S3_OUTW(CUR_Y, (short)pci->y);
	    S3_OUTW(DESTX_DIASTP, (short)x);
	    S3_OUTW(DESTY_AXSTP, (short)y);
	    S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	    S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	    S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR |
		  WRTDATA);

	    x += pci->w;
	    xwmid -= pci->w;
	 }

	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)pci->x);
	 S3_OUTW(CUR_Y, (short)pci->y);
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)endx);
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 y += pci->h;
	 ywmid -= pci->h;
      }

      x = orig_x;
      xwmid = orig_xwmid;

      WaitQueue(7);
      S3_OUTW(CUR_X, (short)(pci->x + startx));
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);      

      x += pci->w - startx;

      while (xwmid > 0) {
	 WaitQueue(7);
	 S3_OUTW(CUR_X, (short)pci->x);
	 S3_OUTW(CUR_Y, (short)pci->y);
	 S3_OUTW(DESTX_DIASTP, (short)x);
	 S3_OUTW(DESTY_AXSTP, (short)y);
	 S3_OUTW(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	 S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
	 S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	 x += pci->w;
	 xwmid -= pci->w;
      }

      WaitQueue(7);
      S3_OUTW(CUR_X, (short)pci->x);
      S3_OUTW(CUR_Y, (short)pci->y);
      S3_OUTW(DESTX_DIASTP, (short)x);
      S3_OUTW(DESTY_AXSTP, (short)y);
      S3_OUTW(MAJ_AXIS_PCNT, (short)endx);
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
   }

   WaitQueue(2);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

int
s3PcachOverflow()
{
   int i;
   int Min;

   Min = cInfo[0].lru;
   
   for (i = 0; i < s3MAX_SLOTS; i++)
      if (cInfo[i].lru < Min)
	 Min = cInfo[i].lru;

   for (i = 0; i < s3MAX_SLOTS; i++)
      cInfo[i].lru -= Min;

   CacheAge -= Min;

   return CacheAge;
}
#endif /* PIXPRIV */
