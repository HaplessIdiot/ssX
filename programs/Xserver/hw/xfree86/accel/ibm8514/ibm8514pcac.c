/* $XConsortium: ibm8514pcac.c,v 1.1 94/03/28 21:05:33 dpw Exp $ */
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
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


/*       Offscreen memory organization:
 *
 *       128      128                         768
 *     +-------+-------+-----------------------------------------------+
 *     |       |       |                                               |
 * 128 |   0   |   1   |                                               |
 *     |       |       |                                               |
 *     +---+---+---+---+                     FONTS                     | 256
 *  64 | 2 | 3 | 4 | 5 |                                               |
 *     +---+---+---+---+                                               |
 *  64 | 6 | 7 | 8 | 9 |                                               |
 *     +---+---+---+---+-----------------------------------------------+
 *
 *     \_______________/
 *       Pixmap Cache
 */

#ifdef PIXPRIV
#include "reg8514.h"
#include "ibm8514im.h"
#include "pixmapstr.h"
#include "ibm8514.h"
#include "micache.h"

#define NEXT_CACHE_ID	CacheIDNum++;

#define	MAX_SLOTS	10
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

void DoCacheExpandPixmap();
void DoCacheTile();
void DoCacheOpStipple();
void DoCacheImageFill();
void ibm8514CacheFreeSlot();

void
ibm8514CacheInit(w, h)
    int w;
    int h;
{
    int i;

    for (i = 0; i < MAX_SLOTS; i++) {
	cInfo[i].id = -1;
	cInfo[i].lru = 0;
    }

    cInfo[0].x =    0; cInfo[0].y =  h;
    cInfo[1].x =  128; cInfo[1].y =  h;
    cInfo[2].x =    0; cInfo[2].y =  h+128;
    cInfo[3].x =   64; cInfo[3].y =  h+128;
    cInfo[4].x =  128; cInfo[4].y =  h+128;
    cInfo[5].x =  192; cInfo[5].y =  h+128;
    cInfo[6].x =    0; cInfo[6].y =  h+192;
    cInfo[7].x =   64; cInfo[7].y =  h+192;
    cInfo[8].x =  128; cInfo[8].y =  h+192;
    cInfo[9].x =  192; cInfo[9].y =  h+192;

    miCacheFreeSlot = ibm8514CacheFreeSlot;
}

void
ibm8514CacheFreeSlot(pix)
    PixmapPtr pix;
{
    if (pix->slot >= 0 && pix->slot <= MAX_SLOTS &&
	cInfo[pix->slot].id == pix->cacheId) {
	cInfo[pix->slot].id = -1;
    }
}

int
DoCachePixmap(pix, cacher)
    PixmapPtr pix;
    void (*cacher)();
{
    int i, next = FIRST_128_SLOT;

    if (pix->slot >= 0 && pix->slot <= MAX_SLOTS &&
	cInfo[pix->slot].id == pix->cacheId)
	return 1;

    if (pix->drawable.width <= 64 && pix->drawable.height <= 64) {
	next = MAX_SLOTS-1;
	for (i = FIRST_64_SLOT; i < MAX_SLOTS; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 64);
		return 1;
	    } else if (cInfo[i].lru > cInfo[next].lru)
		next = i;
    }

    if (pix->drawable.width <= 128 && pix->drawable.height <= 128) {
	for (i = FIRST_128_SLOT; i < FIRST_64_SLOT; i++)
	    if (cInfo[i].id == -1) {
		pix->slot = i;
		(*cacher)(pix, 128);
		return 1;
	    } else if (cInfo[i].lru > cInfo[next].lru)
		next = i;
    } else
	return 0;

    pix->slot = next;
    (*cacher)(pix, (next < FIRST_64_SLOT ? 128 : 64));
    return 1;
}

int
ibm8514CacheTile(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheTile);
}

int
ibm8514CacheStipple(pix)
    PixmapPtr pix;
{
    return DoCachePixmap(pix, DoCacheOpStipple);
}

int
ibm8514CacheOpStipple(pix)
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

    ibm8514ImageWrite(pci->x, pci->y, pci->pix_w, pci->pix_h,
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

    ibm8514ImageStipple(pci->x, pci->y, pci->pix_w, pci->pix_h,
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
ibm8514CImageFill(slot, x, y, w, h, pox, poy, alu, planemask)
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

    for (i = 0; i < MAX_SLOTS; i++)
	if (i != slot)
	    cInfo[i].lru++;
}

void
ibm8514CImageStipple(slot, x, y, w, h, pox, poy, fg, alu, planemask)
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

    for (i = 0; i < MAX_SLOTS; i++)
	if (i != slot)
	    cInfo[i].lru++;
}

void
ibm8514CImageOpStipple(slot, x, y, w, h, pox, poy, fg, bg, alu, planemask)
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

    for (i = 0; i < MAX_SLOTS; i++)
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
