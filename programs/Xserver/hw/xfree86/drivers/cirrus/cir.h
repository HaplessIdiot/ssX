/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir.h,v 1.5 1998/10/06 04:39:35 dawes Exp $ */

/* (c) Itai Nahshon */

#ifndef CIR_H
#define CIR_H

#include "xf86Cursor.h"
#include "xaa.h"
#include "xf86i2c.h"

#define CIR_DEBUG

/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */
enum {
    /* CR regs */
    CR1B,
    CR1D,
    /* SR regs */
    SR07,
    SR0E,
    SR12,
    SR13,
    SR1E,
    /* GR regs */
    GR17,
    GR18,
    /* HDR */
    HDR,
    /* Must be last! */
    CIR_NSAVED
};

typedef struct {
    unsigned char	ExtVga[CIR_NSAVED];
} CIRRegRec, *CIRRegPtr;

/* Card-specific driver information */
#define CIRPTR(p) ((CIRPtr)((p)->driverPrivate))

typedef struct {
    ScreenPtr		pScreen;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    int                 ChipRev;
    int			Rounding;
    int			BppShift;
    Bool		HasFBitBlt;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    long		FbMapSize;
    unsigned char *     HWCursorBits;
    Bool		CursorIsSkewed;
    unsigned char *	CursorBits;
    int			MinClock;
    int			MaxClock;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UseMMIO;
    XAAInfoRecPtr       AccelInfoRec;
    xf86CursorInfoPtr   CursorInfoRec;
    DGAInfoPtr          DGAInfo;
    I2CBusPtr		I2CPtr1;
    I2CBusPtr		I2CPtr2;
    CloseScreenProcPtr  CloseScreen;
    
/* Difference from Laguna start here */
    CIRRegRec		SavedReg;
    CIRRegRec		ModeReg;

/* XXX For XF86Config based mem configuration */
    CARD32		SR0F, SR17;

#if 0
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		AccelFlags;
#endif
} CIRRec, *CIRPtr;


extern Bool CIRHWCursorInit(ScreenPtr pScreen);
extern Bool CIRXAAInit(ScreenPtr pScreen);
extern Bool CIRXAAInitMMIO(ScreenPtr pScreen);
extern Bool CIRDGAInit(ScreenPtr pScreen);
extern Bool CIRI2CInit(ScreenPtr pScreen);

/* CirrusClk.c */
extern CARD16 CirrusSetClock(ScrnInfoPtr pScrn, int freq);

#endif /* CIR_H */
