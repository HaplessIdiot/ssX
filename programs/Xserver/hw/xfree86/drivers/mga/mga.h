/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.39 1999/06/12 14:15:35 dawes Exp $ */
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
#include "vgaHW.h"
#include "colormapst.h"
#include "xf86DDC.h"

#if defined(__alpha__)
#define INREG8(addr) xf86ReadSparse8(pMga->IOBase, (addr))
#define INREG16(addr) xf86ReadSparse16(pMga->IOBase, (addr))
#define INREG(addr) xf86ReadSparse32(pMga->IOBase, (addr))
#define OUTREG8(addr,val) xf86WriteSparse8((val),pMga->IOBase,(addr))
#define OUTREG16(addr,val) xf86WriteSparse16((val),pMga->IOBase,(addr))
#define OUTREG(addr, val) xf86WriteSparse32((val),pMga->IOBase,(addr))
#else /* __alpha__ */
#if defined(EXTRADEBUG)
CARD8 dbg_inreg8(ScrnInfoPtr,int,int);
CARD16 dbg_inreg16(ScrnInfoPtr,int,int);
CARD32 dbg_inreg32(ScrnInfoPtr,int,int);
void dbg_outreg8(ScrnInfoPtr,int,int);
void dbg_outreg16(ScrnInfoPtr,int,int);
void dbg_outreg32(ScrnInfoPtr,int,int);
#define INREG8(addr) dbg_inreg8(pScrn,addr,1)
#define INREG16(addr) dbg_inreg16(pScrn,addr,1)
#define INREG(addr) dbg_inreg32(pScrn,addr,1)
#define OUTREG8(addr,val) dbg_outreg8(pScrn,addr,val)
#define OUTREG16(addr,val) dbg_outreg16(pScrn,addr,val)
#define OUTREG(addr,val) dbg_outreg32(pScrn,addr,val)
#else /* EXTRADEBUG */
#define INREG8(addr) *(volatile CARD8 *)(pMga->IOBase + (addr))
#define INREG16(addr) *(volatile CARD16 *)(pMga->IOBase + (addr))
#define INREG(addr) *(volatile CARD32 *)(pMga->IOBase + (addr))
#define OUTREG8(addr, val) *(volatile CARD8 *)(pMga->IOBase + (addr)) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(pMga->IOBase + (addr)) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(pMga->IOBase + (addr)) = (val)
#endif /* EXTRADEBUG */
#endif /* __alpha__ */

#define PORT_OFFSET 	(0x1F00 - 0x300)

typedef struct {
    unsigned char	ExtVga[6];
    unsigned char 	DacClk[6];
    unsigned char *     DacRegs;
    CARD32		Option;
    CARD32		Option2;
    CARD32		Option3;
} MGARegRec, *MGARegPtr;

typedef struct {
    Bool	isHwCursor;
    int		CursorMaxWidth;
    int 	CursorMaxHeight;
    int		CursorFlags;
    int		CursorOffscreenMemSize;
    Bool	(*UseHWCursor)(ScreenPtr, CursorPtr);
    void	(*LoadCursorImage)(ScrnInfoPtr, unsigned char*);
    void	(*ShowCursor)(ScrnInfoPtr);
    void	(*HideCursor)(ScrnInfoPtr);
    void	(*SetCursorPosition)(ScrnInfoPtr, int, int);
    void	(*SetCursorColors)(ScrnInfoPtr, int, int);
    long	maxPixelClock;
    long	MemoryClock;
    MessageType ClockFrom;
    MessageType MemClkFrom;
    Bool	SetMemClk;
    void	(*LoadPalette)(ScrnInfoPtr, int, int*, LOCO*, short);
    void	(*PreInit)(ScrnInfoPtr);
    void	(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void	(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool	(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
} MGARamdacRec, *MGARamdacPtr;


/* Card-specific driver information */

#define MGAPTR(p) ((MGAPtr)((p)->driverPrivate))

#ifdef DISABLE_VGA_IO
typedef struct mgaSave {
    pciVideoPtr pvp;
    Bool enable;
} MgaSave, *MgaSavePtr;
#endif

typedef struct {
    EntityInfoPtr	pEnt;
    MGABiosInfo		Bios;
    MGABios2Info	Bios2;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    xf86AccessRec	Access;
    int			Chipset;
    int                 ChipRev;
    Bool		Primary;
    Bool		Interleave;
    int			HwBpp;
    int			Rounding;
    int			BppShift;
    Bool		HasFBitBlt;
    int			YDstOrg;
    CARD32		IOAddress;
    CARD32		FbAddress;
    CARD32		ILOADAddress;
    int			FbBaseReg;
    CARD32		BiosAddress;
    MessageType		BiosFrom;
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
    Bool		HasSDRAM;
    Bool		NoAccel;
    Bool		SyncOnGreen;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		ShowCache;
    Bool		Overlay8Plus24;
    Bool		ShadowFB;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
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
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    CARD32		*Atype;
    CARD32		*AtypeNoBLK;
    void		(*PreInit)(ScrnInfoPtr pScrn);
    void		(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void		(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool		(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
    CloseScreenProcPtr	CloseScreen;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed);
    Bool		(*i2cInit)(ScrnInfoPtr);
    I2CBusPtr		I2C;
    Bool		FBDev;
    int			colorKey;
} MGARec, *MGAPtr;

extern CARD32 MGAAtype[16];
extern CARD32 MGAAtypeNoBLK[16];

#define USE_RECTS_FOR_LINES	0x00000001
#define FASTBLT_BUG		0x00000002
#define CLIPPER_ON		0x00000004
#define BLK_OPAQUE_EXPANSION	0x00000008
#define TRANSC_SOLID_FILL	0x00000010
#define	NICE_DASH_PATTERN	0x00000020
#define	TWO_PASS_COLOR_EXPAND	0x00000040
#define	MGA_NO_PLANEMASK	0x00000080
#define USE_LINEAR_EXPANSION	0x00000100
#define LARGE_ADDRESSES		0x00000200


#define TRANSPARENCY_KEY	255
#define KEY_COLOR		0

/* Prototypes */

void MGAAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

void MGAHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr pScrn);

void MGA2064SetupFuncs(ScrnInfoPtr pScrn);
void MGAGSetupFuncs(ScrnInfoPtr pScrn);

void MGAStormSync(ScrnInfoPtr pScrn);
void MGAStormEngineInit(ScrnInfoPtr pScrn);
Bool MGAStormAccelInit(ScreenPtr pScreen);
Bool MGAHWCursorInit(ScreenPtr pScreen);

Bool Mga8AccelInit(ScreenPtr pScreen);
Bool Mga16AccelInit(ScreenPtr pScreen);
Bool Mga24AccelInit(ScreenPtr pScreen);
Bool Mga32AccelInit(ScreenPtr pScreen);

void MGAPolyArcThinSolid(DrawablePtr, GCPtr, int, xArc*);

Bool MGADGAInit(ScreenPtr pScreen);

#endif
