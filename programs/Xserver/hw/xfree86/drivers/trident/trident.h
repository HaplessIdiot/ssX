/*
 * Copyright 1998 by Alan Hourihane <alanh@fairlite.demon.co.uk>
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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident.h,v 1.8 1999/04/15 06:39:02 dawes Exp $ */

#ifndef _TRIDENT_H_
#define _TRIDENT_H_

#include "xf86Cursor.h"
#include "xaa.h"
#include "xf86RamDac.h"
#include "compiler.h"
#include "vgaHW.h"
#include "xf86i2c.h"

typedef struct {
	unsigned char tridentRegs3x4[0x100];
	unsigned char tridentRegs3CE[0x100];
	unsigned char tridentRegs3C4[0x100];
	unsigned char tridentRegsDAC[0x01];
	unsigned char tridentRegsClock[0x03];
	unsigned char DacRegs[0x300];
} TRIDENTRegRec, *TRIDENTRegPtr;

#define TRIDENTPTR(p)	((TRIDENTPtr)((p)->driverPrivate))

typedef struct {
    ScrnInfoPtr		pScrn;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    int			DACtype;
    int			RamDac;
    int                 ChipRev;
    int			HwBpp;
    int			BppShift;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    long		FbMapSize;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		UseGERetry;
    Bool		NewClockCode;
    Bool		IsCyber;
    Bool		Clipping;
    Bool		DstEnable;
    Bool		ROP;
    Bool		HasSGRAM;
    Bool		MUX;
    float		frequency;
    unsigned char	REGPCIReg;
    unsigned char	REGNewMode1;
    int			MinClock;
    int			MaxClock;
    int			MUXThreshold;
    int			MCLK;
    int			bytes;
    TRIDENTRegRec	SavedReg;
    TRIDENTRegRec	ModeReg;
    I2CBusPtr		DDC;
    short		EngineOperation;
    CARD32		AccelFlags;
    CARD32		BltScanDirection;
    CARD32		PCISTAT;
    RamDacRecPtr	RamDacRec;
    xf86CursorInfoPtr	CursorInfoRec;
    XAAInfoRecPtr	AccelInfoRec;
    CloseScreenProcPtr	CloseScreen;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    unsigned char *	XAAScanlineColorExpandBuffers[1];
} TRIDENTRec, *TRIDENTPtr;

/* Prototypes */

unsigned int Tridentddc1Read(ScrnInfoPtr pScrn);
void TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
void TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
Bool TridentInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool TridentAccelInit(ScreenPtr pScreen);
Bool TridentAccelInitMMIO(ScreenPtr pScreen);

void TridentOutIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data);
unsigned char TridentInIndReg(ScrnInfoPtr pScrn, CARD32 reg);
void TridentWriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void TridentReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void TridentWriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char TridentReadData(ScrnInfoPtr pScrn);

void TGUISetRead(int bank);
void TGUISetWrite(int bank);
void TGUISetReadWrite(int bank);

float CalculateMCLK(ScrnInfoPtr pScrn);

/*
 * Trident Chipset Definitions
 */

#define TVGA8200LX	0
#define TVGA8800CS	1
#define TVGA8900B	2
#define TVGA8900C	3
#define TVGA8900CL	4
#define TVGA8900D	5
#define TVGA9000	6
#define TVGA9000i	7
#define TVGA9100B	8
#define TVGA9200CXr	9
#define TGUI9400CXi	10
#define TGUI9420	11
#define TGUI9420DGi	12
#define TGUI9430DGi	13
#define TGUI9440AGi	14
#define CYBER9320	15
#define TGUI96xx	16 /* Backwards compatibility */
#define TGUI9660	16
#define TGUI9680	17
#define PROVIDIA9682	18
#define PROVIDIA9685	19
#define CYBER9382	20
#define CYBER9385	21
#define CYBER9388	22
#define CYBER9397	23
#define CYBER9520	24
#define IMAGE975	25
#define IMAGE985	26
#define CYBER939A	27
#define CYBER9525	28
#define BLADE3D		29

#define HAS_DST_TRANS	(pTrident->Chipset == PROVIDIA9682) 

#define Is3Dchip	((pTrident->Chipset == CYBER9388) || \
			 (pTrident->Chipset == CYBER9397) || \
			 (pTrident->Chipset == CYBER939A) || \
			 (pTrident->Chipset == CYBER9520) || \
			 (pTrident->Chipset == CYBER9525) || \
			 (pTrident->Chipset == IMAGE975)  || \
			 (pTrident->Chipset == IMAGE985)  || \
			 (pTrident->Chipset == BLADE3D))

/*
 * Trident DAC's
 */

#define TKD8001		0
#define TGUIDAC		1

#endif /* _TRIDENT_H_ */

