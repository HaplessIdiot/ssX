/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.17 1998/08/30 04:49:41 dawes Exp $ */
/*
 * MGA Millennium (MGA2064W) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 */

#ifndef MGA_H
#define MGA_H

#include "xaa.h"
#include "xf86Cursor.h"

#if defined(__alpha__)
#define mb() __asm__ __volatile__("mb": : :"memory")
#define INREG8(addr) xf86ReadSparse8(pMga->IOBase, (addr))
#define INREG16(addr) xf86ReadSparse16(pMga->IOBase, (addr))
#define INREG(addr) xf86ReadSparse32(pMga->IOBase, (addr))
#define OUTREG8(addr,val) do { xf86WriteSparse8((val),pMga->IOBase,(addr)); \
				mb();} while(0)
#define OUTREG16(addr,val) do { xf86WriteSparse16((val),pMga->IOBase,(addr)); \
				mb();} while(0)
#define OUTREG(addr, val) do { xf86WriteSparse32((val),pMga->IOBase,(addr)); \
				mb();} while(0)
#else /* __alpha__ */
#define INREG8(addr) *(volatile CARD8 *)(pMga->IOBase + (addr))
#define INREG16(addr) *(volatile CARD16 *)(pMga->IOBase + (addr))
#define INREG(addr) *(volatile CARD32 *)(pMga->IOBase + (addr))
#define OUTREG8(addr, val) *(volatile CARD8 *)(pMga->IOBase + (addr)) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(pMga->IOBase + (addr)) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(pMga->IOBase + (addr)) = (val)
#endif /* __alpha__ */

#define PORT_OFFSET 	(0x1F00 - 0x300)


typedef struct {
    Bool	isHwCursor;
    int		CursorMaxWidth;
    int 	CursorMaxHeight;
    int		CursorFlags;
    int		CursorOffscreenMemSize;
    Bool        (*UseHWCursor)(ScreenPtr, CursorPtr);
    void        (*LoadCursorImage)(ScrnInfoPtr, unsigned char*);
    void        (*ShowCursor)(ScrnInfoPtr);
    void        (*HideCursor)(ScrnInfoPtr);
    void        (*SetCursorPosition)(ScrnInfoPtr, int, int);
    void        (*SetCursorColors)(ScrnInfoPtr, int, int);
    long	maxPixelClock;
    long	MemoryClock;
    MessageType ClockFrom;
    MessageType MemClkFrom;
    Bool	SetMemClk;
    void        (*StoreColors)(ScrnInfoPtr, xColorItem*, int);
} MGARamdacRec, *MGARamdacPtr;

typedef struct {
    unsigned char	ExtVga[6];
    unsigned char 	DacClk[6];
    unsigned char *     DacRegs;
    CARD32		Option;
} MGARegRec, *MGARegPtr;

/* Card-specific driver information */

#define MGAPTR(p) ((MGAPtr)((p)->driverPrivate))

typedef struct {
    MGABiosInfo		Bios;
    MGABios2Info	Bios2;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    int                 ChipRev;
    Bool		Interleave;
    int			HwBpp;
    int			Rounding;
    int			BppShift;
    Bool		HasFBitBlt;
    int			YDstOrg;
    CARD32		IOAddress;
    CARD32		FbAddress;
    CARD32		ILOADAddress;
    CARD32		BiosAddress;
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    unsigned char *	ILOADBase;
    unsigned char *	FbStart;
    long		FbMapSize;
    long		FbUsableSize;
    long		FbCursorOffset;
    MGARamdacRec	Dac;
    Bool		NoAccel;
    Bool		SyncOnGreen;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		ShowCache;
    int			MemClk;
    int			MinClock;
    int			MaxClock;
    MGARegRec		SavedReg;
    MGARegRec		ModeReg;
    int			MaxFastBlitY;
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		DashCMD;
    CARD32		NiceDashCMD;
    CARD32		AccelFlags;
    CARD32		PlaneMask;
    CARD32		FgColor;
    CARD32		BgColor;
    int			StyleLen;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
    CloseScreenProcPtr	CloseScreen;
} MGARec, *MGAPtr;

extern CARD32 MGAAtype[16];
extern CARD32 MGAAtypeNoBLK[16];

#define USE_RECTS_FOR_LINES	0x00000001
#define FASTBLT_BUG		0x00000002
#define CLIPPER_ON		0x00000004
#define BLK_OPAQUE_EXPANSION	0x00000008
#define TRANSC_SOLID_FILL	0x00000010
#define	NICE_DASH_PATTERN	0x00000020


/* Prototypes */

void MGAHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr pScrn);

void MGA3026RamdacInit(ScrnInfoPtr pScrn);
void MGA3026Save(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
		    Bool saveFonts);
void MGA3026Restore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
		    Bool restoreFonts);
Bool MGA3026Init(ScrnInfoPtr pScrn, DisplayModePtr mode);

void MGAGRamdacInit(ScrnInfoPtr pScrn);
void MGAGSave(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
		    Bool saveFonts);
void MGAGRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
		    Bool restoreFonts);
Bool MGAGInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

void MGAStormSync(ScrnInfoPtr pScrn);
void MGAStormEngineInit(ScrnInfoPtr pScrn);
Bool MGAStormAccelInit(ScreenPtr pScreen);
Bool MGAHWCursorInit(ScreenPtr pScreen);

Bool Mga8AccelInit(ScreenPtr pScreen);
Bool Mga16AccelInit(ScreenPtr pScreen);
Bool Mga24AccelInit(ScreenPtr pScreen);
Bool Mga32AccelInit(ScreenPtr pScreen);

#endif
