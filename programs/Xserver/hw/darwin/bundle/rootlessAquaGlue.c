/*
 * Generic rootless to Aqua specific glue code
 *
 * This code acts as a glue between the generic rootless X server code
 * and the Aqua specific implementation, which includes definitions that
 * conflict with stardard X types.
 *
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/rootlessAquaGlue.c,v 1.3 2001/08/01 05:34:06 torrey Exp $ */

#include "regionstr.h"
#include "scrnintstr.h"

#include "darwin.h"
#include "quartz.h"
#include "rootlessAqua.h"
#include "rootlessAquaImp.h"
#include "rootless.h"
#include "globals.h" // dixScreenOrigins[]


/////////////////////////////////////////
// Rootless mode callback glue

static void
AquaGlueCreateFrame(ScreenPtr pScreen, RootlessFramePtr pFrame,
                    RootlessFramePtr pUpper)
{
    int sx = dixScreenOrigins[pScreen->myNum].x + darwinMainScreenX;
    int sy = dixScreenOrigins[pScreen->myNum].y + darwinMainScreenY;

    pFrame->devPrivate = AquaNewWindow(pUpper ? pUpper->devPrivate : NULL,
                                       pFrame->x + sx, pFrame->y + sy,
                                       pFrame->w, pFrame->h,
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
    int sx = dixScreenOrigins[pScreen->myNum].x + darwinMainScreenX;
    int sy = dixScreenOrigins[pScreen->myNum].y + darwinMainScreenY;

    AquaMoveWindow(pFrame->devPrivate, pFrame->x + sx, pFrame->y + sy);
}


static void
AquaGlueStartResizeFrame(ScreenPtr pScreen, RootlessFramePtr pFrame,
                         int oldX, int oldY,
                         unsigned int oldW, unsigned int oldH)
{
    int sx = dixScreenOrigins[pScreen->myNum].x + darwinMainScreenX;
    int sy = dixScreenOrigins[pScreen->myNum].y + darwinMainScreenY;

    AquaStartResizeWindow(pFrame->devPrivate,
                          pFrame->x + sx, pFrame->y + sy, pFrame->w, pFrame->h);
    AquaGetPixmap(pFrame->devPrivate, &pFrame->pixelData,
                  &pFrame->bytesPerRow, &pFrame->depth,
                  &pFrame->bitsPerPixel);
}

static void
AquaGlueFinishResizeFrame(ScreenPtr pScreen, RootlessFramePtr pFrame,
                          int oldX, int oldY,
                          unsigned int oldW, unsigned int oldH)
{
    int sx = dixScreenOrigins[pScreen->myNum].x + darwinMainScreenX;
    int sy = dixScreenOrigins[pScreen->myNum].y + darwinMainScreenY;

    AquaFinishResizeWindow(pFrame->devPrivate,
                           pFrame->x + sx, pFrame->y + sy,
                           pFrame->w, pFrame->h);
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
    int sx = dixScreenOrigins[pScreen->myNum].x + darwinMainScreenX;
    int sy = dixScreenOrigins[pScreen->myNum].y + darwinMainScreenY;

    if (pFrame->isRoot) return; // shouldn't happen; mi or dix covers this
    REGION_TRANSLATE(pScreen, pNewShape, sx, sy);
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
 * AquaDisplayInit
 *  Find all Aqua screens.
 */
void
AquaDisplayInit(void)
{
    darwinScreensFound = AquaDisplayCount();
}


/*
 * AquaAddScreen
 *  Init the framebuffer and record pixmap parameters for the screen.
 */
Bool
AquaAddScreen(int index, ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);

    dfb->pixelInfo.pixelType = kIORGBDirectPixels;
    AquaScreenInit(index, &dfb->x, &dfb->y, &dfb->width, &dfb->height,
                    &dfb->pitch, &dfb->pixelInfo.bitsPerComponent,
                    &dfb->pixelInfo.componentCount, &dfb->bitsPerPixel);
    dfb->colorBitsPerPixel = dfb->pixelInfo.bitsPerComponent *
                             dfb->pixelInfo.componentCount;

    // No frame buffer - it's all in window pixmaps.
    dfb->framebuffer = NULL; // malloc(dfb.pitch * dfb.height);

    return TRUE;
}

/*
 * AquaSetupScreen
 *  Setup the screen for rootless access.
 */
Bool
AquaSetupScreen(int index, ScreenPtr pScreen)
{
    return RootlessInit(pScreen, &aquaRootlessProcs);
}
