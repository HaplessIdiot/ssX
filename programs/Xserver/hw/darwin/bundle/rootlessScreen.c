/*
 * Screen support for Mac OS X rootless X server
 *
 * Greg Parker     gparker@cs.stanford.edu
 *
 * February 2001  Created
 * March 3, 2001  Restructured as generic rootless mode
 */
/* $XFree86: $ */

// See rootless.h for usage

// fixme create frames at CreateWindow instead of RealizeWindow?

#include "mi.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "mivalidate.h"
#include "picturestr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rootless.h"


typedef struct {
    ScreenPtr pScreen;
    RootlessFrameProcs frameProcs;

    CloseScreenProcPtr CloseScreen;

    GetWindowPixmapProcPtr GetWindowPixmap;
    CreateWindowProcPtr CreateWindow;
    DestroyWindowProcPtr DestroyWindow;
    MarkOverlappedWindowsProcPtr MarkOverlappedWindows;
    ValidateTreeProcPtr ValidateTree;
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    MoveWindowProcPtr MoveWindow;
    ResizeWindowProcPtr ResizeWindow;
    RestackWindowProcPtr RestackWindow;
    ChangeBorderWidthProcPtr ChangeBorderWidth;

    CreateGCProcPtr CreateGC;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr PaintWindowBorder;
    CopyWindowProcPtr CopyWindow;
    GetImageProcPtr GetImage;
    SourceValidateProcPtr SourceValidate;

#ifdef SHAPE
    SetShapeProcPtr SetShape;
#endif

#ifdef RENDER
    CompositeProcPtr Composite;
    GlyphsProcPtr Glyphs;
#endif

} RootlessScreenRec;


// Initialize globals
int rootlessGCPrivateIndex = -1;
int rootlessScreenPrivateIndex = -1;
int rootlessWindowPrivateIndex = -1;


// Returns the top-level parent of pWindow.
// The root is the top-level parent of itself, even though the root is
// not otherwise considered to be a top-level window.
WindowPtr TopLevelParent(WindowPtr pWindow)
{
    WindowPtr top = pWindow;

    if (IsRoot(pWindow)) return pWindow; // root is top-level parent of itself
    while (top && ! IsTopLevel(top)) top = top->parent;
    return top;
}

// Returns TRUE if this window is visible inside a frame
// (e.g. it is visible and has a top-level or root parent)
Bool IsFramedWindow(WindowPtr pWin)
{
    WindowPtr top;

    if (! pWin->realized) return FALSE;
    top = TopLevelParent(pWin);
    return (top && WINREC(top));
}

static void SetPixmapBaseToScreen(PixmapPtr pix, int x, int y)
{
    pix->devPrivate.ptr = (char *)(pix->devPrivate.ptr) -
        (pix->drawable.bitsPerPixel/8 * x + y*pix->devKind);
}

static void SetWinRecBaseToScreen(RootlessWindowRec *winRec)
{
    if (winRec->startDrawCount <= 0) {
        ErrorF("\n\nbasetoscreen FAKEPIXMAPBITS!! FAKEPIXMAPBITS!!\n\n");
    }
    winRec->scratchPixmap->devPrivate.ptr = winRec->frame.pixelData;
    SetPixmapBaseToScreen(winRec->scratchPixmap,
                          winRec->frame.x, winRec->frame.y);
}



void RootlessStartDrawing(WindowPtr pWindow)
{
    WindowPtr top = TopLevelParent(pWindow);
    RootlessWindowRec *winRec;

    if (! top) return;
    winRec = WINREC(top);
    if (!winRec) return;
    if (winRec->startDrawCount == 0) {
        ScreenPtr pScreen = pWindow->drawable.pScreen;
        CallFrameProc(pScreen, StartDrawing, (pScreen, &winRec->frame));
        winRec->scratchPixmap = GetScratchPixmapHeader(pScreen,
                                    winRec->frame.w, winRec->frame.h,
                                    winRec->frame.depth,
                                    winRec->frame.bitsPerPixel,
                                    winRec->frame.bytesPerRow,
                                    winRec->frame.pixelData);
        SetPixmapBaseToScreen(winRec->scratchPixmap,
                              winRec->frame.x, winRec->frame.y);
    }
    winRec->startDrawCount++;
}

// Stop drawing.
// Drawing is always really stopped during the block handler.
static void RootlessReallyStopDrawing(WindowPtr pWindow)
{
    WindowPtr top = TopLevelParent(pWindow);
    RootlessWindowRec *winRec;

    if (! top) return;
    winRec = WINREC(top);
    if (!winRec) return;
    winRec->startDrawCount--;
    if (winRec->startDrawCount == 0) {
        ScreenPtr pScreen = pWindow->drawable.pScreen;
        CallFrameProc(pScreen, StopDrawing, (pScreen, &winRec->frame));
        FreeScratchPixmapHeader(winRec->scratchPixmap);
        winRec->scratchPixmap = NULL;
    }
}

static void
RootlessDamageRect (WindowPtr pWindow, int x, int y, int w, int h)
{
    BoxRec box;
    RegionRec region;

    x += pWindow->drawable.x;
    y += pWindow->drawable.y;
    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;
    REGION_INIT (pWindow->drawable.pScreen, &region, &box, 1);
    RootlessDamageRegion (pWindow, &region);
}


// RootlessGetWindowPixmap
// This is the key to pixmap-per-window. fb/cfb expect to get a
// Pixmap whose bits start at screen (0, 0). Our per-window pixmaps
// don't have that many pixels. Instead, we lie about where the bits
// are and trust the fb to correctly clip its drawing to the window.
// The pixels under the window itself really are part of the Pixmap,
// and the fb never notices the switch.
//
// This procedure finds the top-level parent of w. This parent's
// pixmap is returned. Before returning, the pixmap's devPrivate.ptr
// is adjusted to point to the location where screen pixel (0, 0)
// would be if this pixmap were a full-screen pixmap.
// The true base address of the Pixmap's data is saved in
// the pixelData field of the window privates.
static PixmapPtr RootlessGetWindowPixmap(WindowPtr w)
{
    PixmapPtr result;
    WindowPtr top = w;

    RL_DEBUG_MSG("getwindowpixmap (win 0x%x) %d %d ", w, w->drawable.width,
                 w->drawable.height);
    if (! w->realized) RL_DEBUG_MSG("NOT REALIZED! ");
    top = TopLevelParent(w);
    if (w->realized && top  &&  WINREC(top)) {
        // Adjust the pixmap's base address
        SetWinRecBaseToScreen(WINREC(top));
        result = WINREC(top)->scratchPixmap;
    } else {
        result = w->drawable.pScreen->GetScreenPixmap(w->drawable.pScreen);
    }
    return result;
}


static void
RootlessRedisplay (WindowPtr pWindow)
{
    RootlessWindowRec *winRec = WINREC(pWindow);
    ScreenPtr pScreen = pWindow->drawable.pScreen;

    if (REGION_NOTEMPTY (pScreen, &winRec->damage)) {
        REGION_INTERSECT (pScreen, &winRec->damage, &winRec->damage,
                          &pWindow->borderSize);
                          // &WindowTable[pScreen->myNum]->borderSize);

        // move region to window local coords
        REGION_TRANSLATE(pScreen, &winRec->damage,
                         -winRec->frame.x, -winRec->frame.y);
        CallFrameProc(pScreen, UpdateRegion,
                      (pScreen, &winRec->frame, &winRec->damage));
        REGION_EMPTY (pScreen, &winRec->damage);
    }

    while (winRec->startDrawCount) {
        RootlessReallyStopDrawing(pWindow);
    }
}

static void
RootlessRedisplayScreen(ScreenPtr pScreen)
{
    WindowPtr root = WindowTable[pScreen->myNum];

    if (root) {
        WindowPtr win;

        RootlessRedisplay(root);
        for (win = root->firstChild; win; win = win->nextSib) {
            if (WINREC(win)) {
                RootlessRedisplay(win);
            }
        }
    }
}

static Bool
RootlessCloseScreen(int i, ScreenPtr pScreen)
{
    RootlessScreenRec *s;

    s = SCREENREC(pScreen);

    // fixme unwrap everything that was wrapped?
    pScreen->CloseScreen = s->CloseScreen;

    xfree(s);
    return pScreen->CloseScreen(i, pScreen);
}


// "Definition of the Porting Layer for the X11 Sample Server" says
// unwrap and rewrap of screen functions is unnecessary, but
// screen->CreateGC changes after a call to cfbCreateGC (!!?!!?)

#define SCREEN_UNWRAP(screen, fn) \
    screen->fn = SCREENREC(screen)->fn;

#define SCREEN_WRAP(screen, fn) \
    SCREENREC(screen)->fn = screen->fn; \
    screen->fn = Rootless##fn


// HUGE_ROOT and NORMAL_ROOT
// We don't want to clip windows to the edge of the screen.
// HUGE_ROOT temporarily makes the root window really big.
// This is needed as a wrapper around any function that calls
// SetWinSize or SetBorderSize which clip a window against its
// parents, including the root.
static RegionRec hugeRoot = {{-32767, -32767, 32767, 32767}, NULL};

#define HUGE_ROOT(pWin) \
    { \
        WindowPtr w = pWin; \
        while (w->parent) w = w->parent; \
        saveRoot = w->winSize; \
        w->winSize = hugeRoot; \
    }

#define NORMAL_ROOT(pWin) \
    { \
        WindowPtr w = pWin; \
        while (w->parent) w = w->parent; \
        w->winSize = saveRoot; \
    }


// CopyArea and CopyPlane use a GC tied to the dest drawable.
// StartDrawing/StopDrawing wrappers won't be called if source is
// a visible window but the dest isn't. So, we call StartDrawing here
// and leave StopDrawing for the block handler.
static void
RootlessSourceValidate(DrawablePtr pDrawable, int x, int y, int w, int h)
{
    SCREEN_UNWRAP(pDrawable->pScreen, SourceValidate);
    if (pDrawable->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr)pDrawable;
        RootlessStartDrawing(pWin);
    }
    if (pDrawable->pScreen->SourceValidate) {
        pDrawable->pScreen->SourceValidate(pDrawable, x, y, w, h);
    }
    SCREEN_WRAP(pDrawable->pScreen, SourceValidate);
}

// RootlessCreateWindow
// For now, don't create a frame until the window is realized.
// Do reset the window size so it's not clipped by the root window.
static Bool
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
static Bool
RootlessDestroyWindow(WindowPtr pWin)
{
    Bool result;

    SCREEN_UNWRAP(pWin->drawable.pScreen, DestroyWindow);
    result = pWin->drawable.pScreen->DestroyWindow(pWin);
    SCREEN_WRAP(pWin->drawable.pScreen, DestroyWindow);
    return result;
}


static void
RootlessGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h,
                 unsigned int format, unsigned long planeMask, char *pdstLine)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    SCREEN_UNWRAP(pScreen, GetImage);

    if (pDrawable->type == DRAWABLE_WINDOW) {
        /* Many apps use GetImage to sync with the visible frame buffer */
        // fixme entire screen or just window or all screens?
        RootlessRedisplayScreen(pScreen);
        RootlessStartDrawing((WindowPtr)pDrawable);
    }
    pScreen->GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
    if (pDrawable->type == DRAWABLE_WINDOW) {
        RootlessStopDrawing((WindowPtr)pDrawable);
    }
    SCREEN_WRAP(pScreen, GetImage);
}


#ifdef SHAPE
// boundingShape = outside border (like borderClip)
// clipShape = inside border (like clipList)
// Both are in window-local coordinates
// We only care about boundingShape (fixme true?)

// RootlessReallySetShape is used in several places other than SetShape.
// Most importantly, SetShape is often called on unmapped windows, so we
// have to wait until the window is mapped to reshape the frame.
static void RootlessReallySetShape(WindowPtr pWin)
{
    RootlessWindowRec *winRec = WINREC(pWin);
    ScreenPtr pScreen = pWin->drawable.pScreen;
    RegionRec newShape;

    if (IsRoot(pWin)) return;
    if (!IsTopLevel(pWin)) return;
    if (!winRec) return;

    while (winRec->startDrawCount) {
        RootlessReallyStopDrawing(pWin);
    }
    if (wBoundingShape(pWin)) {
        // wBoundingShape is relative to *inner* origin of window.
        // Translate by borderWidth to get the outside-relative position.
        REGION_INIT(pScreen, &newShape, NullBox, 0);
        REGION_COPY(pScreen, &newShape, wBoundingShape(pWin));
        REGION_TRANSLATE(pScreen, &newShape, pWin->borderWidth,
                         pWin->borderWidth);
    } else {
        newShape.data = NULL;
        newShape.extents.x1 = 0;
        newShape.extents.y1 = 0;
        newShape.extents.x2 = winRec->frame.w;
        newShape.extents.y2 = winRec->frame.h;
    }
    RL_DEBUG_MSG("reshaping...");
    RL_DEBUG_MSG("numrects %d, extents %d %d %d %d ",
                 REGION_NUM_RECTS(rgn),
                 newShape.extents.x1, newShape.extents.y1,
                 newShape.extents.x2, newShape.extents.y2);
    CallFrameProc(pScreen, ReshapeFrame,(pScreen, &winRec->frame, &newShape));
    REGION_UNINIT(pScreen, &newShape);
}

static void
RootlessSetShape(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RootlessReallySetShape(pWin);
    SCREEN_UNWRAP(pScreen, SetShape);
    pScreen->SetShape(pWin);
    SCREEN_WRAP(pScreen, SetShape);
}

#endif // SHAPE


#ifdef RENDER

static void
RootlessComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                  INT16 xSrc, INT16 ySrc, INT16  xMask, INT16  yMask,
                  INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    WindowPtr srcWin, maskWin, dstWin;

    srcWin  = (pSrc->pDrawable->type  == DRAWABLE_WINDOW) ?
        (WindowPtr)pSrc->pDrawable  :  NULL;
    maskWin = (pMask->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr)pMask->pDrawable :  NULL;
    dstWin  = (pDst->pDrawable->type  == DRAWABLE_WINDOW) ?
        (WindowPtr)pDst->pDrawable  :  NULL;

    // SCREEN_UNWRAP(ps, Composite);
    ps->Composite = SCREENREC(pScreen)->Composite;

    if (srcWin  && IsFramedWindow(srcWin))  RootlessStartDrawing(srcWin);
    if (maskWin && IsFramedWindow(maskWin)) RootlessStartDrawing(maskWin);
    if (dstWin  && IsFramedWindow(dstWin))  RootlessStartDrawing(dstWin);

    ps->Composite(op, pSrc, pMask, pDst,
                  xSrc, ySrc, xMask, yMask,
                  xDst, yDst, width, height);

    if (srcWin  && IsFramedWindow(srcWin))  RootlessStopDrawing(srcWin);
    if (maskWin && IsFramedWindow(maskWin)) RootlessStopDrawing(maskWin);
    if (dstWin  && IsFramedWindow(dstWin)) {
        RootlessStopDrawing(dstWin);
        RootlessDamageRect(dstWin, xDst, yDst, width, height);
    }

    ps->Composite = RootlessComposite;
    // SCREEN_WRAP(ps, Composite);
}

static void
RootlessGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
               PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
               int nlist, GlyphListPtr list, GlyphPtr *glyphs)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    int x, y;
    int n;
    GlyphPtr glyph;
    WindowPtr srcWin, dstWin;

    srcWin = (pSrc->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr)pSrc->pDrawable  :  NULL;
    dstWin = (pDst->pDrawable->type == DRAWABLE_WINDOW) ?
        (WindowPtr)pDst->pDrawable  :  NULL;

    if (srcWin && IsFramedWindow(srcWin)) RootlessStartDrawing(srcWin);
    if (dstWin && IsFramedWindow(dstWin)) RootlessStartDrawing(dstWin);

    //SCREEN_UNWRAP(ps, Glyphs);
    ps->Glyphs = SCREENREC(pScreen)->Glyphs;
    ps->Glyphs(op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
    ps->Glyphs = RootlessGlyphs;
    //SCREEN_WRAP(ps, Glyphs);

    if (srcWin && IsFramedWindow(srcWin)) RootlessStopDrawing(srcWin);
    if (dstWin && IsFramedWindow(dstWin)) {
        RootlessStopDrawing(dstWin);
        x = xSrc;
        y = ySrc;
        while (nlist--) {
            x += list->xOff;
            y += list->yOff;
            n = list->len;
            while (n--) {
                glyph = *glyphs++;
                RootlessDamageRect(dstWin,
                                   x - glyph->info.x, y - glyph->info.y,
                                   glyph->info.width, glyph->info.height);
                x += glyph->info.xOff;
                y += glyph->info.yOff;
            }
            list++;
        }
    }
}

#endif // RENDER


// RootlessValidateTree
// ValidateTree is modified in two ways:
// * top-level windows don't clip each other
// * windows aren't clipped against root.
// These only matter when validating from the root.
static int
RootlessValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
    int result;
    RegionRec saveRoot;
    SCREEN_UNWRAP(pParent->drawable.pScreen, ValidateTree);
    RL_DEBUG_MSG("VALIDATETREE start ");

    // Use our custom version to validate from root
    if (IsRoot(pParent)) {
        RL_DEBUG_MSG("custom ");
        result = rootlessMiValidateTree(pParent, pChild, kind);
    } else {
        HUGE_ROOT(pParent);
        result = pParent->drawable.pScreen->ValidateTree(pParent, pChild,
                                                         kind);
        NORMAL_ROOT(pParent);
    }

    SCREEN_WRAP(pParent->drawable.pScreen, ValidateTree);
    RL_DEBUG_MSG("VALIDATETREE end\n");

    return result;
}


// RootlessMarkOverlappedWindows
// MarkOverlappedWindows is modified to ignore overlapping
// top-level windows.
static Bool
RootlessMarkOverlappedWindows(WindowPtr pWin, WindowPtr pFirst,
                              WindowPtr *ppLayerWin)
{
    RegionRec saveRoot;
    Bool result;
    SCREEN_UNWRAP(pWin->drawable.pScreen, MarkOverlappedWindows);
    RL_DEBUG_MSG("MARKOVERLAPPEDWINDOWS start ");

    HUGE_ROOT(pWin);
    if (IsRoot(pWin)) {
        // root - mark nothing
        RL_DEBUG_MSG("is root not marking ");
        result = FALSE;
    }
    else if (! IsTopLevel(pWin)) {
        // not top-level window - mark normally
        result = pWin->drawable.pScreen->
        MarkOverlappedWindows(pWin, pFirst, ppLayerWin);
    }
    else {
        //top-level window - mark children ONLY - NO overlaps with sibs (?)
        // This code copied from miMarkOverlappedWindows()

        register WindowPtr pChild;
        Bool anyMarked = FALSE;
        void (* MarkWindow)() = pWin->drawable.pScreen->MarkWindow;

        RL_DEBUG_MSG("is top level! ");
        /* single layered systems are easy */
        if (ppLayerWin) *ppLayerWin = pWin;

        if (pWin == pFirst) {
            /* Blindly mark pWin and all of its inferiors.   This is a slight
            * overkill if there are mapped windows that outside pWin's border,
            * but it's better than wasting time on RectIn checks.
            */
            pChild = pWin;
            while (1) {
                if (pChild->viewable) {
                    if (REGION_BROKEN (pScreen, &pChild->winSize))
                        SetWinSize (pChild);
                    if (REGION_BROKEN (pScreen, &pChild->borderSize))
                        SetBorderSize (pChild);
                    (* MarkWindow)(pChild);
                    if (pChild->firstChild) {
                        pChild = pChild->firstChild;
                        continue;
                    }
                }
                while (!pChild->nextSib && (pChild != pWin))
                    pChild = pChild->parent;
                if (pChild == pWin)
                    break;
                pChild = pChild->nextSib;
            }
            anyMarked = TRUE;
            pFirst = pFirst->nextSib;
        }
        if (anyMarked)
            (* MarkWindow)(pWin->parent);
        result = anyMarked;
    }
    NORMAL_ROOT(pWin);
    SCREEN_WRAP(pWin->drawable.pScreen, MarkOverlappedWindows);
    RL_DEBUG_MSG("MARKOVERLAPPEDWINDOWS end\n");
    return result;
}


// RootlessRealizeWindow
// The frame is created here and not in CreateWindow.
// fixme change this?
static Bool
RootlessRealizeWindow(WindowPtr pWin)
{
    Bool result = FALSE;
    RegionRec saveRoot;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    RL_DEBUG_MSG("realizewindow start ");

    if (IsTopLevel(pWin)  ||  IsRoot(pWin)) {
        DrawablePtr d = &pWin->drawable;
        RootlessWindowRec *winRec = xalloc(sizeof(RootlessWindowRec));

        if (! winRec) goto windowcreatebad;

        winRec->frame.isRoot = (pWin == WindowTable[pScreen->myNum]);
        winRec->frame.x = d->x - pWin->borderWidth;
        winRec->frame.y = d->y - pWin->borderWidth;
        winRec->frame.w = d->width + 2*pWin->borderWidth;
        winRec->frame.h = d->height + 2*pWin->borderWidth;
        winRec->frame.win = pWin;
        winRec->frame.devPrivate = NULL;

        REGION_INIT(pScreen, &winRec->damage, NullBox, 0);
        winRec->startDrawCount = 0;
        winRec->borderWidth = pWin->borderWidth;
        winRec->scratchPixmap = NULL;

        WINREC(pWin) = winRec;

        RL_DEBUG_MSG("creating frame ");
        CallFrameProc(pScreen, CreateFrame,
                      (pScreen, &WINREC(pWin)->frame,
                      pWin->prevSib ? &WINREC(pWin->prevSib)->frame : NULL));

#ifdef SHAPE
        // Shape is usually set before the window is mapped, but
        // (for now) we don't keep track of frames before they're mapped.
        RootlessReallySetShape(pWin);
#endif
    }

    if (!IsRoot(pWin)) HUGE_ROOT(pWin);
    SCREEN_UNWRAP(pWin->drawable.pScreen, RealizeWindow);
    result = pWin->drawable.pScreen->RealizeWindow(pWin);
    SCREEN_WRAP(pWin->drawable.pScreen, RealizeWindow);
    if (!IsRoot(pWin)) NORMAL_ROOT(pWin);
    RL_DEBUG_MSG("realizewindow end\n");
    return result;

windowcreatebad:
    RL_DEBUG_MSG("window create bad! ");
    RL_DEBUG_MSG("realizewindow end\n");
    return NULL;
}

static Bool
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


static void
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
        WindowPtr oldNextW, newNextW, oldPrevW, newPrevW;
        RootlessFramePtr oldNext, newNext, oldPrev, newPrev;

    oldNextW = pOldNextSib;
    while (oldNextW  &&  ! WINREC(oldNextW)) oldNextW = oldNextW->nextSib;
    oldNext = oldNextW ? &WINREC(oldNextW)->frame : NULL;

    newNextW = pWin->nextSib;
    while (newNextW  &&  ! WINREC(newNextW)) newNextW = newNextW->nextSib;
    newNext = newNextW ? &WINREC(newNextW)->frame : NULL;

    oldPrevW = pOldNextSib ? pOldNextSib->prevSib : pWin->parent->lastChild;
    while (oldPrevW  &&  ! WINREC(oldPrevW)) oldPrevW = oldPrevW->prevSib;
    oldPrev = oldPrevW ? &WINREC(oldPrevW)->frame : NULL;

    newPrevW = pWin->prevSib;
    while (newPrevW  &&  ! WINREC(newPrevW)) newPrevW = newPrevW->prevSib;
    newPrev = newPrevW ? &WINREC(newPrevW)->frame : NULL;

    if (pWin->prevSib) {
        WindowPtr w;

        RL_DEBUG_MSG("pwin->prevSib 0x%x winrec 0x%x\n", pWin->prevSib,
                     WINREC(pWin->prevSib));
        w = pWin->prevSib;
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


// FIXME: This all needs to be rewritten for fb
#if 0
extern void cfbDoBitbltCopy(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                            RegionPtr prgnDst, DDXPointPtr pptSrc,
                            unsigned long planemask);
extern void cfb16DoBitbltCopy(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                              RegionPtr prgnDst, DDXPointPtr pptSrc,
                              unsigned long planemask);
extern void cfb24DoBitbltCopy(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                              RegionPtr prgnDst, DDXPointPtr pptSrc,
                              unsigned long planemask);
extern void cfb32DoBitbltCopy(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                              RegionPtr prgnDst, DDXPointPtr pptSrc,
                              unsigned long planemask);
#endif

// Call the cfb blitter
// cfb uses preprocessor magic to clone every procedure for each
// bit depth, so we need to call the right one.
// fixme what if they're different depths?
// fixme this sucks
// fixme is fb any better than cfb?
// fixme can fb handle an alpha channel?
static void
RootlessDoBitbltCopy(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                     RegionPtr prgnDst, DDXPointPtr pptSrc,
                     unsigned long planemask)
{
#if 0
    void (*blitter)(DrawablePtr pSrc, DrawablePtr pDst, int alu,
                    RegionPtr prgnDst, DDXPointPtr pptSrc,
                    unsigned long planemask);

    switch (pSrc->bitsPerPixel) {
        case 8: blitter = cfbDoBitbltCopy; break;
        case 16: blitter = cfb16DoBitbltCopy; break;
     // case 24: blitter = cfb24DoBitbltCopy; break;
        case 32: blitter = cfb32DoBitbltCopy; break;
    }
    blitter(pSrc, pDst, alu, prgnDst, pptSrc, planemask);
#endif
}


/*
 * Window copy procedures
 */

// Globals needed during window resize and copy.
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
static void
RootlessResizeCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                         RegionPtr prgnSrc)
{
    // gravity version - copy from old window pixmap to new pixmap
    // doesn't work
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    RegionRec rgnDst;
    register BoxPtr pbox;
    register int dx, dy;
    register int i, nbox;

    RL_DEBUG_MSG("ROOTLESSRESIZECOPYWINDOW start ");
    REGION_INIT(pWin->drawable.pScreen, &rgnDst, NullBox, 0);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip,
                     prgnSrc);

    pbox = REGION_RECTS(&rgnDst);
    nbox = REGION_NUM_RECTS(&rgnDst);
    if (!nbox || !(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(nbox *
                                    sizeof(DDXPointRec)))) {
        REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
        return;
    }
    ppt = pptSrc;

    for (i = nbox; --i >= 0; ppt++, pbox++) {
        ppt->x = pbox->x1 + dx;
        ppt->y = pbox->y1 + dy;
    }

    RootlessDoBitbltCopy(&gResizeCopyWindowSource->drawable,
                         &WINREC(pWin)->scratchPixmap->drawable,
                         GXcopy, &rgnDst, pptSrc, ~0L);
    DEALLOCATE_LOCAL(pptSrc);
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);

    // don't update - entire window will be updated by resize

    RL_DEBUG_MSG("ROOTLESSRESIZECOPYWINDOW end\n");
}

/* Update *new* location of window. Old location is redrawn with
   PaintWindowBackground/Border. */
static void
RootlessCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    // BoxPtr r = &prgnSrc->extents;
    SCREEN_UNWRAP(pWin->drawable.pScreen, CopyWindow);
    RL_DEBUG_MSG("copywindow start (win 0x%x) ", pWin);

    // Copied from cfbCopyWindow
    // The original always draws on the root pixmap (which we don't have).
    // Instead, draw on the parent window's pixmap.
    {
        DDXPointPtr pptSrc;
        register DDXPointPtr ppt;
        RegionRec rgnDst;
        register BoxPtr pbox;
        register int dx, dy;
        register int i, nbox;

        REGION_INIT(pWin->drawable.pScreen, &rgnDst, NullBox, 0);

        dx = ptOldOrg.x - pWin->drawable.x;
        dy = ptOldOrg.y - pWin->drawable.y;

        REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);
        REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip,
                         prgnSrc);
        {
            int i;

            pbox = REGION_RECTS(&rgnDst);
            for (i = 0; i < REGION_NUM_RECTS(&rgnDst); i++) {
                pbox++;
            }
        }

        pbox = REGION_RECTS(&rgnDst);
        nbox = REGION_NUM_RECTS(&rgnDst);
        if (!nbox || !(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(nbox *
                                        sizeof(DDXPointRec)))) {
            // don't draw
        } else {
            ppt = pptSrc;

            for (i = nbox; --i >= 0; ppt++, pbox++) {
                ppt->x = pbox->x1 + dx;
                ppt->y = pbox->y1 + dy;
            }
            RootlessStartDrawing(pWin);
            RootlessDoBitbltCopy((DrawablePtr)pWin, (DrawablePtr)pWin,
                                 GXcopy, &rgnDst, pptSrc, ~0L);
            RootlessStopDrawing(pWin);
            DEALLOCATE_LOCAL(pptSrc);
        }

        // src has been translated to dst
        RootlessDamageRegion(pWin, prgnSrc);
        REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
    }

    SCREEN_WRAP(pWin->drawable.pScreen, CopyWindow);
    RL_DEBUG_MSG("copywindow end\n");
}


/*
 * Window resize procedures
 */

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

    RL_DEBUG_MSG("RESIZE TOPLEVEL WINDOW ");
    RL_DEBUG_MSG("%d %d %d %d %d   %d %d %d %d %d  ",
                 oldX, oldY, oldW, oldH, oldBW,
                 newX, newY, newW, newH, newBW);

    RootlessRedisplay(pWin);

/*
    // make the frame shape a rect
    if (wBoundingShape(pWin)) {
        RegionPtr saveShape = wBoundingShape(pWin);

        pWin->optional->boundingShape = NULL;
        RL_DEBUG_MSG("RootlessReallySetShape from resize ");
        RootlessReallySetShape(pWin);
        pWin->optional->boundingShape = saveShape;
    }
*/

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
    SetPixmapBaseToScreen(gResizeDeathPix, winRec->frame.x, winRec->frame.y);
    RootlessReallyStopDrawing(pWin);

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
    // pWin currently has new bigger pixmap but is in old position
    // fixme border width change!
    {
        RegionRec r;
        DrawablePtr src = &gResizeDeathPix->drawable;
        DrawablePtr dst = &winRec->scratchPixmap->drawable;

        r.data = NULL;
        r.extents.x1 = max(oldX, newX);
        r.extents.y1 = max(oldY, newY);
        r.extents.x2 = min(oldX + oldW, newX + newW);
        r.extents.y2 = min(oldY + oldH, newY + newH);

        // r is now intersection of of old location and new location
        if (r.extents.x2 > r.extents.x1  &&  r.extents.y2 > r.extents.y1) {
            DDXPointRec srcPt = {r.extents.x1, r.extents.y1};
#if 0
            int dx = newX - oldX;
            int dy = newY - oldY;

            // Correct for border width change
            // fixme need to correct for border width change
            // don't do this REGION_TRANSLATE(pScreen, &r, dx, dy);
#endif
            RootlessDoBitbltCopy(src, dst, GXcopy, &r, &srcPt, ~0L);
        }
        REGION_UNINIT(pScreen, &r);
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

    RootlessStopDrawing(pWin);
    CallFrameProc(pScreen, FinishResizeFrame,
                  (pScreen, &winRec->frame, oldX, oldY, oldW, oldH));

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
//   already changed!!@#$)
static void
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
            newBW = pWin->borderWidth;
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
            RootlessReallyStopDrawing(pWin);
            CallFrameProc(pScreen, MoveFrame, (pScreen, &winRec->frame, oldX, oldY));
        } else {
            FinishFrameResize(pWin, FALSE, oldX, oldY, oldW, oldH, oldBW,
                              newX, newY, newW, newH, newBW);
        }
    }

    RL_DEBUG_MSG("movewindow end\n");
}


// (x,y) is corner of very outer edge, *outside* border
// w,h is width and height *ignoring* border width
// The rect (x, y, w, h) doesn't mean anything.
// (x, y, w+2*bw, h+2*bw) is total rect
// (x+bw, y+bw, w, h) is inner rect

static void
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


// fixme untested!
// pWin inside corner stays the same
// pWin->drawable.[xy] stays the same
static void
RootlessChangeBorderWidth(WindowPtr pWin, unsigned int width)
{
    RegionRec saveRoot;

    RL_DEBUG_MSG("change border width ");
    if (width != pWin->borderWidth) {
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

Bool
RootlessCreateGC(GCPtr pGC)
{
    RootlessGCRec *gcrec;
    RootlessScreenRec *s;
    Bool result;

    SCREEN_UNWRAP(pGC->pScreen, CreateGC);
    s = (RootlessScreenRec *) pGC->pScreen->
            devPrivates[rootlessScreenPrivateIndex].ptr;
    result = s->CreateGC(pGC);
    gcrec = (RootlessGCRec *) pGC->devPrivates[rootlessGCPrivateIndex].ptr;
    gcrec->originalOps = NULL; // don't wrap ops yet
    gcrec->originalFuncs = pGC->funcs;
    pGC->funcs = &rootlessGCFuncs;

    SCREEN_WRAP(pGC->pScreen, CreateGC);
    return result;
}

static void
RootlessPaintWindowBackground(WindowPtr pWin, RegionPtr pRegion, int what)
{
    int oldBackgroundState = 0;
    PixUnion oldBackground;

    SCREEN_UNWRAP(pWin->drawable.pScreen, PaintWindowBackground);
    RL_DEBUG_MSG("paintwindowbackground start (win 0x%x) ", pWin);
    if (IsFramedWindow(pWin)) {
        if (IsRoot(pWin)) {
            // set root background to magic transparent color
            oldBackgroundState = pWin->backgroundState;
            oldBackground = pWin->background;
            pWin->backgroundState = BackgroundPixel;
            pWin->background.pixel = 0x00fffffe;
        }
        RootlessStartDrawing(pWin);
    }

    pWin->drawable.pScreen->PaintWindowBackground(pWin, pRegion, what);

    if (IsFramedWindow(pWin)) {
        RootlessStopDrawing(pWin);
        RootlessDamageRegion(pWin, pRegion);
        if (IsRoot(pWin)) {
            pWin->backgroundState = oldBackgroundState;
            pWin->background = oldBackground;
        }
    }
    SCREEN_WRAP(pWin->drawable.pScreen, PaintWindowBackground);
    RL_DEBUG_MSG("paintwindowbackground end\n");
}

static void
RootlessPaintWindowBorder(WindowPtr pWin, RegionPtr pRegion, int what)
{
    SCREEN_UNWRAP(pWin->drawable.pScreen, PaintWindowBorder);
    RL_DEBUG_MSG("paintwindowborder start (win 0x%x) ", pWin);
    if (IsFramedWindow(pWin)) {
        RootlessStartDrawing(pWin);
    }
    pWin->drawable.pScreen->PaintWindowBorder(pWin, pRegion, what);
    if (IsFramedWindow(pWin)) {
        RootlessStopDrawing(pWin);
        RootlessDamageRegion(pWin, pRegion);
    }
    SCREEN_WRAP(pWin->drawable.pScreen, PaintWindowBorder);
    RL_DEBUG_MSG("paintwindowborder end\n");
}


static Bool
RootlessAllocatePrivates(ScreenPtr pScreen)
{
    RootlessScreenRec *s;
    static int rootlessGeneration = -1;

    if (rootlessGeneration != serverGeneration) {
        rootlessScreenPrivateIndex = AllocateScreenPrivateIndex();
        if (rootlessScreenPrivateIndex == -1) return FALSE;
        rootlessGCPrivateIndex = AllocateGCPrivateIndex();
        if (rootlessGCPrivateIndex == -1) return FALSE;
        rootlessWindowPrivateIndex = AllocateWindowPrivateIndex();
        if (rootlessWindowPrivateIndex == -1) return FALSE;
        rootlessGeneration = serverGeneration;
    }

    // no allocation needed for screen privates
    if (!AllocateGCPrivate(pScreen, rootlessGCPrivateIndex,
                           sizeof(RootlessGCRec)))
        return FALSE;
    if (!AllocateWindowPrivate(pScreen, rootlessWindowPrivateIndex, 0))
        return FALSE;

    s = xalloc(sizeof(RootlessScreenRec));
    if (! s) return FALSE;
    SCREENREC(pScreen) = s;

    return TRUE;
}


// Call RootlessUpdate before blocking on select(). We don't want to leave
// any recent drawing unflushed. This also stops drawing on any
// windows that left drawing on.
static void
RootlessBlockHandler(pointer pbdata, OSTimePtr pTimeout, pointer pReadmask)
{
    RootlessRedisplayScreen((ScreenPtr) pbdata);
}

static void
RootlessWakeupHandler(pointer data, int i, pointer LastSelectMask)
{
    // nothing here
}

static void
RootlessWrap(ScreenPtr pScreen)
{
    RootlessScreenRec *s = (RootlessScreenRec*)
            pScreen->devPrivates[rootlessScreenPrivateIndex].ptr;

#define WRAP(a) \
    if (pScreen->a) { \
        s->a = pScreen->a; \
    } else { \
        RL_DEBUG_MSG("null screen fn " #a "\n"); \
    } \
    pScreen->a = Rootless##a

    WRAP(CloseScreen);
    WRAP(CreateGC);
    WRAP(PaintWindowBackground);
    WRAP(PaintWindowBorder);
    WRAP(CopyWindow);
    WRAP(GetImage);
    WRAP(GetWindowPixmap);
    WRAP(SourceValidate);
    WRAP(CreateWindow);
    WRAP(DestroyWindow);
    WRAP(RealizeWindow);
    WRAP(UnrealizeWindow);
    WRAP(MoveWindow);
    WRAP(ResizeWindow);
    WRAP(RestackWindow); // fixme!?
    WRAP(ChangeBorderWidth);
    WRAP(MarkOverlappedWindows);
    WRAP(ValidateTree);

#ifdef SHAPE
    WRAP(SetShape);
#endif

#ifdef RENDER
    {
        // Composite and Glyphs don't use normal screen wrapping
        PictureScreenPtr ps = GetPictureScreen(pScreen);
        s->Composite = ps->Composite;
        ps->Composite = RootlessComposite;
        s->Glyphs = ps->Glyphs;
        ps->Glyphs = RootlessGlyphs;
    }
#endif

    // WRAP(ClearToBackground); fixme put this back? useful for shaped wins?
    // WRAP(RestoreAreas); fixme put this back?

#undef WRAP
}

/*
 * RootlessInit
 * Rootless wraps lots of stuff and needs a bunch of devPrivates.
 */
Bool RootlessInit(ScreenPtr pScreen, RootlessFrameProcs *procs)
{
    RootlessScreenRec *s;

    if (! RootlessAllocatePrivates(pScreen)) return FALSE;
    s = (RootlessScreenRec*)
            pScreen->devPrivates[rootlessScreenPrivateIndex].ptr;

    s->pScreen = pScreen;
    s->frameProcs = *procs;

    RootlessWrap(pScreen);

    if (!RegisterBlockAndWakeupHandlers (RootlessBlockHandler,
                                         RootlessWakeupHandler,
                                         (pointer) pScreen))
    return FALSE;

    return TRUE;
}
