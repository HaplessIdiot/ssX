/*
 * Common rootless definitions and code
 *
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: $ */

#include "rootlessCommon.h"


RegionRec rootlessHugeRoot = {{-32767, -32767, 32767, 32767}, NULL};


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


// Move the given pixmap's base address to where pixel (0, 0) 
// would be if the pixmap's actual data started at (x, y)
void SetPixmapBaseToScreen(PixmapPtr pix, int x, int y)
{
    pix->devPrivate.ptr = (char *)(pix->devPrivate.ptr) -
        (pix->drawable.bitsPerPixel/8 * x + y*pix->devKind);
}


// Update pWindow's pixmap. 
// This needs to be called every time a window moves relative to 
// its top-level parent, or the parent's pixmap data is reallocated.
void UpdatePixmap(WindowPtr pWindow)
{
    WindowPtr top = TopLevelParent(pWindow);
    RootlessWindowRec *winRec;
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    PixmapPtr pix;
    
    RL_DEBUG_MSG("update pixmap (win 0x%x)", pWindow);
    
    if (! top) {
        RL_DEBUG_MSG("no parent\n");
	return;
    }
    winRec = WINREC(top);
    if (!winRec) {
        RL_DEBUG_MSG("not framed\n");
	return;
    }

    // Destroy the old pixmap we gave to fb
    pix = pScreen->GetWindowPixmap(pWindow);
    if (IsRoot(pWindow)  &&  pix) {
        RL_DEBUG_MSG("not updating root\n");
	return;
    }
    if (pix) FreeScratchPixmapHeader(pix);
    
    // Make a new pixmap
    pix = GetScratchPixmapHeader(pScreen, 
				 winRec->frame.w, winRec->frame.h,
				 winRec->frame.depth, 
				 winRec->frame.bitsPerPixel,
				 winRec->frame.bytesPerRow,
				 winRec->frame.pixelData);
    SetPixmapBaseToScreen(pix, winRec->frame.x, winRec->frame.y);
    pScreen->SetWindowPixmap(pWindow, pix);
    
    RL_DEBUG_MSG("done\n");
}


// pRegion is GLOBAL
void
RootlessDamageRegion(WindowPtr pWindow, RegionPtr pRegion) 
{
    pWindow = TopLevelParent(pWindow);
    if (!pWindow) {
        RL_DEBUG_MSG("RootlessDamageRegion: window is not framed\n");
    } else if (!WINREC(pWindow)) {
        RL_DEBUG_MSG("RootlessDamageRegion: top-level window not a frame\n");
    } else {
        REGION_UNION((pWindow)->drawable.pScreen, &WINREC(pWindow)->damage,
		     &WINREC(pWindow)->damage, (pRegion));
    }
}


// pBox is GLOBAL
void
RootlessDamageBox(WindowPtr pWindow, BoxPtr pBox)
{
    RegionRec region;

    REGION_INIT(pWindow->drawable.pScreen, &region, pBox, 1);
    RootlessDamageRegion(pWindow, &region);
}


// (x, y, w, h) is WINDOW-LOCAL
void
RootlessDamageRect(WindowPtr pWindow, int x, int y, int w, int h)
{
    BoxRec box;
    RegionRec region;

    x += pWindow->drawable.x;
    y += pWindow->drawable.y;
    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;
    REGION_INIT(pWindow->drawable.pScreen, &region, &box, 1);
    RootlessDamageRegion(pWindow, &region);
}


void
RootlessRedisplay(WindowPtr pWindow)
{
    RootlessWindowRec *winRec = WINREC(pWindow);
    ScreenPtr pScreen = pWindow->drawable.pScreen;

    if (REGION_NOTEMPTY(pScreen, &winRec->damage)) {
        REGION_INTERSECT(pScreen, &winRec->damage, &winRec->damage,
                          &pWindow->borderSize);

        // move region to window local coords
        REGION_TRANSLATE(pScreen, &winRec->damage,
                         -winRec->frame.x, -winRec->frame.y);
        CallFrameProc(pScreen, UpdateRegion,
                      (pScreen, &winRec->frame, &winRec->damage));
        REGION_EMPTY(pScreen, &winRec->damage);
    }
}


void
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
