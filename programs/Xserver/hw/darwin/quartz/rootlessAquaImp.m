/*
 * Rootless implementation for Mac OS X Aqua environment
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/rootlessAquaImp.m,v 1.11 2002/01/17 02:44:27 torrey Exp $ */

#include "rootlessAquaImp.h"
#include "fakeBoxRec.h"
#include "quartzCommon.h"
#include "pseudoramiX.h"
#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#import "XView.h"

extern void ErrorF(const char *, ...);

typedef struct {
    NSWindow *window;
    XView *view;
    WindowRef windowRef;
    GWorldPtr rootGWorld;
} AquaWindowRec;

#define AQUA_WINREC(rw) ((AquaWindowRec *)rw)

static SInt32 customWindowDef(short varCode, WindowRef window, 
			      SInt16 message, SInt32 param);
static void customReshapeWindow(WindowRef window, RgnHandle shapeRgn);


/*
 * AquaDisplayCount
 *  Return the number of displays.
 *  Multihead note: When rootless mode uses PseudoramiX, the
 *  X server only sees one screen; only PseudoramiX itself knows
 *  about all of the screens.
 */
int AquaDisplayCount()
{
    aquaNumScreens = [[NSScreen screens] count];

    if (noPseudoramiXExtension) {
        return aquaNumScreens;
    } else {
        return 1; // only PseudoramiX knows about the rest
    }
}


void AquaScreenInit(int index, int *x, int *y, int *width, int *height,
                    int *rowBytes, unsigned long *bps, unsigned long *spp,
                    int *bpp)
{
    *bps = 8;
    *spp = 3;
    *bpp = 32;

    if (noPseudoramiXExtension) {
        NSScreen *screen = [[NSScreen screens] objectAtIndex:index];
        NSRect frame = [screen frame];

        // set x, y so (0,0) is top left of main screen
        *x = NSMinX(frame);
        *y = NSHeight([[NSScreen mainScreen] frame]) - NSHeight(frame) -
            NSMinY(frame);

        *width =  NSWidth(frame);
        *height = NSHeight(frame);
        *rowBytes = (*width) * (*bpp) / 8;

        // Shift the usable part of main screen down to avoid the menu bar.
        if (NSEqualRects(frame, [[NSScreen mainScreen] frame])) {
            *y      += aquaMenuBarHeight;
            *height -= aquaMenuBarHeight;
        }

    } else {
        int i;
        NSRect unionRect = NSMakeRect(0, 0, 0, 0);
        NSArray *screens = [NSScreen screens];

        // Get the union of all screens (minus the menu bar on main screen)
        for (i = 0; i < [screens count]; i++) {
            NSScreen *screen = [screens objectAtIndex:i];
            NSRect frame = [screen frame];
            frame.origin.y = [[NSScreen mainScreen] frame].size.height -
                             frame.size.height - frame.origin.y;
            if (NSEqualRects([screen frame], [[NSScreen mainScreen] frame])) {
                frame.origin.y    += aquaMenuBarHeight;
                frame.size.height -= aquaMenuBarHeight;
            }
            unionRect = NSUnionRect(unionRect, frame);
        }

        // Use unionRect as the screen size for the X server.
        *x = unionRect.origin.x;
        *y = unionRect.origin.y;
        *width = unionRect.size.width;
        *height = unionRect.size.height;
        *rowBytes = (*width) * (*bpp) / 8;

        // Tell PseudoramiX about the real screens.
        // InitOutput() will move the big screen to (0,0),
        // so compensate for that here.
        for (i = 0; i < [screens count]; i++) {
            NSScreen *screen = [screens objectAtIndex:i];
            NSRect frame = [screen frame];
            int j;

            // Skip this screen if it's a mirrored copy of an earlier screen.
            for (j = 0; j < i; j++) {
                if (NSEqualRects(frame, [[screens objectAtIndex:j] frame])) {
                    ErrorF("PseudoramiX screen %d is a mirror of screen %d.\n",
                           i, j);
                    break;
                }
            }
            if (j < i) continue; // this screen is a mirrored copy

            frame.origin.y = [[NSScreen mainScreen] frame].size.height -
                             frame.size.height - frame.origin.y;

            if (NSEqualRects([screen frame], [[NSScreen mainScreen] frame])) {
                frame.origin.y    += aquaMenuBarHeight;
                frame.size.height -= aquaMenuBarHeight;
            }

            ErrorF("PseudoramiX screen %d added: %dx%d @ (%d,%d).\n", i,
                   (int)frame.size.width, (int)frame.size.height,
                   (int)frame.origin.x, (int)frame.origin.y);

            frame.origin.x -= unionRect.origin.x;
            frame.origin.y -= unionRect.origin.y;

            ErrorF("PseudoramiX screen %d placed at X11 coordinate (%d,%d).\n",
                   i, (int)frame.origin.x, (int)frame.origin.y);

            PseudoramiXAddScreen(frame.origin.x, frame.origin.y,
                                 frame.size.width, frame.size.height);
        }
    }
}


/*
 * AquaNewWindow
 *  Create a new on-screen window.
 *  Rootless windows must not autodisplay! Autodisplay can cause a deadlock.
 *    Event thread - autodisplay: locks view hierarchy, then window
 *    X Server thread - window resize: locks window, then view hierarchy
 *  Deadlock occurs if each thread gets one lock and waits for the other.
 *  Autodisplay can also cause NSCarbonWindows to lose their contents.
*/
void *AquaNewWindow(void *upperw, int x, int y, int w, int h, int isRoot)
{
    AquaWindowRec *winRec = (AquaWindowRec *)malloc(sizeof(AquaWindowRec));
    NSRect viewRect = NSMakeRect(0, 0, w, h);
    Rect globalRect;
    WindowRef carbonWindowRef;
    NSWindow *theWindow;
    XView *theView;

    static WindowDefSpec customWindowDefSpec = {0, {0}};

    if (!customWindowDefSpec.u.defProc) {
        customWindowDefSpec.defType = kWindowDefProcPtr;
        customWindowDefSpec.u.defProc = NewWindowDefUPP(customWindowDef);
    }

    // Create a Carbon window so we can later use Carbon-only features.
    // <grumble>We should be able to do everything with Cocoa & CG.</grumble>
    SetRect(&globalRect, x, y, x+w, y+h);
    CreateCustomWindow(&customWindowDefSpec, kDocumentWindowClass,
                       kWindowNoAttributes, &globalRect, &carbonWindowRef);
    if (!isRoot) {
        ShowWindow(carbonWindowRef);
    }

    // Create an NSWindow to wrap the Carbon window
    theWindow = [[NSWindow alloc] initWithWindowRef:carbonWindowRef];
    if (! theWindow) return NULL;

    [theWindow setBackgroundColor:[NSColor clearColor]];  // erase transparent
    [theWindow setAlphaValue:1.0];       // draw opaque
    [theWindow setOpaque:YES];           // changed when window is shaped

    [theWindow useOptimizedDrawing:YES]; // Has no overlapping sub-views
    [theWindow setAutodisplay:NO];       // See comment above
    [theWindow disableFlushWindow];      // We do all the flushing manually
    [theWindow setHasShadow:!isRoot];    // All windows have shadows except root

    theView = [[XView alloc] initWithFrame:viewRect];
    [theWindow setContentView:theView];
    [theWindow setInitialFirstResponder:theView];

    if (upperw) {
        AquaWindowRec *upperRec = AQUA_WINREC(upperw);
        int uppernum = [upperRec->window windowNumber];
        [theWindow orderWindow:NSWindowBelow relativeTo:uppernum];
    } else {
        if (!isRoot) {
            [theWindow orderFront:nil];
        }
    }

    [theWindow setAcceptsMouseMovedEvents:YES];

    winRec->window = theWindow;
    winRec->view = theView;
    winRec->windowRef = carbonWindowRef;

    if (!isRoot) {
        winRec->rootGWorld = NULL;
    } else {
        // Allocate the offscreen graphics world for root window drawing
        GWorldPtr rootGWorld;

        if (NewGWorld(&rootGWorld, 0, &globalRect, NULL, NULL, 0))
            return NULL;
        winRec->rootGWorld = rootGWorld;
    }

    return winRec;
}


void AquaDestroyWindow(void *rw)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);

    [winRec->window release];
    [winRec->view release];
    if (winRec->rootGWorld) {
        DisposeGWorld(winRec->rootGWorld);
    }
    free(rw);
}


void AquaMoveWindow(void *rw, int x, int y)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);

    MoveWindow(winRec->windowRef, x, y, FALSE);
}


/*
 * AquaStartResizeWindow
 *  Undo any shape and resize the on screen window.
 */
void AquaStartResizeWindow(void *rw, int x, int y, int w, int h)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);
    WindowRef win = winRec->windowRef;
    static RgnHandle rgn = NULL; 

    if (!rgn) rgn = NewRgn();

    // Undo current shape. Need to wait for the next SetShape.
    SetRectRgn(rgn, 0, 0, w, h);
    customReshapeWindow(win, rgn);

    SizeWindow(win, w, h, FALSE);
    MoveWindow(win, x, y, FALSE);

    SetEmptyRgn(rgn);
}


void AquaFinishResizeWindow(void *rw, int x, int y, int w, int h)
{
    // refresh everything? fixme yes for testing
    fakeBoxRec box = {0, 0, w, h};
    AquaUpdateRects(rw, &box, 1);
}


/*
 * AquaUpdateRects
 *  Flush rectangular regions from a window's backing buffer
 *  (or PixMap for the root window) to the screen.
 */
void AquaUpdateRects(void *rw, fakeBoxRec *fakeRects, int count)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);
    WindowRef win = winRec->windowRef;
    fakeBoxRec *rects = (fakeBoxRec *)fakeRects;
    fakeBoxRec *end;
    static RgnHandle rgn = NULL;
    static RgnHandle box = NULL;
    GrafPtr port;

    if (!rgn) rgn = NewRgn();
    if (!box) box = NewRgn();

    port = GetWindowPort(win);

#if 0
    if (winRec->rootGWorld) {
        // FIXME: The root window requires special treatment
    }
#endif

    for (rects = fakeRects, end = fakeRects+count; rects < end; rects++) {
        Rect qdrect;
        qdrect.left = rects->x1;
        qdrect.top = rects->y1;
        qdrect.right = rects->x2;
        qdrect.bottom = rects->y2;

        RectRgn(box, &qdrect);
        UnionRgn(rgn, box, rgn);
#if 0
        if (winRec->rootGWorld) {
            // FIXME: Draw from the root PixMap to the normally
            // invisible root window.
        }
#endif
    }

#if 0
    if (winRec->rootGWorld) {
        UnlockPortBits(port);
    }
#endif

    QDFlushPortBuffer(port, rgn);

    SetEmptyRgn(rgn);
    SetEmptyRgn(box);
}


/*
 * AquaRestackWindow
 *  Change the window order. Put the window behind upperw or on top if
 *  upperw is NULL.
 */
void AquaRestackWindow(void *rw, void *upperw)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);

    if (upperw) {
        AquaWindowRec *upperRec = AQUA_WINREC(upperw);
        int uppernum = [upperRec->window windowNumber];
        [winRec->window orderWindow:NSWindowBelow relativeTo:uppernum];
    } else {
        [winRec->window makeKeyAndOrderFront:nil];
    }
}


/*
 * AquaReshapeWindow
 *  Set the shape of a window. The rectangles are the areas that are
 *  part of the new shape.
 */
void AquaReshapeWindow(void *rw, fakeBoxRec *fakeRects, int count)
{
    AquaWindowRec *winRec = AQUA_WINREC(rw);
    WindowRef win = winRec->windowRef;
    fakeBoxRec *rects, *end;
    static RgnHandle rgn = NULL;
    static RgnHandle box = NULL;

    if (!rgn) rgn = NewRgn();
    if (!box) box = NewRgn();

    for (rects = fakeRects, end = fakeRects+count; rects < end; rects++) {
        SetRectRgn(box, rects->x1, rects->y1, rects->x2, rects->y2);
        UnionRgn(rgn, box, rgn);
    }

    customReshapeWindow(win, rgn);

    SetEmptyRgn(rgn);
    SetEmptyRgn(box);
}


/* AquaStartDrawing
 *  When a window's buffer is not being drawn to, the CoreGraphics
 *  window server may compress or move it. Call this routine
 *  to lock down the buffer during direct drawing. It returns
 *  a pointer to the backing buffer and its depth, etc.
 */
void AquaStartDrawing(void *rw, char **bits,
                      int *rowBytes, int *depth, int *bpp)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->windowRef;
    PixMapHandle pix;

    if (! winRec->rootGWorld) {
        GrafPtr port = GetWindowPort(win);
        LockPortBits(port);
        pix = GetPortPixMap(port);
    } else {
        pix = GetGWorldPixMap(winRec->rootGWorld);
        LockPixels(pix);
    }

    *bits = GetPixBaseAddr(pix);
    *rowBytes = GetPixRowBytes(pix) & 0x3fff; // fixme is mask needed?
    *depth = (**pix).cmpCount * (**pix).cmpSize; // fixme use GetPixDepth(pix)?
    *bpp = (**pix).pixelSize;
}


/*
 * AquaStopDrawing
 *  When direct access to a window's buffer is no longer needed, this
 *  routine should be called to allow CoreGraphics to compress or
 *  move it.
 */
void AquaStopDrawing(void *rw)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->windowRef;

    if (! winRec->rootGWorld) {
        GrafPtr port = GetWindowPort(win);
        UnlockPortBits(port);
    } else {
        PixMapHandle pix = GetGWorldPixMap(winRec->rootGWorld);
        UnlockPixels(pix);
    }
}


/////////////////////////////////////////////////////////////////
// Custom window definition
//
// We need a custom window definition to make Carbon windows
// without any frame.

static void getCurrentPortBounds(Rect *inRect)
{
    CGrafPtr thePort;
    GetPort(&thePort);
    GetPortBounds(thePort, inRect);
}

static RgnHandle getWindowContentRegion(WindowRef window, RgnHandle result)
{
    if (IsWindowCollapsed(window)) {
        SetEmptyRgn(result);
    } else {
        RgnHandle rgn = NULL;
        Rect windowRect;

        getCurrentPortBounds(&windowRect);
        GetWindowProperty(window, 'XDar', 'cntR', sizeof(rgn), NULL, &rgn);
        CopyRgn(rgn, result);
        OffsetRgn(result, windowRect.left, windowRect.top);
    }

    return result;
}

static RgnHandle getWindowStructureRegion(WindowRef window, RgnHandle result)
{
    RgnHandle rgn = NULL;
    Rect windowRect;

    getCurrentPortBounds(&windowRect);
    GetWindowProperty(window, 'XDar', 'strR', sizeof(rgn), NULL, &rgn);
    CopyRgn(rgn, result);
    OffsetRgn(result, windowRect.left, windowRect.top);

    return result;
}

static SInt32 customWindowInitialize(WindowRef window, SInt32 param)
{
    RgnHandle contRgn, structRgn;
    Rect frame;
    getCurrentPortBounds(&frame);

    contRgn = NewRgn();
    SetRectRgn(contRgn, 0, 0,
               frame.right-frame.left+1, frame.bottom-frame.top+1);
    structRgn = NewRgn();
    CopyRgn(contRgn, structRgn);

    SetWindowProperty(window, 'XDar', 'cntR', sizeof(contRgn), &contRgn);
    SetWindowProperty(window, 'XDar', 'strR', sizeof(structRgn), &structRgn);

    return 0;
}

static SInt32 customWindowCleanUp(WindowRef window, SInt32 param)
{
    RgnHandle rgn;

    GetWindowProperty(window, 'XDar', 'cntR', sizeof(rgn), NULL, &rgn);
    DisposeRgn(rgn);
    GetWindowProperty(window, 'XDar', 'strR', sizeof(rgn), NULL, &rgn);
    DisposeRgn(rgn);

    return 0;
}

static SInt32 customWindowHitTest(WindowRef window, SInt32 param)
{
    Point hitPoint;
    static RgnHandle rgn = NULL;

    if (!rgn) rgn = NewRgn();
    SetPt(&hitPoint, LoWord(param), HiWord(param));

    if (PtInRgn(hitPoint, getWindowContentRegion(window, rgn)))
        return wInContent;

    return wNoHit; // missed
}

static SInt32 customWindowGetFeatures(WindowRef window, SInt32 param)
{
    *(OptionBits*)param = kWindowCanGetWindowRegion |
                          kWindowDefSupportsColorGrafPort;
    return 1;
}

static SInt32 customWindowGetRegion(WindowRef window, SInt32 param)
{
    GetWindowRegionPtr rgnRec = (GetWindowRegionPtr)param;

    switch (rgnRec->regionCode) {

        case kWindowStructureRgn:
            getWindowStructureRegion(window, rgnRec->winRgn);
            return noErr;

        case kWindowContentRgn:
            getWindowContentRegion(window, rgnRec->winRgn);
            return noErr;

        case kWindowTitleBarRgn:
        case kWindowTitleTextRgn:
        case kWindowCloseBoxRgn:
        case kWindowZoomBoxRgn:
        case kWindowDragRgn:
        case kWindowGrowRgn:
        case kWindowCollapseBoxRgn:
        case kWindowUpdateRgn:
            SetEmptyRgn(rgnRec->winRgn);
            return noErr;

        default:
            return errWindowRegionCodeInvalid;
    }
}

static void customReshapeWindow(WindowRef window, RgnHandle shapeRgn)
{
    RgnHandle contRgn, structRgn;
    RgnHandle tempRgn = NULL;

    GetWindowProperty(window, 'XDar', 'cntR', sizeof(contRgn),
                      NULL, &contRgn);
    CopyRgn(shapeRgn, contRgn);
    GetWindowProperty(window, 'XDar', 'strR', sizeof(structRgn),
                      NULL, &structRgn);
    CopyRgn(shapeRgn, structRgn);

    // Add pixel (0, 0) to work around shape weirdness
    if (!tempRgn) tempRgn = NewRgn();
    SetRectRgn(tempRgn, 0, 0, 1, 1);
    UnionRgn(structRgn, tempRgn, structRgn);
    UnionRgn(contRgn, tempRgn, contRgn);

    ReshapeCustomWindow(window);
}

static SInt32 customWindowDef(short varCode, WindowRef window, 
			      SInt16 message, SInt32 param)
{
    switch (message) {
        case kWindowMsgInitialize:
            return customWindowInitialize(window, param);

        case kWindowMsgCleanUp:
            return customWindowCleanUp(window, param);

        case kWindowMsgHitTest:
            return customWindowHitTest(window, param);

        case kWindowMsgGetFeatures:
            return customWindowGetFeatures(window, param);

        case kWindowMsgGetRegion:
            return customWindowGetRegion(window, param);

        // no-ops
        case kWindowMsgDraw:
        case kWindowMsgCalculateShape:
        case kWindowMsgDrawGrowOutline:
        case kWindowMsgDrawGrowBox:
        case kWindowMsgDragHilite:
        case kWindowMsgModified:
        case kWindowMsgDrawInCurrentPort:
        case kWindowMsgSetupProxyDragImage:
        case kWindowMsgStateChanged:
        case kWindowMsgMeasureTitle:
        case kWindowMsgGetGrowImageRegion:
        default:
            return 0;
    }
}
