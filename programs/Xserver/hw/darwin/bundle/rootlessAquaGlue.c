/*
 * Generic rootless to Aqua specific glue code
 *
 * This code acts as a glue between the generic rootless X server code
 * and the Aqua specific implementation, which includes definitions that
 * conflict with stardard X types.
 * 
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/rootlessAquaGlue.c,v 1.1 2001/06/26 23:29:12 torrey Exp $ */

#include "regionstr.h"
#include "scrnintstr.h"

#include "darwin.h"
#include "quartz.h"
#include "rootlessAqua.h"
#include "rootlessAquaImp.h"
#include "rootless.h"


/////////////////////////////////////////
// Rootless mode callback glue

static void 
AquaGlueCreateFrame(ScreenPtr pScreen, RootlessFramePtr pFrame, 
		    RootlessFramePtr pUpper)
{
    // fixme won't work for multi-screen
    // fixme hardcoded monitor stuff

    pFrame->devPrivate = AquaNewWindow(pUpper ? pUpper->devPrivate : NULL,
				       pFrame->x,pFrame->y,pFrame->w,pFrame->h,
				       pFrame->isRoot);
    AquaGetPixmap(pFrame->devPrivate, &pFrame->pixelData,
                  &pFrame->bytesPerRow, &pFrame->depth,
                  &pFrame->bitsPerPixel);
}


static void 
AquaGlueDestroyFrame(ScreenPtr pScreen, RootlessFramePtr pFrame)
{
    AquaDestroyWindow(pFrame->devPrivate);
}

static void 
AquaGlueMoveFrame(ScreenPtr pScreen, RootlessFramePtr pFrame, 
		  int oldX, int oldY)
{
    AquaMoveWindow(pFrame->devPrivate, pFrame->x, pFrame->y);
}


static void 
AquaGlueStartResizeFrame(ScreenPtr pScreen, RootlessFramePtr pFrame, 
			 int oldX, int oldY, 
			 unsigned int oldW, unsigned int oldH)
{
    AquaStartResizeWindow(pFrame->devPrivate, 
			  pFrame->x, pFrame->y, pFrame->w, pFrame->h);
    AquaGetPixmap(pFrame->devPrivate, &pFrame->pixelData, 
                  &pFrame->bytesPerRow, &pFrame->depth, 
                  &pFrame->bitsPerPixel);
}

static void 
AquaGlueFinishResizeFrame(ScreenPtr pScreen, RootlessFramePtr pFrame, 
			  int oldX, int oldY, 
			  unsigned int oldW, unsigned int oldH)
{
    AquaFinishResizeWindow(pFrame->devPrivate, 
			   pFrame->x, pFrame->y, pFrame->w, pFrame->h);
}


static void 
AquaGlueRestackFrame(ScreenPtr pScreen, RootlessFramePtr pFrame, 
                     RootlessFramePtr pOldPrev, 
                     RootlessFramePtr pNewPrev)
{
    AquaRestackWindow(pFrame->devPrivate, 
		      pNewPrev ? pNewPrev->devPrivate : NULL);
}

static void
AquaGlueReshapeFrame(ScreenPtr pScreen, RootlessFramePtr pFrame,
		     RegionPtr pNewShape)
{
    if (pFrame->isRoot) return; // shouldn't happen; mi or dix covers this
    AquaReshapeWindow(pFrame->devPrivate,
                      (fakeBoxRec *) REGION_RECTS(pNewShape),
                      REGION_NUM_RECTS(pNewShape));
}

static void 
AquaGlueUpdateRegion(ScreenPtr pScreen, RootlessFramePtr pFrame,
		     RegionPtr pDamage)
{
    AquaUpdateRects(pFrame->devPrivate,
                    (fakeBoxRec *) REGION_RECTS(pDamage),
                    REGION_NUM_RECTS(pDamage));
}

#if 0
static void
AquaGlueStartDrawing(ScreenPtr pScreen, RootlessFramePtr pFrame)
{
    AquaStartDrawing(pFrame->devPrivate, &pFrame->pixelData, 
                     &pFrame->bytesPerRow, &pFrame->depth, 
                     &pFrame->bitsPerPixel);
}

static void
AquaGlueStopDrawing(ScreenPtr pScreen, RootlessFramePtr pFrame)
{
    AquaStopDrawing(pFrame->devPrivate);
}
#endif

static RootlessFrameProcs aquaRootlessProcs = {
    AquaGlueCreateFrame, 
    AquaGlueDestroyFrame, 
    AquaGlueMoveFrame,
    AquaGlueStartResizeFrame, 
    AquaGlueFinishResizeFrame, 
    AquaGlueRestackFrame, 
    AquaGlueReshapeFrame,
    AquaGlueUpdateRegion
};


///////////////////////////////////////
// Rootless mode initialization.
// Exported by rootlessAqua.h

/*
 * AquaGlueDisplayInit
 *  Init the framebuffer and record pixmap parameters for the screen.
 */
void 
AquaGlueDisplayInit(void) 
{
    dfb.pixelInfo.pixelType = kIORGBDirectPixels;
    AquaDisplayInit(&dfb.width, &dfb.height, &dfb.pitch,
                    &dfb.pixelInfo.bitsPerComponent,
                    &dfb.pixelInfo.componentCount, &dfb.bitsPerPixel);
    dfb.colorBitsPerPixel = 
        dfb.pixelInfo.bitsPerComponent * dfb.pixelInfo.componentCount;

    // No frame buffer - it's all in window pixmaps.
    dfb.framebuffer = NULL; // malloc(dfb.pitch * dfb.height); 
}


Bool
AquaAddScreen(ScreenPtr pScreen)
{
    if (! RootlessInit(pScreen, &aquaRootlessProcs)) return FALSE;

    return TRUE;
}
