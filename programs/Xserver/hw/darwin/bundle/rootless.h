/*
 * Generic rootless mode.
 *
 * Greg Parker     gparker@cs.stanford.edu     March 3, 2001
 */
/* $XFree86: $ */

#ifndef _ROOTLESS_H
#define _ROOTLESS_H

#include "mi.h"
#include "gcstruct.h"

// RootlessFrameRec
// Describes a single rootless window (aka frame).
// The rootless mode keeps track of window position, and the 
// rootless user is responsible for the pixmap.

typedef struct RootlessFrameRec {
    /* Data maintained by rootless mode */
    /* position and size, including window border, in screen coordinates */
    int x;
    int y;
    unsigned int w;
    unsigned int h;
    WindowPtr win;    /* the top-level window drawn in this frame */
    int isRoot;       /* TRUE if this is the root window */

    /* Data maintained by CALLER! */
    /* normal pixmap data */
    char *pixelData;
    int depth; // color bits per pixel; depth <= bitsPerPixel
    int bitsPerPixel;
    int bytesPerRow;

    void *devPrivate; /* for caller's use */
} RootlessFrameRec, *RootlessFramePtr;


// Create a new frame.
// pUpper is the window above the new frame, or NULL if the new 
//  frame will be on top.
// pFrame is completely initialized. devPrivate is NULL
typedef void (*RootlessCreateFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, RootlessFramePtr pUpper);

// Destroy a frame. Caller must free any private data and and 
// extant pixmap data.
// All drawing is stopped and all updates are flushed before this is called.
typedef void (*RootlessDestroyFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame);

// Move a frame on screen.
// The frame changes position and nothing else.
// pFrame and pFrame->win already contain the information about the 
// new position. oldX and oldY are the old position.
// All updates are flushed before this is called.
// fixme is drawing stopped before this is called?
typedef void (*RootlessMoveFrameProc) 
    (ScreenPtr pScreen, RootlessFramePtr pFrame, int oldX, int oldY);

// Change frame ordering (stacking, layering)
// pFrame->win already has its new siblings.
// pOldNext is the window that was below this one, or NULL if this was 
//  at the bottom. 
// pNewNext is the window that is now below this one, or NULL if this is 
//  now at the bottom.
typedef void (*RootlessRestackFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, 
     RootlessFramePtr pOldNext, RootlessFramePtr pNewNext);

// Flush drawing updates to the screen. 
// pDamage contains all changed pixels. 
// pDamage is in frame-local coordinates.
// pDamage is clipped to the frame bounds and the frame shape.
// fixme is drawing stopped or not?
typedef void (*RootlessUpdateRegionProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, RegionPtr pDamage);

// Prepare for drawing into this frame. 
// The caller must set the frame's pixmap.
// The pixmap must remain valid until StopDrawing is called.
// The pixmap is never accessed outside StartDrawing..StopDrawing.
// The pixmap data is never modified outside the frame bounds or shape.
typedef void (*RootlessStartDrawingProc) 
    (ScreenPtr pScreen, RootlessFramePtr pFrame);

// Done drawing in this frame. 
// The pixmap is allowed to be invalid again.
// The pixmap is never accessed outside StartDrawing..StopDrawing.
typedef void (*RootlessStopDrawingProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame);

// Change the frame's shape.
// pNewShape is in frame-local coordinates.
// Everything outside pNewShape is no longer part of the frame.
// pNewShape is {0, 0, width, height} for a plain-shaped frame.
// Drawing is never enabed when this is called. (fixme correct?)
typedef void (*RootlessReshapeFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, RegionPtr pNewShape);

// Resize call sequence:
// startdraw, stopdraw, startresize, startdraw, stopdraw, endresize

// Frame is about to resize. 
// The frame size is in its new position already.
// You MUST make sure that the next StartDrawing call will get a pixmap 
// that is as big as the new size. You MAY destroy the old pixmap here.
typedef void (*RootlessStartResizeFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, 
     int oldX, int oldY, unsigned int oldW, unsigned int oldH);

// Frame is done resizing.
// You MUST destroy the old pixmap if you haven't already.
typedef void (*RootlessFinishResizeFrameProc)
    (ScreenPtr pScreen, RootlessFramePtr pFrame, 
     int oldX, int oldY, unsigned int oldW, unsigned int oldH);


// The callback function list. 
// Any of these may be NULL.		
typedef struct RootlessFrameProcs {
    RootlessCreateFrameProc CreateFrame;
    RootlessDestroyFrameProc DestroyFrame;

    RootlessMoveFrameProc MoveFrame;
    RootlessStartResizeFrameProc StartResizeFrame;
    RootlessFinishResizeFrameProc FinishResizeFrame;
    RootlessRestackFrameProc RestackFrame;
    RootlessReshapeFrameProc ReshapeFrame;

    RootlessUpdateRegionProc UpdateRegion;

    RootlessStartDrawingProc StartDrawing;
    RootlessStopDrawingProc StopDrawing;
} RootlessFrameProcs;

// Initialize rootless mode on the given screen.
Bool RootlessInit(ScreenPtr pScreen, RootlessFrameProcs *procs);

// Calculate window clip lists
int rootlessMiValidateTree (WindowPtr pParent, WindowPtr pChild, VTKind kind);


#ifdef ROOTLESSDEBUG
#define RL_DEBUG_MSG(a, ...) ErrorF(a, ...)
#else
#define RL_DEBUG_MSG(a, ...)
#endif

#define SCREENREC(pScreen) \
    ((RootlessScreenRec*)(pScreen)->devPrivates[rootlessScreenPrivateIndex].ptr)

#define WINREC(pWin) \
    ((RootlessWindowRec *)(pWin)->devPrivates[rootlessWindowPrivateIndex].ptr)

#define CallFrameProc(pScreen, proc, params) \
    if (SCREENREC(pScreen)->frameProcs.proc) { \
        RL_DEBUG_MSG("calling frame proc " #proc " "); \
        SCREENREC(pScreen)->frameProcs.proc params; \
}

#define TRIM_BOX(box, pGC) { \
    BoxPtr extents = &pGC->pCompositeClip->extents;\
    if(box.x1 < extents->x1) box.x1 = extents->x1; \
    if(box.x2 > extents->x2) box.x2 = extents->x2; \
    if(box.y1 < extents->y1) box.y1 = extents->y1; \
    if(box.y2 > extents->y2) box.y2 = extents->y2; \
}

#define TRANSLATE_BOX(box, pDraw) { \
    box.x1 += pDraw->x; \
    box.x2 += pDraw->x; \
    box.y1 += pDraw->y; \
    box.y2 += pDraw->y; \
}

#define TRIM_AND_TRANSLATE_BOX(box, pDraw, pGC) { \
    TRANSLATE_BOX(box, pDraw); \
    TRIM_BOX(box, pGC); \
}

#define BOX_NOT_EMPTY(box) \
    (((box.x2 - box.x1) > 0) && ((box.y2 - box.y1) > 0))

// Returns TRUE if this window is a top-level window (i.e. child of the root)
// The root is not a top-level window.
#define IsTopLevel(pWin) \
    ((pWin)  &&  (pWin)->parent  &&  !(pWin)->parent->parent)

// Returns TRUE if this window is a root window
#define IsRoot(pWin) \
    ((pWin) == WindowTable[(pWin)->drawable.pScreen->myNum])

// Returns the top-level parent of pWindow.
// The root is the top-level parent of itself, even though the root is
// not otherwise considered to be a top-level window.
WindowPtr TopLevelParent(WindowPtr pWindow);

// Returns TRUE if this window is visible inside a frame
// (e.g. it is visible and has a top-level or root parent)
Bool IsFramedWindow(WindowPtr pWin);

#define RootlessDamageRegion(pWindow, pRegion) { \
    pWindow = TopLevelParent(pWindow); \
    if (!pWindow) { \
        RL_DEBUG_MSG("RootlessDamageRegion: window has no Aqua parent\n"); \
    } else if (!WINREC(pWindow)) { \
        RL_DEBUG_MSG("RootlessDamageRegion: top-level window with no Aqua mirror\n"); \
    } else { \
        REGION_UNION ((pWindow)->drawable.pScreen, &WINREC(pWindow)->damage, \
                      &WINREC(pWindow)->damage, (pRegion)); \
    } \
}

void RootlessStartDrawing(WindowPtr pWindow);

// Stop drawing.
// Used in most places, because drawing doesn't really have to stop.
// Drawing is always really stopped during the block handler.
#define RootlessStopDrawing(pWindow)


// RootlessWindowRec: private per-window data
typedef struct RootlessWindowRec {
    RootlessFrameRec frame;
    RegionRec damage;
    int startDrawCount;       // 0 means not drawing
    unsigned int borderWidth; // needed for MoveWindow(VTOther) (%$#@!!!)
    PixmapPtr scratchPixmap;  // a scratch pixmap maintained by
                              // Start/StopDrawing
} RootlessWindowRec;

typedef struct {
    GCFuncs *originalFuncs;
    GCOps *originalOps;
} RootlessGCRec;

GCFuncs rootlessGCFuncs;

// Global variables
extern int rootlessGCPrivateIndex;
extern int rootlessScreenPrivateIndex;
extern int rootlessWindowPrivateIndex;

#endif /* _ROOTLESS_H */