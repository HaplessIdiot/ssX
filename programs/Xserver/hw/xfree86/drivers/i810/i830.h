
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright © 2002 David Dawes

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830.h,v 1.2 2002/09/12 04:08:25 dawes Exp $ */

/*
 * Authors:
 *   Keith Whitwell <keithw@precisioninsight.com>
 *   David Dawes <dawes@tungstengraphics.com>
 *
 */

#if 0
#define I830DEBUG
#endif

#ifndef REMAP_RESERVED
#define REMAP_RESERVED 0
#endif

#ifndef _I830_H_
#define _I830_H_

#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "i810_reg.h"
#include "xaa.h"
#include "xf86Cursor.h"
#include "xf86xv.h"
#include "xf86int10.h"
#include "vbe.h"
#include "vgaHW.h"

#ifdef XF86DRI
#include "xf86drm.h"
#include "sarea.h"
#define _XF86DRI_SERVER_
#include "xf86dri.h"
#include "dri.h"
#include "GL/glxint.h"
#include "i830_dri.h"
#endif

#include "common.h"

/* I830 Video BIOS support */

/*
 * The mode handling is based upon the VESA driver written by:
 * Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

typedef struct _VESARec {
   vbeInfoPtr pVbe;
   VbeInfoBlock *vbeInfo;
   pointer state, pstate;		/* SVGA state */
   int statePage, stateSize, stateMode;
   CARD32 *savedPal;
   xf86MonPtr monitor;
   CARD8 saveSWF1;
   CARD8 newSWF1;
   Bool overrideBIOSMemSize;
   Bool useDefaultRefresh;		/*
					 * Don't try to set the refresh rate
					 * for any modes.
					 */
   Bool enableDisplays;			/*
					 * Use BIOS call 0x5f64 to explicitly
					 * enable displays.
					 */
   Bool useExtendedRefresh;		/*
					 * Use BIOS call 0x5f05 to set the
					 * refresh rate.
					 */
} VESARec, *VESAPtr;


typedef struct _I830Rec *I830Ptr;

typedef void (*I830WriteIndexedByteFunc)(I830Ptr pI830, IOADDRESS addr,
                                         CARD8 index, CARD8 value);
typedef CARD8(*I830ReadIndexedByteFunc)(I830Ptr pI830, IOADDRESS addr,
                                        CARD8 index);
typedef void (*I830WriteByteFunc)(I830Ptr pI830, IOADDRESS addr, CARD8 value);
typedef CARD8(*I830ReadByteFunc)(I830Ptr pI830, IOADDRESS addr);

/* Linear region allocated in framebuffer. */
typedef struct _I830MemPool *I830MemPoolPtr;
typedef struct _I830MemRange *I830MemRangePtr;
typedef struct _I830MemRange {
   long Start;
   long End;
   unsigned long Size;
   unsigned long Physical;
   unsigned long Offset;		/* Offset of AGP-allocated portion */
   unsigned long Alignment;
   int Key;
   I830MemPoolPtr Pool;
} I830MemRange;

typedef struct _I830MemPool {
   I830MemRange Total;
   I830MemRange Free;
   I830MemRange Fixed;
   I830MemRange Allocated;
} I830MemPool;

typedef struct {
   int tail_mask;
   I830MemRange mem;
   unsigned char *virtual_start;
   int head;
   int tail;
   int space;
} I830RingBuffer;

typedef struct {
   unsigned int Fence[8];
} I830RegRec, *I830RegPtr;

typedef struct _I830Rec {
   unsigned char *MMIOBase;
   unsigned char *FbBase;
   int cpp;

   unsigned int bufferOffset;		/* for I830SelectBuffer */
   BoxRec FbMemBox;
   int CacheLines;

   /* These are set in PreInit and never changed. */
   unsigned long FbMapSize;
   unsigned long TotalVideoRam;
   I830MemRange StolenMemory;		/* pre-allocated memory */

   /* These change according to what has been allocated. */
   unsigned long FreeMemory;
   I830MemRange MemoryAperture;
   I830MemPool StolenPool;
  
   /* Regions allocated either from the above pools, or from agpgart. */
   I830MemRange FrontBuffer;
   I830MemRange	CursorMem;
   I830RingBuffer LpRing;
   I830MemRange Scratch;

#if REMAP_RESERVED
   I830MemRange Dummy;
#endif

#ifdef I830_XV
   /* For Xvideo */
   I830MemRange OverlayMem;
#endif

#ifdef XF86DRI
   I830MemRange BackBuffer;
   I830MemRange DepthBuffer;
   I830MemRange TexMem;
   I830MemRange BufferMem;
   I830MemRange ContextMem;
   int TexGranularity;
#endif

   Bool NeedRingBufferLow;

   int auxPitch;
   int auxPitchBits;

   Bool CursorNeedsPhysical;

   DGAModePtr DGAModes;
   int numDGAModes;
   Bool DGAactive;
   int DGAViewportStatus;

   int Chipset;
   unsigned long LinearAddr;
   unsigned long MMIOAddr;
   IOADDRESS ioBase;
   EntityInfoPtr pEnt;
   pciVideoPtr PciInfo;
   PCITAG PciTag;

   unsigned int BR[20];

   int GttBound;

   unsigned char **ScanlineColorExpandBuffers;
   int NumScanlineColorExpandBuffers;
   int nextColorExpandBuf;

   I830RegRec SavedReg;
   I830RegRec ModeReg;

   Bool noAccel;
   Bool SWCursor;
   XAAInfoRecPtr AccelInfoRec;
   xf86CursorInfoPtr CursorInfoRec;
   CloseScreenProcPtr CloseScreen;
   ScreenBlockHandlerProcPtr BlockHandler;

   I830WriteIndexedByteFunc writeControl;
   I830ReadIndexedByteFunc readControl;
   I830WriteByteFunc writeStandard;
   I830ReadByteFunc readStandard;

   Bool XvEnabled;			/* false if I830_XV not defined. */

#ifdef I830_XV
   int colorKey;
   XF86VideoAdaptorPtr adaptor;
   Bool overlayOn;
#endif
   
   Bool directRenderingDisabled;	/* DRI disabled always. */
   Bool directRenderingEnabled;		/* DRI enabled this generation. */

#ifdef XF86DRI
   Bool directRenderingOpen;
   int LockHeld;
   DRIInfoPtr pDRIInfo;
   int drmSubFD;
   int numVisualConfigs;
   __GLXvisualConfig *pVisualConfigs;
   I830ConfigPrivPtr pVisualConfigsPriv;
   drmHandle buffer_map;
   drmHandle ring_map;
#endif

   OptionInfoPtr Options;

   /* Stolen memory support */
   Bool StolenOnly;

   /* Video BIOS support */
   VESAPtr vesa;

   int configuredDevices;
   /* [0] is Pipe A, [1] is Pipe B. */
   int pipeDevices[2];
   Bool pipeEnabled[2];
   BoxRec pipeDisplaySize[2];
   /* [0] is display plane A, [1] is display plane B. */
   int planeEnabled[2];
} I830Rec;

#define I830PTR(p) ((I830Ptr)((p)->driverPrivate))
#define I830REGPTR(p) (&(I830PTR(p)->ModeReg))

#define I830_SELECT_FRONT	0
#define I830_SELECT_BACK	1
#define I830_SELECT_DEPTH	2

/* I830 specific functions */
extern int I830WaitLpRing(ScrnInfoPtr pScrn, int n, int timeout_millis);
extern void I830SetPIOAccess(I830Ptr pI830);
extern void I830SetMMIOAccess(I830Ptr pI830);
extern void I830PrintErrorState(ScrnInfoPtr pScrn);
extern void I830Sync(ScrnInfoPtr pScrn);
extern void I830InitHWCursor(ScrnInfoPtr pScrn);
extern Bool I830CursorInit(ScreenPtr pScreen);
extern void I830EmitInvarientState(ScrnInfoPtr pScrn);
extern void I830SelectBuffer(ScrnInfoPtr pScrn, int buffer);

extern void I830RefreshRing(ScrnInfoPtr pScrn);
extern void I830EmitFlush(ScrnInfoPtr pScrn);

extern Bool I830DGAInit(ScreenPtr pScreen);

extern void I830InitVideo(ScreenPtr pScreen);

#ifdef XF86DRI
extern Bool I830Allocate3DMemory(ScrnInfoPtr pScrn);
extern void I830SetupMemoryTiling(ScrnInfoPtr pScrn);
extern Bool I830DoPoolAllocation(ScrnInfoPtr pScrn, I830MemPool *pool);
extern Bool I830DRIScreenInit(ScreenPtr pScreen);
extern Bool I830DRIDoMappings(ScreenPtr pScreen);
extern void I830DRICloseScreen(ScreenPtr pScreen);
extern Bool I830DRIFinishScreenInit(ScreenPtr pScreen);
extern Bool I830InitDma(ScrnInfoPtr pScrn);
extern Bool I830CleanupDma(ScrnInfoPtr pScrn);
#endif
extern Bool I830AccelInit(ScreenPtr pScreen);
extern void I830SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
					   int ydir, int rop,
					   unsigned int planemask,
					   int trans_color);
extern void I830SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int srcX,
					     int srcY, int dstX, int dstY,
					     int w, int h);
extern void I830SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				  unsigned int planemask);
extern void I830SubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y,
					int w, int h);

extern int I830CheckAvailableMemory(ScrnInfoPtr pScrn);
extern Bool I830Allocate2DMemory(ScrnInfoPtr pScrn, Bool initial);
extern Bool I830FixupOffsets(ScrnInfoPtr pScrn);
extern Bool I830BindGARTMemory(ScrnInfoPtr pScrn);
extern Bool I830UnbindGARTMemory(ScrnInfoPtr pScrn);
extern unsigned long I830AllocVidMem(ScrnInfoPtr pScrn, I830MemRange *result,
				     I830MemPool *pool, unsigned long size,
				     unsigned long alignment, int flags);

extern void I830PrintAllRegisters(I830RegPtr i830Reg);
extern void I830ReadAllRegisters(I830Ptr pI830, I830RegPtr i830Reg);

/*
 * 8000KB is the amount reported by VBE when 8MB is reserved.
 * 132K of that is used for tables, leaving 8060KB, but VBE reports in
 * multiples of 64K so rounding down gives 8000KB.
 */
#define I830_MINIMUM_VBIOS_MEM		8000
#define I830_DEFAULT_VIDEOMEM		(MB(8) / 1024)

/* Flags for I830AllocVidMem() */
#define FROM_ANYWHERE			0x0000
#define FROM_POOL_ONLY			0x0001
#define FROM_NEW_ONLY			0x0002
#define FROM_MASK			0x000f
#define ALLOCATE_AT_TOP			0x0010
#define ALLOCATE_AT_BOTTOM		0x0020
#define	FORCE_GAPS			0x0040
#define NEED_PHYSICAL_ADDR		0x0100
#define ALIGN_BOTH_ENDS			0x0200

#endif /* _I830_H_ */
