/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 *
 * By Gregory Robert Parker
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.c,v 1.11 2001/07/01 02:13:41 torrey Exp $ */

// X headers
#include "scrnintstr.h"

// System headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

// We need CoreGraphics in ApplicationServices, but we leave out
// QuickDraw, which has symbol conflicts with the basic X includes.
#define __QD__
#define __PRINTCORE__
#include <ApplicationServices/ApplicationServices.h>

#include "darwin.h"
#include "quartz.h"
#include "quartzAudio.h"
#include "quartzCursor.h"
#include "rootlessAqua.h"

static BOOL xhidden = FALSE;

static CGDisplayCount       quartzDisplayCount = 0;
static CGDirectDisplayID   *quartzDisplayList = NULL;


/*
 * QuartzStoreColors
 *  FIXME: need to implement if Quartz supports PsuedoColor
 */
static void QuartzStoreColors(
    ColormapPtr     pmap,
    int             numEntries,
    xColorItem      *pdefs)
{
}

/*
===========================================================================

 Screen functions

===========================================================================
*/

/*
 * QuartzPMThread
 * Handle power state notifications, FIXME
 */
#if 0
static void *QuartzPMThread(void *arg)
{
    for (;;) {
        mach_msg_return_t       kr;
        mach_msg_empty_rcv_t    msg;

        kr = mach_msg((mach_msg_header_t*) &msg, MACH_RCV_MSG, 0,
                      sizeof(msg), pmNotificationPort, 0, MACH_PORT_NULL);
        kern_assert(kr);

        // computer just woke up
        if (msg.header.msgh_id == 1) {
            if (!xhidden) {
                int i;

                for (i = 0; i < screenInfo.numScreens; i++) {
                    if (screenInfo.screens[i]) 
                        xf86SetRootClip(screenInfo.screens[i], true);
                }
            }
        }
    }
    return NULL;
}
#endif


/* 
 * QuartzFSAddScreen
 *  Do initialization of each screen for Quartz in full screen mode.
 */
Bool QuartzFSAddScreen(
    int index,
    ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    CGDirectDisplayID cgID = quartzDisplayList[index];
    CGRect bounds;

    dfb->pixelInfo.pixelType = kIORGBDirectPixels;	// FIXME
    dfb->pixelInfo.bitsPerComponent = CGDisplayBitsPerSample(cgID);
    dfb->pixelInfo.componentCount = CGDisplaySamplesPerPixel(cgID);
#if FALSE
    // FIXME: endian and 24 bit color specific
    dfb->pixelInfo.componentMasks[0] = 0x00ff0000;
    dfb->pixelInfo.componentMasks[1] = 0x0000ff00;
    dfb->pixelInfo.componentMasks[2] = 0x000000ff;
#endif

    bounds = CGDisplayBounds(cgID);
    dfb->x = bounds.origin.x;
    // flip so (0, 0) is top left of main screen
    dfb->y = CGDisplayBounds(kCGDirectMainDisplay).size.height - 
                bounds.size.height - bounds.origin.y;
    dfb->width  = bounds.size.width;
    dfb->height = bounds.size.height;
    dfb->pitch = CGDisplayBytesPerRow(cgID);
    dfb->bitsPerPixel = CGDisplayBitsPerPixel(cgID);
    dfb->colorBitsPerPixel = (dfb->pixelInfo.componentCount * 
                              dfb->pixelInfo.bitsPerComponent);

    dfb->framebuffer = CGDisplayBaseAddress(cgID);

    // initialize colormap handling as needed
    if (dfb->pixelInfo.pixelType == kIOCLUTPixels) {
        pScreen->StoreColors = QuartzStoreColors;
    }

    return TRUE;
}


/*
 * QuartzAddScreen
 *  Do mode dependent initialization of each screen for Quartz.
 */
Bool QuartzAddScreen(
    int index,
    ScreenPtr pScreen)
{
    // do full screen or rootless specific initialization
    if (quartzRootless) {
        return AquaAddScreen(index, pScreen);
    } else {
        return QuartzFSAddScreen(index, pScreen); 
    }
}


/*
 * QuartzSetupScreen
 *  Finalize mode specific setup of each screen.
 */
Bool QuartzSetupScreen(
    int index,
    ScreenPtr pScreen)
{
    // setup cursor support
    if (! QuartzInitCursor(pScreen))
        return FALSE;

    // do full screen or rootless specific setup
    if (quartzRootless) {
        if (! AquaSetupScreen(index, pScreen))
            return FALSE;
    } else {
        // capture full screen because X doesn't like read-only framebuffer
        CGDisplayCapture(quartzDisplayList[index]); 
    }

    return TRUE;
}


/*
 * QuartzCapture
 *  Capture the screen so we can draw.
 */
void QuartzCapture(void)
{
    int i;

    for (i = 0; i < quartzDisplayCount; i++) {
        CGDirectDisplayID cgID = quartzDisplayList[i];

        if (!CGDisplayIsCaptured(cgID) && !quartzRootless) {
            CGDisplayCapture(cgID);
        }
    }
}


/* 
 * QuartzRelease
 *  Release the screen so others can draw.
 */
static void QuartzRelease(void)
{
    int i;

    for (i = 0; i < quartzDisplayCount; i++) {
        CGDirectDisplayID cgID = quartzDisplayList[i];

        if (CGDisplayIsCaptured(cgID) && !quartzRootless) {
            CGDisplayRelease(cgID);
        }
        QuartzMessageMainThread(kQuartzServerHidden);
    }
}


/*
 * QuartzFSDisplayInit
 *  Find all the CoreGraphics displays.
 */
static void QuartzFSDisplayInit(void) 
{
    CGGetActiveDisplayList(0, NULL, &quartzDisplayCount);
    quartzDisplayList = xalloc(quartzDisplayCount * sizeof(CGDirectDisplayID));
    CGGetActiveDisplayList(quartzDisplayCount, quartzDisplayList, 
                           &quartzDisplayCount);

    darwinScreensFound = quartzDisplayCount;
    atexit(QuartzRelease);
}


/*
 * QuartzInitOutput
 *  Quartz display initialization.
 */
void QuartzInitOutput(void)
{
    QuartzAudioInit();

    if (quartzRootless) {
        ErrorF("Display mode: Rootless Quartz\n");
        AquaDisplayInit();
    } else {
        ErrorF("Display mode: Full screen Quartz\n");
        QuartzFSDisplayInit();
    }
}


/* 
 * QuartzShow
 *  Show the X server on screen. Does nothing if already shown.
 *  Restore the X clip regions the X server cursor state.
 */
void QuartzShow(
    int x,	// cursor location
    int y )
{
    int i;

    if (xhidden) {
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
	        if (!quartzRootless)
                    xf86SetRootClip(screenInfo.screens[i], TRUE);
                QuartzResumeXCursor(screenInfo.screens[i], x, y);
            }
        }
    }
    xhidden = FALSE;
}


/* 
 * QuartzHide
 *  Remove the X server display from the screen. Does nothing if already
 *  hidden. Release the screen, set X clip regions to prevent drawing, and
 *  restore the Aqua cursor.
 */
void QuartzHide(void)
{
    int i;

    if (!xhidden) {
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
                QuartzSuspendXCursor(screenInfo.screens[i]);
                if (!quartzRootless)
                    xf86SetRootClip(screenInfo.screens[i], FALSE);
            }
        }
    } 
    QuartzRelease();
    xhidden = TRUE;
}


/*
 * QuartzGiveUp
 *  Cleanup before X server shutdown
 *  Release the screen and restore the Aqua cursor.
 */
void QuartzGiveUp(void)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        if (screenInfo.screens[i]) {
            QuartzSuspendXCursor(screenInfo.screens[i]);
        }
    }
    QuartzRelease();
}

