/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86.h,v 3.90 1999/02/12 22:51:55 hohndel Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains declarations for public XFree86 functions and variables,
 * and definitions of public macros.
 *
 * "public" means available to video drivers.
 */

#ifndef _XF86_H
#define _XF86_H

#include "xf86str.h"
#include "xf86Opt.h"
#include <X11/Xfuncproto.h>
#include <stdarg.h>

/* General parameters */
extern int xf86ScreenIndex;		/* Index into pScreen.devPrivates */
extern int xf86PixmapIndex;
extern ScrnInfoPtr *xf86Screens;	/* List of pointers to ScrnInfoRecs */
extern const unsigned char byte_reversed[256];

#define XF86SCRNINFO(p) ((ScrnInfoPtr)((p)->devPrivates[xf86ScreenIndex].ptr))

#define XF86FLIP_PIXELS() \
	do { \
	    if (xf86GetFlipPixels()) { \
		pScreen->whitePixel = (pScreen->whitePixel) ? 0 : 1; \
		pScreen->blackPixel = (pScreen->blackPixel) ? 0 : 1; \
	   } \
	while (0)

#define BOOLTOSTRING(b) ((b) ? "TRUE" : "FALSE")

#define PIX24TOBPP(p) (((p) == Pix24Use24) ? 24 : \
			(((p) == Pix24Use32) ? 32 : 0))

/* Function Prototypes */
#ifndef _NO_XF86_PROTOTYPES

/* xf86Bus.c */

Bool xf86CheckPciSlot(int bus, int device, int func, BusResource res);
Bool xf86ClaimPciSlot(int bus, int device, int func, BusResource res,
		      DriverPtr drvp, int chipset, int scrnIndex);
void xf86ReleasePciSlot(int bus, int device, int func);
pciVideoPtr *xf86GetPciVideoInfo(void);
int xf86GetPciInfoForScreen(int scrnIndex, pciVideoPtr **pPciList,
			    BusResource **pRes);
Bool xf86CheckIsaSlot(BusResource res);
Bool xf86ClaimIsaSlot(BusResource res, DriverPtr drvp, int chipset, 
		      int scrnIndex);
void xf86ReleaseIsaSlot(BusResource res);
int xf86GetIsaInfoForScreen(int scrnIndex, BusResource **pRes);
void xf86FreeBusSlots(int scrnIndex);
Bool xf86ParsePciBusString(const char *busID, int *bus, int *device,
			   int *func);
Bool xf86ParseIsaBusString(const char *busID);
Bool xf86ComparePciBusString(const char *busID, int bus, int device, int func);
void xf86DeleteBusSlotsForScreen(int scrnIndex);
Bool xf86IsPciBus(int scrnIndex);
Bool xf86IsIsaBus(int scrnIndex);
int xf86FindChipsetsForScreen(int scrnIndex, DriverPtr drv, int **chipsets);
void xf86AddControlledResource(ScrnInfoPtr pScreen, resType rt);
void xf86DelControlledResource(xf86ScrnAccessPtr pScAcc, Bool enable);
void xf86EnableAccess(xf86ScrnAccessPtr pScAcc);
Bool xf86IsPrimaryPci(pciVideoPtr pPci);
Bool xf86IsPrimaryIsa(void);
int xf86CheckPciGAType(pciVideoPtr pPci);

/* xf86Config.c */


/* xf86Cursor.c */

void xf86LockZoom(ScreenPtr pScreen, int lock);
void xf86InitViewport(ScrnInfoPtr pScr);
void xf86SetScreenLayout(int scrnIndex, int left, int right, int up, int down);
void xf86SetViewport(ScreenPtr pScreen, int x, int y);
Bool xf86ZoomLocked(ScreenPtr pScreen);
void xf86ZoomViewport(ScreenPtr pScreen, int zoom);
void *xf86GetPointerScreenFuncs(void);

/* xf86DPMS.c */

Bool xf86DPMSInit(ScreenPtr pScreen, DPMSSetProcPtr set, int flags);

/* xf86DGA.c */

DGAInfoPtr DGACreateInfoRec(void);
void DGADestroyInfoRec(DGAInfoPtr pDGAInfo);
Bool DGAInit(ScreenPtr pScreen, DGAInfoPtr pDGAInfo, int flags);

/* xf86Events.c */

void SetTimeSinceLastInputEvent(void);

/* xf86Helper.c */

void xf86AddDriver(DriverPtr driver, pointer module, int flags);
void xf86DeleteDriver(int drvIndex);
ScrnInfoPtr xf86AllocateScreen(DriverPtr drv, int flags);
void xf86DeleteScreen(int scrnIndex, int flags);
int xf86AllocateScrnInfoPrivateIndex(void);
Bool xf86AddPixFormat(ScrnInfoPtr pScrn, int depth, int bpp, int pad);
Bool xf86SetDepthBpp(ScrnInfoPtr scrp, int depth, int bpp, int fbbpp,
		     int depth24flags);
void xf86PrintDepthBpp(ScrnInfoPtr scrp);
Bool xf86SetWeight(ScrnInfoPtr scrp, rgb weight, rgb mask);
Bool xf86SetDefaultVisual(ScrnInfoPtr scrp, int visual);
Bool xf86SetGamma(ScrnInfoPtr scrp, Gamma gamma);
void xf86SetDpi(ScrnInfoPtr pScrn, int x, int y);
void xf86SetBlackWhitePixels(ScreenPtr pScreen);
Bool xf86SaveRestoreImage(int scrnIndex, SaveRestoreFlags what);
void xf86VDrvMsgVerb(int scrnIndex, MessageType type, int verb,
		     const char *format, va_list args);
void xf86DrvMsgVerb(int scrnIndex, MessageType type, int verb,
		    const char *format, ...);
void xf86DrvMsg(int scrnIndex, MessageType type, const char *format, ...);
void xf86MsgVerb(MessageType type, int verb, const char *format, ...);
void xf86Msg(MessageType type, const char *format, ...);
void xf86ErrorFVerb(int verb, const char *format, ...);
void xf86ErrorF(const char *format, ...);
const char *xf86TokenToString(SymTabPtr table, int token);
int xf86StringToToken(SymTabPtr table, const char *string);
void xf86ShowClocks(ScrnInfoPtr scrp, MessageType from);
void xf86PrintChipsets(const char *drvname, const char *drvmsg,
		       SymTabPtr chips);
int xf86MatchDevice(const char *drivername, GDevPtr **driversectlist);
int xf86MatchPciInstances(const char *driverName, int vendorID, 
			  SymTabPtr chipsets, PciChipsets *PCIchipsets,
			  GDevPtr *devList, int numDevs,
			  GDevPtr **foundDevs, pciVideoPtr **foundPCI, 
			  int **foundChips);
BusResource xf86FindPciResource(int numChipset, PciChipsets *PCIchipsets);
int xf86MatchIsaInstances(const char *driverName, SymTabPtr chipsets,
			  IsaChipsets *ISAchipsets, FindIsaDevProc,
			  GDevPtr *devList, int numDevs, GDevPtr *foundDev);
BusResource xf86FindIsaResource(int numChipset, IsaChipsets *ISAchipsets);
void xf86GetClocks(ScrnInfoPtr pScrn, int num,
		   Bool (*ClockFunc)(ScrnInfoPtr, int),
		   void (*ProtectRegs)(ScrnInfoPtr, Bool),
		   void (*BlankScreen)(ScrnInfoPtr, Bool), int vertsyncreg,
		   int maskval, int knownclkindex, int knownclkvalue);
const char *xf86GetVisualName(int visual);
int xf86GetVerbosity(void);
Pix24Flags xf86GetPix24(void);
int xf86GetDepth(void);
rgb xf86GetWeight(void);
Gamma xf86GetGamma(void);
Bool xf86GetFlipPixels(void);
const char *xf86GetServerName(void);
Bool xf86ServerIsExiting(void);
Bool xf86ServerIsResetting(void);
Bool xf86CaughtSignal(void);
pointer xf86LoadSubModule(ScrnInfoPtr pScrn, const char *name);
void xf86LoaderReqSymLists(const char **, ...);
void xf86LoaderReqSymbols(const char *, ...);
void xf86Break1(void);
void xf86Break2(void);
void xf86Break3(void);
void xf86SetBackingStore(ScreenPtr pScreen);
int xf86NewSerialNumber(WindowPtr p, pointer unused);


/* xf86Init.c */

PixmapFormatPtr xf86GetPixFormat(ScrnInfoPtr pScrn, int depth);
int xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth);
void xf86ScanPciRegister(void(*func)(int));

/* xf86Mode.c */

int xf86GetNearestClock(ScrnInfoPtr scrp, int freq, Bool allowDiv2,
			int DivFactor, int MulFactor, int *divider);
const char *xf86ModeStatusToString(ModeStatus status);
ModeStatus xf86LookupMode(ScrnInfoPtr scrp, DisplayModePtr modep,
			  ClockRangePtr clockRanges, LookupModeFlags strategy);
ModeStatus xf86CheckModeForMonitor(DisplayModePtr mode, MonPtr monitor);
ModeStatus xf86InitialCheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode,
					 int maxPitch, int virtualX,
					 int virtualY);
ModeStatus xf86CheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode,
				  int flags);
int xf86ValidateModes(ScrnInfoPtr scrp, DisplayModePtr availModes,
		      char **modeNames, ClockRangePtr clockRanges,
		      int *linePitches, int minPitch, int maxPitch,
		      int minHeight, int maxHeight, int pitchInc,
		      int virtualX, int virtualY, int apertureSize,
		      LookupModeFlags strategy);
void xf86DeleteMode(DisplayModePtr *modeList, DisplayModePtr mode);
void xf86PruneDriverModes(ScrnInfoPtr scrp);
void xf86PruneMonitorModes(MonPtr monp);
void xf86SetCrtcForModes(ScrnInfoPtr scrp, int adjustFlags);
void xf86PrintModes(ScrnInfoPtr scrp);
void xf86ShowClockRanges(ScrnInfoPtr scrp, ClockRangePtr clockRanges);

/* xf86Option.c */

void xf86CollectOptions(ScrnInfoPtr pScrn, pointer extraOpts);

#endif /* _NO_XF86_PROTOTYPES */

#endif /* _XF86_H */
