/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128_dri.c,v 1.4 2000/06/26 05:41:32 martin Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Rickard E. Faith <faith@precisioninsight.com>
 *   Daryll Strauss <daryll@precisioninsight.com>
 *
 */


				/* X and server generic header files */
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86RAC.h"
#include "xf86cmap.h"
#include "xf86fbman.h"

				/* Backing store, software cursor, and
                                   colormap initialization */
#include "mibstore.h"
#include "mipointer.h"
#include "micmap.h"

				/* CFB support */
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"

				/* XAA and Cursor Support */
#include "xaa.h"
#include "xf86Cursor.h"

				/* PCI support */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

				/* DDC support */
#include "xf86DDC.h"

				/* DRI support */
#include "GL/glxint.h"
#include "GL/glxtokens.h"
#include "xf86drm.h"
#include "xf86drmR128.h"
#include "sarea.h"
#define _XF86DRI_SERVER_
#include "xf86dri.h"
#include "dri.h"
#include "r128_dri.h"
#include "r128_sarea.h"
#include "r128_dripriv.h"

				/* Driver data structures */
#include "r128.h"
#include "r128_reg.h"

#define R128_WATERMARK_L      16
#define R128_WATERMARK_M       8
#define R128_WATERMARK_N       8
#define R128_WATERMARK_K     128

static int CCEFifoSlots = 0;

#define R128CCEWaitForFifo(pScrn, entries)                                   \
do {                                                                         \
    if (CCEFifoSlots < entries) R128WaitForFifoFunction(pScrn, entries);     \
    CCEFifoSlots -= entries;                                                 \
} while (0)

/* Wait for at least `entries' slots are free.  The actual number of
   slots available is stored in info->CCEFifoSize. */
static void R128CCEWaitForFifoFunction(ScrnInfoPtr pScrn, int entries)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int            i;

    for (;;) {
	for (i = 0; i < R128_TIMEOUT; i++) {
	    CCEFifoSlots = INREG(R128_PM4_STAT) & R128_PM4_FIFOCNT_MASK;
	    if (CCEFifoSlots >= entries) return;
	}
	R128EngineReset(pScrn);
	if (info->CCE2D) R128CCEStart(pScrn);
    }
}

/* Wait until the CCE is completely idle: the FIFO has drained and the
   CCE is idle. */
void R128CCEWaitForIdle(ScrnInfoPtr pScrn)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int            i;

    if (!info->CCEInUse || info->CCEMode == R128_PM4_NONPM4) return;

    if (R128CCE_USE_RING_BUFFER(info->CCEMode)) {
	volatile CARD32 *r128RingReadPtr =
	    (volatile CARD32 *)(info->ringReadPtr);
	R128SAREAPrivPtr pSAREAPriv;

	OUTREGP(R128_PM4_BUFFER_DL_WPTR,
		R128_PM4_BUFFER_DL_DONE, ~R128_PM4_BUFFER_DL_DONE);

	pSAREAPriv = (R128SAREAPrivPtr)DRIGetSAREAPrivate(pScrn->pScreen);

	for (;;) {
	    for (i = 0; i < R128_TIMEOUT; i++) {
		if (*r128RingReadPtr == pSAREAPriv->ringWrite) {
		    int pm4stat = INREG(R128_PM4_STAT);
		    if ((pm4stat & R128_PM4_FIFOCNT_MASK) >= info->CCEFifoSize
			&& !(pm4stat & (R128_PM4_BUSY|R128_PM4_GUI_ACTIVE))) {
			R128EngineFlush(pScrn);
			return;
		    }
		}
	    }
	    R128EngineReset(pScrn);
	    if (info->CCE2D) R128CCEStart(pScrn);
	}
    } else {
	R128CCEWaitForFifoFunction(pScrn, info->CCEFifoSize);

	for (;;) {
	    for (i = 0; i < R128_TIMEOUT; i++) {
		if (!(INREG(R128_PM4_STAT)
		      & (R128_PM4_BUSY | R128_PM4_GUI_ACTIVE))) {
		    R128EngineFlush(pScrn);
		    return;
		}
	    }
	    R128EngineReset(pScrn);
	    if (info->CCE2D) R128CCEStart(pScrn);
	}
    }
}

/* Reset the ring buffer status, if the engine was reset */
void R128CCEResetRing(ScrnInfoPtr pScrn)
{
    R128InfoPtr       info            = R128PTR(pScrn);
    unsigned char    *R128MMIO        = info->MMIO;
    R128SAREAPrivPtr  pSAREAPriv;
    volatile CARD32  *r128RingReadPtr;

    if (!info->CCEInUse || info->CCEMode == R128_PM4_NONPM4) return;

    r128RingReadPtr = (volatile CARD32 *)(info->ringReadPtr);
    pSAREAPriv      = (R128SAREAPrivPtr)DRIGetSAREAPrivate(pScrn->pScreen);

    OUTREG(R128_PM4_BUFFER_DL_WPTR, 0);
    OUTREG(R128_PM4_BUFFER_DL_RPTR, 0);
    pSAREAPriv->ringWrite = 0;
    *r128RingReadPtr = 0;

    /* Resetting the ring turns off the CCE */
    info->CCEInUse = FALSE;
}

/* Start the CCE, but only if it is not already in use and the requested
   mode is a CCE mode.  The mode is stored in info->CCEMode. */
void R128CCEStart(ScrnInfoPtr pScrn)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

    if (info->CCEInUse || info->CCEMode == R128_PM4_NONPM4) return;

    R128WaitForIdle(pScrn);
    OUTREG(R128_PM4_BUFFER_CNTL, info->CCEMode | info->ringSizeLog2QW);
    (void)INREG(R128_PM4_BUFFER_ADDR); /* as per the sample code */
    OUTREG(R128_PM4_MICRO_CNTL, R128_PM4_MICRO_FREERUN);
    info->CCEInUse = TRUE;
}

/* Stop the CCE, but only if it is in use and the requested mode is not
   the non-CCE mode.  This function also flushes any outstanding
   requests before switching modes.*/
void R128CCEStop(ScrnInfoPtr pScrn)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

    if (!info->CCEInUse || info->CCEMode == R128_PM4_NONPM4) return;

    R128CCEWaitForIdle(pScrn);
    OUTREG(R128_PM4_MICRO_CNTL, 0);
    OUTREG(R128_PM4_BUFFER_CNTL, R128_PM4_NONPM4);
    R128EngineReset(pScrn);
    info->CCEInUse = FALSE;
}

/* Initialize the visual configs that are supported by the hardware.
   These are combined with the visual configs that the indirect
   rendering core supports, and the intersection is exported to the
   client. */
static Bool R128InitVisualConfigs(ScreenPtr pScreen)
{
    ScrnInfoPtr       pScrn            = xf86Screens[pScreen->myNum];
    R128InfoPtr       pR128            = R128PTR(pScrn);
    int               numConfigs       = 0;
    __GLXvisualConfig *pConfigs        = 0;
    R128ConfigPrivPtr pR128Configs     = 0;
    R128ConfigPrivPtr *pR128ConfigPtrs = 0;
    int               i, accum, stencil;

    switch (pR128->CurrentLayout.pixel_code) {
    case 8:  /* 8bpp mode is not support */
    case 15: /* FIXME */
    case 24: /* FIXME */
	return FALSE;

#define R128_USE_ACCUM   1
#define R128_USE_STENCIL 1

    case 16:
	numConfigs = 1;
	if (R128_USE_ACCUM)   numConfigs *= 2;
	if (R128_USE_STENCIL) numConfigs *= 2;

	if (!(pConfigs
	      = (__GLXvisualConfig*)xnfcalloc(sizeof(__GLXvisualConfig),
					      numConfigs))) {
	    return FALSE;
	}
	if (!(pR128Configs
	      = (R128ConfigPrivPtr)xnfcalloc(sizeof(R128ConfigPrivRec),
					     numConfigs))) {
	    xfree(pConfigs);
	    return FALSE;
	}
	if (!(pR128ConfigPtrs
	      = (R128ConfigPrivPtr*)xnfcalloc(sizeof(R128ConfigPrivPtr),
					      numConfigs))) {
	    xfree(pConfigs);
	    xfree(pR128Configs);
	    return FALSE;
	}

	i = 0;
	for (accum = 0; accum <= R128_USE_ACCUM; accum++) {
	    for (stencil = 0; stencil <= R128_USE_STENCIL; stencil++) {
		pR128ConfigPtrs[i] = &pR128Configs[i];

		pConfigs[i].vid                = -1;
		pConfigs[i].class              = -1;
		pConfigs[i].rgba               = TRUE;
		pConfigs[i].redSize            = 5;
		pConfigs[i].greenSize          = 6;
		pConfigs[i].blueSize           = 5;
		pConfigs[i].alphaSize          = 0;
		pConfigs[i].redMask            = 0x0000F800;
		pConfigs[i].greenMask          = 0x000007E0;
		pConfigs[i].blueMask           = 0x0000001F;
		pConfigs[i].alphaMask          = 0x00000000;
		if (accum) { /* Simulated in software */
		    pConfigs[i].accumRedSize   = 16;
		    pConfigs[i].accumGreenSize = 16;
		    pConfigs[i].accumBlueSize  = 16;
		    pConfigs[i].accumAlphaSize = 0;
		} else {
		    pConfigs[i].accumRedSize   = 0;
		    pConfigs[i].accumGreenSize = 0;
		    pConfigs[i].accumBlueSize  = 0;
		    pConfigs[i].accumAlphaSize = 0;
		}
		pConfigs[i].doubleBuffer       = TRUE;
		pConfigs[i].stereo             = FALSE;
		pConfigs[i].bufferSize         = 16;
		pConfigs[i].depthSize          = 16;
		if (stencil)
		    pConfigs[i].stencilSize    = 8; /* Simulated in software */
		else
		    pConfigs[i].stencilSize    = 0;
		pConfigs[i].auxBuffers         = 0;
		pConfigs[i].level              = 0;
		pConfigs[i].visualRating       = GLX_NONE_EXT;
		pConfigs[i].transparentPixel   = GLX_NONE;
		pConfigs[i].transparentRed     = 0;
		pConfigs[i].transparentGreen   = 0;
		pConfigs[i].transparentBlue    = 0;
		pConfigs[i].transparentAlpha   = 0;
		pConfigs[i].transparentIndex   = 0;
		i++;
	    }
	}
	break;
    case 32:
	numConfigs = 1;
	if (R128_USE_ACCUM)   numConfigs *= 2;
	if (R128_USE_STENCIL) numConfigs *= 2;

	if (!(pConfigs
	      = (__GLXvisualConfig*)xnfcalloc(sizeof(__GLXvisualConfig),
					      numConfigs))) {
	    return FALSE;
	}
	if (!(pR128Configs
	      = (R128ConfigPrivPtr)xnfcalloc(sizeof(R128ConfigPrivRec),
					     numConfigs))) {
	    xfree(pConfigs);
	    return FALSE;
	}
	if (!(pR128ConfigPtrs
	      = (R128ConfigPrivPtr*)xnfcalloc(sizeof(R128ConfigPrivPtr),
					      numConfigs))) {
	    xfree(pConfigs);
	    xfree(pR128Configs);
	    return FALSE;
	}

	i = 0;
	for (accum = 0; accum <= R128_USE_ACCUM; accum++) {
	    for (stencil = 0; stencil <= R128_USE_STENCIL; stencil++) {
		pR128ConfigPtrs[i] = &pR128Configs[i];

		pConfigs[i].vid                = -1;
		pConfigs[i].class              = -1;
		pConfigs[i].rgba               = TRUE;
		pConfigs[i].redSize            = 8;
		pConfigs[i].greenSize          = 8;
		pConfigs[i].blueSize           = 8;
		pConfigs[i].alphaSize          = 8;
		pConfigs[i].redMask            = 0x00FF0000;
		pConfigs[i].greenMask          = 0x0000FF00;
		pConfigs[i].blueMask           = 0x000000FF;
		pConfigs[i].alphaMask          = 0xFF000000;
		if (accum) { /* Simulated in software */
		    pConfigs[i].accumRedSize   = 16;
		    pConfigs[i].accumGreenSize = 16;
		    pConfigs[i].accumBlueSize  = 16;
		    pConfigs[i].accumAlphaSize = 16;
		} else {
		    pConfigs[i].accumRedSize   = 0;
		    pConfigs[i].accumGreenSize = 0;
		    pConfigs[i].accumBlueSize  = 0;
		    pConfigs[i].accumAlphaSize = 0;
		}
		pConfigs[i].doubleBuffer       = TRUE;
		pConfigs[i].stereo             = FALSE;
		pConfigs[i].bufferSize         = 24;
		if (stencil) {
		    pConfigs[i].depthSize      = 24;
		    pConfigs[i].stencilSize    = 8;
		} else {
		    pConfigs[i].depthSize      = 32;
		    pConfigs[i].stencilSize    = 0;
		}
		pConfigs[i].auxBuffers         = 0;
		pConfigs[i].level              = 0;
		pConfigs[i].visualRating       = GLX_NONE_EXT;
		pConfigs[i].transparentPixel   = GLX_NONE;
		pConfigs[i].transparentRed     = 0;
		pConfigs[i].transparentGreen   = 0;
		pConfigs[i].transparentBlue    = 0;
		pConfigs[i].transparentAlpha   = 0;
		pConfigs[i].transparentIndex   = 0;
		i++;
	    }
	}
	break;
    }

    pR128->numVisualConfigs   = numConfigs;
    pR128->pVisualConfigs     = pConfigs;
    pR128->pVisualConfigsPriv = pR128Configs;
    GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pR128ConfigPtrs);
    return TRUE;
}

/* Create the Rage 128-specific context information */
static Bool R128CreateContext(ScreenPtr pScreen, VisualPtr visual,
			      drmContext hwContext, void *pVisualConfigPriv,
			      DRIContextType contextStore)
{
    /* Nothing yet */
    return TRUE;
}

/* Destroy the Rage 128-specific context information */
static void R128DestroyContext(ScreenPtr pScreen, drmContext hwContext,
			       DRIContextType contextStore)
{
    /* Nothing yet */
}

/* Called when the X server is woken up to allow the last client's
   context to be saved and the X server's context to be loaded.  This is
   not necessary for the Rage 128 since the client detects when it's
   context is not currently loaded and then load's it itself.  Since the
   registers to start and stop the CCE are privileged, only the X server
   can start/stop the engine. */
static void R128EnterServer(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128 = R128PTR(pScrn);

    if (pR128->accel) pR128->accel->NeedToSync = TRUE;

#if 1
    if (!pR128->CCE2D) R128CCEStop(pScrn);
#else
    if (pR128->CCE2D) R128CCEWaitForIdle(pScrn);
    else              R128CCEStop(pScrn);
#endif
}

/* Called when the X server goes to sleep to allow the X server's
   context to be saved and the last client's context to be loaded.  This
   is not necessary for the Rage 128 since the client detects when it's
   context is not currently loaded and then load's it itself.  Since the
   registers to start and stop the CCE are privileged, only the X server
   can start/stop the engine. */
static void R128LeaveServer(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128 = R128PTR(pScrn);

#if 1
    if (!pR128->CCE2D) R128CCEStart(pScrn);
#else
    if (pR128->CCE2D) R128CCEWaitForIdle(pScrn);
    else              R128CCEStart(pScrn);
#endif
}

/* Contexts can be swapped by the X server if necessary.  This callback
   is currently only used to perform any functions necessary when
   entering or leaving the X server, and in the future might not be
   necessary. */
static void R128DRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
			       DRIContextType oldContextType, void *oldContext,
			       DRIContextType newContextType, void *newContext)
{
    if ((syncType==DRI_3D_SYNC) && (oldContextType==DRI_2D_CONTEXT) &&
	(newContextType==DRI_2D_CONTEXT)) { /* Entering from Wakeup */
	R128EnterServer(pScreen);
    }
    if ((syncType==DRI_2D_SYNC) && (oldContextType==DRI_NO_CONTEXT) &&
	(newContextType==DRI_2D_CONTEXT)) { /* Exiting from Block Handler */
	R128LeaveServer(pScreen);
    }
}

/* Initialize the state of the back and depth buffers. */
static void R128DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
    /* FIXME: This routine needs to have acceleration turned on */
    ScreenPtr   pScreen = pWin->drawable.pScreen;
    ScrnInfoPtr pScrn   = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128   = R128PTR(pScrn);
    BoxPtr      pbox;
    int         nbox;
    int         depth;

    /* FIXME: Use accel when CCE 2D code is written */
    if (pR128->CCE2D) return;

    /* FIXME: This should be based on the __GLXvisualConfig info */
    switch (pScrn->bitsPerPixel) {
    case  8: depth = 0x000000ff; break;
    case 16: depth = 0x0000ffff; break;
    case 24: depth = 0x00ffffff; break;
    case 32: depth = 0xffffffff; break;
    default: depth = 0x00000000; break;
    }

    /* FIXME: Copy XAAPaintWindow() and use REGION_TRANSLATE() */
    /* FIXME: Only initialize the back and depth buffers for contexts
       that request them */

    pbox = REGION_RECTS(prgn);
    nbox = REGION_NUM_RECTS(prgn);

    (*pR128->accel->SetupForSolidFill)(pScrn, 0, GXcopy, -1);
    for (; nbox; nbox--, pbox++) {
	(*pR128->accel->SubsequentSolidFillRect)(pScrn,
						 pbox->x1 + pR128->fbX,
						 pbox->y1 + pR128->fbY,
						 pbox->x2 - pbox->x1,
						 pbox->y2 - pbox->y1);
	(*pR128->accel->SubsequentSolidFillRect)(pScrn,
						 pbox->x1 + pR128->backX,
						 pbox->y1 + pR128->backY,
						 pbox->x2 - pbox->x1,
						 pbox->y2 - pbox->y1);
    }

    (*pR128->accel->SetupForSolidFill)(pScrn, depth, GXcopy, -1);
    for (; nbox; nbox--, pbox++)
	(*pR128->accel->SubsequentSolidFillRect)(pScrn,
						 pbox->x1 + pR128->depthX,
						 pbox->y1 + pR128->depthY,
						 pbox->x2 - pbox->x1,
						 pbox->y2 - pbox->y1);

    pR128->accel->NeedToSync = TRUE;
}

/* Copy the back and depth buffers when the X server moves a window. */
static void R128DRIMoveBuffers(WindowPtr pWin, DDXPointRec ptOldOrg,
			       RegionPtr prgnSrc, CARD32 index)
{
    ScreenPtr   pScreen = pWin->drawable.pScreen;
    ScrnInfoPtr pScrn   = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128   = R128PTR(pScrn);

    /* FIXME: This routine needs to have acceleration turned on */
    /* FIXME: Copy XAACopyWindow() and use REGION_TRANSLATE() */
    /* FIXME: Only initialize the back and depth buffers for contexts
       that request them */

    /* FIXME: Use accel when CCE 2D code is written */
    if (pR128->CCE2D) return;
}

/* Initialize the AGP state.  Request memory for use in AGP space, and
   initialize the Rage 128 registers to point to that memory. */
static Bool R128DRIAgpInit(R128InfoPtr pR128, ScreenPtr pScreen)
{
    unsigned char *R128MMIO = pR128->MMIO;
    unsigned long mode;
    unsigned int  vendor, device;
    int           ret;
    unsigned long cntl;
    int           s, l;
    int           flags;

    if (drmAgpAcquire(pR128->drmFD) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] AGP not available\n");
	return FALSE;
    }

				/* Modify the mode if the default mode is
                                   not appropriate for this particular
                                   combination of graphics card and AGP
                                   chipset. */

    mode   = drmAgpGetMode(pR128->drmFD);	/* Default mode */
    vendor = drmAgpVendorId(pR128->drmFD);
    device = drmAgpDeviceId(pR128->drmFD);

    mode &= ~R128_AGP_MODE_MASK;
    switch (pR128->agpMode) {
    case 2:          mode |= R128_AGP_2X_MODE;
    case 1: default: mode |= R128_AGP_1X_MODE;
    }

    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Mode 0x%08lx [AGP 0x%04x/0x%04x; Card 0x%04x/0x%04x]\n",
	       mode, vendor, device,
	       pR128->PciInfo->vendor,
	       pR128->PciInfo->chipType);

    if (drmAgpEnable(pR128->drmFD, mode) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] AGP not enabled\n");
	drmAgpRelease(pR128->drmFD);
	return FALSE;
    }

    pR128->agpOffset = 0;

    if ((ret = drmAgpAlloc(pR128->drmFD, pR128->agpSize*1024*1024, 0, NULL,
			   &pR128->agpMemHandle)) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Out of memory (%d)\n", ret);
	drmAgpRelease(pR128->drmFD);
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] %d kB allocated with handle 0x%08x\n",
	       pR128->agpSize*1024, pR128->agpMemHandle);

    if (drmAgpBind(pR128->drmFD, pR128->agpMemHandle, pR128->agpOffset) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Could not bind\n");
	drmAgpFree(pR128->drmFD, pR128->agpMemHandle);
	drmAgpRelease(pR128->drmFD);
	return FALSE;
    }

				/* Initialize the CCE ring buffer data */
    pR128->ringStart       = pR128->agpOffset;
    pR128->ringMapSize     = pR128->ringSize*1024*1024 + 4096;
    pR128->ringSizeLog2QW  = R128MinBits(pR128->ringSize*1024*1024/8) - 1;

    pR128->ringReadOffset  = pR128->ringStart + pR128->ringMapSize;
    pR128->ringReadMapSize = 4096;

				/* Reserve space for the vertex buffer */
    pR128->vbStart         = pR128->ringReadOffset + pR128->ringReadMapSize;
    pR128->vbMapSize       = pR128->vbSize*1024*1024;

				/* Reserve space for the indirect buffer */
    pR128->indStart        = pR128->vbStart + pR128->vbMapSize;
    pR128->indMapSize      = pR128->indSize*1024*1024;

				/* Reserve the rest for AGP textures */
    pR128->agpTexStart     = pR128->indStart + pR128->indMapSize;
    s = (pR128->agpSize*1024*1024 - pR128->agpTexStart);
    l = R128MinBits((s-1) / R128_NR_TEX_REGIONS);
    if (l < R128_LOG_TEX_GRANULARITY) l = R128_LOG_TEX_GRANULARITY;
    pR128->agpTexMapSize   = (s >> l) << l;
    pR128->log2AGPTexGran  = l;

    if (pR128->CCESecure) flags = DRM_READ_ONLY;
    else                  flags = 0;

    if (drmAddMap(pR128->drmFD, pR128->ringStart, pR128->ringMapSize,
		  DRM_AGP, flags, &pR128->ringHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add ring mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] ring handle = 0x%08lx\n", pR128->ringHandle);

    if (drmMap(pR128->drmFD, pR128->ringHandle, pR128->ringMapSize,
	       (drmAddressPtr)&pR128->ring) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Could not map ring\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Ring mapped at 0x%08lx\n",
	       (unsigned long)pR128->ring);

    if (drmAddMap(pR128->drmFD, pR128->ringReadOffset, pR128->ringReadMapSize,
		  DRM_AGP, flags, &pR128->ringReadPtrHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add ring read ptr mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] ring read ptr handle = 0x%08lx\n",
	       pR128->ringReadPtrHandle);

    if (drmMap(pR128->drmFD, pR128->ringReadPtrHandle, pR128->ringReadMapSize,
	       (drmAddressPtr)&pR128->ringReadPtr) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map ring read ptr\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Ring read ptr mapped at 0x%08lx\n",
	       (unsigned long)pR128->ringReadPtr);

    if (drmAddMap(pR128->drmFD, pR128->vbStart, pR128->vbMapSize,
		  DRM_AGP, 0, &pR128->vbHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add vertex buffers mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] vertex buffers handle = 0x%08lx\n", pR128->vbHandle);

    if (drmMap(pR128->drmFD, pR128->vbHandle, pR128->vbMapSize,
	       (drmAddressPtr)&pR128->vb) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map vertex buffers\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Vertex buffers mapped at 0x%08lx\n",
	       (unsigned long)pR128->vb);

    if (drmAddMap(pR128->drmFD, pR128->indStart, pR128->indMapSize,
		  DRM_AGP, flags, &pR128->indHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add indirect buffers mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] indirect buffers handle = 0x%08lx\n", pR128->indHandle);

    if (drmMap(pR128->drmFD, pR128->indHandle, pR128->indMapSize,
	       (drmAddressPtr)&pR128->ind) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map indirect buffers\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Indirect buffers mapped at 0x%08lx\n",
	       (unsigned long)pR128->ind);

    if (drmAddMap(pR128->drmFD, pR128->agpTexStart, pR128->agpTexMapSize,
		  DRM_AGP, 0, &pR128->agpTexHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add AGP texture map mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] AGP texture map handle = 0x%08lx\n",
	       pR128->agpTexHandle);

    if (drmMap(pR128->drmFD, pR128->agpTexHandle, pR128->agpTexMapSize,
	       (drmAddressPtr)&pR128->agpTex) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map AGP texture map\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] AGP Texture map mapped at 0x%08lx\n",
	       (unsigned long)pR128->agpTex);

				/* Initialize Rage 128's AGP registers */
    cntl  = INREG(R128_AGP_CNTL);
    cntl &= ~R128_AGP_APER_SIZE_MASK;
    switch (pR128->agpSize) {
    case 256: cntl |= R128_AGP_APER_SIZE_256MB; break;
    case 128: cntl |= R128_AGP_APER_SIZE_128MB; break;
    case  64: cntl |= R128_AGP_APER_SIZE_64MB;  break;
    case  32: cntl |= R128_AGP_APER_SIZE_32MB;  break;
    case  16: cntl |= R128_AGP_APER_SIZE_16MB;  break;
    case   8: cntl |= R128_AGP_APER_SIZE_8MB;   break;
    case   4: cntl |= R128_AGP_APER_SIZE_4MB;   break;
    default:
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Illegal aperture size %d kB\n",
		   pR128->agpSize*1024);
	return FALSE;
    }
    OUTREG(R128_AGP_BASE, pR128->ringHandle); /* Ring buf is at AGP offset 0 */
    OUTREG(R128_AGP_CNTL, cntl);

    return TRUE;
}

/* Fake the vertex buffers for PCI cards. */
static Bool R128DRIPciInit(R128InfoPtr pR128, ScreenPtr pScreen)
{
    pR128->vbStart = 0;
    pR128->vbMapSize = pR128->vbSize*1024*1024;

    return TRUE;
}

/* Add a map for the MMIO registers that will be accessed by any
   DRI-based clients. */
static Bool R128DRIMapInit(R128InfoPtr pR128, ScreenPtr pScreen)
{
    int flags;

    if (pR128->CCESecure) flags = DRM_READ_ONLY;
    else                  flags = 0;

				/* Map registers */
    pR128->registerSize = R128_MMIOSIZE;
    if (drmAddMap(pR128->drmFD, pR128->MMIOAddr, pR128->registerSize,
		  DRM_REGISTERS, flags, &pR128->registerHandle) < 0) {
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] register handle = 0x%08lx\n", pR128->registerHandle);

    return TRUE;
}

/* Initialize the ring buffer state for use in the X server and any
   DRI-based clients. */
static void R128DRICCEInitRingBuffer(ScrnInfoPtr pScrn)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    unsigned long  addr;

    /* FIXME: When we use the CCE for the X server, we should move this
       function (and the support functions above) to r128_accel.c */

				/* The manual (p. 2) says this address is
                                   in "VM space".  This means it's an
                                   offset from the start of AGP space. */
    OUTREG(R128_PM4_BUFFER_OFFSET, info->ringStart | 0x02000000);

    OUTREG(R128_PM4_BUFFER_DL_WPTR, 0);
    OUTREG(R128_PM4_BUFFER_DL_RPTR, 0);

				/* DL_RPTR_ADDR is a physical address.
                                   This should be in the SAREA. */
    *(volatile long unsigned *)(info->ringReadPtr) = 0;
    OUTREG(R128_PM4_BUFFER_DL_RPTR_ADDR, (info->ringReadPtrHandle));

				/* Set watermark control */
    OUTREG(R128_PM4_BUFFER_WM_CNTL,
	   ((R128_WATERMARK_L/4) << R128_WMA_SHIFT)
	   | ((R128_WATERMARK_M/4) << R128_WMB_SHIFT)
	   | ((R128_WATERMARK_N/4) << R128_WMC_SHIFT)
	   | ((R128_WATERMARK_K/64) << R128_WB_WM_SHIFT));

    addr = INREG(R128_PM4_BUFFER_ADDR);	/* Force read.  Why?  Because it's
                                           in the examples... */

#if 0
    R128CCEWaitForIdle(pScrn);
#endif

				/* Turn on bus mastering */
    info->BusCntl &= ~R128_BUS_MASTER_DIS;
    OUTREGP(R128_BUS_CNTL, 0, ~R128_BUS_MASTER_DIS);
}

/* Initialize the kernel data structures. */
static int R128DRIKernelInit(R128InfoPtr pR128, ScreenPtr pScreen)
{
    drmR128Init      drmInfo;

    drmInfo.sarea_priv_offset   = sizeof(XF86DRISAREARec);
    drmInfo.is_pci              = pR128->IsPCI;
    drmInfo.cce_mode            = pR128->CCEMode;
    drmInfo.cce_fifo_size       = pR128->CCEFifoSize;
    drmInfo.cce_secure          = pR128->CCESecure;
    drmInfo.ring_size           = pR128->ringSize*1024*1024;
    drmInfo.usec_timeout        = pR128->CCEusecTimeout;

    drmInfo.fb_offset           = pR128->LinearAddr;
    drmInfo.agp_ring_offset     = pR128->ringHandle;
    drmInfo.agp_read_ptr_offset = pR128->ringReadPtrHandle;
    drmInfo.agp_vertbufs_offset = pR128->vbHandle;
    drmInfo.agp_indbufs_offset  = pR128->indHandle;
    drmInfo.agp_textures_offset = pR128->agpTexHandle;
    drmInfo.mmio_offset         = pR128->registerHandle;

    if (drmR128InitCCE(pR128->drmFD, &drmInfo) < 0) return FALSE;

    return TRUE;
}

/* Add a map for the vertex buffers that will be accessed by any
   DRI-based clients. */
static Bool R128DRIBufInit(R128InfoPtr pR128, ScreenPtr pScreen)
{
				/* Initialize vertex buffers */
    if ((pR128->vbNumBufs = drmAddBufs(pR128->drmFD,
				       pR128->vbMapSize / pR128->vbBufSize,
				       pR128->vbBufSize,
				       DRM_AGP_BUFFER,
				       pR128->vbStart)) <= 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Could not create vertex buffers list\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] Added %d %d byte vertex buffers\n",
	       pR128->vbNumBufs, pR128->vbBufSize);

    if (drmMarkBufs(pR128->drmFD, 0.133333, 0.266666)) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Failed to mark vertex buffers list\n");
	return FALSE;
    }

    if (!(pR128->vbBufs = drmMapBufs(pR128->drmFD))) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Failed to map vertex buffers list\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] Mapped %d vertex buffers\n",
	       pR128->vbBufs->count);

    return TRUE;
}

/* Load the microcode for the CCE */
static void R128DRILoadMicrocode(ScrnInfoPtr pScrn)
{
    unsigned char *R128MMIO = R128PTR(pScrn)->MMIO;
    int            i;
    unsigned long  R128Microcode[] = {
	/* CCE microcode (from ATI) */
    0, 276838400, 0, 268449792, 2, 142, 2, 145, 0, 1076765731, 0, 1617039951,
    0, 774592877, 0, 1987540286, 0, 2307490946U, 0, 599558925, 0, 589505315, 0,
    596487092, 0, 589505315, 1, 11544576, 1, 206848, 1, 311296, 1, 198656, 2,
    912273422, 11, 262144, 0, 0, 1, 33559837, 1, 7438, 1, 14809, 1, 6615, 12,
    28, 1, 6614, 12, 28, 2, 23, 11, 18874368, 0, 16790922, 1, 409600, 9, 30, 1,
    147854772, 16, 420483072, 3, 8192, 0, 10240, 1, 198656, 1, 15630, 1, 51200,
    10, 34858, 9, 42, 1, 33559823, 2, 10276, 1, 15717, 1, 15718, 2, 43, 1,
    15936948, 1, 570480831, 1, 14715071, 12, 322123831, 1, 33953125, 12, 55, 1,
    33559908, 1, 15718, 2, 46, 4, 2099258, 1, 526336, 1, 442623, 4, 4194365, 1,
    509952, 1, 459007, 3, 0, 12, 92, 2, 46, 12, 176, 1, 15734, 1, 206848, 1,
    18432, 1, 133120, 1, 100670734, 1, 149504, 1, 165888, 1, 15975928, 1,
    1048576, 6, 3145806, 1, 15715, 16, 2150645232U, 2, 268449859, 2, 10307, 12,
    176, 1, 15734, 1, 15735, 1, 15630, 1, 15631, 1, 5253120, 6, 3145810, 16,
    2150645232U, 1, 15864, 2, 82, 1, 343310, 1, 1064207, 2, 3145813, 1, 15728,
    1, 7817, 1, 15729, 3, 15730, 12, 92, 2, 98, 1, 16168, 1, 16167, 1, 16002,
    1, 16008, 1, 15974, 1, 15975, 1, 15990, 1, 15976, 1, 15977, 1, 15980, 0,
    15981, 1, 10240, 1, 5253120, 1, 15720, 1, 198656, 6, 110, 1, 180224, 1,
    103824738, 2, 112, 2, 3145839, 0, 536885440, 1, 114880, 14, 125, 12,
    206975, 1, 33559995, 12, 198784, 0, 33570236, 1, 15803, 0, 15804, 3,
    294912, 1, 294912, 3, 442370, 1, 11544576, 0, 811612160, 1, 12593152, 1,
    11536384, 1, 14024704, 7, 310382726, 0, 10240, 1, 14796, 1, 14797, 1,
    14793, 1, 14794, 0, 14795, 1, 268679168, 1, 9437184, 1, 268449792, 1,
    198656, 1, 9452827, 1, 1075854602, 1, 1075854603, 1, 557056, 1, 114880, 14,
    159, 12, 198784, 1, 1109409213, 12, 198783, 1, 1107312059, 12, 198784, 1,
    1109409212, 2, 162, 1, 1075854781, 1, 1073757627, 1, 1075854780, 1, 540672,
    1, 10485760, 6, 3145894, 16, 274741248, 9, 168, 3, 4194304, 3, 4209949, 0,
    0, 0, 256, 14, 174, 1, 114857, 1, 33560007, 12, 176, 0, 10240, 1, 114858,
    1, 33560018, 1, 114857, 3, 33560007, 1, 16008, 1, 114874, 1, 33560360, 1,
    114875, 1, 33560154, 0, 15963, 0, 256, 0, 4096, 1, 409611, 9, 188, 0,
    10240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
    };

    R128WaitForIdle(pScrn);

    OUTREG(R128_PM4_MICROCODE_ADDR, 0);
    for (i = 0; i < 256; i += 1) {
        OUTREG(R128_PM4_MICROCODE_DATAH, R128Microcode[i*2]);
        OUTREG(R128_PM4_MICROCODE_DATAL, R128Microcode[i*2 + 1]);
    }
}

/* Initialize the CCE state, and start the CCE (if used by the X server) */
static void R128DRICCEInit(ScrnInfoPtr pScrn)
{
    R128InfoPtr    info     = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

				/* CCEMode is initialized in r128_driver.c */
    switch (info->CCEMode) {
    case R128_PM4_NONPM4:                 info->CCEFifoSize = 0;   break;
    case R128_PM4_192PIO:                 info->CCEFifoSize = 192; break;
    case R128_PM4_192BM:                  info->CCEFifoSize = 192; break;
    case R128_PM4_128PIO_64INDBM:         info->CCEFifoSize = 128; break;
    case R128_PM4_128BM_64INDBM:          info->CCEFifoSize = 128; break;
    case R128_PM4_64PIO_128INDBM:         info->CCEFifoSize = 64;  break;
    case R128_PM4_64BM_128INDBM:          info->CCEFifoSize = 64;  break;
    case R128_PM4_64PIO_64VCBM_64INDBM:   info->CCEFifoSize = 64;  break;
    case R128_PM4_64BM_64VCBM_64INDBM:    info->CCEFifoSize = 64;  break;
    case R128_PM4_64PIO_64VCPIO_64INDPIO: info->CCEFifoSize = 64;  break;
    }

    if (info->CCE2D) {
				/* Make sure the CCE is on for the X server */
	R128CCEStart(pScrn);
    } else {
				/* Make sure the CCE is off for the X server */
	OUTREG(R128_PM4_MICRO_CNTL, 0);
	OUTREG(R128_PM4_BUFFER_CNTL, R128_PM4_NONPM4);
    }
}

/* Initialize the screen-specific data structures for the DRI and the
   Rage 128.  This is the main entry point to the device-specific
   initialization code.  It calls device-independent DRI functions to
   create the DRI data structures and initialize the DRI state. */
Bool R128DRIScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr   pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr   pR128 = R128PTR(pScrn);
    DRIInfoPtr    pDRIInfo;
    R128DRIPtr    pR128DRI;
    int           major, minor, patch;
    drmVersionPtr version;
    
    /* Check that the GLX, DRI, and DRM modules have been loaded by testing
     * for known symbols in each module. */
    if (!xf86LoaderCheckSymbol("GlxSetVisualConfigs")) return FALSE;
    if (!xf86LoaderCheckSymbol("DRIScreenInit"))       return FALSE;
    if (!xf86LoaderCheckSymbol("drmAvailable"))        return FALSE;
    if (!xf86LoaderCheckSymbol("DRIQueryVersion")) {
      xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "R128DRIScreenInit failed (libdri.a too old)\n");
      return FALSE;
    }

    /* Check the DRI version */
    DRIQueryVersion(&major, &minor, &patch);
    if (major != 3 || minor != 0 || patch < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "R128DRIScreenInit failed "
		   "(DRI version = %d.%d.%d, expected 3.0.x).  "
		   "Disabling DRI.\n",
		   major, minor, patch);
	return FALSE;
    }

    switch (pR128->CurrentLayout.pixel_code) {
    case 8:
	/* These modes are not supported (yet). */
    case 15:
    case 24:
	return FALSE;

	/* Only 16 and 32 color depths are supports currently. */
    case 16:
    case 32:
	break;
    }

    /* Create the DRI data structure, and fill it in before calling the
       DRIScreenInit(). */
    if (!(pDRIInfo = DRICreateInfoRec())) return FALSE;

    pR128->pDRIInfo                      = pDRIInfo;
    pDRIInfo->drmDriverName              = R128_NAME;
    pDRIInfo->clientDriverName           = R128_NAME;
    pDRIInfo->busIdString                = xalloc(64);
    sprintf(pDRIInfo->busIdString,
	    "PCI:%d:%d:%d",
	    pR128->PciInfo->bus,
	    pR128->PciInfo->device,
	    pR128->PciInfo->func);
    pDRIInfo->ddxDriverMajorVersion      = R128_VERSION_MAJOR;
    pDRIInfo->ddxDriverMinorVersion      = R128_VERSION_MINOR;
    pDRIInfo->ddxDriverPatchVersion      = R128_VERSION_PATCH;
    pDRIInfo->frameBufferPhysicalAddress = pR128->LinearAddr;
    pDRIInfo->frameBufferSize            = pR128->FbMapSize;
    pDRIInfo->frameBufferStride          = (pScrn->displayWidth *
					    pR128->CurrentLayout.pixel_bytes);
    pDRIInfo->ddxDrawableTableEntry      = R128_MAX_DRAWABLES;
    pDRIInfo->maxDrawableTableEntry      = (SAREA_MAX_DRAWABLES
					    < R128_MAX_DRAWABLES
					    ? SAREA_MAX_DRAWABLES
					    : R128_MAX_DRAWABLES);

#ifdef NOT_DONE
    /* FIXME: Need to extend DRI protocol to pass this size back to
     * client for SAREA mapping that includes a device private record
     */
    pDRIInfo->SAREASize =
	((sizeof(XF86DRISAREARec) + 0xfff) & 0x1000); /* round to page */
    /* + shared memory device private rec */
#else
    /* For now the mapping works by using a fixed size defined
     * in the SAREA header
     */
    if (sizeof(XF86DRISAREARec)+sizeof(R128SAREAPriv)>SAREA_MAX) {
	ErrorF("Data does not fit in SAREA\n");
	return FALSE;
    }
    pDRIInfo->SAREASize = SAREA_MAX;
#endif

    if (!(pR128DRI = (R128DRIPtr)xnfcalloc(sizeof(R128DRIRec),1))) {
	DRIDestroyInfoRec(pR128->pDRIInfo);
	pR128->pDRIInfo = NULL;
	return FALSE;
    }
    pDRIInfo->devPrivate     = pR128DRI;
    pDRIInfo->devPrivateSize = sizeof(R128DRIRec);
    pDRIInfo->contextSize    = sizeof(R128DRIContextRec);

    pDRIInfo->CreateContext  = R128CreateContext;
    pDRIInfo->DestroyContext = R128DestroyContext;
    pDRIInfo->SwapContext    = R128DRISwapContext;
    pDRIInfo->InitBuffers    = R128DRIInitBuffers;
    pDRIInfo->MoveBuffers    = R128DRIMoveBuffers;
    pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;

    if (!DRIScreenInit(pScreen, pDRIInfo, &pR128->drmFD)) {
	xfree(pDRIInfo->devPrivate);
	pDRIInfo->devPrivate = NULL;
	DRIDestroyInfoRec(pDRIInfo);
	pDRIInfo = NULL;
	return FALSE;
    }

    /* Check the r128 DRM version */
    version = drmGetVersion(pR128->drmFD);
    if (version) {
	if (version->version_major != 1 ||
	    version->version_minor != 0 ||
	    version->version_patchlevel < 0) {
            /* incompatible drm version */
            xf86DrvMsg(pScreen->myNum, X_ERROR,
                       "R128DRIScreenInit failed "
		       "(DRM version = %d.%d.%d, expected 1.0.x).  "
		       "Disabling DRI.\n",
                       version->version_major,
                       version->version_minor,
                       version->version_patchlevel);
            drmFreeVersion(version);
            R128DRICloseScreen(pScreen);
            return FALSE;
	}
	drmFreeVersion(version);
    }

				/* Initialize AGP */
    if (!pR128->IsPCI && !R128DRIAgpInit(pR128, pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }
				/* Initialize PCI */
    if (pR128->IsPCI && !R128DRIPciInit(pR128, pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }

				/* DRIScreenInit doesn't add all the
                                   common mappings.  Add additional
                                   mappings here. */
    if (!R128DRIMapInit(pR128, pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }

				/* Initialize the ring buffer */
    if (!pR128->IsPCI) R128DRICCEInitRingBuffer(pScrn);

				/* Initialize the kernel data structures */
    if (!R128DRIKernelInit(pR128, pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }

				/* Initialize vertex buffers list */
    if (!pR128->IsPCI && !R128DRIBufInit(pR128, pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }

				/* FIXME: When are these mappings unmapped? */

    if (!R128InitVisualConfigs(pScreen)) {
	R128DRICloseScreen(pScreen);
	return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Visual configs initialized\n");

				/* Load the CCE Microcode */
    R128DRILoadMicrocode(pScrn);

				/* Reset the Graphics Engine */
    R128EngineReset(pScrn);

    return TRUE;
}

/* Finish initializing the device-dependent DRI state, and call
   DRIFinishScreenInit() to complete the device-independent DRI
   initialization. */
Bool R128DRIFinishScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr      pScrn      = xf86Screens[pScreen->myNum];
    R128InfoPtr      pR128      = R128PTR(pScrn);
    R128SAREAPrivPtr pSAREAPriv;
    R128DRIPtr       pR128DRI;

    /* Init and start the CCE */
    R128DRICCEInit(pScrn);

    pSAREAPriv = (R128SAREAPrivPtr)DRIGetSAREAPrivate(pScreen);
    memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

    pR128->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;
    /* pR128->pDRIInfo->driverSwapMethod = DRI_SERVER_SWAP; */

    pR128DRI                 = (R128DRIPtr)pR128->pDRIInfo->devPrivate;
    pR128DRI->registerHandle = pR128->registerHandle;
    pR128DRI->registerSize   = pR128->registerSize;

    pR128DRI->ringHandle     = pR128->ringHandle;
    pR128DRI->ringMapSize    = pR128->ringMapSize;
    pR128DRI->ringSize       = pR128->ringSize*1024*1024;

    pR128DRI->ringReadPtrHandle = pR128->ringReadPtrHandle;
    pR128DRI->ringReadMapSize   = pR128->ringReadMapSize;

    pR128DRI->vbHandle       = pR128->vbHandle;
    pR128DRI->vbMapSize      = pR128->vbMapSize;
    pR128DRI->vbOffset       = pR128->vbStart;
    pR128DRI->vbBufSize      = pR128->vbBufSize;

    pR128DRI->indHandle      = pR128->indHandle;
    pR128DRI->indMapSize     = pR128->indMapSize;

    pR128DRI->agpTexHandle   = pR128->agpTexHandle;
    pR128DRI->agpTexMapSize  = pR128->agpTexMapSize;
    pR128DRI->log2AGPTexGran = pR128->log2AGPTexGran;
    pR128DRI->agpTexOffset   = pR128->agpTexStart;

    pR128DRI->deviceID       = pR128->Chipset;
    pR128DRI->width          = pScrn->virtualX;
    pR128DRI->height         = pScrn->virtualY;
    pR128DRI->depth          = pScrn->depth;
    pR128DRI->bpp            = pScrn->bitsPerPixel;

    pR128DRI->fbX            = pR128->fbX;
    pR128DRI->fbY            = pR128->fbY;
    pR128DRI->backX          = pR128->backX;
    pR128DRI->backY          = pR128->backY;
    pR128DRI->depthX         = pR128->depthX;
    pR128DRI->depthY         = pR128->depthY;
    pR128DRI->textureX       = pR128->textureX;
    pR128DRI->textureY       = pR128->textureY;
    pR128DRI->textureSize    = pR128->textureSize;
    pR128DRI->log2TexGran    = pR128->log2TexGran;

    pR128DRI->IsPCI          = pR128->IsPCI;

    pR128DRI->CCEMode        = pR128->CCEMode;
    pR128DRI->CCEFifoSize    = pR128->CCEFifoSize;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "0x%08lx %d\n",
	       pR128DRI->registerHandle, pR128DRI->registerSize);
    return DRIFinishScreenInit(pScreen);
}

/* The screen is being closed, so clean up any state and free any
   resources used by the DRI. */
void R128DRICloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128 = R128PTR(pScrn);

				/* Stop the CCE if it is still in use */
    if (pR128->CCE2D) R128CCEStop(pScrn);

				/* De-allocate vertex buffers */
    if (pR128->vbBufs) {
	drmUnmapBufs(pR128->vbBufs);
	pR128->vbBufs = NULL;
    }

				/* De-allocate all kernel resources */
    drmR128CleanupCCE(pR128->drmFD);

				/* De-allocate all AGP resources */
    if (pR128->agpTex) {
	drmUnmap(pR128->agpTex, pR128->agpTexMapSize);
	pR128->agpTex = NULL;
    }
    if (pR128->ind) {
	drmUnmap(pR128->ind, pR128->indMapSize);
	pR128->ind = NULL;
    }
    if (pR128->vb) {
	drmUnmap(pR128->vb, pR128->vbMapSize);
	pR128->vb = NULL;
    }
    if (pR128->ringReadPtr) {
	drmUnmap(pR128->ringReadPtr, pR128->ringReadMapSize);
	pR128->ringReadPtr = NULL;
    }
    if (pR128->ring) {
	drmUnmap(pR128->ring, pR128->ringMapSize);
	pR128->ring = NULL;
    }
    if (pR128->agpMemHandle) {
	drmAgpUnbind(pR128->drmFD, pR128->agpMemHandle);
	drmAgpFree(pR128->drmFD, pR128->agpMemHandle);
	pR128->agpMemHandle = 0;
	drmAgpRelease(pR128->drmFD);
    }

				/* De-allocate all DRI resources */
    DRICloseScreen(pScreen);

				/* De-allocate all DRI data structures */
    if (pR128->pDRIInfo) {
	if (pR128->pDRIInfo->devPrivate) {
	    xfree(pR128->pDRIInfo->devPrivate);
	    pR128->pDRIInfo->devPrivate = NULL;
	}
	DRIDestroyInfoRec(pR128->pDRIInfo);
	pR128->pDRIInfo = NULL;
    }
    if (pR128->pVisualConfigs) {
	xfree(pR128->pVisualConfigs);
	pR128->pVisualConfigs = NULL;
    }
    if (pR128->pVisualConfigsPriv) {
	xfree(pR128->pVisualConfigsPriv);
	pR128->pVisualConfigsPriv = NULL;
    }
}
