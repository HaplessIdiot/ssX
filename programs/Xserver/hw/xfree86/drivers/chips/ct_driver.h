/* $XConsortium: ct_driver.h /main/3 1996/10/27 11:49:29 kaleb $ */
/*
 * Modified 1996 by Egbert Eich <Egbert.Eich@Physik.TH-Darmstadt.DE>
 * Modified 1996 by David Bateman <dbateman@ee.uts.edu.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_driver.h,v 1.14 1998/11/01 12:35:51 dawes Exp $ */


#ifndef _CT_DRIVER_H_
#define _CT_DRIVER_H_

#include "xaa.h"
#include "xaalocal.h"		/* XAA internals as we replace some of XAA */
#include "xf86Cursor.h"
#include "xf86i2c.h"

/* Supported chipsets */
typedef enum {
    CHIPS_CT65520,
    CHIPS_CT65525,
    CHIPS_CT65530,
    CHIPS_CT65535,
    CHIPS_CT65540,
    CHIPS_CT65545,
    CHIPS_CT65546,
    CHIPS_CT65548,
    CHIPS_CT65550,
    CHIPS_CT65554,
    CHIPS_CT65555,
    CHIPS_CT68554,
    CHIPS_CT69000,
    CHIPS_CT64200,
    CHIPS_CT64300
} CHIPSType;

/* Clock related */
typedef struct {
    unsigned char msr;		/* Dot Clock Related */
    unsigned char fcr;
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr33;
    unsigned char xr54;
    unsigned char fr03;
    int Clock;
} CHIPSClockReg, *CHIPSClockPtr;

typedef struct {
    unsigned int Max;		/* Memory Clock Related */
    unsigned int ProbedClk;
    unsigned int Clk;
    unsigned char M;
    unsigned char N;
    unsigned char P;
    unsigned char PSN;
    unsigned char xrCC;
    unsigned char xrCD;
    unsigned char xrCE;
} CHIPSMemClockReg, *CHIPSMemClockPtr;

#define TYPE_HW 0x01
#define TYPE_PROGRAMMABLE 0x02
#define GET_TYPE 0x0F
#define OLD_STYLE 0x10
#define NEW_STYLE 0x20
#define HiQV_STYLE 0x30
#define WINGINE_1_STYLE 0x40        /* 64300: external clock; 4 clocks    */
#define WINGINE_2_STYLE 0x50        /* 64300: internal clock; 2 hw-clocks */
#define GET_STYLE 0xF0
#define LCD_TEXT_CLK_FREQ 25000	    /* lcd textclock if TYPE_PROGRAMMABLE */
#define CRT_TEXT_CLK_FREQ 28322     /* crt textclock if TYPE_PROGRAMMABLE */
#define Fref 14318180               /* The reference clock in Hertz       */

/* The capability flags for the C&T chipsets */
#define ChipsLinearSupport	0x00000001
#define ChipsAccelSupport	0x00000002
#define ChipsMMIOSupport	0x00000004
#define ChipsHDepthSupport	0x00000008
#define ChipsDPMSSupport	0x00000010
#define ChipsTMEDSupport	0x00000020

/* Options flags for the C&T chipsets */
#define ChipsUseVClk1		0x00000040
#define ChipsHWCursor		0x00000080

/* Architecture type flags */
#define ChipsHiQV		0x00000100
#define ChipsWingine		0x00000200
#define IS_Wingine(x)		((x->Flags) & ChipsWingine)
#define IS_HiQV(x)		((x->Flags) & ChipsHiQV)

/* Acceleration flags for the C&T chipsets */
#define ChipsColorTransparency	0x0100000

/* Flag Bus Types */
#define ChipsUnknown	0
#define ChipsISA	1
#define ChipsVLB	2
#define ChipsPCI	3
#define ChipsCPUDirect	4
#define ChipsPIB	5
#define ChipsMCB	6

/* Macro's to select the 32 bit acceleration registers */
#define DR(x) cPtr->Regs32[x]	/* For CT655xx naming scheme  */
#define MR(x) cPtr->Regs32[x]	/* CT655xx MMIO naming scheme */
#define BR(x) cPtr->Regs32[x]	/* For HiQV naming scheme     */
#define MMIOmeml(x) *(unsigned int *)(cPtr->MMIOBase + (x))
#define MMIOmemw(x) *(unsigned short *)(cPtr->MMIOBase + (x))

/* Definitions for IO access to ports */
#define write_xr(num,val) {outb(0x3D6, num);outb(0x3D7, val);}
#define read_xr(num,var) {outb(0x3D6, num);var=inb(0x3D7);}
#define write_fr(num,val) {outb(0x3D0, num);outb(0x3D1, val);}
#define read_fr(num,var) {outb(0x3D0, num);var=inb(0x3D1);}

/* Monitor or flat panel type flags */
#define ChipsCRT	0x0010
#define ChipsLCD	0x1000
#define ChipsTFT	0x0100
#define ChipsDS		0x0200
#define ChipsDD		0x0400
#define ChipsSS		0x0800
#define IS_STN(x)	((x) & 0xE00)

/* Storage for the registers of the C&T chipsets */
typedef struct {
	unsigned char XR[0xFF];
	unsigned char CR[0x80];
	unsigned char FR[0x80];
	CHIPSClockReg Clock;
} CHIPSRegRec, *CHIPSRegPtr;

/* Storage for the flat panel size */
typedef struct {
    int HDisplay;
    int HRetraceStart;
    int HRetraceEnd;
    int HTotal;
    int VDisplay;
    int VRetraceStart;
    int VTotal;
} CHIPSPanelSizeRec, *CHIPSPanelSizePtr;

/* Some variables needed in the XAA acceleration */
typedef struct {
    /* General variable */ 
    unsigned int CommandFlags;
    unsigned int BytesPerPixel;
    unsigned int PitchInBytes;
    unsigned int ScratchAddress;
    /* 64k for color expansion and imagewrites */
    unsigned char * BltDataWindow;
    /* Hardware cursor address */
    unsigned int CursorAddress;
    Bool UseHWCursor;
    /* Boundaries of the pixmap cache */
    unsigned int CacheStart;
    unsigned int CacheEnd;
    /* Storage for pattern mask */
    int planemask;
    /* Storage for foreground and background color */
    int fgColor;
    int bgColor;
    /* For the 8x8 pattern fills */
    int patternyrot;
    /* For cached stipple fills */
    int SlotWidth;
    /* Variables for the 24bpp fill */
    unsigned char fgpixel;
    unsigned char bgpixel;
    unsigned char xorpixel;
    Bool fastfill;
    Bool rgb24equal;
    int fillindex;
    unsigned int width24bpp;
    unsigned int color24bpp;
    unsigned int rop24bpp;
} CHIPSACLRec, *CHIPSACLPtr;
#define CHIPSACLPTR(p)	&((CHIPSPtr)((p)->driverPrivate))->Accel

/* Storage for some register values that are messed up by suspend/resumes */
typedef struct {
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr14;
    unsigned char xr15;
    unsigned char vgaIOBaseFlag;
} CHIPSSuspendHackRec, *CHIPSSuspendHackPtr;

/* The privates of the C&T driver */
#define CHIPSPTR(p)	((CHIPSPtr)((p)->driverPrivate))

typedef struct {
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned int	IOBase;
    unsigned char *	FbBase;
    unsigned char *	MMIOBase;
    long		FbMapSize;
    OptionInfoPtr	Options;
    CHIPSPanelSizeRec	PanelSize;
    int			FrameBufferSize;
    Bool		SyncResetIgn;
    Bool		UseMMIO;
    int			Monitor;
    int			MinClock;
    int			MaxClock;
    CHIPSClockReg	SaveClock;		/* Storage for ClockSelect */
    CHIPSMemClockReg	MemClock;
    unsigned char	ClockType;
    unsigned char	ConsoleClk[4];
    int			ClockMulFactor;
    int			Rounding;
    CHIPSSuspendHackRec	SuspendHack;
    CARD32		PanelType;
    CHIPSRegRec		ModeReg;
    CHIPSRegRec		SavedReg;
    unsigned int *	Regs32;
    unsigned int	Flags;
    CARD32		Bus;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
    CHIPSACLRec		Accel;
    unsigned int	HWCursorContents;
    Bool		HWCursorShown;
    CloseScreenProcPtr	CloseScreen;
#ifdef __arm32__
#ifdef __NetBSD__
    int			TVMode;
#endif
    int			Bank;
#endif
    unsigned char       ddc_mask;
    I2CBusPtr           I2C;
} CHIPSRec, *CHIPSPtr;

typedef struct _CHIPSi2c {
  unsigned char i2cClockBit;
  unsigned char i2cDataBit;
} CHIPSI2CRec, *CHIPSI2CPtr;

/* External variables */
extern int ChipsAluConv[];
extern int ChipsAluConv2[];
extern int ChipsAluConv3[];
extern unsigned int ChipsReg32[];
extern unsigned int ChipsReg32HiQV[];

/* Prototypes */
/* banking */
int CHIPSSetRead(ScreenPtr pScreen, int bank);
int CHIPSSetWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetRead(ScreenPtr pScreen, int bank);
int CHIPSWINSetWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWritePlanar(ScreenPtr pScreen, int bank);

/* acceleration */
Bool CHIPSAccelInit(ScreenPtr pScreen);
void CHIPSSync(ScrnInfoPtr pScrn);
Bool CHIPSMMIOAccelInit(ScreenPtr pScreen);
void CHIPSMMIOSync(ScrnInfoPtr pScrn);
Bool CHIPSHiQVAccelInit(ScreenPtr pScreen);
void CHIPSHiQVSync(ScrnInfoPtr pScrn);
Bool CHIPSCursorInit(ScreenPtr pScreen);

/* ddc */
extern void chips_ddc1(ScrnInfoPtr pScrn);
extern Bool chips_i2cInit(ScrnInfoPtr pScrn);

/* To aid debugging of 32 bit register access we make the following defines */
/*
#define DEBUG
#define CT_HW_DEBUG 
*/
#if defined(DEBUG) & defined(CT_HW_DEBUG)
#define HW_DEBUG(x) {usleep(500000); ErrorF("Register/Address: 0x%X\n",x);}
#else
#define HW_DEBUG(x) 
#endif
#endif
