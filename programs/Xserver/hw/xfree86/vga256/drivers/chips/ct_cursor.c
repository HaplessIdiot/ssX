/*
 *
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Simon P. Cooper not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Simon P. Cooper makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SIMON P. COOPER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SIMON P. COOPER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 */

/* $XFree86$ */

/*
 * Hardware cursor handling. Adapted from cirrus/cir_cursor.c and
 * ark/ark_cursor.c.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"
#include "ct_driver.h"

static Bool CHIPSRealizeCursor();
static Bool CHIPSUnrealizeCursor();
static void CHIPSSetCursor();
static void CHIPSMoveCursor();
static void CHIPSRecolorCursor();
static void CHIPSHideCursor();

#ifdef DEBUG
unsigned char *hex2bmp(unsigned char, unsigned char *);

#endif

static miPointerSpriteFuncRec CHIPSPointerSpriteFuncs =
{
    CHIPSRealizeCursor,
    CHIPSUnrealizeCursor,
    CHIPSSetCursor,
    CHIPSMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int ctCursorGeneration = -1;
Bool ctHWcursorShown = FALSE;
int ctRealizedCursorCount = 0;

/*
 * This is the set variables that defines the cursor state within the
 * driver.
 */

int ctCursorHotX = 0;
int ctCursorHotY = 0;
int ctCursorClipX = 1;
int ctCursorClipY = 1;
int ctCursorWidth;
int ctCursorHeight;
static CursorPtr ctCursorpCurs;

static unsigned char byte_reversed[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

Bool
CHIPSCursorInit(pm, pScr)
    char *pm;
    ScreenPtr pScr;
{

    CHIPSHideCursor();
    if (ctCursorGeneration != serverGeneration) {
	if (!(miPointerInitialize(pScr, &CHIPSPointerSpriteFuncs,
		    &xf86PointerScreenFuncs, FALSE)))
	    return FALSE;
    }
    ctCursorGeneration = serverGeneration;

    /* 1kB alignment. Note this allocation is now done in FbInit. */
#if 0
    ctCursorAddress = (((vga256InfoRec.videoRam << 10) - ctFrameBufferSize) 
       - 2048 + 0x3FF) & 0xFFFFFC00;
#endif
    /* load address to card when cursor is loaded to card */
    if (ctisHiQV32) {
	outb(0x3D6, 0xA2);
	outb(0x3D7, (ctCursorAddress >> 8) & 0xFF);
	outb(0x3D6, 0xA3);
	outb(0x3D7, (ctCursorAddress >> 16) & 0x3F);
    } else
	outl(0xB3D0, ctCursorAddress);

    return TRUE;
}

void
CHIPSShowCursor()
{
    /* turn the cursor on */
    if (ctisHiQV32)
	outw(0x3D6, 0x11A0);
    else
	outw(0xA3D0, 0x21);
    ctHWcursorShown = TRUE;
}

void
CHIPSHideCursor()
{
    /* turn the cursor off */
    if (ctisHiQV32)
	outw(0x3D6, 0x10A0);
    else
	outw(0xA3D0, 0x20);
    ctHWcursorShown = FALSE;
}

/*
 * This function is called when a new cursor image is requested by
 * the server. The main thing to do is convert the bitwise image
 * provided by the server into a format that the graphics card
 * can conveniently handle, and store that in system memory.
 * Adapted from ark/ark_cursor.c.
 */

static Bool
CHIPSRealizeCursor(pScr, pCurs)
    ScreenPtr pScr;
    CursorPtr pCurs;
{
    register int i, j, k;
    unsigned char *pServMsk;
    unsigned char *pServSrc;
    int index = pScr->myNum;
    pointer *pPriv = &pCurs->bits->devPriv[index];
    int wsrc, h;
    unsigned char *ram;
    CursorBitsPtr bits = pCurs->bits;

#ifdef DEBUG
    unsigned char bmp[] = "12345678";

#endif

    if (pCurs->bits->refcnt > 1)
	return TRUE;

    ram = (unsigned char *)xalloc(1024);
    *pPriv = (pointer) ram;
    if (!ram)
	return FALSE;

    pServSrc = (unsigned char *)bits->source;
    pServMsk = (unsigned char *)bits->mask;

#define MAX_CURS 32

    ctCursorWidth = bits->width;
    ctCursorHeight = h = bits->height;

    if (h > MAX_CURS)
	h = MAX_CURS;

    wsrc = PixmapBytePad(bits->width, 1);	/* Bytes per line. */

    if (!ctisHiQV32) {
	for (i = 0; i < MAX_CURS; i++) {
	    for (j = 0; j < MAX_CURS / 8; j++) {
		unsigned char mask, source;

		if (i < h && j < wsrc) {
		    mask = *pServMsk++;
		    source = *pServSrc++;

		    if (j < MAX_CURS / 8) {
			*ram++ = ~byte_reversed[mask];	/*chip depend */
			*ram++ = byte_reversed[source];		/*chip depend */
#ifdef DEBUG
			ErrorF("m:%s\n", hex2bmp(mask, bmp));
			ErrorF("s:%s\n", hex2bmp(source, bmp));
#endif
		    }
		} else {
		    *ram++ = 0xFF;     /*chip depend */
		    *ram++ = 0x00;     /*chip depend */
		}
	    }
	    /*
	     * if we still have more bytes on this line (j < wsrc),
	     * we have to ignore the rest of the line.
	     */
	    while (j++ < wsrc)
		pServMsk++, pServSrc++;
	}
    } else {
	for (i = 0; i < MAX_CURS; i += 2) {
	    for (j = 0; j < 2; j++) {
		for (k = 0; k < MAX_CURS / 8; k++) {
		    unsigned char mask;

		    if ((i + j) < h && k < wsrc) {
			mask = *pServMsk++;

			*ram++ = ~byte_reversed[mask];	/*chip depend */
#ifdef DEBUG
			ErrorF("m:%s\n", hex2bmp(mask, bmp));
#endif
		    } else {
			*ram++ = 0xFF; /*chip depend */
		    }
		}
		/*
		 * if we still have more bytes on this line (j < wsrc),
		 * we have to ignore the rest of the line.
		 */
		while (k++ < wsrc)
		    pServMsk++;
	    }
	    for (j = 0; j < 2; j++) {
		for (k = 0; k < MAX_CURS / 8; k++) {
		    unsigned char source;

		    if ((i + j) < h && k < wsrc) {
			source = *pServSrc++;

			*ram++ = byte_reversed[source];		/*chip depend */
#ifdef DEBUG
			ErrorF("s:%s\n", hex2bmp(source, bmp));
#endif
		    } else {
			*ram++ = 0x00; /*chip depend */
		    }
		}
		/*
		 * if we still have more bytes on this line (j < wsrc),
		 * we have to ignore the rest of the line.
		 */
		while (k++ < wsrc)
		    pServSrc++;
	    }
	}
    }

    ctRealizedCursorCount++;	       /* to erase cursor when exit the server */

    return TRUE;
}

/*
 * This is called when a cursor is no longer used. The intermediate
 * cursor image storage that we created needs to be deallocated.
 */
static Bool
CHIPSUnrealizeCursor(pScr, pCurs)
    ScreenPtr pScr;
    CursorPtr pCurs;
{
    pointer priv;

    if (pCurs->bits->refcnt <= 1 &&
	(priv = pCurs->bits->devPriv[pScr->myNum])) {
	xfree(priv);
	pCurs->bits->devPriv[pScr->myNum] = 0x0;
    }
    if (--ctRealizedCursorCount == 0) {		/* count down to erase cursor when exit the server */
	if (ctisHiQV32) {
	    outb(0x3D6, 0x20);
	    while (inb(0x3D7) & 0x1) {
	    };
	} else {
	    while (inw(0x93D2) & 0x10) {
	    };
	}
	CHIPSHideCursor();
    }
    return TRUE;
}

CHIPSLoadCursorToCard(pScr, pCurs, x, y)
    ScreenPtr pScr;
    CursorPtr pCurs;
    int x, y;
{
    unsigned char *cursor_image;
    int index = pScr->myNum;

    if (!xf86VTSema)
	return;

    /* Check if blitter is active. Mustn't touch the video ram till it is finished */
    if (ctisHiQV32) {
	outb(0x3D6, 0x20);
	while (inb(0x3D7) & 0x1) {
	};
    } else {
	while (inw(0x93D2) & 0x10) {
	};
    }

    cursor_image = pCurs->bits->devPriv[index];

    if (ctLinearSupport) {
#ifdef DEBUG
	ErrorF("CHIPSLoadCursorToCard: memcpy to 0x%X\n",
	    (unsigned char *)vgaLinearBase + ctCursorAddress);
#endif
	memcpy((unsigned char *)vgaLinearBase + ctCursorAddress,
	    cursor_image, 256);
    } else {
	/*
	 * The cursor can only be in the last 16K of video memory,
	 * which fits in the last banking window.
	 */
	vgaSaveBank();
	if (ctisHiQV32) {
	    CHIPSHiQVSetWrite(ctCursorAddress >> 16);
	    memcpy((unsigned char *)vgaBase + (ctCursorAddress & 0xFFFF),
		cursor_image, 256);
	    outb(0x3D6, 0xA2);
	    outb(0x3D7, ((unsigned int)((ctCursorAddress & 0xFFFF) +
			(unsigned char *)vgaBase) >> 8) & 0xFF);
	    outb(0x3D6, 0xA3);
	    outb(0x3D7, ((unsigned int)((ctCursorAddress & 0xFFFF) +
			(unsigned char *)vgaBase) >> 16) & 0x3F);
	} else {
	    CHIPSSetWrite(ctCursorAddress >> 16);
	    memcpy((unsigned char *)vgaBase + (ctCursorAddress & 0xFFFF),
		cursor_image, 256);
	    outl(0xB3D0,
		 (int)((unsigned char *)vgaBase + (ctCursorAddress & 0xFFFF)));
	}
	vgaRestoreBank();

    }
    /* set cursor address here or we loose the cursor on video mode change */
    if (ctisHiQV32) {
	outb(0x3D6, 0xA2);
	outb(0x3D7, (ctCursorAddress >> 8) & 0xFF);
	outb(0x3D6, 0xA3);
	outb(0x3D7, (ctCursorAddress >> 16) & 0x3F);
    } else
	outl(0xB3D0, ctCursorAddress);

}

/*
 * This function should make the graphics chip display new cursor that
 * has already been "realized". We need to upload it to video memory,
 * make the graphics chip display it.
 * This is a local function that is not called from outside of this
 * module (although it largely corresponds to what the SetCursor
 * function in the Pointer record needs to do).
 */
static void
CHIPSLoadCursor(pScr, pCurs, x, y)
    ScreenPtr pScr;
    CursorPtr pCurs;
    int x, y;
{
    if (!xf86VTSema)
	return;

    if (!pCurs)
	return;

    /* Remember the cursor currently loaded into this cursor slot. */
    ctCursorpCurs = pCurs;

    CHIPSHideCursor();

    CHIPSLoadCursorToCard(pScr, pCurs, x, y);

    CHIPSRecolorCursor(pScr, pCurs, 1);

    /* Position cursor */
    CHIPSMoveCursor(pScr, x, y);

    if (ctisHiQV32) {
	outb(0x3D6, 0x20);
	while (inb(0x3D7) & 0x1) {
	};
    } else {
	while (inw(0x93D2) & 0x10) {
	};
    }

    /* Turn it on. */
    CHIPSShowCursor();
}

static void
CHIPSSetCursor(pScr, pCurs, x, y, generateEvent)
    ScreenPtr pScr;
    CursorPtr pCurs;
    int x, y;
    Bool generateEvent;
{
    if (!pCurs)
	return;

    ctCursorHotX = pCurs->bits->xhot;
    ctCursorHotY = pCurs->bits->yhot;

    ctCursorClipX = -1 - ctCursorHotX;
    ctCursorClipY = -1 - ctCursorHotY;

    CHIPSLoadCursor(pScr, pCurs, x, y);
}

/*
 * This function should display a new cursor at a new position.
 */
void
CHIPSRestoreCursor(pScr)
    ScreenPtr pScr;
{
    int index = pScr->myNum;
    int x, y;

    miPointerPosition(&x, &y);

    CHIPSLoadCursor(pScr, ctCursorpCurs, x, y);
}

/*
 * This function should redisplay a cursor that has been
 * displayed earlier. It is called by the SVGA server.
 */
static void
CHIPSMoveCursor(pScr, x, y)
    ScreenPtr pScr;
    int x, y;
{
    if (!xf86VTSema)
	return;

    x -= vga256InfoRec.frameX0 + ctCursorHotX;
    y -= vga256InfoRec.frameY0 + ctCursorHotY;

    /*
     * If the cursor is partly out of screen at the left or top,
     * we need set the origin.
     */

    if (x < ctCursorClipX) {
	x = (ctCursorHotX + 1) | 0x8000;
    } else if (x < 0) {
	x = ~(x - 1) | 0x8000;
    }
    if (y < ctCursorClipY) {
	y = (ctCursorHotY + 1) | 0x8000;
    } else if (y < 0) {
	y = ~(y - 1) | 0x8000;
    }
    if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
	y *= 2;

    /* Program the cursor origin (offset into the cursor bitmap). */
    if (ctisHiQV32) {
	outw(0x3D6, ((x & 0xFF) << 8) | 0xA4);
	outw(0x3D6, (x & 0x8700) | 0xA5);
	outw(0x3D6, ((y & 0xFF) << 8) | 0xA6);
	outw(0x3D6, (y & 0x8700) | 0xA7);
    } else {
	unsigned long xy;

	xy = y;
	xy = (xy << 16) | x;
	outl(0xAFD0, xy);
    }

}

/*
 * This is a local function that programs the colors of the cursor
 * on the graphics chip.
 * Adapted from accel/s3/s3Cursor.c.
 */
static void
CHIPSRecolorCursor(pScr, pCurs, displayed)
    ScreenPtr pScr;
    CursorPtr pCurs;
    Bool displayed;
{
    ColormapPtr pmap;
    unsigned long packedcolfg, packedcolbg;
    xColorItem sourceColor, maskColor;

    if (!xf86VTSema)
	return;

    if (ctisHiQV32) {
	unsigned char xr80;

	outb(0x3D6, 0x80);
	xr80 = inb(0x3D7);
	outb(0x3D7, xr80 | 0x1);       /* Enable extended palette addressing */

	/* Write the new colours to the extended VGA palette. Palette
	 * index is incremented after each write, so only write index
	 * once 
	 */
	outb(0x3C8, 0x4);
	outb(0x3C9, (pCurs->backRed >> 10) & 0xFF);
	outb(0x3C9, (pCurs->backGreen >> 10) & 0xFF);
	outb(0x3C9, (pCurs->backBlue >> 10) & 0xFF);
	outb(0x3C9, (pCurs->foreRed >> 10) & 0xFF);
	outb(0x3C9, (pCurs->foreGreen >> 10) & 0xFF);
	outb(0x3C9, (pCurs->foreBlue >> 10) & 0xFF);
	outb(0x3D6, 0x80);
	outb(0x3D7, xr80);	       /* Enable normal palette addressing */
    } else {
	if (xf86weight.green == 5) {
	    packedcolbg = ((pCurs->backRed & 0xf800) >> 1)
		| ((pCurs->backGreen & 0xf800) >> 6)
		| ((pCurs->backBlue & 0xf800) >> 11);
	    packedcolfg = ((pCurs->foreRed & 0xf800) >> 1)
		| ((pCurs->foreGreen & 0xf800) >> 6)
		| ((pCurs->foreBlue & 0xf800) >> 11);
	} else {
	    packedcolfg = ((pCurs->foreRed & 0xf800) >> 0)
		| ((pCurs->foreGreen & 0xfc00) >> 5)
		| ((pCurs->foreBlue & 0xf800) >> 11);
	    packedcolbg = ((pCurs->backRed & 0xf800) >> 0)
		| ((pCurs->backGreen & 0xfc00) >> 5)
		| ((pCurs->backBlue & 0xf800) >> 11);
	}
	packedcolfg = (packedcolfg << 16) | packedcolbg;
	outl(0xA7D0, packedcolfg);
    }
}

void
CHIPSWarpCursor(pScr, x, y)
    ScreenPtr pScr;
    int x, y;
{
    miPointerWarpCursor(pScr, x, y);
    xf86Info.currentScreen = pScr;
}

void
CHIPSQueryBestSize(class, pwidth, pheight)
    int class;
    short *pwidth;
    short *pheight;
{
    if (*pwidth > 0) {
	switch (class) {
	case CursorShape:
	    if (*pwidth > ctCursorWidth)
		*pwidth = ctCursorWidth;
	    if (*pheight > ctCursorHeight)
		*pheight = ctCursorHeight;
	    break;

	default:
	    (void)mfbQueryBestSize(class, pwidth, pheight);
	    break;
	}
    }
}

#ifdef DEBUG
unsigned char *
hex2bmp(unsigned char val, unsigned char *bmp)
{
    unsigned char *ret = bmp;
    int i;

    for (i = 0; i < 8; i++) {
	*bmp++ = (val & (0x80 >> i)) ? '1' : '.';
    }
    return ret;
}
#endif
