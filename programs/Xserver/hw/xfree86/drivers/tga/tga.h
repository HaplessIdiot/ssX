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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tga/tga.h,v 1.5 1998/08/29 05:43:34 dawes Exp $ */

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
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    long		FbMapSize;
    unsigned int	regOffset;
    Bool		NoAccel;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    int			MinClock;
    int			MaxClock;
    TGARegRec		SavedReg;
    TGARegRec		ModeReg;
    CARD32		AccelFlags;
    RamDacRecPtr	RamDacRec;
    XAAInfoRecPtr	AccelInfoRec;
    CloseScreenProcPtr	CloseScreen;
    int                 CardType;
    unsigned char       Bt463modeReg[59];
    unsigned char       Bt463saveReg[59];
} TGARec, *TGAPtr;

/* Prototypes */

void DEC21030Restore(ScrnInfoPtr pScrn,/* vgaRegPtr vgaReg,*/
		      TGARegPtr tgaReg/*, Bool restoreFonts*/);
void DEC21030Save(ScrnInfoPtr pScrn, /*vgaRegPtr vgaReg,*/ TGARegPtr tgaReg/*,
		   Bool saveFonts*/);
Bool DEC21030Init(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool DEC21030AccelInit(ScreenPtr pScreen);

void tgaBTOutIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data);
unsigned char tgaBTInIndReg(ScrnInfoPtr pScrn, CARD32 reg);
void tgaBTWriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void tgaBTReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void tgaBTWriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char tgaBTReadData(ScrnInfoPtr pScrn);

void BT463ramdacSave(ScrnInfoPtr pScrn, unsigned char *data);
void BT463ramdacRestore(ScrnInfoPtr pScrn, unsigned char *data);

#endif /* _TGA_H_ */

