/*
 * Rootless implementation for Mac OS X Aqua environment
 *
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: $ */

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <stdio.h>

#include "rootlessAquaImp.h"

// Private glue structure. rootless.c sees only void* per window.
typedef struct AquaWindowRec {
    WindowRef window;
    PixMap *rootPix;
} AquaWindowRec;


/*
 * Helper functions for custom window procedures
 */

static void getCurrentPortBounds(Rect* inRect)
{
    CGrafPtr thePort;

    GetPort(&thePort);
    GetPortBounds(thePort,inRect);
}


static RgnHandle getWindowContentRegion(WindowRef window, RgnHandle result)
{
    if (IsWindowCollapsed(window)) {
        SetEmptyRgn(result);
    } else {
        RgnHandle rgn = NULL;
        Rect windowRect;

        getCurrentPortBounds(&windowRect);
        GetWindowProperty(window,'XDar','cntR',sizeof(rgn),NULL,&rgn);
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


/*
 * Custom window procedures
 */

static SInt32 customWindowGetFeatures(WindowRef window, SInt32 param)
{
    *(OptionBits*)param = 
        kWindowCanGetWindowRegion |
        kWindowDefSupportsColorGrafPort;
    return 1;
}


static SInt32 customWindowInitialize(WindowRef window, SInt32 param)
{
    RgnHandle contRgn, structRgn;
    Rect frame;
    getCurrentPortBounds(&frame);

    contRgn = NewRgn();
    SetRectRgn(contRgn, 0, 0, frame.right-frame.left, frame.bottom-frame.top);
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


static SInt32 customWindowGetRegion(WindowRef window, SInt32 param)
{
    GetWindowRegionPtr rgnRec=(GetWindowRegionPtr)param;

    switch(rgnRec->regionCode){
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


static void customReshapeWindow(WindowPtr window, RgnHandle shapeRgn)
{
    RgnHandle contRgn, structRgn;
    RgnHandle tempRgn = NULL;

    GetWindowProperty(window, 'XDar', 'cntR', sizeof(contRgn), NULL, &contRgn);
    CopyRgn(shapeRgn, contRgn);
    GetWindowProperty(window, 'XDar', 'strR', sizeof(structRgn),NULL,&structRgn);
    CopyRgn(shapeRgn, structRgn);

    // Add pixel (0, 0) to work around shape weirdness
    if (!tempRgn) tempRgn = NewRgn();
    SetRectRgn(tempRgn, 0, 0, 1, 1);
    UnionRgn(structRgn, tempRgn, structRgn);
    UnionRgn(contRgn, tempRgn, contRgn);

    ReshapeCustomWindow(window);
}


// Custom window definition
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


/*
 * Drawing functions
 */

// CopyBits from src to dst in rect r, 
// but set alpha to opaque everywhere 
// and set pixel to transparent where pixel is the magic color (RGB 0xfffffe)
// 
static void myerasecopy(PixMap *src, PixMap *dst, Rect *r)
{
    UInt32 *srcbits = (unsigned long *)src->baseAddr;
    UInt32 *dstbits = (unsigned long *)dst->baseAddr;
    UInt16 srcrowbytes = src->rowBytes & 0x3fff;
    UInt16 dstrowbytes = dst->rowBytes & 0x3fff;
    UInt32 *dstp, *srcp, *dstline, *srcline, *dstend, *srcend;
    int height = r->bottom - r->top;

    dstline = dstbits + r->top*dstrowbytes/4;
    srcline = srcbits + r->top*srcrowbytes/4;

    while (height--) {
        dstp = dstline + r->left;
        srcp = srcline + r->left;
        dstend = dstline + r->right;
        srcend = srcline + r->right;

        while (dstp != dstend) {
            if ((*srcp & 0x00ffffff) == 0x00fffffe) 
                *dstp = 0x00000000;
            else 
                *dstp = *srcp | 0xff000000;
            dstp++;
            srcp++;
        }

        dstline += dstrowbytes/4;
        srcline += srcrowbytes/4;
    }
}


// rects are X-flipped and LOCAL to window!
static void dodraw(void *rw, fakeBoxRec *fakerects, int count)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;
    fakeBoxRec *rects = (fakeBoxRec *)fakerects;
    fakeBoxRec *end;
    static RgnHandle rgn = NULL;
    static RgnHandle box = NULL;
    unsigned char *bits;
    unsigned short rowBytes;
    GrafPtr port;
    PixMapHandle pix;
    Rect bounds;

    if (!rgn) rgn = NewRgn();
    if (!box) box = NewRgn();

    port = GetWindowPort(win);
    LockPortBits(port);
    pix = GetPortPixMap(port);
    HLock((Handle)pix);

    GetPixBounds(pix, &bounds);
    bits = GetPixBaseAddr(pix);
    rowBytes = GetPixRowBytes(pix) & 0x3fff; // fixme is mask needed?

    for (rects = fakerects, end = fakerects+count; rects < end; rects++) {
        Rect qdrect;
        qdrect.left = rects->x1;
        qdrect.top = rects->y1;
        qdrect.right = rects->x2;
        qdrect.bottom = rects->y2;

        RectRgn(box, &qdrect);
        UnionRgn(rgn, box, rgn);
        if (winRec->rootPix) {
        myerasecopy(winRec->rootPix, *pix, &qdrect);
        }
    }

    HUnlock((Handle)pix);
    UnlockPortBits(port);
    QDFlushPortBuffer(port, rgn);

    SetEmptyRgn(rgn);
    SetEmptyRgn(box);
}


static void dorestack(WindowRef win, WindowRef upper)
{
    if (upper == NULL) {
        SelectWindow(win);
    }
    else {
        SendBehind(win, upper);
        SelectWindow(FrontNonFloatingWindow());
    }
}


/*
 * Rootless frame procedures (wrapped in rootlessAquaGlue.c)
 */

void AquaDisplayInit(int *width, int *height, int *rowBytes, 
                     unsigned long *bps, unsigned long *spp, int *bpp)
{
    NSWindowDepth depth = [[NSScreen deepestScreen] depth];
  
    *bps = NSBitsPerSampleFromDepth(depth);
    *spp = NSNumberOfColorComponents(NSColorSpaceFromDepth(depth));
    *bpp = 32; // fixme! doesn't work: NSBitsPerPixelFromDepth(depth);

    // fixme assuming only one screen
    // fixme need to adjust for menu bar? but not dock!?
    *width = [[NSScreen mainScreen] frame].size.width;
    *height = [[NSScreen mainScreen] frame].size.height;
    *rowBytes = (*width) * (*bpp) / 8;
}


// FIXME: This is a nasty hack of catching window events in a
// Carbon event handler and repackaging the events off for a Cocoa
// routine to handle that will ultimately pass the event to the
// X server thread. Hopefully this can change soon.
//
// inUserData is a WindowRef
static OSStatus 
HandleWindowEvent(EventHandlerCallRef inHandlerCallRef, 
                  EventRef inEvent, void *inUserData)
{
#if 0
    Point pos;
    EventMouseButton button;
    NXEvent ev;
    EventKind kind = GetEventKind(inEvent);
    WindowRef hitWindow;
    UInt32 modifiers;

    // hitWindow may be NULL
    hitWindow = (WindowRef) inUserData;

    GetEventParameter(inEvent, kEventParamMouseLocation, 
                      typeQDPoint, NULL, sizeof(pos), NULL, &pos);
    ev.location.x = pos.h;
    ev.location.y = pos.v;

    GetEventParameter(inEvent, kEventParamKeyModifiers, 
                      typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);

    ev.flags = 0;
    // fixme incomplete
    // fixme send correct-button event instead? 
    // no, darwin.c needs to unpress the modifier
    if (modifiers & cmdKey) ev.flags |= NX_COMMANDMASK;
    if (modifiers & shiftKey) ev.flags |= NX_SHIFTMASK;
    if (modifiers & optionKey) ev.flags |= NX_ALTERNATEMASK;
    if (modifiers & controlKey) ev.flags |= NX_CONTROLMASK;
  
    switch (kind) {
        case kEventMouseDown:
            if (hitWindow == NULL) {
                // mouse down outside window - don't report
                // This should never happen.
                NSLog(@"rootless carbon event handler: window is NULL!");
                return eventNotHandledErr;
            }
            if (! [NSApp isActive]) {
                [NSApp activateIgnoringOtherApps:YES];
                // Our windows aren't in the cocoa window menu, but this 
                // successfully brings them all to the front anyway.
                // Carbon window manipulators refuse to bring the windows in front 
                // of other apps' windows, for some reason, or something.
                [NSApp arrangeInFront:nil];
            }
            // FALL-THROUGH to kEventMouseUp
        case kEventMouseUp:
            // *do* report mouseUP outside windows (e.g. window resize)
            GetEventParameter(inEvent, kEventParamMouseButton, 
                            typeMouseButton, NULL, sizeof(button), NULL, &button);
            if (button == 1) {
                ev.type = (kind == kEventMouseDown) ? NSLeftMouseDown : NSLeftMouseUp;
            } else {
                // nasty magic to match undocumented Cocoa behavior
                unsigned long delta = 0, buttons = 0;
                ev.type = NSSystemDefined;
                ev.data.compound.subType = 7;
                delta |= 1 << (button - 1);
                if (kind == kEventMouseDown) buttons |= 1 << (button - 1);
                ev.data.compound.misc.L[0] = delta;
                ev.data.compound.misc.L[1] = buttons;
            }
            break;

        case kEventMouseMoved:
        case kEventMouseDragged:
            ev.type = NSMouseMoved;
            break;

        default:
            return eventNotHandledErr;
    }

    [gXserver sendNXEvent:&ev];
#endif
    return noErr;
}


// x,y,w,h is X-flipped screen coords
void *AquaNewWindow(void *upperw, int x, int y, int w, int h, int isRoot)
{
    static EventTypeSpec mouseEvents[] = {
        // mouseDown MUST BE FIRST
        { kEventClassMouse, kEventMouseDown }, 
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, kEventMouseDragged },
        { kEventClassMouse, kEventMouseUp }
    };

    static EventHandlerUPP winHandler = NULL;
    static WindowDefSpec customWindowDefSpec;
    Rect globalBounds;
    AquaWindowRec *winRec = 
        (AquaWindowRec *) malloc(sizeof(AquaWindowRec));
  
    SetRect(&globalBounds, x, y, x+w, y+h);
    if (winHandler == NULL) {
        winHandler = NewEventHandlerUPP(HandleWindowEvent);
        customWindowDefSpec.defType = kWindowDefProcPtr;
        customWindowDefSpec.u.defProc = NewWindowDefUPP(customWindowDef);

        // Catch mouseUp and mouseMoved outside windows
        InstallApplicationEventHandler(winHandler,GetEventTypeCount(mouseEvents)-1,
                                       mouseEvents+1, NULL, NULL);
    }

    if (isRoot) {
        CreateNewWindow(kOverlayWindowClass, 0, &globalBounds, &winRec->window);
        SetWindowGroup(winRec->window,GetWindowGroupOfClass(kFloatingWindowClass));
    } else {
        CreateCustomWindow(&customWindowDefSpec, kDocumentWindowClass, 0, 
                           &globalBounds, &winRec->window);
    }
    InstallWindowEventHandler(winRec->window, winHandler, 
                              GetEventTypeCount(mouseEvents), mouseEvents, 
                              winRec->window /*inUserData*/, NULL /*outRef*/);

    if (!isRoot) {
        winRec->rootPix = NULL;
    } else {
        PixMapPtr winpix = *GetPortPixMap(GetWindowPort(winRec->window));
        PixMapPtr rootpix = malloc(sizeof(PixMap));
        Rect bounds;

        *rootpix = *winpix;
        SetRect(&bounds, 0, 0, w, h);
        rootpix->bounds = bounds;
        rootpix->rowBytes = winpix->rowBytes & 0xbfff;
        rootpix->baseAddr = malloc(h * (rootpix->rowBytes & 0x3fff));
        winRec->rootPix = rootpix;
    }

    ShowWindow(winRec->window);
    dorestack(winRec->window, 
              upperw ? ((AquaWindowRec *) upperw)->window : NULL);

    return winRec;
}


void AquaDestroyWindow(void *rw)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;

    if (winRec->rootPix) {
        free(winRec->rootPix->baseAddr);
        free(winRec->rootPix);
    }
    HideWindow(win);
    ReleaseWindow(win);
    free(rw);
}


void AquaMoveWindow(void *rw, int x, int y)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;

    MoveWindow(win, x, y, false);

    // this erases bits of the window unnecessarily
    // GetWindowBounds(win, kWindowContentRgn, &bounds);
    // OffsetRect(&bounds, -bounds.left+x, -bounds.top+y);
    // SetWindowBounds(win, kWindowContentRgn, &bounds);
}


void AquaStartResizeWindow(void *rw, int x, int y, int w, int h)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;
    RgnHandle rgn = NULL; 
    if (!rgn) rgn = NewRgn();

    // Undo current shape. Need to wait for the next SetShape.
    SetRectRgn(rgn, 0, 0, w, h);
    customReshapeWindow(win, rgn);

    // fixme doesn't work, hmm - use MoveWindow instead
    // SetRect(&bounds, x, y, x+w, y+h);
    // SetWindowBounds(win, kWindowContentRgn, &bounds);
    MoveWindow(win, x, y, false);

    SetEmptyRgn(rgn);
}


void AquaFinishResizeWindow(void *rw, int x, int y, int w, int h)
{
    WindowRef win = ((AquaWindowRec *)rw)->window;
    Rect qdbounds;
    fakeBoxRec bounds;

    GetWindowBounds(win, kWindowContentRgn, &qdbounds);
    OffsetRect(&qdbounds, -x, -y);
    AquaUpdateRects(rw, &bounds, 1);
}


void AquaRestackWindow(void *rw, void *upperw)
{
    WindowRef win = ((AquaWindowRec *)rw)->window;
    WindowRef upper = upperw ? ((AquaWindowRec *)upperw)->window : NULL;

    dorestack(win, upper);
}


void AquaStartDrawing(void *rw, char **bits, 
                      int *rowBytes, int *depth, int *bpp)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;
    PixMapHandle pix;
    GrafPtr port;
    OSStatus err;

    if (! winRec->rootPix) {
        port = GetWindowPort(win);
        err = LockPortBits(port);
        pix = GetPortPixMap(port);
    } else {
        pix = &winRec->rootPix;
    }
    *bits = GetPixBaseAddr(pix);
    *rowBytes = GetPixRowBytes(pix) & 0x3fff; // fixme is mask needed?
    *depth = (**pix).cmpCount * (**pix).cmpSize; // fixme use GetPixDepth(pix)?
    *bpp = (**pix).pixelSize;
}


void AquaStopDrawing(void *rw)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;
    WindowRef win = winRec->window;
    GrafPtr port;

    if (! winRec->rootPix) {
        port = GetWindowPort(win);
        UnlockPortBits(port);
    }
}


// rects are X-flipped, LOCAL to window!
void AquaUpdateRects(void *rw, fakeBoxRec *fakerects, int count)
{
    dodraw(rw, fakerects, count);
}


// rects are X-flipped, LOCAL to window!
void AquaReshapeWindow(void *rw, fakeBoxRec *fakerects, int count)
{
    AquaWindowRec *winRec = (AquaWindowRec *)rw;  
    WindowRef win = winRec->window;
    fakeBoxRec *rects, *end;
    RgnHandle rgn = NULL;
    RgnHandle box = NULL;

    if (!rgn) rgn = NewRgn();
    if (!box) box = NewRgn();

    for (rects = fakerects, end = fakerects+count; rects < end; rects++) {
        SetRectRgn(box, rects->x1, rects->y1, rects->x2, rects->y2);
        UnionRgn(rgn, box, rgn);
    }

    customReshapeWindow(win, rgn);

    SetEmptyRgn(rgn);
    SetEmptyRgn(box);
}
