/* (c) 1998 Corin Anderson
   Inspired by cir.h
 */

/* $XFree86$ */

#ifndef LG_H
#define LG_H

#include "xf86Cursor.h"
#include "xaa.h"
#include "xf86i2c.h"

#define LG_DEBUG

/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */
enum {
    /* CR regs */
    CR1A,
    CR1B,
    CR1D,
    CR1E,
    /* SR regs */
    SR07,
    SR0E,
    SR12,
    SR13,
    SR1E,
    /* Must be last! */
    LG_LAST_REG
};

typedef struct {
    unsigned char	ExtVga[LG_LAST_REG];
  
    /* Laguna regs */
  CARD8 TILE, BCLK;
  CARD16 FORMAT, DTTC, TileCtrl, CONTROL;
  CARD32 VSC;
} LgRegRec, *LgRegPtr;

/* Card-specific driver information */
#define LGPTR(p) ((LgPtr)((p)->driverPrivate))

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
    
    LgRegRec		SavedReg;
    LgRegRec		ModeReg;
    XAAInfoRecPtr       AccelInfoRec;
    xf86CursorInfoPtr   CursorInfoRec;
    DGAInfoPtr          DGAInfo;
#if 0
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		AccelFlags;
#endif
    CloseScreenProcPtr  CloseScreen;
} LgRec, *LgPtr;

typedef struct {
  int tilesPerLine;  /* Number of tiles per line */
  int pitch;         /* Display pitch, in bytes */
  int width;         /* Tile width.  0 = 128 byte  1 = 256 byte */
} LgLineDataRec, *LgLineDataPtr;

/* cir_driver.c */
extern SymTabRec CIRChipsets[];

Bool	CIRMapMem(ScrnInfoPtr pScrn);
Bool	CIRUnmapMem(ScrnInfoPtr pScrn);

/* CirrusClk.c */
extern Bool CirrusSetClock(ScrnInfoPtr pScrn, int freq);

#endif /* LG_H */
