/*
 * Copyright 1992-1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_regs.h,v 1.1 1998/09/06 13:48:00 dawes Exp $ */

/* General Registers */
#define SPR	0x1F		/* Software Programming Register (videoram) */

/* 3C4 */
#define ConfPort 0x0C
#define NewMode2 0x0D
#define OldMode2 0x0D
#define OldMode1 0x0E
#define NewMode1 0x0E

/* 3x4 */
#define RevisionID 0x09
#define VCLK_A 0x18
#define VCLK_B 0x19
#define CRTCModuleTest 0x1E
#define FIFOControl 0x20
#define LinearAddReg 0x21
#define DRAMTiming 0x23
#define CRTHiOrd 0x27
#define AddColReg 0x29
#define InterfaceSel 0x2A
#define Performance 0x2F
#define GraphEngReg 0x36
#define PixelBusReg 0x38
#define PCIReg 0x39
#define DRAMControl 0x3A
#define PCIRetry 0x62
#define TVinterface 0xC0
#define TVMode 0xC1
#define ClockControl 0xCF

/* 3CE */
#define MiscExtFunc 0x0F
#define MiscIntContReg 0x2F

/* Graphics Engine for 9420/9430 */

#define GER_INDEX	0x210A
#define GER_BYTE0	0x210C
#define GER_BYTE1	0x210D
#define GER_BYTE2	0x210E
#define GER_BYTE3	0x210F
#define MMIOBASE	0x7C
#define OLDGER_STATUS	0x90
#define OLDGER_MWIDTH	0xB8
#define OLDGER_MFORMAT	0xBC
#define OLDGER_STYLE	0xC4
#define OLDGER_FMIX	0xC8
#define OLDGER_BMIX	0xC8
#define OLDGER_FCOLOUR	0xD8
#define OLDGER_BCOLOUR	0xDC
#define OLDGER_DIMXY	0xE0
#define OLDGER_DESTLINEAR	0xE4
#define OLDGER_DESTXY	0xF8
#define OLDGER_COMMAND	0xFC
#define		OLDGE_FILL	0x000A0000	/* Area Fill */

/* Graphics Engine for 9440/9660/9680 */

#define GER_STATUS	0x20		
#define		GE_BUSY	0x80
#define GER_OPERMODE	0x22		/* Byte for 9440, Word for 96xx */
#define GER_COMMAND	0x24		
#define		GE_NOP		0x00	/* No Operation */
#define		GE_BLT		0x01	/* BitBLT ROP3 only */
#define		GE_BLT_ROP4	0x02	/* BitBLT ROP4 (96xx only) */
#define		GE_SCANLINE	0x03	/* Scan Line */
#define		GE_BRESLINE	0x04	/* Bresenham Line */
#define		GE_SHVECTOR	0x05	/* Short Vector */
#define		GE_FASTLINE	0x06	/* Fast Line (96xx only) */
#define		GE_TRAPEZ	0x07	/* Trapezoidal fill (96xx only) */
#define		GE_ELLIPSE	0x08	/* Ellipse (96xx only) (RES) */
#define		GE_ELLIP_FILL	0x09	/* Ellipse Fill (96xx only) (RES)*/
#define	GER_FMIX	0x27
#define GER_DRAWFLAG	0x28		/* long */
#define		STENCIL		0x8000
#define		SOLIDFILL	0x4000
#define		TRANS_ENABLE	0x1000
#define 	TRANS_REVERSE	0x2000
#define		YMAJ		0x0400
#define		XNEG		0x0200
#define		YNEG		0x0100
#define		SRCMONO		0x0040
#define		PATMONO		0x0020
#define		SCR2SCR		0x0004
#define		PAT2SCR		0x0002
#define GER_FCOLOUR	0x2C		/* Word for 9440, long for 96xx */
#define GER_BCOLOUR	0x30		/* Word for 9440, long for 96xx */
#define GER_PATLOC	0x34		/* Word */
#define GER_DEST_XY	0x38
#define GER_DEST_X	0x38		/* Word */
#define GER_DEST_Y	0x3A		/* Word */
#define GER_SRC_XY	0x3C
#define GER_SRC_X	0x3C		/* Word */
#define GER_SRC_Y	0x3E		/* Word */
#define GER_DIM_XY	0x40
#define GER_DIM_X	0x40		/* Word */
#define GER_DIM_Y	0x42		/* Word */
#define GER_PATTERN	0x80		/* from 0x2180 to 0x21FF */

/* Additional - Graphics Engine for 96xx */
#define GER_SRCCLIP_XY	0x48
#define GER_SRCCLIP_X	0x48		/* Word */
#define GER_SRCCLIP_Y	0x4A		/* Word */
#define GER_DSTCLIP_XY	0x4C
#define GER_DSTCLIP_X	0x4C		/* Word */
#define GER_DSTCLIP_Y	0x4E		/* Word */

/* ROPS */
#define TGUIROP_0		0x00		/* 0 */
#define TGUIROP_1		0xFF		/* 1 */
#define TGUIROP_SRC		0xCC		/* S */
#define TGUIROP_DST		0xAA		/* D */
#define TGUIROP_NOT_SRC		0x33		/* Sn */
#define TGUIROP_NOT_DST		0x55		/* Dn */
#define TGUIROP_AND		0x88		/* DSa */
#define TGUIROP_SRC_AND_NOT_DST	0x44		/* SDna */
#define TGUIROP_NOT_SRC_AND_DST 0x22		/* DSna */
#define TGUIROP_NAND		0x77		/* DSan */
#define TGUIROP_OR		0xEE		/* DSo */
#define TGUIROP_SRC_OR_NOT_DST	0xDD		/* SDno */
#define TGUIROP_NOT_SRC_OR_DST	0xBB		/* DSno */
#define TGUIROP_NOR		0x11		/* DSon */
#define TGUIROP_XOR		0x66		/* DSx */
#define TGUIROP_XNOR		0x99		/* SDxn */

#define TGUIROP_PAT		0xF0		/* P */
#define TGUIROP_NOT_PAT		0x0F		/* Pn */
#define TGUIROP_AND_PAT		0xA0		/* DPa */
#define TGUIROP_PAT_AND_NOT_DST	0x50		/* PDna */
#define TGUIROP_NOT_PAT_AND_DST 0x0A		/* DPna */
#define TGUIROP_NAND_PAT	0x5F		/* DPan */
#define TGUIROP_OR_PAT		0xFA		/* DPo */
#define TGUIROP_PAT_OR_NOT_DST	0xF5		/* PDno */
#define TGUIROP_NOT_PAT_OR_DST	0xAF		/* DPno */
#define TGUIROP_NOR_PAT		0x05		/* DPon */
#define TGUIROP_XOR_PAT		0x5A		/* DPx */
#define TGUIROP_XNOR_PAT	0xA5		/* PDxn */

#define REPLICATE(x)  				\
	x = x & ((1 << pScrn->bitsPerPixel) - 1);	\
	if (pScrn->bitsPerPixel < 32) {		\
		x |= x << 16;			\
		if (pScrn->bitsPerPixel < 16)	\
			x |= x << 8;		\
	}

#define CHECKCLIPPING				\
	if (pTrident->Clipping)	{		\
		pTrident->Clipping = FALSE;	\
		TGUI_SRCCLIP_XY(0,0);		\
		TGUI_DSTCLIP_XY(4095,2047);	\
	}


/* Merge XY */
#define XY_MERGE(x,y) \
		(((unsigned long)(y) << 16) | ((unsigned long)(x) & 0xffff))

/* MMIO */
#ifndef PC98_TGUI
#define GER_BASE 0x2100
#else
#define GER_BASE 0x0000
#endif /* PC98_TGUI */

#define TRIDENT_WRITE_REG(v,r)					\
        *(unsigned int *)((char*)pTrident->IOBase+r) = v;		

#define TRIDENT_READ_REG(r) \
	*(unsigned int *)((char*)pTrident->IOBase+r)

#ifdef TRIDENT_MMIO
#define BLTBUSY(b) \
		b = (*(unsigned char *)(tguiMMIOBase + GER_STATUS)) & GE_BUSY;
#define OLDBLTBUSY(b) \
		b = (*(unsigned char *)(tguiMMIOBase + OLDGER_STATUS))&GE_BUSY;
#define TGUI_STATUS(c) \
		*(unsigned char *)(tguiMMIOBase + GER_STATUS) = c;
#define OLDTGUI_STATUS(c) \
		*(unsigned char *)(tguiMMIOBase + OLDGER_STATUS) = c;
#define TGUI_OPERMODE(c) \
		*(unsigned short *)(tguiMMIOBase + GER_OPERMODE) = c;
#define OLDTGUI_OPERMODE(c) \
		{ \
			*(unsigned short *)(tguiMMIOBase + OLDGER_MWIDTH) = \
				            vga256InfoRec.displayWidth - 1; \
			*(unsigned char *)(tguiMMIOBase + OLDGER_MFORMAT) = c; \
		}
#define TGUI_FCOLOUR(c) \
		*(unsigned long *)(tguiMMIOBase + GER_FCOLOUR) = c;
#define OLDTGUI_FCOLOUR(c) \
		*(unsigned long *)(tguiMMIOBase + OLDGER_FCOLOUR) = c;
#define TGUI_BCOLOUR(c) \
		*(unsigned long *)(tguiMMIOBase + GER_BCOLOUR) = c;
#define OLDTGUI_BCOLOUR(c) \
		*(unsigned long *)(tguiMMIOBase + OLDGER_BCOLOUR) = c;
#define TGUI_DRAWFLAG(c) \
		*(unsigned long *)(tguiMMIOBase + GER_DRAWFLAG) = c;
#define OLDTGUI_STYLE(c) \
		*(unsigned short *)(tguiMMIOBase + OLDGER_STYLE) = c;
#define TGUI_FMIX(c) \
		*(unsigned char *)(tguiMMIOBase + GER_FMIX) = c;
#define OLDTGUI_FMIX(c) \
		*(unsigned char *)(tguiMMIOBase + OLDGER_FMIX) = c;
#define OLDTGUI_BMIX(c) \
		*(unsigned char *)(tguiMMIOBase + OLDGER_BMIX) = c;
#define TGUI_DIM_XY(w,h) \
		*(unsigned long *)(tguiMMIOBase + GER_DIM_XY) = XY_MERGE(w-1,h-1);
#define OLDTGUI_DIMXY(w,h) \
		*(unsigned long *)(tguiMMIOBase + OLDGER_DIMXY) = XY_MERGE(w-1,h-1);
#define TGUI_SRC_XY(x,y) \
		*(unsigned long *)(tguiMMIOBase + GER_SRC_XY) = XY_MERGE(x,y);
#define TGUI_DEST_XY(x,y) \
		*(unsigned long *)(tguiMMIOBase + GER_DEST_XY) = XY_MERGE(x,y);
#define OLDTGUI_DESTXY(x,y) \
		*(unsigned long *)(tguiMMIOBase + OLDGER_DESTXY) = XY_MERGE(x,y);
#define OLDTGUI_DESTLINEAR(c) \
		*(unsigned long *)(tguiMMIOBase + OLDGER_DESTLINEAR) = c;
#define TGUI_SRCCLIP_XY(x,y) \
		*(unsigned long *)(tguiMMIOBase + GER_SRCCLIP_XY) = XY_MERGE(x,y);
#define TGUI_DSTCLIP_XY(x,y) \
		*(unsigned long *)(tguiMMIOBase + GER_DSTCLIP_XY) = XY_MERGE(x,y);
#define TGUI_PATLOC(addr) \
		*(unsigned short *)(tguiMMIOBase +GER_PATLOC) = addr;
#define TGUI_OUTB(addr, c) \
		*(unsigned char *)(tguiMMIOBase + addr) = c;
#define TGUI_COMMAND(c) \
		{ \
		TGUISync(); \
		*(unsigned char *)(tguiMMIOBase + GER_COMMAND) = c; \
		}
#define OLDTGUI_COMMAND(c) \
		{ \
		OLDTGUI_OPERMODE(GE_OP); \
		OLDTGUISync(); \
		*(unsigned long *)(tguiMMIOBase + OLDGER_COMMAND) = c; \
		}
#else
#define BLTBUSY(b) \
		b = inb(GER_BASE+GER_STATUS) & GE_BUSY;
#define OLDBLTBUSY(b) \
		{	\
			outb(GER_INDEX, OLDGER_STATUS); \
			b = inb(GER_BYTE0) & GE_BUSY; \
		}
#define TGUI_STATUS(c) \
		outb(GER_BASE+GER_STATUS, c);
#define OLDTGUI_STATUS(c) \
		{	\
			outb(GER_INDEX, OLDGER_STATUS); \
			outb(GER_BYTE0, 0x00); \
		}
#define TGUI_OPERMODE(c) \
		outw(GER_BASE+GER_OPERMODE, c);
#define OLDTGUI_OPERMODE(c) \
		{	\
			outb(GER_INDEX, OLDGER_MWIDTH); \
			outw(GER_BYTE0, vga256InfoRec.displayWidth - 1); \
			outb(GER_INDEX, OLDGER_MFORMAT); \
			outb(GER_BYTE0, c); \
		}
#define TGUI_FCOLOUR(c) \
		outl(GER_BASE+GER_FCOLOUR, c);
#define OLDTGUI_FCOLOUR(c) \
		{	\
			outb(GER_INDEX, OLDGER_FCOLOUR); \
			outl(GER_BYTE0, c); \
		}
#define TGUI_BCOLOUR(c) \
		outl(GER_BASE+GER_BCOLOUR, c);
#define OLDTGUI_BCOLOUR(c) \
		{	\
			outb(GER_INDEX, OLDGER_BCOLOUR); \
			outl(GER_BYTE0, c); \
		}
#define TGUI_DRAWFLAG(c) \
		outl(GER_BASE+GER_DRAWFLAG, c);
#define OLDTGUI_STYLE(c) \
		{	\
			outb(GER_INDEX, OLDGER_STYLE); \
			outw(GER_BYTE0, c); \
		}
#define TGUI_FMIX(c) \
		outb(GER_BASE+GER_FMIX, c);
#define OLDTGUI_FMIX(c) \
		{	\
			outb(GER_INDEX, OLDGER_FMIX); \
			outb(GER_BYTE0, c); \
		}
#define OLDTGUI_BMIX(c) \
		{	\
			outb(GER_INDEX, OLDGER_BMIX); \
			outb(GER_BYTE1, c); \
		}
#define TGUI_DIM_XY(w,h) \
		outl(GER_BASE+GER_DIM_XY, XY_MERGE(w-1,h-1));
#define OLDTGUI_DIMXY(w,h) \
		{	\
			outb(GER_INDEX, OLDGER_DIMXY); \
			outl(GER_BYTE0, XY_MERGE(w-1,h-1)); \
		}
#define TGUI_SRC_XY(x,y) \
		outl(GER_BASE+GER_SRC_XY, XY_MERGE(x,y));
#define TGUI_DEST_XY(x,y) \
		outl(GER_BASE+GER_DEST_XY, XY_MERGE(x,y));
#define OLDTGUI_DESTXY(x,y) \
		{	\
			outb(GER_INDEX, OLDGER_DESTXY); \
			outl(GER_BYTE0, XY_MERGE(x,y)); \
		}
#define OLDTGUI_DESTLINEAR(c) \
		{	\
			outb(GER_INDEX, OLDGER_DESTLINEAR); \
			outl(GER_BYTE0, c); \
		}
#define TGUI_SRCCLIP_XY(x,y) \
		outl(GER_BASE+GER_SRCCLIP_XY, XY_MERGE(x,y));
#define TGUI_DSTCLIP_XY(x,y) \
		outl(GER_BASE+GER_DSTCLIP_XY, XY_MERGE(x,y));
#define TGUI_PATLOC(addr) \
		outw(GER_BASE+GER_PATLOC, addr);
#define TGUI_OUTB(addr, c) \
		outb(GER_BASE+addr, c);
#define TGUI_OUTW(addr, c) \
		outw(GER_BASE+addr, c);
#define TGUI_COMMAND(c) \
		outb(GER_BASE+GER_COMMAND, c);
#define OLDTGUI_COMMAND(c) \
		{ \
		OLDTGUI_OPERMODE(GE_OP); \
		OLDTGUISync(); \
		outb(GER_INDEX, OLDGER_COMMAND); \
		outl(GER_BYTE0, c); \
		}
#endif
