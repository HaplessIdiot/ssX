/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir.h,v 1.12 1999/12/03 19:17:32 eich Exp $ */

/* (c) Itai Nahshon */

#ifndef CIR_H
#define CIR_H

#include "xf86Cursor.h"
#include "xaa.h"
#include "xf86i2c.h"

#define CIR_DEBUG

/* Card-specific driver information */

typedef struct {
	ScrnInfoPtr			pScrn;
	pciVideoPtr			PciInfo;
	PCITAG				PciTag;
	EntityInfoPtr		pEnt;
	int					Chipset;
	int					ChipRev;
	int					Rounding;
	int					BppShift;
	Bool				HasFBitBlt;
	CARD32				IOAddress;
	CARD32				FbAddress;
	unsigned char *		IOBase;
	unsigned char *		FbBase;
	long				FbMapSize;
	long				IoMapSize;
	int					MinClock;
	int					MaxClock;
	Bool				NoAccel;
	Bool				HWCursor;
	Bool				UseMMIO;
	XAAInfoRecPtr		AccelInfoRec;
	xf86CursorInfoPtr	CursorInfoRec;
#if 0
	DGAInfoPtr			DGAInfo;
#endif
	I2CBusPtr			I2CPtr1;
	I2CBusPtr			I2CPtr2;
	CloseScreenProcPtr	CloseScreen;

	Bool				CursorIsSkewed;
} CirRec, *CirPtr;

/* CirrusClk.c */
extern Bool
CirrusFindClock(int *rfreq, int max_clock, int *num_out, int *den_out);

/* cir_driver.c */
extern SymTabRec CIRChipsets[];
extern PciChipsets CIRPciChipsets[];

extern Bool CirMapMem(CirPtr pCir, int scrnIndex);
extern Bool CirUnmapMem(CirPtr pCir, int scrnIndex);

#endif /* CIR_H */
