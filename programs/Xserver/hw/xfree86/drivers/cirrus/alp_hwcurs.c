/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp_hwcurs.c,v 1.6 1999/04/25 10:02:07 dawes Exp $ */

/* (c) Itai Nahshon */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"
#include "alp.h"

#define CURSORWIDTH	32
#define CURSORHEIGHT	32
#define CURSORSIZE      (CURSORWIDTH*CURSORHEIGHT/8)

static void
AlpSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef ALP_DEBUG
	ErrorF("AlpSetCursorColors\n");
#endif
#if 1
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]|0x02);
	hwp->writeDacWriteAddr(hwp, 0x00);
	hwp->writeDacData(hwp, 0x3f & (bg >> 18));
	hwp->writeDacData(hwp, 0x3f & (bg >> 10));
	hwp->writeDacData(hwp, 0x3f & (bg >>  2));
	hwp->writeDacWriteAddr(hwp, 0x0F);
	hwp->writeDacData(hwp, 0x3F & (fg >> 18));
	hwp->writeDacData(hwp, 0x3F & (fg >> 10));
	hwp->writeDacData(hwp, 0x3F & (fg >>  2));
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
#else
	outw(0x3C4, ((pAlp->ModeReg.ExtVga[SR12] | 0x02) << 8) | 0x12);
	outb(0x3c8, 0x00); outb(0x3c9, 0x3f & (bg >> 18)); outb(0x3c9, 0x3f & (bg >> 10)); outb(0x3c9, 0x3f & (bg >> 2));
	outb(0x3c8, 0x0f); outb(0x3c9, 0x3f & (fg >> 18)); outb(0x3c9, 0x3f & (fg >> 10)); outb(0x3c9, 0x3f & (fg >> 2));
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR12] << 8) | 0x12);
#endif
}

static void
AlpLoadSkewedCursor(unsigned char *memx, unsigned char *CursorBits,
					int x, int y)
{
	unsigned char mem[2*CURSORSIZE];
	unsigned char *p1, *p2;
	int i, j, m, a, b;

	if (x > 0) x = 0; else x = -x;
	if (y > 0) y = 0; else y = -y;

	a = (x+y*CURSORWIDTH)>>3;
	b = x & 7;

	/* Copy the skewed mask bits */
	p1 = mem;
	p2 = CursorBits+a;
	for (i = 0; i < CURSORSIZE-a-1; i++) {
		*p1++ = (p2[0] << b) | (p2[1] >> (8-b));
		p2++;
	}
	/* last mask byte */
	*p1++ = (p2[0] << b);

	/* Clear to end (bottom) of mask. */
	for (i = i+1; i < CURSORSIZE; i++)
		*p1++ = 0;

	/* Now copy the cursor bits */
	/* p1 is already right */
	p2 = CursorBits+CURSORSIZE+a;
	for (i = 0; i < CURSORSIZE-a-1; i++) {
		*p1++ = (p2[0] << b) | (p2[1] >> (8-b));
		p2++;
	}
	/* last cursor  byte */
	*p1++ = (p2[0] << b);

	/* Clear to end (bottom) of cursor. */
	for (i = i+1; i < CURSORSIZE; i++)
		*p1++ = 0;

	/* Clear the right unused area of the mask
	and cyrsor bits.  */
	p2 = mem + CURSORWIDTH/8 - (x>>3) - 1;
	for (i = 0; i < 2*CURSORHEIGHT; i++) {
		m = (-1)<<(x&7);
		p1 = p2;
		p2 += CURSORWIDTH/8;
		for (j = x>>3; j >= 0; j--) {
			*p1 &= m;
			m = 0;
			p1++;
		}
	}
	memcpy(memx, mem, 2*CURSORSIZE);
}


static void
AlpSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#if 0
#ifdef ALP_DEBUG
	ErrorF("AlpSetCursorPosition %d %d\n", x, y);
#endif
#endif

	if (x < 0 || y < 0) {
		if (x+CURSORWIDTH <= 0 || y+CURSORHEIGHT <= 0) {
#if 1
			hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12] & ~0x01);
#else
			outw(0x3C4, ((pAlp->ModeReg.ExtVga[SR12] & ~0x01) << 8) | 0x12);
#endif
			return;
		}
		AlpLoadSkewedCursor(pAlp->HWCursorBits, pAlp->CursorBits, x, y);
		pAlp->CirRec.CursorIsSkewed = TRUE;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	} else if (pAlp->CirRec.CursorIsSkewed) {
		memcpy(pAlp->HWCursorBits, pAlp->CursorBits, 2*CURSORSIZE);
		pAlp->CirRec.CursorIsSkewed = FALSE;
	}
#if 1
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
	hwp->writeSeq(hwp, ((x << 5)|0x10)&0xff, x >> 3);
	hwp->writeSeq(hwp, ((y << 5)|0x11)&0xff, y >> 3);
#else
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR12] << 8) | 0x12);
	outw(0x3C4, (x << 5) | 0x10);
	outw(0x3C4, (y << 5) | 0x11);
#endif
}

static void
AlpLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpLoadCursorImage\n");
#endif

	pAlp->CursorBits = bits;
	memcpy(pAlp->HWCursorBits, bits, 2*CURSORSIZE);
	pAlp->ModeReg.ExtVga[SR13] = 0x3f;
#if 1
	hwp->writeSeq(hwp, 0x13, 0x3f);
#else
	outw(0x3C4, 0x3f13);
#endif
}

static void
AlpHideCursor(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpHideCursor\n");
#endif
	pAlp->ModeReg.ExtVga[SR12] &= ~0x01;
#if 1
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
#else
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR12] << 8) | 0x12);
#endif
}

static void
AlpShowCursor(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpShowCursor\n");
#endif
	pAlp->ModeReg.ExtVga[SR12] |= 0x01;
#if 1
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
#else
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR12] << 8) | 0x12);
#endif
}

static Bool
AlpUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
#ifdef ALP_DEBUG
	ErrorF("AlpUseHWCursor\n");
#endif
	if (pScrn->bitsPerPixel < 8)
		return FALSE;

	return TRUE;
}

Bool
AlpHWCursorInit(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	AlpPtr pAlp = ALPPTR(pScrn);
	xf86CursorInfoPtr infoPtr;

#ifdef ALP_DEBUG
	ErrorF("AlpHWCursorInit\n");
#endif

	infoPtr = xf86CreateCursorInfoRec();
	if (!infoPtr) return FALSE;

	pAlp->CirRec.CursorInfoRec = infoPtr;
	pAlp->HWCursorBits = pAlp->CirRec.FbBase + 1024*pScrn->videoRam - 2*CURSORSIZE;
	pAlp->CirRec.CursorIsSkewed = FALSE;
	pAlp->CursorBits = NULL;

	infoPtr->MaxWidth = CURSORWIDTH;
	infoPtr->MaxHeight = CURSORHEIGHT;
	infoPtr->Flags =
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
						HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
#endif
						HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
						HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED;
	infoPtr->SetCursorColors = AlpSetCursorColors;
	infoPtr->SetCursorPosition = AlpSetCursorPosition;
	infoPtr->LoadCursorImage = AlpLoadCursorImage;
	infoPtr->HideCursor = AlpHideCursor;
	infoPtr->ShowCursor = AlpShowCursor;
	infoPtr->UseHWCursor = AlpUseHWCursor;
	infoPtr->RealizeCursor = NULL;

#ifdef ALP_DEBUG
	ErrorF("AlpHWCursorInit before xf86InitCursor\n");
#endif

	return(xf86InitCursor(pScreen, infoPtr));
}
