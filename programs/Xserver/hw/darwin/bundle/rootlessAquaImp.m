/*
 * Rootless implementation for Mac OS X Aqua environment
 */
/* $XFree86: $ */

#include "rootlessAquaImp.h"
#include "XWindow.h"
#include "fakeBoxRec.h"


typedef struct {
    XWindow *window;
} AquaWindowRec;


#define WINREC(rw) ((AquaWindowRec *)rw)

void AquaDisplayInit(int *width, int *height, int *rowBytes,
                     unsigned long *bps, unsigned long *spp, int *bpp)
{
    // fixme
    *bps = 8;
    *spp = 3;
    *bpp = 32;

    *width =  [[NSScreen mainScreen] frame].size.width;
    *height = [[NSScreen mainScreen] frame].size.height;
    *rowBytes = (*width) * (*bpp) / 8;
}

void *AquaNewWindow(void *upperw, int x, int y, int w, int h, int isRoot)
{
    AquaWindowRec *winRec = (AquaWindowRec *)malloc(sizeof(AquaWindowRec));
    NSRect frame = {{x, y}, {w, h}};

    frame.origin.y = [[NSScreen mainScreen] frame].size.height - y - h;
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
    NSPoint topLeft = {x, y};

    topLeft.y = [[NSScreen mainScreen] frame].size.height - topLeft.y;
    [winRec->window setFrameTopLeftPoint:topLeft];
}

void AquaStartResizeWindow(void *rw, int x, int y, int w, int h)
{
    AquaWindowRec *winRec = WINREC(rw);
    NSRect frame = {{x, y}, {w, h}};

    frame.origin.y = [[NSScreen mainScreen] frame].size.height - y - h;
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

void AquaReshapeWindow(void *rw, fakeBoxRec *rects, int count)
{
    // fixme reimplement shape
}

void AquaGetPixmap(void *rw, char **bits,
		   int *rowBytes, int *depth, int *bpp)
{
    AquaWindowRec *winRec = WINREC(rw);
    [winRec->window getBits:bits rowBytes:rowBytes depth:depth
                    bitsPerPixel:bpp];
}
