/* $XConsortium: mach32pcach.c,v 1.1 94/03/28 21:08:53 dpw Exp $ */
/*
 * Copyright 1992,1993 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN AND RICKARD E. FAITH DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL KEVIN E. MARTIN OR RICKARD E. FAITH BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified for the Mach-8 by Rickard E. Faith
 * Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Dynamic cache allocation added by Rickard E. Faith (faith@cs.unc.edu)
 *
 */


/*       Offscreen memory organization for one 256-line cache set:
 *
 *       128      128                         768
 *     +-------+-------+-----------------------------------------------+
 *     |       |       |                                               |
 * 128 |   0   |   1   |                                               |
 *     |       |       |                                               |
 *     +---+---+---+---+                   8 FONTS                     | 256
 *  64 | 2 | 3 | 4 | 5 |                                               |
 *     +---+---+---+---+                                               |
 *  64 | 6 | 7 | 8 | 9 |                                               |
 *     +---+---+---+---+-----------------------------------------------+
 *
 *
 *
 *       Offscreen memory organization for two 256-line cache sets:
 *
 *            256                             768
 *     +---------------+-----------------------------------------------+
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 * 256 |       0       |                                               |
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 *     +-------+-------+                  16 FONTS                     | 512
 *     |       |       |                                               |
 * 128 |   1   |   2   |                                               |
 *     |       |       |                                               |
 *     +---+---+---+---+                                               |
 *  64 | 3 | 4 | 5 | 6 |                                               |
 *     +---+---+---+---+                                               |
 *  64 | 7 | 8 | 9 | A |                                               |
 *     +---+---+---+---+-----------------------------------------------+
 *
 *
 *
 *       Offscreen memory organization for three 256-line cache sets:
 *
 *            256                            768
 *     +---------------+-----------------------------------------------+
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 * 256 |       0       |                                               |     
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 *     +---------------+                  16 FONTS                     | 512
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 * 256 |       1       |                                               |    
 *     |               |                                               |
 *     |               |                                               |
 *     |               |                                               |
 *     +-------+-------+-------+-------+-------+-------+-------+-------+
 *     |       |       |       |       |       |       |       |       |
 * 128 |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |
 *     |       |       |       |       |       |       |       |       |
 *     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+    
 *  64 | A | B | C | D | E | F | 10| 11| 12| 13| 14| 15| 16| 17| 18| 19|
 *     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *  64 | 1A| 1B| 1C| 1D| 1E| 1F| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29|
 *     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 */

#ifdef PIXPRIV
#include "regmach32.h"
#include "mach32im.h"
#include "pixmapstr.h"
#include "mach32.h"
#include "micache.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

extern Bool xf86Verbose;

#define NEXT_CACHE_ID (++CacheIDNum ? CacheIDNum : ++CacheIDNum)

typedef struct _CacheInfo {
    int id;
    int x;
    int y;
    int w;
    int h;
    int nx;
    int ny;
    int pix_w;
    int pix_h;
    unsigned int lru;
} CacheInfo, *CacheInfoPtr;

static CacheInfoPtr cInfo = NULL;
static int CacheIDNum = 1;

static int MaxSlots;
static int First256Slot;
static int First128Slot;
static int First64Slot;

static unsigned int pixmap_cache_clock = 1;

void DoCacheExpandPixmap();
void DoCacheTile();
void DoCacheOpStipple();
void DoCacheImageFill();
void mach32CacheFreeSlot();

void
mach32CacheInit(w, h)
    int w;
    int h;
{
    int i;
    int free_ram = mach32InfoRec.videoRam * 1024 - w * h;
    int lines    = free_ram / w;
    int cache_sets;


				/* 8514 can't access lines > 1535 */
    if (h + lines > 1535)
	  lines = 1536 - h;
    if (lines < 0)
	  lines = 0;
    
    
				/* If cache_sets is changes, remember
				 * to change mach32MaxY in mach32c.im.
				 */

    if (!cInfo) {
       cache_sets = lines / 256;
       switch (cache_sets) {
       case 0:			/* Assume at least 256 lines reserved */
       case 1:
	  First256Slot = -1;
	  First128Slot =  0;
	  First64Slot  =  First128Slot +  2;
	  MaxSlots     =  First64Slot  +  8;
	  break;
       case 2:
	  First256Slot =  0;
	  First128Slot =  First256Slot +  1;
	  First64Slot  =  First128Slot +  2;
	  MaxSlots     =  First64Slot  +  8;
	  break;
       case 3:
       default:
	  First256Slot = 0;
	  First128Slot = First256Slot +  2;
	  First64Slot  = First128Slot +  8;
	  MaxSlots     = First64Slot  + 32;
	  break;
       }
       cInfo = (CacheInfoPtr)Xcalloc( MaxSlots * sizeof( CacheInfo ) );

       switch (cache_sets) {
       case 0:
       case 1:
				/* No 256x256 pixel cache */
	  for (i = First128Slot; i < First64Slot; i++) {
	     cInfo[i].x = ((i - First128Slot) & 1) * 128;
	     cInfo[i].y = h
		   + 128 * ((i - First128Slot) >> 1);
	  }
	  
	  for (i = First64Slot; i < MaxSlots; i++) {
	     cInfo[i].x = ((i - First64Slot) & 3) * 64;
	     cInfo[i].y = h
		   + 128 * (First64Slot >> 1)
		   + 64 * ((i - First64Slot) >> 2);
	  }
	  break;
       case 2:
	  for (i = First256Slot; i < First128Slot; i++) {
	     cInfo[i].x = 0;
	     cInfo[i].y = h
		   + 256 * (i - First256Slot);
	  }
	  
	  for (i = First128Slot; i < First64Slot; i++) {
	     cInfo[i].x = ((i - First128Slot) & 1) * 128;
	     cInfo[i].y = h
		   + 256 * First128Slot
		   + 128 * ((i - First128Slot) >> 1);
	  }
	  
	  for (i = First64Slot; i < MaxSlots; i++) {
	     cInfo[i].x = ((i - First64Slot) & 3) * 64;
	     cInfo[i].y = h +
		   256 * First128Slot
		   + 128 * ((First64Slot - First128Slot) >> 1)
		   + 64 * ((i - First64Slot) >> 2);
	  }
	  break;
      case 3:
      default:
	  for (i = First256Slot; i < First128Slot; i++) {
	     cInfo[i].x = 0;
	     cInfo[i].y = h
		   + 256 * (i - First256Slot);
	  }
	  
	  for (i = First128Slot; i < First64Slot; i++) {
	     cInfo[i].x = ((i - First128Slot) & 7) * 128;
	     cInfo[i].y = h
		   + 256 * First128Slot
		   + 128 * ((i - First128Slot) >> 3);
	  }
	  
	  for (i = First64Slot; i < MaxSlots; i++) {
	     cInfo[i].x = ((i - First64Slot) & 15) * 64;
	     cInfo[i].y = h +
		   256 * First128Slot
		   + 128 * ((First64Slot - First128Slot) >> 3)
		   + 64 * ((i - First64Slot) >> 4);
	  }
	  break;
       }
       
       if (xf86Verbose) {
	  ErrorF("%s %s: Pixmap cache: %d 256x256 slots, %d 128x128 slots, %d 64x64 slots\n",
		  XCONFIG_PROBED, mach32InfoRec.name,
		  First256Slot == -1 ? 0 : First128Slot - First256Slot,
		  First64Slot - First128Slot,
		  MaxSlots - First64Slot );
       }
    }
    
    for (i = 0; i < MaxSlots; i++) {
       cInfo[i].id = -1;
       cInfo[i].lru = pixmap_cache_clock;
    }

    miCacheFreeSlot = mach32CacheFreeSlot;
}

void
mach32CacheFreeSlot(pix)
    PixmapPtr pix;
{
    /* This is not really necessary but for completeness... */
    /* Anyway, the pixmap cache is reinitialized upon VTEnter */
    if (!xf86VTSema)
	return;

    if (pix->slot >= 0 && pix->slot <= MaxSlots &&
	cInfo[pix->slot].id == pix->cacheId) {
	cInfo[pix->slot].id = -1;
	pix->slot = -1;
    }
}

int
DoCachePixmap(pix, cacher)
    PixmapPtr pix;
    void (*cacher)();
{
    int i, next = (First256Slot != -1) ? First256Slot : First128Slot;

#ifdef PIXMAP_CACHE_OFF
    return 0;
#endif

    if (pix->slot >= 0 && pix->slot <= MaxSlots &&
	cInfo[pix->slot].id == pix->cacheId) {
	return 1;
    }
    
    if (First256Slot == -1) {
       if (pix->drawable.width > 128 || pix->drawable.height > 128)
	     return 0;
    } else {
       if (pix->drawable.width > 256 || pix->drawable.height > 256)
	     return 0;
    }
				/* The pixmap is of cachable size, so we
				 * want to cache it.  If there are available
				 * cache slots, we use the largest, since this
				 * optimizes tiling.  If, however, there are
				 * no available cache slots, we use the least
				 * recently used, *regardless* of size (except,
				 * of course, that the pixmap has to fit into
				 * the cache slot.  Rik Faith, 22July93. */

    if (pix->drawable.width <= 64 && pix->drawable.height <= 64) {
        next = MaxSlots - 1;	/* Last 64Slot */
	for (i = First64Slot; i < MaxSlots; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 64);
		return 1;
	    } else if (cInfo[i].lru < cInfo[next].lru)
		next = i;
    } else {
        next = First64Slot - 1;	/* Last 128Slot */
    }

    if (pix->drawable.width <= 128 && pix->drawable.height <= 128) {
	for (i = First128Slot; i < First64Slot; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 128);
		return 1;
	    } else if (cInfo[i].lru < cInfo[next].lru)
		next = i;
    } else if (First256Slot != -1) {
        next = First128Slot - 1; /* Last 256Slot */
    }

    if (First256Slot != -1
	&& pix->drawable.width <= 256 && pix->drawable.height <= 256) {
       
       for (i = First256Slot; i < First128Slot; i++)
	     if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 256);
		return 1;
	     } else if (cInfo[i].lru < cInfo[next].lru)
		   next = i;
    } 

    pix->slot = next;
    
/* #define DEBUG_LRU */
#ifdef DEBUG_LRU
    for (i = 0; i < MaxSlots; i++)
	  ErrorF( "lru[%d]=%d, ", i, cInfo[i].lru );
    ErrorF( "Using %d for %dx%d\n", next, pix->drawable.width,
	    pix->drawable.height );
#endif
    
    (*cacher)(pix, (next < First64Slot
		    ? (next < First128Slot ? 256 : 128) : 64));
    return 1;
}

int
mach32CacheTile(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheTile);
}

int
mach32CacheStipple(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheOpStipple);
}

int
mach32CacheOpStipple(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheOpStipple);
}

Bool
mach32CachableTile(pix)
    PixmapPtr pix;
{
   if (First256Slot != -1)
	 return (pix->drawable.width <= 256 && pix->drawable.height <= 256);
   return (pix->drawable.width <= 128 && pix->drawable.height <= 128);
}

Bool
mach32CachableStipple(pix)
    PixmapPtr pix;
{
   if (First256Slot != -1)
	 return (pix->drawable.width <= 256 && pix->drawable.height <= 256);
   return (pix->drawable.width <= 128 && pix->drawable.height <= 128);
}

Bool
mach32CachableOpStipple(pix)
    PixmapPtr pix;
{
   if (First256Slot != -1)
	 return (pix->drawable.width <= 256 && pix->drawable.height <= 256);
   return (pix->drawable.width <= 128 && pix->drawable.height <= 128);
}

void
DoCacheTile(pix, size)
    PixmapPtr pix;
    int size;
{
    CacheInfoPtr pci = &cInfo[pix->slot];

    pci->pix_w = pix->drawable.width;
    pci->pix_h = pix->drawable.height;
    pci->nx = size/pix->drawable.width;
    pci->ny = size/pix->drawable.height;
    pci->w = pci->nx * pci->pix_w;
    pci->h = pci->ny * pci->pix_h;
    pci->lru = pixmap_cache_clock;
    if (pix->cacheId)
	pci->id = pix->cacheId;
    else
	pci->id = pix->cacheId = NEXT_CACHE_ID;

    (mach32ImageWriteFunc)(pci->x, pci->y, pci->pix_w, pci->pix_h,
		      pix->devPrivate.ptr, pix->devKind, 0, 0,
		      MIX_SRC, 0xffff);

    DoCacheExpandPixmap(pci);

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}

void
DoCacheOpStipple(pix, size)
    PixmapPtr pix;
    int size;
{
    CacheInfoPtr pci = &cInfo[pix->slot];

    pci->pix_w = pix->drawable.width;
    pci->pix_h = pix->drawable.height;
    pci->nx = size/pix->drawable.width;
    pci->ny = size/pix->drawable.height;
    pci->w = pci->nx * pci->pix_w;
    pci->h = pci->ny * pci->pix_h;
    pci->lru = pixmap_cache_clock;
    if (pix->cacheId)
	pci->id = pix->cacheId;
    else
	pci->id = pix->cacheId = NEXT_CACHE_ID;

    mach32ImageOpStipple(pci->x, pci->y, pci->pix_w, pci->pix_h,
			  pix->devPrivate.ptr, pix->devKind,
			  pci->pix_w, pci->pix_h, pci->x, pci->y,
			  255, 0, MIX_SRC, 0xffff);

    DoCacheExpandPixmap(pci);

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}

void
DoCacheExpandPixmap(pci)
    CacheInfoPtr pci;
{
    int cur_w = pci->pix_w;
    int cur_h = pci->pix_h;

    WaitQueue(7);
    outw(EXT_SCISSOR_T, 0);
    outw(EXT_SCISSOR_L, 0);
    outw(EXT_SCISSOR_R, mach32MaxX);
    outw(EXT_SCISSOR_B, mach32MaxY);
    outw(FRGD_MIX, FSS_BITBLT | MIX_SRC);
    outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
    outw(WRT_MASK, 0xffff);

    /* Expand in the x direction */
    while (cur_w * 2 <= pci->w) {
	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)(pci->x + cur_w));
	outw(DESTY_AXSTP, (short)pci->y);
	outw(MAJ_AXIS_PCNT, (short)(cur_w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	cur_w *= 2;
    }

    if (cur_w != pci->w) {
	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)(pci->x + cur_w));
	outw(DESTY_AXSTP, (short)pci->y);
	outw(MAJ_AXIS_PCNT, (short)(pci->w - cur_w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	cur_w = pci->w;
    }

    /* Expand in the y direction */
    while (cur_h * 2 <= pci->h) {
	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)pci->x);
	outw(DESTY_AXSTP, (short)(pci->y + cur_h));
	outw(MAJ_AXIS_PCNT, (short)(cur_w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(cur_h -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	cur_h *= 2;
    }

    if (cur_h != pci->h) {
	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)pci->x);
	outw(DESTY_AXSTP, (short)(pci->y + cur_h));
	outw(MAJ_AXIS_PCNT, (short)(cur_w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - cur_h -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
    }

    WaitQueue(2);
    outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
    outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
}

void
mach32CImageFill(slot, x, y, w, h, pox, poy, alu, planemask)
    int slot;
    int x;
    int y;
    int w;
    int h;
    int pox;
    int poy;
    short alu;
    short planemask;
{
    int i;

    DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, MIX_SRC, FSS_BITBLT, BSS_BITBLT, planemask);

    if (!(cInfo[slot].lru = ++pixmap_cache_clock))
	  for (i = 0; i < MaxSlots; i++)
		cInfo[i].lru = pixmap_cache_clock;

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}

void
mach32CImageStipple(slot, x, y, w, h, pox, poy, fg, alu, planemask)
    int slot;
    int x;
    int y;
    int w;
    int h;
    int pox;
    int poy;
    int fg;
    short alu;
    short planemask;
{
    int i;

    WaitQueue(3);
    outw(FRGD_COLOR, fg);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
    outw(RD_MASK, 0x01);

    DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, MIX_DST, FSS_FRGDCOL, BSS_BKGDCOL, planemask);

    WaitQueue(2);
    outw(RD_MASK, 0xff);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);

    if (!(cInfo[slot].lru = ++pixmap_cache_clock))
	  for (i = 0; i < MaxSlots; i++)
		cInfo[i].lru = pixmap_cache_clock;

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}

void
mach32CImageOpStipple(slot, x, y, w, h, pox, poy, fg, bg, alu, planemask)
    int slot;
    int x;
    int y;
    int w;
    int h;
    int pox;
    int poy;
    int fg;
    int bg;
    short alu;
    short planemask;
{
    int i;

    WaitQueue(4);
    outw(FRGD_COLOR, fg);
    outw(BKGD_COLOR, bg);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
    outw(RD_MASK, 0x01);

    DoCacheImageFill(slot, x, y, w, h, pox, poy, alu, alu, FSS_FRGDCOL, BSS_BKGDCOL, planemask);

    WaitQueue(2);
    outw(RD_MASK, 0xff);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);

    if (!(cInfo[slot].lru = ++pixmap_cache_clock))
	  for (i = 0; i < MaxSlots; i++)
		cInfo[i].lru = pixmap_cache_clock;

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}

void
DoCacheImageFill(slot, x, y, w, h, pox, poy, fgalu, bgalu, fgmix, bgmix, planemask)
    int slot;
    int x;
    int y;
    int w;
    int h;
    int pox;
    int poy;
    short fgalu;
    short bgalu;
    short fgmix;
    short bgmix;
    short planemask;
{
    CacheInfoPtr pci = &cInfo[slot];
    int xwmid, ywmid, orig_xwmid;
    int startx, starty, endx, endy;
    int orig_x = x;

    if (w == 0 || h == 0)
	return;

    modulus(x - pox, pci->w, startx);
    modulus(y - poy, pci->h, starty);
    modulus(x - pox + w - 1, pci->w, endx);
    modulus(y - poy + h - 1, pci->h, endy);

    orig_xwmid = xwmid = w - (pci->w - startx + endx + 1);
    ywmid = h - (pci->h - starty + endy + 1);

    WaitQueue(7);
    outw(EXT_SCISSOR_T, 0);
    outw(EXT_SCISSOR_L, 0);
    outw(EXT_SCISSOR_R, mach32MaxX);
    outw(EXT_SCISSOR_B, mach32MaxY);
    outw(FRGD_MIX, fgmix | fgalu);
    outw(BKGD_MIX, bgmix | bgalu);
    outw(WRT_MASK, planemask);

    if (starty + h - 1 < pci->h) {
	if (startx + w - 1 < pci->w) {
	    WaitQueue(7);
	    outw(CUR_X, (short)(pci->x + startx));
	    outw(CUR_Y, (short)(pci->y + starty));
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(w - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h -  1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
	} else {
	    WaitQueue(7);
	    outw(CUR_X, (short)(pci->x + startx));
	    outw(CUR_Y, (short)(pci->y + starty));
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h -  1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    x += pci->w - startx;

	    while (xwmid > 0) {
		WaitQueue(7);
		outw(CUR_X, (short)pci->x);
		outw(CUR_Y, (short)(pci-> y + starty));
		outw(DESTX_DIASTP, (short)x);
		outw(DESTY_AXSTP, (short)y);
		outw(MAJ_AXIS_PCNT, (short)(pci->w - 1));
		outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h -  1));
		outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR |
			   WRTDATA);
		x += pci->w;
		xwmid -= pci->w;
	    }

	    WaitQueue(7);
	    outw(CUR_X, (short)pci->x);
	    outw(CUR_Y, (short)(pci->y + starty));
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)endx);
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(h -  1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
	}
    } else if (startx + w - 1 < pci->w) {
	WaitQueue(7);
	outw(CUR_X, (short)(pci->x + startx));
	outw(CUR_Y, (short)(pci->y + starty));
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)(w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	y += pci->h - starty;

	while (ywmid > 0) {
	    WaitQueue(7);
	    outw(CUR_X, (short)(pci->x + startx));
	    outw(CUR_Y, (short)pci->y);
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(w - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h -  1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    y += pci->h;
	    ywmid -= pci->h;
	}

	WaitQueue(7);
	outw(CUR_X, (short)(pci->x + startx));
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)(w - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
    } else {
	WaitQueue(7);
	outw(CUR_X, (short)(pci->x + startx));
	outw(CUR_Y, (short)(pci->y + starty));
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty -  1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	x += pci->w - startx;

	while (xwmid > 0) {
	    WaitQueue(7);
	    outw(CUR_X, (short)pci->x);
	    outw(CUR_Y, (short)(pci->y + starty));
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT |
				  (short)(pci->h - starty - 1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    x += pci->w;
	    xwmid -= pci->w;
	}

	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)(pci->y + starty));
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)endx);
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - starty - 1));
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	y += pci->h - starty;

	while (ywmid > 0) {
	    x = orig_x;
	    xwmid = orig_xwmid;

	    WaitQueue(7);
	    outw(CUR_X, (short)(pci-> x + startx));
	    outw(CUR_Y, (short)pci->y);
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    x += pci->w - startx;

	    while (xwmid > 0) {
		WaitQueue(7);
		outw(CUR_X, (short)pci->x);
		outw(CUR_Y, (short)pci->y);
		outw(DESTX_DIASTP, (short)x);
		outw(DESTY_AXSTP, (short)y);
		outw(MAJ_AXIS_PCNT, (short)(pci->w - 1));
		outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
		outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR |
			   WRTDATA);

		x += pci->w;
		xwmid -= pci->w;
	    }

	    WaitQueue(7);
	    outw(CUR_X, (short)pci->x);
	    outw(CUR_Y, (short)pci->y);
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)endx);
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)(pci->h - 1));
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    y += pci->h;
	    ywmid -= pci->h;
	}

	x = orig_x;
	xwmid = orig_xwmid;

	WaitQueue(7);
	outw(CUR_X, (short)(pci->x + startx));
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)(pci->w - startx - 1));
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	x += pci->w - startx;

	while (xwmid > 0) {
	    WaitQueue(7);
	    outw(CUR_X, (short)pci->x);
	    outw(CUR_Y, (short)pci->y);
	    outw(DESTX_DIASTP, (short)x);
	    outw(DESTY_AXSTP, (short)y);
	    outw(MAJ_AXIS_PCNT, (short)(pci->w - 1));
	    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
	    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

	    x += pci->w;
	    xwmid -= pci->w;
	}

	WaitQueue(7);
	outw(CUR_X, (short)pci->x);
	outw(CUR_Y, (short)pci->y);
	outw(DESTX_DIASTP, (short)x);
	outw(DESTY_AXSTP, (short)y);
	outw(MAJ_AXIS_PCNT, (short)endx);
	outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (short)endy);
	outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
    }

    WaitQueue(2);
    outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
    outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);

    WaitIdleEmpty(); /* Make sure that all commands have finished */
}
#endif /* PIXPRIV */
