/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 *
 * By Gregory Robert Parker
 *
 **************************************************************/

#ifdef DARWIN_WITH_QUARTZ

#include "mi.h"
#include "mipointer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// CoreGraphics and IOKit include conflicting type definitions. 
// Include MacTypes first to pre-empt them.
#include <CarbonCore/MacTypes.h>
#include <CoreGraphics/CGDirectDisplay.h>

#include "darwin.h"
#include "quartz.h"

#include "opaque.h" // for display variable

#define kDarwinMaxScreens 100
static ScreenPtr darwinScreens[kDarwinMaxScreens];
static int darwinNumScreens = 0;
static char darwinEventFifoName[PATH_MAX] = "";
static BOOL xhidden = false;

static void xf86SetRootClip (ScreenPtr pScreen, BOOL enable);

/*
 * QuartzStoreColors
 *  FIXME: need to implement if Quartz supports PsuedoColor
 */
void QuartzStoreColors(
    ColormapPtr     pmap,
    int             numEntries,
    xColorItem      *pdefs)
{
}

/*
===========================================================================

 Pointer functions

===========================================================================
*/

/*
 * QuartzCursorOffScreen
 */
static Bool QuartzCursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    return FALSE;
}


/*
 * QuartzCrossScreen
 */
static void QuartzCrossScreen(ScreenPtr pScreen, Bool entering)
{
    return;
}


/*
 * QuartzWarpCursor
 *  Change the cursor position without generating an event or motion history
 */
static void
QuartzWarpCursor(
    ScreenPtr               pScreen,
    int                     x,
    int                     y)
{
    CGDisplayErr            cgErr;
    CGPoint                 cgPoint;

    cgPoint = CGPointMake(x, y);
    cgErr = CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, cgPoint);
    if (cgErr != CGDisplayNoErr) {
        ErrorF("Could not set cursor position with error code 0x%x.\n", cgErr);
    }
    miPointerWarpCursor(pScreen, x, y);
}

static miPointerScreenFuncRec quartzScreenFuncsRec = {
  QuartzCursorOffScreen,
  QuartzCrossScreen,
  QuartzWarpCursor,
};

/* 
 * QuartzInitCursor
 */
Bool QuartzInitCursor(ScreenPtr pScreen) 
{
    // initialize software cursor handling
    if (!miDCInitialize(pScreen, &quartzScreenFuncsRec)) {
        return FALSE;
    }

    return TRUE;
}


/*
===========================================================================

 Screen functions

===========================================================================
*/

/* 
 * QuartzAddScreen
 *  Quartz keeps a list of all screens for QuartzShow and QuartzHide.
 */
Bool QuartzAddScreen(ScreenPtr screen) 
{
    if (darwinNumScreens == kDarwinMaxScreens) {
        return FALSE;
    }

    darwinScreens[darwinNumScreens++] = screen;
    return TRUE;
}


/* 
 * QuartzCapture
 *  Capture the screen so we can draw and hide the Aqua cursor.
 */
static void QuartzCapture(void)
{
    if (! CGDisplayIsCaptured(kCGDirectMainDisplay)) {
        CGDisplayCapture(kCGDirectMainDisplay);
    }
    CGDisplayHideCursor(kCGDirectMainDisplay);
}


/* 
 * QuartzRelease
 *  Release the screen so others can draw and restore the Aqua cursor.
 */
static void QuartzRelease(void)
{
    CGDisplayShowCursor(kCGDirectMainDisplay);
    if (CGDisplayIsCaptured(kCGDirectMainDisplay)) {
        CGDisplayRelease(kCGDirectMainDisplay);
    }
}


/*
 * QuartzDisplayInit
 *  Init the framebuffer and claim the display from CoreGraphics.
 */
static void QuartzDisplayInit(void) 
{
    dfb.pixelInfo.pixelType = kIORGBDirectPixels;
    dfb.pixelInfo.bitsPerComponent=CGDisplayBitsPerSample(kCGDirectMainDisplay);
    dfb.pixelInfo.componentCount=CGDisplaySamplesPerPixel(kCGDirectMainDisplay);
#if FALSE
    // FIXME: endian and 24 bit color specific
    dfb.pixelInfo.componentMasks[0] = 0x00ff0000;
    dfb.pixelInfo.componentMasks[1] = 0x0000ff00;
    dfb.pixelInfo.componentMasks[2] = 0x000000ff;
#endif

    dfb.width  = CGDisplayPixelsWide(kCGDirectMainDisplay);
    dfb.height = CGDisplayPixelsHigh(kCGDirectMainDisplay);
    dfb.pitch = CGDisplayBytesPerRow(kCGDirectMainDisplay);
    dfb.bitsPerPixel = CGDisplayBitsPerPixel(kCGDirectMainDisplay);
    dfb.colorBitsPerPixel = (dfb.pixelInfo.componentCount * 
                            dfb.pixelInfo.bitsPerComponent);

    dfb.framebuffer = CGDisplayBaseAddress(kCGDirectMainDisplay);

    // need to capture because X doesn't like read-only framebuffer...
    QuartzCapture(); 
    atexit(QuartzRelease);
}


/* 
 * QuartzFifoInit
 *  Initialize the named pipe to which the X server listens for events.
 *  The fifo is named /tmp/.X<display>-fifo, right next to the 
 *  /tmp/.X<display> lock file.
 */
static void QuartzFifoInit(void)
{
    int i;
    int err;
    mode_t oldmode;

    // copied from os/utils.c
    #define LOCK_DIR "/tmp"
    #define LOCK_PREFIX "/.X"
    #define FIFO_SUFFIX "-fifo"

    sprintf(darwinEventFifoName, LOCK_DIR LOCK_PREFIX "%s" FIFO_SUFFIX, display);
    unlink(darwinEventFifoName); // nuke it if it exists
    oldmode = umask(0); // fifo is world-everything, ignore umask (fixme)
    err = mkfifo(darwinEventFifoName, 0666);
    umask(oldmode);
    if (err) {
        FatalError("couldn't make event fifo '%s' (errno=%d)\n", 
                    darwinEventFifoName, errno);
    }

    for (i = 0; i < 10; i++) {
        darwinEventFD = open( darwinEventFifoName, O_RDONLY | O_NONBLOCK, 0 );
        if (darwinEventFD < 0) {
            ErrorF("Could not open event fifo '%s' (errno %d)\n", 
                    darwinEventFifoName, errno);
        } else {
            break;
        }
    }
    if (darwinEventFD < 0) {
        // never got fifo
        FatalError("giving up\n");
    }
}


/*
 * QuartzOsVendorInit
 * One-time quartz initialization.
 */
void QuartzOsVendorInit(void)
{
    QuartzDisplayInit();
    QuartzFifoInit();
}


/* 
 * QuartzShow
 * Show the X server on screen. Does nothing if already shown.
 * recapture the screen, restore the X clip regions.
 */
void QuartzShow(void) {
    int i;

    QuartzCapture();
    if (xhidden) {
        for (i = 0; i < darwinNumScreens; i++) {
        if (darwinScreens[i]) 
            xf86SetRootClip(darwinScreens[i], true);
        }
    }
    xhidden = false;
}


/* 
 * QuartzHide
 *  Remove the X server display from the screen. Does nothing if already hidden.
 *  Release the screen, set X clip regions to prevent drawing.
 */
void QuartzHide(void)
{
    int i;

    if (!xhidden) {
        for (i = 0; i < darwinNumScreens; i++) {
            if (darwinScreens[i]) 
                xf86SetRootClip(darwinScreens[i], false);	
        }
    } 
    QuartzRelease();
    xhidden = true;
}


/*
 * QuartzGiveUp
 *  Cleanup before X server shutdown
 *  release the screen, remove the event fifo
 */
void QuartzGiveUp(void)
{
    if (darwinEventFifoName[0]) {
        unlink(darwinEventFifoName);
        darwinEventFifoName[0] = '\0';
    }
    QuartzRelease();
}


#include "mivalidate.h" // for union _Validate used by windowstr.h
#include "windowstr.h"  // for struct _Window
#include "scrnintstr.h" // for struct _Screen

// This is copied from Xserver/hw/xfree86/common/xf86Helper.c.
// Quartz mode uses this when switching in and out of Quartz. 
// Copyright (c) 1997-1998 by The XFree86 Project, Inc.

/*
 * xf86SetRootClip --
 *	Enable or disable rendering to the screen by
 *	setting the root clip list and revalidating
 *	all of the windows
 */

static void
xf86SetRootClip (ScreenPtr pScreen, BOOL enable)
{
    WindowPtr	pWin = WindowTable[pScreen->myNum];
    WindowPtr	pChild;
    Bool	WasViewable = (Bool)(pWin->viewable);
    Bool	anyMarked;
    RegionPtr	pOldClip, bsExposed;
#ifdef DO_SAVE_UNDERS
    Bool	dosave = FALSE;
#endif
    WindowPtr   pLayerWin;
    BoxRec	box;

    if (WasViewable)
    {
	for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib)
	{
	    (void) (*pScreen->MarkOverlappedWindows)(pChild,
						     pChild,
						     &pLayerWin);
	}
	(*pScreen->MarkWindow) (pWin);
	anyMarked = TRUE;
	if (pWin->valdata)
	{
	    if (HasBorder (pWin))
	    {
		RegionPtr	borderVisible;

		borderVisible = REGION_CREATE(pScreen, NullBox, 1);
		REGION_SUBTRACT(pScreen, borderVisible,
				&pWin->borderClip, &pWin->winSize);
		pWin->valdata->before.borderVisible = borderVisible;
	    }
	    pWin->valdata->before.resized = TRUE;
	}
    }
    
    /*
     * Use REGION_BREAK to avoid optimizations in ValidateTree
     * that assume the root borderClip can't change well, normally
     * it doesn't...)
     */
    if (enable)
    {
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pScreen->width;
	box.y2 = pScreen->height;
	REGION_RESET(pScreen, &pWin->borderClip, &box);
	REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }
    else
    {
	REGION_EMPTY(pScreen, &pWin->borderClip);
	REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }
    
    ResizeChildrenWinSize (pWin, 0, 0, 0, 0);
    
    if (WasViewable)
    {
	if (pWin->backStorage)
	{
	    pOldClip = REGION_CREATE(pScreen, NullBox, 1);
	    REGION_COPY(pScreen, pOldClip, &pWin->clipList);
	}

	if (pWin->firstChild)
	{
	    anyMarked |= (*pScreen->MarkOverlappedWindows)(pWin->firstChild,
							   pWin->firstChild,
							   (WindowPtr *)NULL);
	}
	else
	{
	    (*pScreen->MarkWindow) (pWin);
	    anyMarked = TRUE;
	}

#ifdef DO_SAVE_UNDERS
	if (DO_SAVE_UNDERS(pWin))
	{
	    dosave = (*pScreen->ChangeSaveUnder)(pLayerWin, pLayerWin);
	}
#endif /* DO_SAVE_UNDERS */

	if (anyMarked)
	    (*pScreen->ValidateTree)(pWin, NullWindow, VTOther);
    }

    if (pWin->backStorage &&
	((pWin->backingStore == Always) || WasViewable))
    {
	if (!WasViewable)
	    pOldClip = &pWin->clipList; /* a convenient empty region */
	bsExposed = (*pScreen->TranslateBackingStore)
			     (pWin, 0, 0, pOldClip,
			      pWin->drawable.x, pWin->drawable.y);
	if (WasViewable)
	    REGION_DESTROY(pScreen, pOldClip);
	if (bsExposed)
	{
	    RegionPtr	valExposed = NullRegion;
    
	    if (pWin->valdata)
		valExposed = &pWin->valdata->after.exposed;
	    (*pScreen->WindowExposures) (pWin, valExposed, bsExposed);
	    if (valExposed)
		REGION_EMPTY(pScreen, valExposed);
	    REGION_DESTROY(pScreen, bsExposed);
	}
    }
    if (WasViewable)
    {
	if (anyMarked)
	    (*pScreen->HandleExposures)(pWin);
#ifdef DO_SAVE_UNDERS
	if (dosave)
	    (*pScreen->PostChangeSaveUnder)(pLayerWin, pLayerWin);
#endif /* DO_SAVE_UNDERS */
	if (anyMarked && pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(pWin, NullWindow, VTOther);
    }
    if (pWin->realized)
	WindowsRestructured ();
    FlushAllOutput ();
}


#else // not DARWIN_WITH_QUARTZ

// No Quartz support. All Quartz functions are no-ops

BOOL QuartzAddScreen(ScreenPtr screen) {
    FatalError("QuartzAddScreen called without Quartz support compiled.");
}

void QuartzStoreColors(ColormapPtr pmap, int numEntries, xColorItem *pdefs) {
    FatalError("QuartzStoreColors called without Quartz support compiled.");
}

void QuartzInitCursor(ScreenPtr screen) {
    FatalError("QuartzInitCursor called without Quartz support compiled.");
}

void QuartzOsVendorInit(void) {
    FatalError("QuartzOsVendorInit called without Quartz support compiled.");
}

void QuartzGiveUp(void) {
    FatalError("QuartzGiveUp called without Quartz support compiled.");
}

void QuartzHide(void) {
    FatalError("QuartzHide called without Quartz support compiled.");
}
void QuartzShow(void); {
    FatalError("QuartzShow called without Quartz support compiled.");
}

#endif
