/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 *
 * By Gregory Robert Parker
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.c,v 1.23 2001/12/22 05:28:35 torrey Exp $ */

#include "quartzCommon.h"
#include "quartz.h"
#include "darwin.h"
#include "quartzAudio.h"
#include "quartzCursor.h"
#include "fullscreen.h"
#include "rootlessAqua.h"
#include "pseudoramiX.h"

// X headers
#include "scrnintstr.h"
#include "colormapst.h"

// System headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

// Shared global variables for Quartz modes
int                     quartzEventWriteFD = -1;
int                     quartzStartClients = 1;
int                     quartzRootless = -1;
int                     quartzUseSysBeep = 0;
int                     quartzServerVisible = TRUE;
int                     quartzScreenIndex = 0;
int                     aquaMenuBarHeight = 0;
int                     noPseudoramiXExtension = TRUE;
int                     aquaNumScreens = 0;


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
            if (quartzServerVisible) {
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
 * QuartzAddScreen
 *  Do mode dependent initialization of each screen for Quartz.
 */
Bool QuartzAddScreen(
    int index,
    ScreenPtr pScreen)
{
    // allocate space for private per screen Quartz specific storage
    QuartzScreenPtr displayInfo = xcalloc(sizeof(QuartzScreenRec), 1);
    QUARTZ_PRIV(pScreen) = displayInfo;

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
    // do full screen or rootless specific setup
    if (quartzRootless) {
        if (! AquaSetupScreen(index, pScreen))
            return FALSE;
    } else {
        if (! QuartzFSSetupScreen(index, pScreen))
            return FALSE;
    }

    // setup cursor support
    if (! QuartzInitCursor(pScreen))
        return FALSE;

    return TRUE;
}


/*
 * QuartzInitOutput
 *  Quartz display initialization.
 */
void QuartzInitOutput(
    int argc,
    char **argv )
{
    static unsigned long generation = 0;

    // Allocate private storage for each screen's Quartz specific info
    if (generation != serverGeneration) {
        quartzScreenIndex = AllocateScreenPrivateIndex();
        generation = serverGeneration;
    }

    if (serverGeneration == 0) {
        QuartzAudioInit();
    }

    if (quartzRootless) {
        ErrorF("Display mode: Rootless Quartz\n");
        AquaDisplayInit();
    } else {
        ErrorF("Display mode: Full screen Quartz\n");
        QuartzFSDisplayInit();
    }

    // Init PseudoramiX implementation of Xinerama.
    // This should be in InitExtensions, but that causes link errors
    // for servers that don't link in pseudoramiX.c.
    if (!noPseudoramiXExtension) {
        PseudoramiXExtensionInit(argc, argv);
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

    if (!quartzServerVisible) {
        quartzServerVisible = TRUE;
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
                QuartzResumeXCursor(screenInfo.screens[i], x, y);
                if (!quartzRootless)
                    xf86SetRootClip(screenInfo.screens[i], TRUE);
            }
        }
    }
}


/*
 * QuartzHide
 *  Remove the X server display from the screen. Does nothing if already
 *  hidden. Set X clip regions to prevent drawing, and restore the Aqua
 *  cursor.
 */
void QuartzHide(void)
{
    int i;

    if (quartzServerVisible) {
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
                QuartzSuspendXCursor(screenInfo.screens[i]);
                if (!quartzRootless)
                    xf86SetRootClip(screenInfo.screens[i], FALSE);
            }
        }
    }
    quartzServerVisible = FALSE;
    QuartzMessageMainThread(kQuartzServerHidden);
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
    if (!quartzRootless)
        QuartzFSRelease();
}
