/*
 * Rootless implementation for Mac OS X Aqua environment
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/rootlessAquaImp.m,v 1.8 2001/11/06 23:18:14 torrey Exp $ */

#include "rootlessAquaImp.h"
#include "XWindow.h"
#include "fakeBoxRec.h"
#include "quartzCommon.h"


typedef struct {
    XWindow *window;
} AquaWindowRec;


#define WINREC(rw) ((AquaWindowRec *)rw)

int AquaDisplayCount()
{
    return [[NSScreen screens] count];
}

void AquaScreenInit(int index, int *x, int *y, int *width, int *height,
                    int *rowBytes, unsigned long *bps, unsigned long *spp,
                    int *bpp)
{
    NSScreen *screen = [[NSScreen screens] objectAtIndex:index];
    NSRect frame = [screen frame];

    // fixme
    *bps = 8;
    *spp = 3;
    *bpp = 32;

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
}

void *AquaNewWindow(void *upperw, int x, int y, int w, int h, int isRoot)
{
    AquaWindowRec *winRec = (AquaWindowRec *)malloc(sizeof(AquaWindowRec));
    NSRect frame = NSMakeRect(x, NSHeight([[NSScreen mainScreen] frame]) -
                              y - h, w, h);

    winRec->window = [[XWindow alloc] initWithContentRect:frame isRoot:isRoot];

    if (upperw) {
        AquaWindowRec *upperRec = WINREC(upperw);
        int uppernum = [upperRec->window windowNumber];
        [winRec->window orderWindow:NSWindowBelow relativeTo:uppernum];
    } else {
        [winRec->window orderFront:nil];
    }

    // fixme hide root for now
    if (isRoot) [winRec->window orderOut:nil];

    return winRec;
}

void AquaDestroyWindow(void *rw)
{
    AquaWindowRec *winRec = WINREC(rw);

    [winRec->window release];
}

void AquaMoveWindow(void *rw, int x, int y)
{
    AquaWindowRec *winRec = WINREC(rw);
    NSPoint topLeft = NSMakePoint(x, NSHeight([[NSScreen mainScreen] frame]) -
                                  y);

    [winRec->window setFrameTopLeftPoint:topLeft];
}

void AquaStartResizeWindow(void *rw, int x, int y, int w, int h)
{
    AquaWindowRec *winRec = WINREC(rw);
    NSRect frame = NSMakeRect(x, NSHeight([[NSScreen mainScreen] frame]) -
                              y - h, w, h);

    [winRec->window setFrame:frame display:NO];
}

void AquaFinishResizeWindow(void *rw, int x, int y, int w, int h)
{
    // refresh everything? fixme yes for testing
    fakeBoxRec box = {0, 0, w, h};
    AquaWindowRec *winRec = WINREC(rw);

    [winRec->window refreshRects:&box count:1];
}

void AquaUpdateRects(void *rw, fakeBoxRec *rects, int count)
{
    AquaWindowRec *winRec = WINREC(rw);

    [winRec->window refreshRects:rects count:count];
}

// fixme is this upperw or lowerw?
void AquaRestackWindow(void *rw, void *upperw)
{
    AquaWindowRec *winRec = WINREC(rw);

    if (upperw) {
        AquaWindowRec *upperRec = WINREC(upperw);
        int uppernum = [upperRec->window windowNumber];
        [winRec->window orderWindow:NSWindowBelow relativeTo:uppernum];
    } else {
        [winRec->window orderFront:nil];
    }
    // [winRec->window setAcceptsMouseMovedEvents:YES];
    // fixme prefer to orderFront whenever possible - pass upperw, not lowerw
}

// rects are the areas not part of the new shape
void AquaReshapeWindow(void *rw, fakeBoxRec *rects, int count)
{
    AquaWindowRec *winRec = WINREC(rw);

    // make transparent if window is now shaped
    // transparent windows never go back to opaque
    if (count > 0) {
        [winRec->window setOpaque:NO];
    }

    [[winRec->window contentView] reshapeRects:rects count:count];

    if (! [winRec->window isOpaque]) {
        // force update of window shadow
        [winRec->window setHasShadow:NO];
        [winRec->window setHasShadow:YES];
    }
}

void AquaGetPixmap(void *rw, char **bits,
                   int *rowBytes, int *depth, int *bpp)
{
    AquaWindowRec *winRec = WINREC(rw);

    [winRec->window getBits:bits rowBytes:rowBytes depth:depth
                    bitsPerPixel:bpp];
}
