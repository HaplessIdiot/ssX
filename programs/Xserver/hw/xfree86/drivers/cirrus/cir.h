/* (c) Itai Nahshon */

#ifndef CIR_H
#define CIR_H

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
    
    CIRRegRec		SavedReg;
    CIRRegRec		ModeReg;
    XAAInfoRecPtr       AccelInfoRec;
    XAACursorInfoPtr    CursorInfoRec;
    DGAInfoPtr          DGAInfo;
#if 0
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		AccelFlags;
#endif
    CloseScreenProcPtr  CloseScreen;
} CIRRec, *CIRPtr;


extern Bool CIRHWCursorInit(ScreenPtr pScreen);
extern Bool CIRDGAInit(ScreenPtr pScreen);
extern Bool CirrusSetClock(ScrnInfoPtr pScrn, int freq);


#endif /* CIR_H */
