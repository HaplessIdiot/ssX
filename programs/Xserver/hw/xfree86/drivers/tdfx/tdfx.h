/*
   Voodoo Banshee driver version 1.0.2

   Author: Daryll Strauss

   Copyright: 1998,1999
*/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tdfx/tdfx.h,v 1.2 1999/08/29 12:42:57 dawes Exp $ */

#ifndef _TDFX_H_
#define _TDFX_H_

#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "tdfxdefs.h"

#ifdef XF86DRI
#include "xf86drm.h"
#include "sarea.h"
#define _XF86DRI_SERVER_
#include "xf86dri.h"
#include "dri.h"
#include "GL/glxint.h"
#include "tdfx_dri.h"
#include "tdfx_dripriv.h"
#endif

struct _TDFXRec;
typedef struct _TDFXRec *TDFXPtr;

#ifdef PROP_3DFX
#include "tdfx_priv.h"
#endif
extern Bool TDFXInitPrivate(ScreenPtr pScreen);
extern void TDFXShutdownPrivate(ScreenPtr pScreen);

#ifdef XF86DRI
extern void FillPrivateDRI(TDFXPtr pTDFX, TDFXDRIPtr pTDFXDRI);
#endif

#if 0
/* These are not normally turned on. They are only included for debugging. */
#define TRACEACCEL 1
#define TRACE 1
#define TRACECURS 1
#define REGDEBUG 1
#endif

#ifdef TRACE
#define TDFXTRACE ErrorF
#else
#define TDFXTRACE 0 && (unsigned long)
#endif

#ifdef TRACEACCEL
#define TDFXTRACEACCEL ErrorF
#else
#define TDFXTRACEACCEL 0 && (unsigned long)
#endif

#ifdef TRACECURS
#define TDFXTRACECURS ErrorF
#else
#define TDFXTRACECURS 0 && (unsigned long)
#endif

#ifdef TRACEREG
#define TDFXTRACEREG ErrorF
#else
#define TDFXTRACEREG 0 && (unsigned long)
#endif

#include <xaa.h>
#include <xf86Cursor.h>

typedef void (*TDFXWriteIndexedByteFunc)(TDFXPtr pTDFX, int addr, 
					 char index, char value);
typedef char (*TDFXReadIndexedByteFunc)(TDFXPtr pTDFX, int addr, 
					char index);
typedef void (*TDFXWriteWordFunc)(TDFXPtr pTDFX, int addr, int value);
typedef int (*TDFXReadWordFunc)(TDFXPtr pTDFX, int addr);
typedef void (*TDFXSyncFunc)(ScrnInfoPtr pScrn);

typedef struct {
  unsigned int vidcfg;
  unsigned int vidpll;
  unsigned int dacmode;
  unsigned int vgainit0;
  unsigned int vgainit1;
  unsigned int screensize;
  unsigned int stride;
  unsigned int cursloc;
  unsigned int startaddr;
  unsigned int clip0min;
  unsigned int clip0max;
  unsigned int clip1min;
  unsigned int clip1max;
  unsigned char ExtVga[2];
} TDFXRegRec, *TDFXRegPtr;

typedef struct _TDFXRec {
  unsigned char *MMIOBase;
  unsigned char *FbBase;
  unsigned int PIOBase;
  long FbMapSize;
  int stride;
  int cpp;
  int maxClip;
  int MaxClock;
  int Chipset;
  int LinearAddr;
  int MMIOAddr;
  EntityInfoPtr pEnt;
  pciVideoPtr PciInfo;
  PCITAG PciTag;
  int HasSGRAM;
  int PciCnt;
  int DrawState;
  int Cmd;
  BoxRec prevBlitDest;
  TDFXRegRec SavedReg;
  TDFXRegRec ModeReg;
  XAAInfoRecPtr AccelInfoRec;
  xf86CursorInfoPtr CursorInfoRec;
  CloseScreenProcPtr CloseScreen;
  Bool usePIO;
  Bool NoAccel;
  DGAModePtr DGAModes;
  Bool DGAactive;
  int DGAViewportStatus;
  int lowMemLoc;
  int cursorOffset;
  int fbOffset;
  int texOffset;
  TDFXWriteIndexedByteFunc writeControl;
  TDFXReadIndexedByteFunc readControl;
  TDFXWriteWordFunc writeLong;
  TDFXReadWordFunc readLong;
  TDFXSyncFunc sync;
  int syncDone;
#ifdef PROPDATA
  PROPDATA;
#endif
#ifdef XF86DRI
  Bool directRenderingEnabled;
  DRIInfoPtr pDRIInfo;
  int drmSubFD;
  int numVisualConfigs;
  __GLXvisualConfig* pVisualConfigs;
  TDFXConfigPrivPtr pVisualConfigsPriv;
  TDFXRegRec DRContextRegs;
#endif
} TDFXRec;

#define TDFXPTR(p) ((TDFXPtr)((p)->driverPrivate))

#define DRAW_STATE_CLIPPING 0x1
#define DRAW_STATE_TRANSPARENT 0x2

#define TDFX2XCUTOFF 135000

extern Bool TDFXAccelInit(ScreenPtr pScreen);
extern Bool TDFXCursorInit(ScreenPtr pScreen);
extern void TDFXSync(ScrnInfoPtr pScrn);
extern Bool TDFXDRIScreenInit(ScreenPtr pScreen);
extern void TDFXDRICloseScreen(ScreenPtr pScreen);
extern Bool TDFXDRIFinishScreenInit(ScreenPtr pScreen);
extern Bool TDFXDGAInit(ScreenPtr pScreen);

extern Bool TDFXSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
extern void TDFXAdjustFrame(int scrnIndex, int x, int y, int flags);

extern void TDFXSetPIOAccess(TDFXPtr pTDFX);
extern void TDFXSetMMIOAccess(TDFXPtr pTDFX);
extern void TDFXWriteLongMMIO(TDFXPtr pTDFX, int addr, int val);
extern int TDFXReadLongMMIO(TDFXPtr pTDFX, int addr);

extern void TDFXNeedSync(ScrnInfoPtr pScrn);
extern void TDFXCheckSync(ScrnInfoPtr pScrn);

#endif


