/*
 * Copyright 1997,1998 by Alan Hourihane <alanh@fairlite.demon.co.uk>
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tga/tga.h,v 1.9 1999/04/25 10:02:24 dawes Exp $ */

#ifndef _TGA_H_
#define _TGA_H_

#include "xaa.h"
#include "xf86RamDac.h"

typedef struct {
	unsigned long tgaRegs[0x100];
} TGARegRec, *TGARegPtr;

#define TGAPTR(p)	((TGAPtr)((p)->driverPrivate))

typedef struct {
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    RamDacHelperRecPtr	RamDac;
    int                 ChipRev;
    int			HwBpp;
    int			BppShift;
    int			pprod;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned char *     FbBase;
    unsigned char *     IOBase;
    long		FbMapSize;
    unsigned long	regOffset;
    Bool		NoAccel;
/*      Bool		Dac6Bit; */
    Bool		HWCursor;
    Bool		UsePCIRetry;
    int			MinClock;
    int			MaxClock;
    TGARegRec		SavedReg;
    TGARegRec		ModeReg;
    CARD32		AccelFlags;
    RamDacRecPtr	RamDacRec;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr   CursorInfoRec;
    CloseScreenProcPtr	CloseScreen;
    int                 CardType;
    unsigned char       Bt463modeReg[59];
    unsigned char       Bt463saveReg[59];
    EntityInfoPtr       pEnt;
} TGARec, *TGAPtr;

/* Prototypes */

/* tga_dac.c */
void DEC21030Restore(ScrnInfoPtr pScrn,/* vgaRegPtr vgaReg,*/
		      TGARegPtr tgaReg/*, Bool restoreFonts*/);
void DEC21030Save(ScrnInfoPtr pScrn, /*vgaRegPtr vgaReg,*/ TGARegPtr tgaReg/*,
		   Bool saveFonts*/);
Bool DEC21030Init(ScrnInfoPtr pScrn, DisplayModePtr mode);

/* tga_accel8.c */
Bool DEC21030AccelInit8(ScreenPtr pScreen);

/* tga_accel32.c */
Bool DEC21030AccelInit32(ScreenPtr pScreen);

/* BTramdac.c */
void tgaBTOutIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data);
unsigned char tgaBTInIndReg(ScrnInfoPtr pScrn, CARD32 reg);
void tgaBTWriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void tgaBTReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void tgaBTWriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char tgaBTReadData(ScrnInfoPtr pScrn);

/* BT463ramdac.c */
void BT463ramdacSave(ScrnInfoPtr pScrn, unsigned char *data);
void BT463ramdacRestore(ScrnInfoPtr pScrn, unsigned char *data);

/* tga_cursor.c */
Bool TGAHWCursorInit(ScreenPtr pScreen);

#endif /* _TGA_H_ */

