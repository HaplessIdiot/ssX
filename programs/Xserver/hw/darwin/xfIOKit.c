/**************************************************************
 *
 * IOKit support for the Darwin X Server
 *
 * HISTORY:
 * Original port to Mac OS X Server by John Carmack
 * Port to Darwin 1.0 by Dave Zarzycki
 * Significantly rewritten for XFree86 4.0.1 by Torrey Lyons
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/darwin.c,v 1.2 2000/12/01 19:47:38 dawes Exp $ */

#define NDEBUG 1

#include "X.h"
#include "Xproto.h"
#include "os.h"
#include "servermd.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "mibstore.h"
#include "mipointer.h"
#include "micmap.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <mach/mach_interface.h>

#define NO_CFPLUGIN
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <drivers/event_status_driver.h>

// Define this to work around bugs in the display drivers for
// older PowerBook G3's. If the X server starts without this
// #define, you don't need it.
#undef OLD_POWERBOOK_G3

#include "darwin.h"
#include "xfIOKit.h"

EvGlobals *             evg;
mach_port_t             masterPort;
mach_port_t             notificationPort;
IONotificationPortRef   NotificationPortRef;

/*
 * XFIOKitStoreColors
 * This is a callback from X to change the hardware colormap
 * when using PsuedoColor.
 */
void XFIOKitStoreColors(
    ColormapPtr     pmap,
    int             numEntries,
    xColorItem      *pdefs)
{
    kern_return_t   kr;
    int             i;
    IOColorEntry    *newColors;

    assert( newColors = (IOColorEntry *)
                xalloc( numEntries*sizeof(IOColorEntry) ));

    // Convert xColorItem values to IOColorEntry
    // assume the colormap is PsuedoColor
    // as we do not support DirectColor
    for (i = 0; i < numEntries; i++) {
        newColors[i].index = pdefs[i].pixel;
        newColors[i].red =   pdefs[i].red;
        newColors[i].green = pdefs[i].green;
        newColors[i].blue =  pdefs[i].blue;
    }

    kr = IOFBSetCLUT( dfb.fbService, 0, numEntries,
                      kSetCLUTByValue, newColors );
    kern_assert( kr );

    xfree( newColors );
}

/*
 * XFIOKitGiveUp
 * FIXME: Crashes kernel if used
 */
void XFIOKitGiveUp( void )
{
#if 0
    // we must close the HID System first
    // because it is a client of the framebuffer
    NXCloseEventStatus( dfb.hidParam );
    IOServiceClose( dfb.hidService );
    IOServiceClose( dfb.fbService );
#endif
}

/*
 * ClearEvent
 * Clear an event from the HID System event queue
 */
static void ClearEvent(NXEvent * ep)
{
    static NXEvent nullEvent = {NX_NULLEVENT, {0, 0 }, 0, -1, 0 };

    *ep = nullEvent;
    ep->data.compound.subType = ep->data.compound.misc.L[0] =
                                ep->data.compound.misc.L[1] = 0;
}

/*
 * XFIOKitHIDThread
 * Read the HID System event queue and pass to pipe
 */
static void *XFIOKitHIDThread(void *arg)
{
    int iokitEventWriteFD = (int)arg;

    for (;;) {
        IOReturn kr;
        NXEvent ev;
        NXEQElement *oldHead;
        struct {
            mach_msg_header_t	header;
            mach_msg_trailer_t	trailer;
        } msg;

        kr = mach_msg((mach_msg_header_t*) &msg, MACH_RCV_MSG, 0,
                      sizeof(msg), notificationPort, 0, MACH_PORT_NULL);
        assert(KERN_SUCCESS == kr);

        while (evg->LLEHead != evg->LLETail) {
            oldHead = (NXEQElement*)&evg->lleq[evg->LLEHead];
            ev_lock(&oldHead->sema);
            ev = oldHead->event;
            ClearEvent(&oldHead->event);
            evg->LLEHead = oldHead->next;
            ev_unlock(&oldHead->sema);

            write(iokitEventWriteFD, &ev, sizeof(ev));
        }
    }
    return NULL;
}

static void SetupFBandHID(void)
{
    kern_return_t           kr;
    io_service_t            service;
    io_iterator_t           iter;
    io_name_t               name;
    vm_address_t            shmem, vram;
    vm_size_t               shmemSize;
    int                     i;
    UInt32                  numModes;
    IODisplayModeInformation modeInfo;
    IODisplayModeID         displayMode, *allModes;
    IOIndex                 displayDepth;
    IOFramebufferInformation fbInfo;
    StdFBShmem_t            *cshmem;

    dfb.fbService = 0;
    dfb.hidService = 0;

    // find and open the IOFrameBuffer service
    kr = IOServiceGetMatchingServices( masterPort,
                        IOServiceMatching( IOFRAMEBUFFER_CONFORMSTO ),
                        &iter );
    kern_assert( kr );

    assert(service = IOIteratorNext(iter));

    kr = IOServiceOpen( service, mach_task_self(),
                        kIOFBServerConnectType, &dfb.fbService );
    if (kr != KERN_SUCCESS)
        FatalError("failed to connect as window server!\nMake sure you have quit the Mac OS X window server.\n");

    IOObjectRelease( service );
    IOObjectRelease( iter );

    // create the slice of shared memory containing cursor state data
    kr = IOFBCreateSharedCursor( dfb.fbService, kIOFBCurrentShmemVersion,
                               	 32, 32 );
    kern_assert( kr );

    // SET THE SCREEN PARAMETERS
    // get the current screen resolution, refresh rate and depth
    kr = IOFBGetCurrentDisplayModeAndDepth( dfb.fbService, &displayMode,
                                            &displayDepth );
    kern_assert( kr );

    // use the current screen resolution if the user
    // only wants to change the refresh rate
    if (darwinDesiredRefresh != -1 && darwinDesiredWidth == 0) {
        kr = IOFBGetDisplayModeInformation( dfb.fbService, displayMode,
                                            &modeInfo );
        kern_assert( kr );
        darwinDesiredWidth = modeInfo.nominalWidth;
        darwinDesiredHeight = modeInfo.nominalHeight;
    }

    // use the current resolution and refresh rate
    // if the user doesn't have a preference
    if (darwinDesiredWidth == 0) {

        // change the pixel depth if desired
        if (darwinDesiredDepth != -1) {
            kr = IOFBGetDisplayModeInformation( dfb.fbService, displayMode,
                                                &modeInfo );
            kern_assert( kr );
            if (modeInfo.maxDepthIndex < darwinDesiredDepth)
                FatalError("Current screen resolution does not support desired pixel depth!\n");

            displayDepth = darwinDesiredDepth;
            kr = IOFBSetDisplayModeAndDepth( dfb.fbService, displayMode,
                                             displayDepth );
            kern_assert( kr );
        }
 
    // look for display mode with correct resolution and refresh rate
    } else {

        // get an array of all supported display modes
        kr = IOFBGetDisplayModeCount( dfb.fbService, &numModes );
        kern_assert( kr );
        assert(allModes = (IODisplayModeID *)
                xalloc( numModes * sizeof(IODisplayModeID) ));
        kr = IOFBGetDisplayModes( dfb.fbService, numModes, allModes );
        kern_assert( kr );

        for (i = 0; i < numModes; i++) {
            kr = IOFBGetDisplayModeInformation( dfb.fbService, allModes[i],
                                                &modeInfo );
            kern_assert( kr );

            if (modeInfo.flags & kDisplayModeValidFlag &&
                modeInfo.nominalWidth == darwinDesiredWidth &&
                modeInfo.nominalHeight == darwinDesiredHeight) {

                if (darwinDesiredDepth == -1)
                    darwinDesiredDepth = modeInfo.maxDepthIndex;
                if (modeInfo.maxDepthIndex < darwinDesiredDepth)
                    FatalError("Desired screen resolution does not support desired pixel depth!\n");
                if ((darwinDesiredRefresh == -1 ||
                    (darwinDesiredRefresh << 16) == modeInfo.refreshRate)) {
                    displayMode = allModes[i];
                    displayDepth = darwinDesiredDepth;
                    kr = IOFBSetDisplayModeAndDepth( dfb.fbService, displayMode,
                                                     displayDepth );
                    kern_assert( kr );
                    break;
                }
            }
        }

        xfree( allModes );
        if (i >= numModes)
            FatalError("Desired screen resolution or refresh rate is not supported!\n");
    }

    kr = IOFBGetPixelInformation( dfb.fbService, displayMode, displayDepth,
                                  kIOFBSystemAperture, &dfb.pixelInfo );
    kern_assert( kr );

#ifdef OLD_POWERBOOK_G3
    if (dfb.pixelInfo.pixelType == kIOCLUTPixels)
        dfb.pixelInfo.pixelType = kIOFixedCLUTPixels;
#endif

    kr = IOFBGetFramebufferInformationForAperture( dfb.fbService, kIOFBSystemAperture,
                                                   &fbInfo );
    kern_assert( kr );

    kr = IOConnectMapMemory( dfb.fbService, kIOFBCursorMemory,
                             mach_task_self(), (vm_address_t *) &cshmem,
                             &shmemSize, kIOMapAnywhere );
    kern_assert( kr );
    dfb.cursorShmem = cshmem;

    kr = IOConnectMapMemory( dfb.fbService, kIOFBSystemAperture, mach_task_self(),
                             &vram, &shmemSize, kIOMapAnywhere );
    kern_assert( kr );

    dfb.framebuffer = (void*)vram;
    dfb.width = fbInfo.activeWidth;
    dfb.height = fbInfo.activeHeight;
    dfb.pitch = fbInfo.bytesPerRow;
    dfb.bitsPerPixel = fbInfo.bitsPerPixel;
    dfb.colorBitsPerPixel = dfb.pixelInfo.componentCount *
                            dfb.pixelInfo.bitsPerComponent;

    // find and open the HID System Service
    kr = IOServiceGetMatchingServices( masterPort,
                                       IOServiceMatching( kIOHIDSystemClass ),
                                       &iter );
    kern_assert( kr );

    assert( service = IOIteratorNext( iter ) );

    kr = IORegistryEntryGetName( service, name );
    kern_assert( kr );

    kr = IOServiceOpen( service, mach_task_self(), kIOHIDServerConnectType,
                        &dfb.hidService );
    kern_assert( kr );

    IOObjectRelease( service );
    IOObjectRelease( iter );

    kr = IOHIDCreateSharedMemory( dfb.hidService, kIOHIDCurrentShmemVersion );
    kern_assert( kr );

    kr = IOHIDSetEventsEnable(dfb.hidService, TRUE);
    kern_assert( kr );

    // Inform the HID system that the framebuffer is also connected to it
    kr = IOConnectAddClient( dfb.hidService, dfb.fbService );
    kern_assert( kr );

    kr = IOHIDSetCursorEnable(dfb.hidService, TRUE);
    kern_assert( kr );

    kr = IOConnectMapMemory( dfb.hidService, kIOHIDGlobalMemory, mach_task_self(),
                             &shmem, &shmemSize, kIOMapAnywhere );
    kern_assert( kr );

    evg = (EvGlobals *)(shmem + ((EvOffsets *)shmem)->evGlobalsOffset);

    assert(sizeof(EvGlobals) == evg->structSize);

    NotificationPortRef = IONotificationPortCreate( masterPort );

    notificationPort = IONotificationPortGetMachPort(NotificationPortRef);

    kr = IOConnectSetNotificationPort( dfb.hidService, kIOHIDEventNotification,
                                       notificationPort, 0 );
    kern_assert( kr );

    evg->movedMask |= NX_MOUSEMOVEDMASK;
}

/*
 * XFIOKitOsVendorInit
 *  One-time initialization of IOKit support.
 *  Connect to framebuffer and HID system.
 */
void XFIOKitOsVendorInit(void)
{
    kern_return_t           kr;
    int                     fd[2];

    kr = IOMasterPort(bootstrap_port, &masterPort);
    kern_assert( kr );

    SetupFBandHID();

    assert( pipe(fd) == 0 );
    darwinEventFD = fd[0];
    fcntl(darwinEventFD, F_SETFL, O_NONBLOCK);
    pthread_create(&dfb.hidThread, NULL, XFIOKitHIDThread, (void *) fd[1]);
}
