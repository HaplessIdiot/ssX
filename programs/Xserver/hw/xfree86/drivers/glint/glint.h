/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/glint.h,v 1.9 1998/09/05 06:36:46 dawes Exp $ */
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
 *           Dirk Hohndel, <hohndel@suse.de>
 *	     Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 */
#ifndef _GLINT_H_
#define _GLINT_H_

#include "xaa.h"
#include "xf86RamDac.h"

typedef struct {
	unsigned long glintRegs[0x2000];
	unsigned long DacRegs[0x100];
} GLINTRegRec, *GLINTRegPtr;

#define GLINTPTR(p)	((GLINTPtr)((p)->driverPrivate))

typedef struct {
    pciVideoPtr		PciInfo;
    pciVideoPtr		PciInfoDelta;
    PCITAG		PciTag;
    PCITAG		PciTagDelta;
    RamDacHelperRecPtr	RamDac;
    int			MemClock;
    int			Chipset;
    int                 ChipRev;
    int			HwBpp;
    int			BppShift;
    int			pprod;
    int			ForeGroundColor;
    int			BackGroundColor;
    int			bppalign;
    int			startxdom;
    int			startxsub;
    int			starty;
    int			count;
    int			dy;
    int			dxdom;
    int			rectxy;
    int			rectwh;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned char *     IOBase;
    unsigned char *	FbBase;
    long		FbMapSize;
    Bool		DoubleBuffer;
    Bool		NoAccel;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		ClippingOn;
    Bool		UsePCIRetry;
    Bool		UseBlockWrite;
    Bool		UseFireGL3000;
    Bool		VGAcore;
    int			MinClock;
    int			MaxClock;
    GLINTRegRec		SavedReg;
    GLINTRegRec		ModeReg;
    CARD32		AccelFlags;
    CARD32		ROP;
    CARD32		FrameBufferReadMode;
    CARD32		BltScanDirection;
    RamDacRecPtr	RamDacRec;
    xf86CursorInfoPtr	CursorInfoRec;
    XAAInfoRecPtr	AccelInfoRec;
    CloseScreenProcPtr	CloseScreen;
    GCPtr		CurrentGC;
} GLINTRec, *GLINTPtr;

/* Defines for PCI data */

#define PCI_VENDOR_TI_CHIP_PERMEDIA2	\
			((PCI_VENDOR_TI << 16) | PCI_CHIP_TI_PERMEDIA2)
#define PCI_VENDOR_TI_CHIP_PERMEDIA	\
			((PCI_VENDOR_TI << 16) | PCI_CHIP_TI_PERMEDIA)
#define PCI_VENDOR_3DLABS_CHIP_PERMEDIA	\
			((PCI_VENDOR_3DLABS << 16) | PCI_CHIP_PERMEDIA)
#define PCI_VENDOR_3DLABS_CHIP_PERMEDIA2	\
			((PCI_VENDOR_3DLABS << 16) | PCI_CHIP_PERMEDIA2)
#define PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V	\
			((PCI_VENDOR_3DLABS << 16) | PCI_CHIP_PERMEDIA2V)
#define PCI_VENDOR_3DLABS_CHIP_500TX	\
			((PCI_VENDOR_3DLABS << 16) | PCI_CHIP_500TX)
#define PCI_VENDOR_3DLABS_CHIP_MX	\
			((PCI_VENDOR_3DLABS << 16) | PCI_CHIP_MX)

/* Prototypes */

void Permedia2StoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs);
void Permedia2InstallColormap(ColormapPtr pmap);
void Permedia2UninstallColormap(ColormapPtr pmap);
int  Permedia2ListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);
void Permedia2HandleColormaps(ScreenPtr pScreen, ScrnInfoPtr scrnp);
void Permedia2RestoreDACValues(ScrnInfoPtr pScrn);
void Permedia2Restore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
void Permedia2Save(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
Bool Permedia2Init(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool Permedia2AccelInit(ScreenPtr pScreen);
Bool Permedia2HWCursorInit(ScreenPtr pScreen);

void PermediaRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
void PermediaSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
Bool PermediaInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool PermediaAccelInit(ScreenPtr pScreen);
void Permedia2VRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
void Permedia2VSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
Bool Permedia2VInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool Permedia2vHWCursorInit(ScreenPtr pScreen);

void TXRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
void TXSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg);
Bool TXInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool TXAccelInit(ScreenPtr pScreen);

void glintOutIBMRGBIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data);
unsigned char glintInIBMRGBIndReg(ScrnInfoPtr pScrn, CARD32 reg);
void glintIBMWriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void glintIBMReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void glintIBMWriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char glintIBMReadData(ScrnInfoPtr pScrn);
Bool glintIBM526HWCursorInit(ScreenPtr pScreen);
Bool glintIBM640HWCursorInit(ScreenPtr pScreen);

void Permedia2OutIndReg(ScrnInfoPtr pScrn,
		     CARD32, unsigned char mask, unsigned char data);
unsigned char Permedia2InIndReg(ScrnInfoPtr pScrn, CARD32);
void Permedia2WriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void Permedia2ReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void Permedia2WriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char Permedia2ReadData(ScrnInfoPtr pScrn);

void Permedia2vOutIndReg(ScrnInfoPtr pScrn,
		   CARD32, unsigned char mask, unsigned char data);
unsigned char Permedia2vInIndReg(ScrnInfoPtr pScrn, CARD32);
#endif /* _GLINT_H_ */

