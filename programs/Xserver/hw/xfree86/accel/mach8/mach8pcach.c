/* $XConsortium: mach8pcach.c,v 1.1 94/03/28 21:12:30 dpw Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
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
 * AND FITNESS, IN NO EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified for the Mach-8 by Rickard E. Faith
 * Small fix to enable use without font or pixmap cache.
 * Hans Nasten. (nasten@everyware.se)
 *
 */


/*       Offscreen memory organization at 1024x768:
 *
 *       128     64 64                        768
 *     +-------+---+---+-----------------------------------------------+
 *     |       | 2 | 3 |                                               |
 * 128 |   0   +---+---+                                               |
 *     |       | 4 | 5 |                                               |
 *     +-------+---+---+                     FONTS                     | 256
 *     |       | 6 | 7 |                                               |
 * 128 |   1   +---+---+                                               |
 *     |       | 8 | 9 |                                               |
 *     +-------+---+---+-----------------------------------------------+
 *
 *     \_______________/
 *       Pixmap Cache
 */

#ifdef PIXPRIV
#include "regmach8.h"
#include "mach8im.h"
#include "pixmapstr.h"
#include "mach8.h"
#include "micache.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
extern Bool xf86Verbose;

#define NEXT_CACHE_ID	CacheIDNum++;

#define	MAX_SLOTS	128
#define	FIRST_128_SLOT	0
#define	FIRST_64_SLOT	2

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

static CacheInfo cInfo[MAX_SLOTS];
static int CacheIDNum = 1;
static int CacheSlots = 0;
static int CacheFirst128 = 0;
static int CacheFirst64 = 0;

void DoCacheExpandPixmap();
void DoCacheTile();
void DoCacheOpStipple();
void DoCacheImageFill();
void mach8CacheFreeSlot();


/*
 *  The 8514/a, which this code was written for only had 1 Mb of memory 
 *  available to it, and always treated display memory as a 1024x1024
 *  array of pixels.
 *
 *  The Mach8 shares the 1Mb limit, but it can vary the logical screen
 *  width.  The Mach32 can support up to 4Mb of video memory, so
 *  A 1024x1024 pixel restriction is pretty silly there.  This server
 *  adjusts the logical screen width to whatever the virtual X resolution
 *  is.  That should give us a lot more memory to play with.
 *
 *  What we do is give the right 3/4 of the top 256 rows of offscreen
 *  memory to the font cache, and divide the rest between 128x128
 *  pixmap caches and 64x64 pixmap caches.
 *
 */

void
mach8CacheInit(void)
{
    int i,l;
    int totalx,totaly,freey,freex;
    int count128 = 0,count64 = 0;
    int next=0;
  
    if( mach8InfoRec.videoRam > 512 )
	totalx = 1024;
    else
	totalx = mach8InfoRec.virtualX;

    totaly = (mach8InfoRec.videoRam*1024) / totalx; 

    if (totaly>1024) totaly=1024;

    freex = totalx;
    freey = totaly-mach8InfoRec.virtualY;
  
    if (xf86Verbose) {
	if( mach8InfoRec.videoRam > 512 && mach8InfoRec.virtualX < 1024 )
	    ErrorF("%s %s: Total memory %dx%d (%dx%d + %dx%d in cache)\n",
		   XCONFIG_PROBED, mach8InfoRec.name,
		   totalx, totaly, freex, freey,
		   1024 - mach8InfoRec.virtualX, mach8InfoRec.virtualY);
	else
	    ErrorF("%s %s: Total memory %dx%d (%dx%d in cache)\n",
		   XCONFIG_PROBED, mach8InfoRec.name,
		   totalx, totaly, freex, freey);
    }
    for (i = 0; i < MAX_SLOTS; i++) {
	cInfo[i].id = -2;
	cInfo[i].lru = 0;
    }
  
    /*
     * Okay, we'll give the font cache the right 3/4 of the display, with
     * up to 256 rows.  Below 256 rows, we'll use everything for the 
     * 64-pixel pixmap cache.
     */
   
    if((freex>>2)>=128)
      {
      for(i=0; i< (freey>>7); i++)
      {
	cInfo[next].id=-1;
        cInfo[next].x=0;
        cInfo[next].y=mach8InfoRec.virtualY+(128*i);
        next++;
        count128++;
      }
    }
  
    CacheFirst64=next;  

    for(i=0;i<(freey>>6);i++)
      {
	for(l=0;l<((freex>>8)-2);l++)
	  {
	    cInfo[next].id=-1;
	    cInfo[next].x=128+l*64;
	    cInfo[next].y=mach8InfoRec.virtualY+(64*i);
	    next++;
	    count64++;
	  }
      }

    if(freey>=320)
      {
	for(i=0;i<((freex-(freex>>2))>>6);i++)
	  for(l=0;l<(freey-256)>>6;l++)
	    {
	      cInfo[next].id=-1;
	      cInfo[next].x=(freex>>2)+64*i;
	      cInfo[next].y=mach8InfoRec.virtualY+256+(64*l);
	      next++;
	      count64++;
	    }
      }

   CacheSlots=next;


   if (xf86Verbose)
       ErrorF("%s %s: Cache Slots %dx128, %dx64 Total: %d\n", XCONFIG_PROBED,
	      mach8InfoRec.name, count128, count64, CacheSlots);

#if 0
    for(i=0;i<CacheSlots;i++)
      {
	ErrorF("Cache Slot %d at (%d,%d)\n",i,cInfo[i].x,cInfo[i].y);
      }
#endif
    
    miCacheFreeSlot = mach8CacheFreeSlot;
}

void
mach8CacheFreeSlot(pix)
    PixmapPtr pix;
{
    if (pix->slot >= 0 && pix->slot <= CacheSlots &&
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
    int i, next = FIRST_128_SLOT;

    if (pix->slot >= 0 && pix->slot <= CacheSlots &&
	cInfo[pix->slot].id == pix->cacheId)
	return 1;

    if( !CacheSlots )
	return( 0 );

    if (pix->drawable.width <= 64 && pix->drawable.height <= 64) {
	next = CacheSlots-1;
	for (i = CacheFirst64; i < CacheSlots; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 64);
		return 1;
	    } else if (cInfo[i].lru > cInfo[next].lru)
		next = i;
    }

    if (pix->drawable.width <= 128 && pix->drawable.height <= 128) {
	for (i = CacheFirst128; i < CacheFirst64; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 128);
		return 1;
	    } else if (cInfo[i].lru > cInfo[next].lru)
		next = i;
    } else
	return 0;

    pix->slot = next;
    (*cacher)(pix, (next < CacheFirst64 ? 128 : 64));
    return 1;
}

int
mach8CacheTile(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheTile);
}

int
mach8CacheStipple(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheOpStipple);
}

int
mach8CacheOpStipple(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheOpStipple);
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
    pci->lru = 0;
    if (pix->cacheId)
	pci->id = pix->cacheId;
    else
	pci->id = pix->cacheId = NEXT_CACHE_ID;

    (mach8ImageWriteFunc)(pci->x, pci->y, pci->pix_w, pci->pix_h,
			  pix->devPrivate.ptr, pix->devKind, 0, 0,
			  MIX_SRC, 0xffff);

    DoCacheExpandPixmap(pci);
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
    pci->lru = 0;
    if (pix->cacheId)
	pci->id = pix->cacheId;
    else
	pci->id = pix->cacheId = NEXT_CACHE_ID;

    (mach8ImageStippleFunc)(pci->x, pci->y, pci->pix_w, pci->pix_h,
			    pix->devPrivate.ptr, pix->devKind,
			    pci->pix_w, pci->pix_h, pci->x, pci->y,
			    255, 0, MIX_SRC, 0xffff, 1);

    DoCacheExpandPixmap(pci);
}

void
DoCacheExpandPixmap(pci)
    CacheInfoPtr pci;
{
    int cur_w = pci->pix_w;
    int cur_h = pci->pix_h;

    WaitQueue(7);
    outw(MULTIFUNC_CNTL, SCISSORS_T | 0);
    outw(MULTIFUNC_CNTL, SCISSORS_L | 0);
    outw(MULTIFUNC_CNTL, SCISSORS_R | 1023);
    outw(MULTIFUNC_CNTL, SCISSORS_B | 1023);
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
mach8CImageFill(slot, x, y, w, h, pox, poy, alu, planemask)
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

    for (i = 0; i < CacheSlots; i++)
	if (i != slot)
	    cInfo[i].lru++;
}

void
mach8CImageStipple(slot, x, y, w, h, pox, poy, fg, alu, planemask)
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

    for (i = 0; i < CacheSlots; i++)
	if (i != slot)
	    cInfo[i].lru++;
}

void
mach8CImageOpStipple(slot, x, y, w, h, pox, poy, fg, bg, alu, planemask)
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

    for (i = 0; i < CacheSlots; i++)
	if (i != slot)
	    cInfo[i].lru++;
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
    outw(MULTIFUNC_CNTL, SCISSORS_T | 0);
    outw(MULTIFUNC_CNTL, SCISSORS_L | 0);
    outw(MULTIFUNC_CNTL, SCISSORS_R | 1023);
    outw(MULTIFUNC_CNTL, SCISSORS_B | 1023);
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
}
#endif /* PIXPRIV */
