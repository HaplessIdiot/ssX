/* $XFree86$ */


/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */
#include "xf86_ansic.h"

/* This is used for module versioning */
#include "xf86Version.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers using the vgahw module need this */
#include "vgaHW.h"

/* Drivers using the mi banking wrapper need this */
#include "mibank.h"

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

/* Needed for the 1 and 4 bpp framebuffers */
#include "xf1bpp.h"
#include "xf4bpp.h"

/* Drivers using cfb need this */

#define PSZ 8
#include "cfb.h"
#undef PSZ

/* Drivers supporting bpp 16, 24 or 32 with cfb need these */

#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

/* Drivers using the XAA interface ... */
#include "xaa.h"
#include "xaalocal.h"
#include "xf86Cursor.h"
#include "xf86fbman.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

/* I2C support */
#include "xf86i2c.h"

/* DDC support */
#include "xf86DDC.h"

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned long	u32;

#define NoSEQRegs	0x20
#define NoCRTRegs	0x1F
#define NoGRCRegs	0x09
#define	NoATCRegs	0x15

enum {
    XR80, XRC0, XRD0, XRE0, XRE8, XREC, XR140, XR144, XR148, XR14C, NoEXRegs
};

#if 0
static unsigned short XR_addr[NoEXRegs] = {
    0x80, 0xC0, 0xD0, 0xE0, 0xE8, 0xEC, 0x140, 0x144, 0x148, 0x14C
};
#endif

typedef struct {
	unsigned char	MISC;
	unsigned char	FCTRL;
	unsigned char	SEQ[NoSEQRegs];
	unsigned char	CRT[NoCRTRegs];
	unsigned char	GRC[NoGRCRegs];
	unsigned char	ATC[NoATCRegs];
	unsigned int	EX[NoEXRegs];
} ApmRegStr, *ApmRegPtr;

typedef struct {
    pciVideoPtr	PciInfo;
    PCITAG	PciTag;
    int		scrnIndex;
    int		Chipset;
    int		ChipRev;
    CARD32	LinAddress;
    unsigned long	LinMapSize;
    CARD32	FbMapSize;
    pointer	LinMap;
    pointer	FbBase;
    char	*VGAMap;
    char	*MemMap;
    pointer	BltMap;
    Bool	UnlockCalled;
    int		xbase;
    unsigned char	savedSR10;
    unsigned char	d9, db;
    unsigned long	saveCmd;
    pointer	FontInfo;
    Bool	hwCursor;
    Bool	noLinear;
    ApmRegStr	ModeReg, SavedReg;
    CloseScreenProcPtr	CloseScreen;
    Bool	UsePCIRetry;          /* Do we use PCI-retry or busy-waiting */
    Bool	NoAccel;      /* Do we use the XAA acceleration architecture */
    int		MinClock;                                /* Min ramdac clock */
    int		MaxClock;                                /* Max ramdac clock */
    int		apmMMIO_Init;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
#if 0
    DGAInfoPtr		DGAInfo;
#endif
    int		BaseCursorAddress, CursorAddress, DisplayedCursorAddress;
    int		OffscreenReserved;
    unsigned int	Setup_DEC;
    int			blitxdir, blitydir;
    Bool		apmTransparency, apmClip;
    I2CBusPtr		I2CPtr;
} ApmRec, *ApmPtr;

typedef struct {
    u16		ca;
    u8		font;
    u8		pad;
} ApmFontBuf;

typedef struct {
    u16		ca;
    u8		font;
    u8		pad;
    u16		ca2;
    u8		font2;
    u8		pad2;
} ApmTextBuf;

#define APMDECL(p)	ApmPtr pApm = ((ApmPtr)(((ScrnInfoPtr)(p))->driverPrivate))
#define APMPTR(p)	((ApmPtr)(((ScrnInfoPtr)(p))->driverPrivate))

extern int	ApmHWCursorInit(ScreenPtr pScreen);
extern int	ApmAccelInit(ScreenPtr pScreen);
extern Bool	ApmI2CInit(ScreenPtr pScreen);
extern void	ApmCheckMMIO_Init(ScrnInfoPtr pScrn);
extern void	ApmCheckMMIO_Init_IOP(ScrnInfoPtr pScrn);
extern void	ApmCheckMMIO_Init24(ScrnInfoPtr pScrn);
extern void	ApmCheckMMIO_Init24_IOP(ScrnInfoPtr pScrn);
extern unsigned char curr[0x54];

#include "apm_regs.h"
