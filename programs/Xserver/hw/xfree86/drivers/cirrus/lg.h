/*
 * Common strutures and function for CL-GD546x -- The Laguna family
 *
 * lg.h
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 *  Inspired by cir.h
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/lg.h,v 1.4 1998/12/06 06:08:30 dawes Exp $ */

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
    ScrnInfoPtr         pScrn;
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
    int			MinClock;
    int			MaxClock;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UseMMIO;
    XAAInfoRecPtr       AccelInfoRec;
    xf86CursorInfoPtr   CursorInfoRec;
#if 0
    DGAInfoPtr          DGAInfo;
#endif    
    I2CBusPtr		I2CPtr1;
    I2CBusPtr		I2CPtr2;
    CloseScreenProcPtr  CloseScreen;

/* Difference from Cirrus start here */
    CARD32              HWCursorAddr;
    int                 HWCursorImageX;
    int                 HWCursorImageY;
    int                 HWCursorTileWidth;
    int                 HWCursorTileHeight;
    Bool		CursorIsSkewed;

    int                 lineDataIndex;

    int                 memInterleave;

    LgRegRec		SavedReg;
    LgRegRec		ModeReg;

    CARD32              oldBitmask;
    Bool                blitTransparent;
    int                 blitYDir;
#if 0
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		AccelFlags;
#endif
} LgRec, *LgPtr;

typedef struct {
  int tilesPerLine;  /* Number of tiles per line */
  int pitch;         /* Display pitch, in bytes */
  int width;         /* Tile width.  0 = 128 byte  1 = 256 byte */
} LgLineDataRec, *LgLineDataPtr;


/* lg_driver.c */
extern LgLineDataRec LgLineData[];

/* cir_driver.c */
extern SymTabRec CIRChipsets[];

Bool	CIRMapMem(ScrnInfoPtr pScrn);
Bool	CIRUnmapMem(ScrnInfoPtr pScrn);

/* CirrusClk.c */
extern CARD16 CirrusSetClock(ScrnInfoPtr pScrn, int freq);

/* lg_xaa.c */
extern Bool LgXAAInit(ScreenPtr pScreen);

/* lg_hwcurs.c */
extern Bool LgHWCursorInit(ScreenPtr pScreen);
extern void LgHideCursor(ScrnInfoPtr pScrn);
extern void LgShowCursor(ScrnInfoPtr pScrn);

/* lg_i2c.c */
extern Bool LGI2CInit(ScreenPtr pScreen);

#endif /* LG_H */
