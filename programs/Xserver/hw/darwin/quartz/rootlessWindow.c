/*
 * Rootless window management
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* Portions of this file are based on fbwindow.c, which contains the
 * following copyright:
 *
 * Copyright © 1998 Keith Packard
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/rootlessWindow.c,v 1.7 2002/06/15 03:59:36 torrey Exp $ */

#include "rootlessCommon.h"
#include "rootlessWindow.h"

#include "fb.h"


// RootlessCreateWindow
// For now, don't create a frame until the window is realized.
// Do reset the window size so it's not clipped by the root window.
Bool
RootlessCreateWindow(WindowPtr pWin)
{
    Bool result;
    RegionRec saveRoot;

    WINREC(pWin) = NULL;
    SCREEN_UNWRAP(pWin->drawable.pScreen, CreateWindow);
    if (!IsRoot(pWin)) {
        // win/border size set by DIX, not by wrapped CreateWindow, so
        // correct it here.
        // Don't HUGE_ROOT when pWin is the root!
        HUGE_ROOT(pWin);
        SetWinSize(pWin);
        SetBorderSize(pWin);
    }
    result = pWin->drawable.pScreen->CreateWindow(pWin);
    if (pWin->parent) {
        NORMAL_ROOT(pWin);
    }
    SCREEN_WRAP(pWin->drawable.pScreen, CreateWindow);
    return result;
}


// RootlessDestroyWindow
// For now, all window destruction takes place in UnrealizeWindow
Bool
RootlessDestroyWindow(WindowPtr pWin)
{
    Bool result;

    SCREEN_UNWRAP(pWin->drawable.pScreen, DestroyWindow);
    result = pWin->drawable.pScreen->DestroyWindow(pWin);
    SCREEN_WRAP(pWin->drawable.pScreen, DestroyWindow);
    return result;
}


#ifdef SHAPE

// RootlessSetShape
// Shape is usually set before the window is mapped, but (for now) we
// don't keep track of frames before they're mapped. So we just record
// that the shape needs to updated later.
void
RootlessSetShape(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RootlessDamageShape(pWin);
    SCREEN_UNWRAP(pScreen, SetShape);
    pScreen->SetShape(pWin);
    SCREEN_WRAP(pScreen, SetShape);
}

#endif // SHAPE


// Disallow ParentRelative background on top-level windows
// because the root window doesn't really have the right background
// and fb will try to draw on the root instead of on the window.
// ParentRelative prevention is also in PaintWindowBackground/Border()
// so it is no longer really needed here.
Bool
RootlessChangeWindowAttributes(WindowPtr pWin, unsigned long vmask)
{
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("change window attributes start ");

    SCREEN_UNWRAP(pScreen, ChangeWindowAttributes);
    result = pScreen->ChangeWindowAttributes(pWin, vmask);
    SCREEN_WRAP(pScreen, ChangeWindowAttributes);

    if (WINREC(pWin)) {
        // disallow ParentRelative background state
        if (pWin->backgroundState == ParentRelative) {
            XID pixel = 0;
            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
        }
    }

    RL_DEBUG_MSG("change window attributes end\n");
    return result;
}


/* RootlessPositionWindow
 *  This is a hook for when DIX moves or resizes a window.
 *  Update the frame position now. (x, y) are *inside* position.
 *  After this, mi and fb are expecting the pixmap to be at the new location.
 */
Bool
RootlessPositionWindow(WindowPtr pWin, int x, int y)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);
    Bool result;

    RL_DEBUG_MSG("positionwindow start (win 0x%x)\n", pWin);

    if (winRec) {
        if (winRec->drawing) {
            // Reset frame's pixmap and move it to the new position.
            int bw = wBorderWidth(pWin);

            winRec->pixmap->devPrivate.ptr = winRec->frame.pixelData;
            SetPixmapBaseToScreen(winRec->pixmap, x - bw, y - bw);

            // Move damaged region to correspond to new window position
            if (REGION_NOTEMPTY(pScreen, &winRec->damage)) {
                REGION_TRANSLATE(pScreen, &winRec->damage,
                                 x - bw - winRec->frame.x,
                                 y - bw - winRec->frame.y);
            }
        }
    }

    SCREEN_UNWRAP(pScreen, PositionWindow);
    result = pScreen->PositionWindow(pWin, x, y);
    SCREEN_WRAP(pScreen, PositionWindow);

    RL_DEBUG_MSG("positionwindow end\n");
    return result;
}


// RootlessRealizeWindow
// The frame is created here and not in CreateWindow so that windows do
// not eat memory until they are realized.
Bool
RootlessRealizeWindow(WindowPtr pWin)
{
    Bool result = FALSE;
    RegionRec saveRoot;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("realizewindow start (win 0x%x) ", pWin);

    if (IsTopLevel(pWin)  ||  IsRoot(pWin)) {
        DrawablePtr d = &pWin->drawable;
        RootlessWindowRec *winRec = xalloc(sizeof(RootlessWindowRec));
        int bw = wBorderWidth(pWin);

        RL_DEBUG_MSG("Top level window ");
        if (! winRec) goto windowcreatebad;

        winRec->frame.isRoot = (pWin == WindowTable[pScreen->myNum]);
        winRec->frame.x = d->x - bw;
        winRec->frame.y = d->y - bw;
        winRec->frame.w = d->width + 2*bw;
        winRec->frame.h = d->height + 2*bw;
        winRec->frame.win = pWin;
        winRec->frame.devPrivate = NULL;

        REGION_INIT(pScreen, &winRec->damage, NullBox, 0);
        winRec->borderWidth = bw;
        winRec->drawing = FALSE;

        winRec->pixmap = NULL;

        WINREC(pWin) = winRec;

        RL_DEBUG_MSG("creating frame ");
        CallFrameProc(pScreen, CreateFrame,
                      (pScreen, &WINREC(pWin)->frame,
                      pWin->prevSib ? &WINREC(pWin->prevSib)->frame : NULL));
        if (!winRec->frame.devPrivate) goto windowcreatebad;

        // Disallow ParentRelative background state on top-level windows.
        // This might have been set before the window was mapped.
        if (pWin->backgroundState == ParentRelative) {
            XID pixel = 0;
            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
        }

#ifdef SHAPE
        // Shape is usually set before the window is mapped, but
        // (for now) we don't keep track of frames before they're mapped.
        winRec->shapeDamage = TRUE;
#endif
    }

    if (!IsRoot(pWin)) HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, RealizeWindow);
    result = pScreen->RealizeWindow(pWin);
    SCREEN_WRAP(pScreen, RealizeWindow);
    if (!IsRoot(pWin)) NORMAL_ROOT(pWin);

    RL_DEBUG_MSG("realizewindow end\n");
    return result;

windowcreatebad:
    RL_DEBUG_MSG("window create bad! ");
    RL_DEBUG_MSG("realizewindow end\n");
    return NULL;
}


Bool
RootlessUnrealizeWindow(WindowPtr pWin)
{
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("unrealizewindow start ");

    if (IsTopLevel(pWin) || IsRoot(pWin)) {
        RootlessWindowRec *winRec = WINREC(pWin);

        RootlessRedisplay(pWin);
        CallFrameProc(pScreen, DestroyFrame, (pScreen, &winRec->frame));

        REGION_UNINIT(pScreen, &winRec->damage);

        xfree(winRec);
        WINREC(pWin) = NULL;
    }

    SCREEN_UNWRAP(pScreen, UnrealizeWindow);
    result = pScreen->UnrealizeWindow(pWin);
    SCREEN_WRAP(pScreen, UnrealizeWindow);
    RL_DEBUG_MSG("unrealizewindow end\n");
    return result;
}


void
RootlessRestackWindow(WindowPtr pWin, WindowPtr pOldNextSib)
{
    RegionRec saveRoot;
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("restackwindow start ");
    if (winRec) RL_DEBUG_MSG("restack top level \n");

    HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, RestackWindow);
    if (pScreen->RestackWindow) pScreen->RestackWindow(pWin, pOldNextSib);
    SCREEN_WRAP(pScreen, RestackWindow);
    NORMAL_ROOT(pWin);

    if (winRec) {
        // fixme simplify the following

        WindowPtr oldNextW, newNextW, oldPrevW, newPrevW;
        RootlessFramePtr oldNext, newNext, oldPrev, newPrev;

        oldNextW = pOldNextSib;
        while (oldNextW  &&  ! WINREC(oldNextW)) oldNextW = oldNextW->nextSib;
        oldNext = oldNextW ? &WINREC(oldNextW)->frame : NULL;

        newNextW = pWin->nextSib;
        while (newNextW  &&  ! WINREC(newNextW)) newNextW = newNextW->nextSib;
        newNext = newNextW ? &WINREC(newNextW)->frame : NULL;

        oldPrevW= pOldNextSib ? pOldNextSib->prevSib : pWin->parent->lastChild;
        while (oldPrevW  &&  ! WINREC(oldPrevW)) oldPrevW = oldPrevW->prevSib;
        oldPrev = oldPrevW ? &WINREC(oldPrevW)->frame : NULL;

        newPrevW = pWin->prevSib;
        while (newPrevW  &&  ! WINREC(newPrevW)) newPrevW = newPrevW->prevSib;
        newPrev = newPrevW ? &WINREC(newPrevW)->frame : NULL;

        if (pWin->prevSib) {
            WindowPtr w = pWin->prevSib;
            while (w) {
                RL_DEBUG_MSG("w 0x%x\n", w);
                w = w->parent;
            }
        }

        CallFrameProc(pScreen, RestackFrame,
                      (pScreen, &winRec->frame, oldPrev, newPrev));
    }

    RL_DEBUG_MSG("restackwindow end\n");
}


/*
 * Specialized window copy procedures
 */

// Globals needed during window resize and move.
static PixmapPtr gResizeDeathPix = NULL;
static pointer gResizeDeathBits = NULL;
static PixmapPtr gResizeCopyWindowSource = NULL;
static CopyWindowProcPtr gResizeOldCopyWindowProc = NULL;

// CopyWindow() that doesn't do anything.
// For MoveWindow() of top-level windows.
static void
RootlessNoCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                     RegionPtr prgnSrc)
{
    // some code expects the region to be translated
    int dx = ptOldOrg.x - pWin->drawable.x;
    int dy = ptOldOrg.y - pWin->drawable.y;
    RL_DEBUG_MSG("ROOTLESSNOCOPYWINDOW ");

    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);
}


// CopyWindow used during ResizeWindow for gravity moves.
// Cloned from fbCopyWindow
// The original always draws on the root pixmap (which we don't have).
// Instead, draw on the parent window's pixmap.
// Resize version: the old location's pixels are in gResizeCopyWindowSource
static void
RootlessResizeCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                         RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    SCREEN_UNWRAP(pScreen, CopyWindow);
    RL_DEBUG_MSG("resizecopywindowFB start (win 0x%x) ", pWin);

    {
        RegionRec   rgnDst;
        int         dx, dy;

        dx = ptOldOrg.x - pWin->drawable.x;
        dy = ptOldOrg.y - pWin->drawable.y;
        REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
        REGION_INIT(pScreen, &rgnDst, NullBox, 0);
        REGION_INTERSECT(pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

        RootlessStartDrawing(pWin);
        fbCopyRegion (&gResizeCopyWindowSource->drawable,
                      &pScreen->GetWindowPixmap(pWin)->drawable,
                      0,
                      &rgnDst, dx, dy, fbCopyWindowProc, 0, 0);

        // don't update - resize will update everything
        // fixme DO update?
        REGION_UNINIT(pScreen, &rgnDst);
        fbValidateDrawable (&pWin->drawable);
    }

    SCREEN_WRAP(pScreen, CopyWindow);
    RL_DEBUG_MSG("resizecopywindowFB end\n");
}


/* Update *new* location of window. Old location is redrawn with
 *  PaintWindowBackground/Border.
 * Cloned from fbCopyWindow
 * The original always draws on the root pixmap (which we don't have).
 * Instead, draw on the parent window's pixmap.
 */
void
RootlessCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    SCREEN_UNWRAP(pScreen, CopyWindow);
    RL_DEBUG_MSG("copywindowFB start (win 0x%x) ", pWin);

    {
        RegionRec   rgnDst;
        int         dx, dy;

        dx = ptOldOrg.x - pWin->drawable.x;
        dy = ptOldOrg.y - pWin->drawable.y;
        REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);

        REGION_INIT(pScreen, &rgnDst, NullBox, 0);
        REGION_INTERSECT(pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

        RootlessStartDrawing(pWin);
        fbCopyRegion ((DrawablePtr)pWin, (DrawablePtr)pWin,
                      0, &rgnDst, dx, dy, fbCopyWindowProc, 0, 0);

        // prgnSrc has been translated to dst position
        RootlessDamageRegion(pWin, prgnSrc);
        REGION_UNINIT(pScreen, &rgnDst);
        fbValidateDrawable (&pWin->drawable);
    }

    SCREEN_WRAP(pScreen, CopyWindow);
    RL_DEBUG_MSG("copywindowFB end\n");
}


/*
 * Window resize procedures
 */

// Prepare to resize a top-level window.
// The old window's pixels are saved and the implementation is told
// to change the window size.
// (x,y,w,h) is outer frame of window (outside border)
static void
StartFrameResize(WindowPtr pWin, Bool gravity,
                 int oldX, int oldY,
                 unsigned int oldW, unsigned int oldH, unsigned int oldBW,
                 int newX, int newY,
                 unsigned int newW, unsigned int newH, unsigned int newBW)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);

    RL_DEBUG_MSG("RESIZE TOPLEVEL WINDOW with gravity %i ", gravity);
    RL_DEBUG_MSG("%d %d %d %d %d   %d %d %d %d %d  ",
                 oldX, oldY, oldW, oldH, oldBW,
                 newX, newY, newW, newH, newBW);

    RootlessRedisplay(pWin);

    // Make a copy of the current pixmap and all its data.
    // The original will go away when we ask the frame manager to
    // allocate the new pixmap.
    RootlessStartDrawing(pWin);
    gResizeDeathBits = xalloc(winRec->frame.bytesPerRow * winRec->frame.h);
    memcpy(gResizeDeathBits, winRec->frame.pixelData,
           winRec->frame.bytesPerRow * winRec->frame.h);
    gResizeDeathPix =
        GetScratchPixmapHeader(pScreen, winRec->frame.w, winRec->frame.h,
                               winRec->frame.depth, winRec->frame.bitsPerPixel,
                               winRec->frame.bytesPerRow, gResizeDeathBits);
    SetPixmapBaseToScreen(gResizeDeathPix, oldX, oldY);
    RootlessStopDrawing(pWin);

    winRec->frame.x = newX;
    winRec->frame.y = newY;
    winRec->frame.w = newW;
    winRec->frame.h = newH;
    winRec->borderWidth = newBW;

    CallFrameProc(pScreen, StartResizeFrame,
                  (pScreen, &winRec->frame, oldX, oldY, oldW, oldH));
    RootlessStartDrawing(pWin);

    // Use custom CopyWindow when moving gravity bits around
    // ResizeWindow assumes the old window contents are in the same
    // pixmap, but here they're in deathPix instead.
    if (gravity) {
        gResizeCopyWindowSource = gResizeDeathPix;
        gResizeOldCopyWindowProc = pScreen->CopyWindow;
        pScreen->CopyWindow = RootlessResizeCopyWindow;
    }

    // Copy pixels in intersection from src to dst.
    // ResizeWindow assumes these pixels are already present when
    // making gravity adjustments.
    // pWin currently has new-sized pixmap but is in old position.
    // fixme border width change!
    {
        BoxRec rect;
        DrawablePtr src = &gResizeDeathPix->drawable;
        DrawablePtr dst = &pScreen->GetWindowPixmap(pWin)->drawable;
        // These vars are needed because implicit unsigned->signed fails
        int oldX2 = (int)(oldX + oldW), newX2 = (int)(newX + newW);
        int oldY2 = (int)(oldY + oldH), newY2 = (int)(newY + newH);

        rect.x1 = max(oldX, newX);
        rect.y1 = max(oldY, newY);
        rect.x2 = min(oldX2, newX2);
        rect.y2 = min(oldY2, newY2);

        RL_DEBUG_MSG("Resize copy rect %d %d %d %d  ",
                     rect.x1, rect.y1, rect.x2, rect.y2);

        // rect is the intersection of the old location and new location
        if (BOX_NOT_EMPTY(rect)) {
            fbCopyWindowProc(src, dst, NULL, &rect, 1, 0, 0,
                             FALSE, FALSE, 0, 0);
        }
    }
}


static void
FinishFrameResize(WindowPtr pWin, Bool gravity,
                  int oldX, int oldY,
                  unsigned int oldW, unsigned int oldH, unsigned int oldBW,
                  int newX, int newY,
                  unsigned int newW, unsigned int newH, unsigned int newBW)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RootlessWindowRec *winRec = WINREC(pWin);

    CallFrameProc(pScreen, FinishResizeFrame,
                  (pScreen, &winRec->frame, oldX, oldY, oldW, oldH));
    if (wBoundingShape(pWin)) {
        RootlessDamageShape(pWin);
    }

    // Destroy temp pixmap
    FreeScratchPixmapHeader(gResizeDeathPix);
    xfree(gResizeDeathBits);
    gResizeDeathPix = gResizeDeathBits = NULL;

    if (gravity) {
        pScreen->CopyWindow = gResizeOldCopyWindowProc;
        gResizeCopyWindowSource = NULL;
    }
}


// If kind==VTOther, window border is resizing (and borderWidth is
//   already changed!!@#$)  This case works like window resize, not move.
void
RootlessMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pSib, VTKind kind)
{
    CopyWindowProcPtr oldCopyWindowProc = NULL;
    RegionRec saveRoot;
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    int oldX = 0, oldY = 0, newX = 0, newY = 0;
    unsigned int oldW = 0, oldH = 0, oldBW = 0, newW = 0, newH = 0, newBW = 0;

    RL_DEBUG_MSG("movewindow start \n");

    if (winRec) {
        if (kind == VTMove) {
            oldX = winRec->frame.x;
            oldY = winRec->frame.y;
            RootlessRedisplay(pWin);
            RootlessStartDrawing(pWin);
        } else {
            RL_DEBUG_MSG("movewindow border resizing ");
            oldBW = winRec->borderWidth;
            oldX = winRec->frame.x;
            oldY = winRec->frame.y;
            oldW = winRec->frame.w;
            oldH = winRec->frame.h;
            newBW = wBorderWidth(pWin);
            newX = x;
            newY = y;
            newW = pWin->drawable.width  + 2*newBW;
            newH = pWin->drawable.height + 2*newBW;
            StartFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                             newX, newY, newW, newH, newBW);
        }
    }

    HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, MoveWindow);
    if (winRec) {
        oldCopyWindowProc = pScreen->CopyWindow;
        pScreen->CopyWindow = RootlessNoCopyWindow;
    }
    pScreen->MoveWindow(pWin, x, y, pSib, kind);
    if (winRec) {
        pScreen->CopyWindow = oldCopyWindowProc;
    }
    NORMAL_ROOT(pWin);
    SCREEN_WRAP(pScreen, MoveWindow);

    if (winRec) {
        if (kind == VTMove) {
            winRec->frame.x = x;
            winRec->frame.y = y;
            RootlessStopDrawing(pWin);
            CallFrameProc(pScreen, MoveFrame,
                          (pScreen, &winRec->frame, oldX, oldY));
        } else {
            FinishFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW);
        }
    }

    RL_DEBUG_MSG("movewindow end\n");
}


// Note: (x, y, w, h) as passed to this procedure don't match
//  the frame definition.
// (x,y) is corner of very outer edge, *outside* border
// w,h is width and height *inside* border, *ignoring* border width
// The rect (x, y, w, h) doesn't mean anything.
// (x, y, w+2*bw, h+2*bw) is total rect
// (x+bw, y+bw, w, h) is inner rect

void
RootlessResizeWindow(WindowPtr pWin, int x, int y,
                     unsigned int w, unsigned int h, WindowPtr pSib)
{
    RegionRec saveRoot;
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    int oldX = 0, oldY = 0, newX = 0, newY = 0;
    unsigned int oldW = 0, oldH = 0, oldBW = 0, newW = 0, newH = 0, newBW = 0;

    RL_DEBUG_MSG("resizewindow start (win 0x%x) ", pWin);

    if (winRec) {
        oldBW = winRec->borderWidth;
        oldX = winRec->frame.x;
        oldY = winRec->frame.y;
        oldW = winRec->frame.w;
        oldH = winRec->frame.h;

        newBW = oldBW;
        newX = x;
        newY = y;
        newW = w + 2*newBW;
        newH = h + 2*newBW;

        StartFrameResize(pWin, TRUE, oldX, oldY, oldW, oldH, oldBW,
                         newX, newY, newW, newH, newBW);
    }

    HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pScreen, ResizeWindow);
    pScreen->ResizeWindow(pWin, x, y, w, h, pSib);
    SCREEN_WRAP(pScreen, ResizeWindow);
    NORMAL_ROOT(pWin);

    if (winRec) {
        FinishFrameResize(pWin, TRUE, oldX, oldY, oldW, oldH, oldBW,
                          newX, newY, newW, newH, newBW);
    }

    RL_DEBUG_MSG("resizewindow end\n");
}


/*
 * SetPixmapOfAncestors
 *  Set the Pixmaps on all ParentRelative windows up the ancestor chain.
 */
static void
SetPixmapOfAncestors(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr topWin = TopLevelParent(pWin);
    RootlessWindowRec *topWinRec = WINREC(topWin);

    while (pWin->backgroundState == ParentRelative) {
        if (pWin == topWin) {
            // disallow ParentRelative background state on top level
            XID pixel = 0;
            ChangeWindowAttributes(pWin, CWBackPixel, &pixel, serverClient);
            RL_DEBUG_MSG("Cleared ParentRelative on 0x%x.\n", pWin);
            break;
        }

        pWin = pWin->parent;
        pScreen->SetWindowPixmap(pWin, topWinRec->pixmap);
    }
}


#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

/*
 * AquaFillRegionTiled
 *  Fill using a tile while leaving the alpha channel untouched.
 *  Based on fbfillRegionTiled.
 */
void
AquaFillRegionTiled(DrawablePtr pDrawable,
                    RegionPtr   pRegion,
                    PixmapPtr   pTile)
{
    FbBits      *dst;
    FbStride    dstStride;
    int         dstBpp;
    int         dstXoff, dstYoff;
    FbBits      *tile;
    FbStride    tileStride;
    int         tileBpp;
    int         tileXoff, tileYoff; /* XXX assumed to be zero */
    int         tileWidth, tileHeight;
    int         n = REGION_NUM_RECTS(pRegion);
    BoxPtr      pbox = REGION_RECTS(pRegion);
    int         xRot = pDrawable->x;
    int         yRot = pDrawable->y;
    FbBits      planeMask;

#ifdef PANORAMIX
    if(!noPanoramiXExtension)
    {
        int index = pDrawable->pScreen->myNum;
        if(&WindowTable[index]->drawable == pDrawable)
        {
            xRot -= panoramiXdataPtr[index].x;
            yRot -= panoramiXdataPtr[index].y;
        }
    }
#endif
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    fbGetDrawable (&pTile->drawable, tile, tileStride, tileBpp,
                   tileXoff, tileYoff);
    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;
    planeMask = FB_ALLONES & ~AquaAlphaMask(dstBpp);

    while (n--)
    {
        fbTile (dst + (pbox->y1 + dstYoff) * dstStride,
                dstStride,
                (pbox->x1 + dstXoff) * dstBpp,
                (pbox->x2 - pbox->x1) * dstBpp,
                pbox->y2 - pbox->y1,
                tile,
                tileStride,
                tileWidth * dstBpp,
                tileHeight,
                GXcopy,
                planeMask,
                dstBpp,
                xRot * dstBpp,
                yRot - pbox->y1);
        pbox++;
    }
}


/*
 * RootlessPaintWindowBackground
 *  Paint the window background while filling in the alpha channel with all on.
 */
void
RootlessPaintWindowBackground(WindowPtr pWin, RegionPtr pRegion, int what)
{
    int oldBackgroundState = 0;
    PixUnion oldBackground;

    RL_DEBUG_MSG("paintwindowbackground start (win 0x%x, framed %i) ",
                 pWin, framed);
    if (IsFramedWindow(pWin)) {
        if (IsRoot(pWin)) {
            // set root background to magic transparent color
            oldBackgroundState = pWin->backgroundState;
            oldBackground = pWin->background;
            pWin->backgroundState = BackgroundPixel;
            pWin->background.pixel = 0x00fffffe;
        }
        RootlessStartDrawing(pWin);
        RootlessDamageRegion(pWin, pRegion);

        // For ParentRelative windows, we have to make sure the window
        // pixmap is set correctly all the way up the ancestor chain.
        if (pWin->backgroundState == ParentRelative) {
            SetPixmapOfAncestors(pWin);
        }
    }

    // We can't use fbPaintWindow because it zeros the alpha channel.
    switch (pWin->backgroundState) {
        case None:
            break;
        case ParentRelative:
            do {
                pWin = pWin->parent;
            } while (pWin->backgroundState == ParentRelative);
            (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
                                                                what);
            break;
        case BackgroundPixmap:
            AquaFillRegionTiled (&pWin->drawable,
                                  pRegion,
                                  pWin->background.pixmap);
            break;
        case BackgroundPixel:
        {
            Pixel pixel = pWin->background.pixel |
                          AquaAlphaMask(pWin->drawable.bitsPerPixel);
            fbFillRegionSolid (&pWin->drawable, pRegion, 0,
                                fbReplicatePixel (pixel,
                                                  pWin->drawable.bitsPerPixel));
            break;
        }
    }
    fbValidateDrawable (&pWin->drawable);

    if (IsRoot(pWin)) {
        pWin->backgroundState = oldBackgroundState;
        pWin->background = oldBackground;
    }

    RL_DEBUG_MSG("paintwindowbackground end\n");
}


/*
 * RootlessPaintWindowBorder
 *  Paint the window border while filling in the alpha channel with all on.
 */
void
RootlessPaintWindowBorder(WindowPtr pWin, RegionPtr pRegion, int what)
{
    RL_DEBUG_MSG("paintwindowborder start (win 0x%x) ", pWin);

    if (IsFramedWindow(pWin)) {
        RootlessStartDrawing(pWin);
        RootlessDamageRegion(pWin, pRegion);

        // For ParentRelative windows with tiled borders, we have to make
        // sure the window pixmap is set correctly all the way up the
        // ancestor chain.
        if (!pWin->borderIsPixel &&
            pWin->backgroundState == ParentRelative)
        {
            SetPixmapOfAncestors(pWin);
        }
    }

    // We can't use fbPaintWindow because it zeros the alpha channel.
    if (pWin->borderIsPixel)
    {
        Pixel pixel = pWin->border.pixel |
                      AquaAlphaMask(pWin->drawable.bitsPerPixel);
        fbFillRegionSolid (&pWin->drawable, pRegion, 0,
                            fbReplicatePixel (pixel,
                                              pWin->drawable.bitsPerPixel));
    }
    else
    {
        WindowPtr pBgWin;
        for (pBgWin = pWin; pBgWin->backgroundState == ParentRelative;
             pBgWin = pBgWin->parent);

        AquaFillRegionTiled (&pBgWin->drawable,
                              pRegion,
                              pWin->border.pixmap);
    }
    fbValidateDrawable (&pWin->drawable);

    RL_DEBUG_MSG("paintwindowborder end\n");
}


// fixme untested!
// pWin inside corner stays the same
// pWin->drawable.[xy] stays the same
// frame moves and resizes
void
RootlessChangeBorderWidth(WindowPtr pWin, unsigned int width)
{
    RegionRec saveRoot;

    RL_DEBUG_MSG("change border width ");
    if (width != wBorderWidth(pWin)) {
        RootlessWindowRec *winRec = WINREC(pWin);
        int oldX = 0, oldY = 0, newX = 0, newY = 0;
        unsigned int oldW = 0, oldH = 0, oldBW = 0;
        unsigned int newW = 0, newH = 0, newBW = 0;

        if (winRec) {
            oldBW = winRec->borderWidth;
            oldX = winRec->frame.x;
            oldY = winRec->frame.y;
            oldW = winRec->frame.w;
            oldH = winRec->frame.h;

            newBW = width;
            newX = pWin->drawable.x - newBW;
            newY = pWin->drawable.y - newBW;
            newW = pWin->drawable.width  + 2*newBW;
            newH = pWin->drawable.height + 2*newBW;

            StartFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                            newX, newY, newW, newH, newBW);
        }

        HUGE_ROOT(pWin);
        SCREEN_UNWRAP(pWin->drawable.pScreen, ChangeBorderWidth);
        pWin->drawable.pScreen->ChangeBorderWidth(pWin, width);
        SCREEN_WRAP(pWin->drawable.pScreen, ChangeBorderWidth);
        NORMAL_ROOT(pWin);

        if (winRec) {
            FinishFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW);
        }
    }
    RL_DEBUG_MSG("change border width end\n");
}
