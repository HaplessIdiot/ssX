/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 *
 * By Gregory Robert Parker
 *
 **************************************************************/

#ifdef DARWIN_WITH_QUARTZ

// X headers
#include "mi.h"
#include "mipointer.h"
#include "scrnintstr.h"
#include "opaque.h"     // for display variable

// System headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <CoreGraphics/CGDirectDisplay.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "darwin.h"
#include "quartz.h"

#define kDarwinMaxScreens 100
static ScreenPtr darwinScreens[kDarwinMaxScreens];
static int darwinNumScreens = 0;
static char darwinEventFifoName[PATH_MAX] = "";
static BOOL xhidden = false;
static mach_port_t pmNotificationPort;


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
static Bool QuartzInitCursor(ScreenPtr pScreen) 
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

                for (i = 0; i < darwinNumScreens; i++) {
                    if (darwinScreens[i]) 
                        xf86SetRootClip(darwinScreens[i], true);
                }
            }
        }
    }
    return NULL;
}
#endif


/* 
 * QuartzAddScreen
 *  Quartz keeps a list of all screens for QuartzShow and QuartzHide.
 *  FIXME: So does ddx, use that instead.
 */
Bool QuartzAddScreen(ScreenPtr pScreen) 
{
    if (darwinNumScreens == kDarwinMaxScreens) {
        return FALSE;
    }

    darwinScreens[darwinNumScreens++] = pScreen;

    // setup cursor support
    if (! QuartzInitCursor(pScreen)) {
        return FALSE;
    }

    // initialize colormap handling as needed
    if (dfb.pixelInfo.pixelType == kIOCLUTPixels) {
        pScreen->StoreColors = QuartzStoreColors;
    }

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
        CGDisplayHideCursor(kCGDirectMainDisplay);
        HideMenuBar();
    }
}


/* 
 * QuartzRelease
 *  Release the screen so others can draw and restore the Aqua cursor.
 */
static void QuartzRelease(void)
{
    if (CGDisplayIsCaptured(kCGDirectMainDisplay)) {
        CGDisplayShowCursor(kCGDirectMainDisplay);
        CGDisplayRelease(kCGDirectMainDisplay);
        ShowMenuBar();
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
 * One-time Quartz initialization.
 */
void QuartzOsVendorInit(void)
{
    kern_return_t       kr;
    io_connect_t        pwrService;
    pthread_t           pmThread;

    QuartzDisplayInit();
    QuartzFifoInit();

#if 0
    // Register for power management events from the Root Power Domain
    kr = IOCreateReceivePort(kOSNotificationMessageID, &pmNotificationPort);
    kern_assert(kr);
    pwrService = IORegisterForSystemPower(pmNotificationPort, 0);
    if (pwrService) {
        // initialize power manager handling
        pthread_create(&pmThread, NULL, QuartzPMThread, NULL);
    } else {
        ErrorF("Power management registration failed.\n");
    }
#endif
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


#else // not DARWIN_WITH_QUARTZ

// No Quartz support. All Quartz functions are no-ops

BOOL QuartzAddScreen(ScreenPtr pScreen) {
    FatalError("QuartzAddScreen called without Quartz support compiled.\n");
}

void QuartzOsVendorInit(void) {
    FatalError("QuartzOsVendorInit called without Quartz support compiled.\n");
}

void QuartzGiveUp(void) {
    FatalError("QuartzGiveUp called without Quartz support compiled.\n");
}

void QuartzHide(void) {
    FatalError("QuartzHide called without Quartz support compiled.\n");
}
void QuartzShow(void); {
    FatalError("QuartzShow called without Quartz support compiled.\n");
}

#endif
