/**************************************************************
 *
 * Shared code for the Darwin X Server
 * running with Quartz or the IOKit
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/darwin.c,v 1.3 2001/01/14 16:44:55 herrb Exp $ */

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
#include "site.h"
#include "xf86Version.h"

#include <sys/types.h>
#include <sys/time.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <pthread.h>

//#include <mach/mach_interface.h>

#define NO_CFPLUGIN
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
//#include <IOKit/hidsystem/IOHIDShared.h>
//#include <IOKit/graphics/IOGraphicsLib.h>
//#include <drivers/event_status_driver.h>

#include "darwin.h"
#include "quartz.h"
#include "xfIOKit.h"

// Shared global variables
DarwinFramebufferRec    dfb;
unsigned char           darwinKeyCommandL = 0, darwinKeyOptionL = 0;
int                     darwinEventFD;
Bool                    quartz = FALSE;
UInt32                  darwinDesiredWidth = 0, darwinDesiredHeight = 0;
IOIndex                 darwinDesiredDepth = -1;
SInt32                  darwinDesiredRefresh = -1;

// Quit after this many seconds if no quartz event poster is found.
// Leave undefined for no safety quit.
#define QUARTZ_SAFETY_DELAY 10

/* Fake button press/release for scroll wheel move. */
#define	SCROLLWHEELUPFAKE	4
#define	SCROLLWHEELDOWNFAKE	5

static	DeviceIntPtr    darwinPointer;
static	DeviceIntPtr    darwinKeyboard;
static	Bool            fake3Buttons = FALSE;

// Common pixmap formats
static PixmapFormatRec formats[] = {
        { 1,    1,      BITMAP_SCANLINE_PAD },
        { 4,    8,      BITMAP_SCANLINE_PAD },
        { 8,    8,      BITMAP_SCANLINE_PAD },
        { 15,   16,     BITMAP_SCANLINE_PAD },
        { 16,   16,     BITMAP_SCANLINE_PAD },
        { 24,   32,     BITMAP_SCANLINE_PAD }
};
const int NUMFORMATS = sizeof(formats)/sizeof(formats[0]);

#ifndef OSNAME
#define OSNAME " Darwin"
#endif
#ifndef OSVENDOR
#define OSVENDOR ""
#endif
#ifndef PRE_RELEASE
#define PRE_RELEASE (XF86_VERSION_BETA || XF86_VERSION_ALPHA)
#endif

static void
DarwinPrintBanner()
{
#if PRE_RELEASE
  ErrorF("\n"
    "This is a pre-release version of XFree86, and is not supported in any\n"
    "way.  Bugs may be reported to XFree86@XFree86.Org and patches submitted\n"
    "to fixes@XFree86.Org.  Before reporting bugs in pre-release versions,\n"
    "please check the latest version in the XFree86 CVS repository\n"
    "(http://www.XFree86.Org/cvs)\n");
#endif
  ErrorF("\nXFree86 Version%s", XF86_VERSION);
#ifdef XF86_CUSTOM_VERSION
  ErrorF("(%s) ", XF86_CUSTOM_VERSION);
#endif
  ErrorF("/ X Window System\n");
  ErrorF("(protocol Version %d, revision %d, vendor release %d)\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, VENDOR_RELEASE );
  ErrorF("Release Date: %s\n", XF86_DATE);
  ErrorF("\tIf the server is older than 6-12 months, or if your hardware is\n"
	 "\tnewer than the above date, look for a newer version before\n"
	 "\treporting problems.  (See http://www.XFree86.Org/FAQ)\n");
  ErrorF("Operating System:%s%s\n", OSNAME, OSVENDOR);
#if defined(BUILDERSTRING)
  ErrorF("%s \n",BUILDERSTRING);
#endif
#if defined(DARWIN_WITH_QUARTZ)
  ErrorF("Mac OS X Quartz support available.\n");
#endif
}

/*
 * DarwinSaveScreen
 *  X screensaver support. Not implemented.
 */
static Bool DarwinSaveScreen(ScreenPtr pScreen, int on)
{
    // FIXME
    if (on == SCREEN_SAVER_FORCER) {
    } else if (on == SCREEN_SAVER_ON) {
    } else {
    }
    return TRUE;
}

/*
 * DarwinAddScreen
 *  This is a callback from X during AddScreen() from InitOutput()
 */
static Bool DarwinAddScreen(
    int         index,
    ScreenPtr   pScreen,
    int         argc,
    char        **argv )
{
    int         bitsPerRGB, i;
    VisualPtr   visual;

    if (quartz) {
        if (! QuartzAddScreen(pScreen)) {
            return FALSE;
        }
    }

    /* Communicate the information about our initialized screen back to X. */
    bitsPerRGB = dfb.pixelInfo.bitsPerComponent;

    // reset the visual list
    miClearVisualTypes();

    // setup a single visual appropriate for our pixel type
    // Note: Darwin kIORGBDirectPixels = X window TrueColor, not DirectColor
    if (dfb.pixelInfo.pixelType == kIORGBDirectPixels) {
        if (!miSetVisualTypes( dfb.colorBitsPerPixel, TrueColorMask,
                                bitsPerRGB, TrueColor )) {
            return FALSE;
        }
    } else if (dfb.pixelInfo.pixelType == kIOCLUTPixels) {
        if (!miSetVisualTypes( dfb.colorBitsPerPixel, PseudoColorMask,
                                bitsPerRGB, PseudoColor )) {
            return FALSE;
        }
    } else if (dfb.pixelInfo.pixelType == kIOFixedCLUTPixels) {
        if (!miSetVisualTypes( dfb.colorBitsPerPixel, StaticColorMask,
                                bitsPerRGB, StaticColor )) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    // machine independent screen init
    // setup _Screen structure in pScreen
    if ( dfb.bitsPerPixel == 32 ) {
        if (!cfb32ScreenInit(pScreen,
                dfb.framebuffer,
                dfb.width, dfb.height,
                    75, 75,		/* screen size in dpi, which we have no accurate knowledge of */
                dfb.pitch / (dfb.bitsPerPixel/8))) {
            return FALSE;
        }
    } else if ( dfb.bitsPerPixel == 16 ) {
        if (!cfb16ScreenInit(pScreen,
                dfb.framebuffer,
                dfb.width, dfb.height,
                    75, 75,		/* screen size in dpi, which we have no accurate knowledge of */
                dfb.pitch / (dfb.bitsPerPixel/8))) {
            return FALSE;
        }
    } else if ( dfb.bitsPerPixel == 8 ) {
        if (!cfbScreenInit(pScreen,
                dfb.framebuffer,
                dfb.width, dfb.height,
                    75, 75,		/* screen size in dpi, which we have no accurate knowledge of */
                dfb.pitch / (dfb.bitsPerPixel/8))) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    // set the RGB order correctly for TrueColor, it is byte swapped by X
    // FIXME: make work on x86 darwin if it ever gets buildable
    if (dfb.bitsPerPixel > 8) {
        for (i = 0, visual = pScreen->visuals;  // someday we may have more than 1
            i < pScreen->numVisuals; i++, visual++) {
            if (visual->class == TrueColor) {
                visual->offsetRed = bitsPerRGB * 2;
                visual->offsetGreen = bitsPerRGB;
                visual->offsetBlue = 0;
#if TRUE
                visual->redMask = ((1<<bitsPerRGB)-1) << visual->offsetRed;
                visual->greenMask = ((1<<bitsPerRGB)-1) << visual->offsetGreen;
                visual->blueMask = ((1<<bitsPerRGB)-1) << visual->offsetBlue;
#else
                visual->redMask = dfb.pixelInfo.componentMasks[0];
                visual->greenMask = dfb.pixelInfo.componentMasks[1];
                visual->blueMask = dfb.pixelInfo.componentMasks[2];
#endif
            }
        }
    }

#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif

    // setup cursor support, use hardware cursor if possible
    if (quartz) {
        if (!QuartzInitCursor(pScreen)) {
            return FALSE;
        }
    } else {
        if (!XFIOKitInitCursor(pScreen)) {
            return FALSE;
        }
    }

    // this must be initialized (why doesn't X have a default?)
    pScreen->SaveScreen = DarwinSaveScreen;

    // initialize colormap handling as needed
    if (dfb.pixelInfo.pixelType == kIOCLUTPixels) {
        if (quartz) {
            pScreen->StoreColors = QuartzStoreColors;
        } else {
            pScreen->StoreColors = XFIOKitStoreColors;
        }
    }

    // create and install the default colormap and
    // set pScreen->blackPixel / pScreen->white
    if (!miCreateDefColormap( pScreen )) {
        return FALSE;
    }
    
    return TRUE;
}

/*
 =============================================================================

 mouse callbacks
 
 =============================================================================
*/

/*
 * DarwinChangePointerControl
 *  Set mouse acceleration and thresholding
 *  FIXME: We currently ignore the threshold in ctrl->threshold.
 */
static void DarwinChangePointerControl(
    DeviceIntPtr    device,
    PtrCtrl         *ctrl )
{
    kern_return_t   kr;
    double          acceleration;

    acceleration = ctrl->num / ctrl->den;
    kr = IOHIDSetMouseAcceleration( dfb.hidParam, acceleration );
    if (kr != KERN_SUCCESS)
        ErrorF( "Could not set mouse acceleration with kernel return = 0x%x.\n", kr );
}


/*
 * Motion history between events is not required to be supported.
 * FIXME: This routine is obsolete?
 */
static int DarwinGetMotionEvents( DeviceIntPtr pDevice, xTimecoord *buff,
                           unsigned long start, unsigned long stop, ScreenPtr pScr)
{
	return 0;
}


/*
 * DarwinMouseProc
 *  Handle the initialization, etc. of a mouse
 */

static int DarwinMouseProc(
    DeviceIntPtr    pPointer,
    int             what )
{
    char map[6];
    
    switch (what) {
    
        case DEVICE_INIT:
            pPointer->public.on = FALSE;

            map[1] = 1;
            map[2] = 2;
            map[3] = 3;
            map[4] = 4;
            map[5] = 5;
            InitPointerDeviceStruct( (DevicePtr)pPointer,
                        map,
                        5,   // numbuttons (4 & 5 are scroll wheel)
                        miPointerGetMotionEvents,
                        DarwinChangePointerControl,
                        0 );
            break;

        case DEVICE_ON:
            pPointer->public.on = TRUE;
            AddEnabledDevice( darwinEventFD ); 
            return Success;

        case DEVICE_CLOSE:
        case DEVICE_OFF:
            pPointer->public.on = FALSE;
            RemoveEnabledDevice( darwinEventFD ); 
            return Success;
    }

    return Success;
}

/*
 * DarwinKeybdProc
 *  Callback from X
 */
static int DarwinKeybdProc( DeviceIntPtr pDev, int onoff )
{
    switch ( onoff ) {
        case DEVICE_INIT:
            DarwinKeyboardInit( pDev );
            break;
        case DEVICE_ON:
            pDev->public.on = TRUE;
            AddEnabledDevice( darwinEventFD );
            break;
        case DEVICE_OFF:
            pDev->public.on = FALSE;
            RemoveEnabledDevice( darwinEventFD );
            break;
        case DEVICE_CLOSE:
            break;
    }

    return Success;
}

/*
 * DarwinSimulateMouseClick
 *  Send a mouse click to X when multiple mouse buttons are simulated
 *  with modifier-clicks, such as command-click for button 2. The dix
 *  layer is told that the previously pressed modifier key(s) are
 *  released, the simulated click event is sent, and the modifier keys
 *  are reverted to their actual (pressed) state. This is usually
 *  closest to what the user wants. Ie. the user typically wants to
 *  simulate a button 2 press instead of Command-button 2.
 */
void DarwinSimulateMouseClick(
    xEvent xe,          // event template with time and
                        // mouse position filled in
    int whichButton,    // mouse button to be pressed
    int whichEvent,     // ButtonPress or ButtonRelease
    int keycodesUsed[], // list of keycodes of the modifiers used
                        // to create the fake click
    int numKeycodes )   // number of keycodes in list
{  
    int i;

    // first fool X into forgetting about the keys
    for (i = 0; i < numKeycodes; i++) {
    xe.u.u.type = KeyRelease;
    xe.u.u.detail = keycodesUsed[i] + MIN_KEYCODE;
    (darwinKeyboard->public.processInputProc)
        ( &xe, darwinKeyboard, 1 );
    }

    // push the mouse button
    xe.u.u.type = whichEvent;
    xe.u.u.detail = whichButton;			// de.key = button n
    (darwinPointer->public.processInputProc)
    ( &xe, darwinPointer, 1 );

    // reset the keys
    for (i = 0; i < numKeycodes; i++) {
    xe.u.u.type = KeyPress;
    xe.u.u.detail = keycodesUsed[i] + MIN_KEYCODE;
    (darwinKeyboard->public.processInputProc)
        ( &xe, darwinKeyboard, 1 );
    }
}

/*
===========================================================================

 Functions needed to link against device independent X

===========================================================================
*/

/*
 * ProcessInputEvents
 *  Read events from the event queue
 */
void ProcessInputEvents(void)
{
    xEvent	xe;
    NXEvent	ev;
    int	r;
    struct timeval tv;
    struct timezone tz;
    static int startsec = 0;
    static Bool gotread = false;
    static int old_state = 0;

#if defined(DARWIN_WITH_QUARTZ)  && defined(QUARTZ_SAFETY_DELAY)
    // Quartz safety quit. Bail if we don't get any events from the event pipe.
    // If the event writer fails to find us, we will have captured the screen 
    // but not be seeing any events and be unkillable from the console.
    if (quartz  &&  ! gotread) {
        gettimeofday(&tv, &tz);
        if (startsec == 0) startsec = tv.tv_sec;
        if (startsec + QUARTZ_SAFETY_DELAY< tv.tv_sec) {
            QuartzGiveUp();
            FatalError("%d second safety quit", QUARTZ_SAFETY_DELAY);
        }
    }
#endif

    // try to read from our pipe
    // FIXME: safely handle SIGPIPE in quartz mode
    r = read( darwinEventFD, &ev, sizeof(ev));
    if ((r == -1) && (errno != EAGAIN)) {
        ErrorF("read(darwinEventFD) failed, errno=%d: %s\n", errno, strerror(errno));
        return;
    } else if ((r == -1) && (errno == EAGAIN)) {
        return;
    } else if ( r != sizeof( ev ) ) {
        ErrorF( "Only read %i bytes from darwinPipe!", r );
        return;
    }

    gotread = true;
    gettimeofday(&tv, &tz);

    // translate it to an X event and post it
    memset(&xe, 0, sizeof(xe));

    xe.u.keyButtonPointer.rootX = ev.location.x;
    xe.u.keyButtonPointer.rootY = ev.location.y;
    //xe.u.keyButtonPointer.time = ev.time;
    xe.u.keyButtonPointer.time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    /* A newer kernel generates multi-button events by NX_SYSDEFINED.
       See iokit/Families/IOHIDSystem/IOHIDSystem.cpp version 1.1.1.7,
       2000/08/10 00:23:37 or later. */

    switch( ev.type ) {
        case NX_MOUSEMOVED:
            xe.u.u.type = MotionNotify;
            (darwinPointer->public.processInputProc)( &xe, darwinPointer, 1 );
            break;
 
        case NX_LMOUSEDOWN:
            // Mimic multi-button mouse with Command and Option
            if (fake3Buttons && ev.flags & (NX_COMMANDMASK | NX_ALTERNATEMASK)) {
                int button;
                int keycode;
                if (ev.flags & NX_COMMANDMASK) {
                    button = 2;
                    keycode = darwinKeyCommandL;
                } else {
                    button = 3;
                    keycode = darwinKeyOptionL;
                }
                DarwinSimulateMouseClick(xe, button, ButtonPress, &keycode, 1);
            } else {
                xe.u.u.detail = 1;			//de.key = button 1;
                xe.u.u.type = ButtonPress;
                (darwinPointer->public.processInputProc)
                        ( &xe, darwinPointer, 1 );
            }
            break;

        case NX_LMOUSEUP:
            // Mimic multi-button mouse with Command and Option
            if (fake3Buttons && ev.flags & (NX_COMMANDMASK | NX_ALTERNATEMASK)) {
                int button;
                int keycode;
                if (ev.flags & NX_COMMANDMASK) {
                    button = 2;
                    keycode = darwinKeyCommandL;
                } else {
                    button = 3;
                    keycode = darwinKeyOptionL;
                }
                DarwinSimulateMouseClick(xe, button, ButtonRelease, &keycode, 1);
            } else {
                xe.u.u.detail = 1;			//de.key = button 1;
                xe.u.u.type = ButtonRelease;
                (darwinPointer->public.processInputProc)
                        ( &xe, darwinPointer, 1 );
            }
            break;

// Button 2 isn't handled correctly by older kernels anyway. Just let
// NX_SYSDEFINED events handle these.
        case NX_RMOUSEDOWN:
#if 0
            xe.u.u.detail = 2; //de.key;
            xe.u.u.type = ButtonPress;
            (darwinPointer->public.processInputProc)( &xe, darwinPointer, 1 );
#endif
            break;

        case NX_RMOUSEUP:
#if 0
            xe.u.u.detail = 2; //de.key;
            xe.u.u.type = ButtonRelease;
            (darwinPointer->public.processInputProc)( &xe, darwinPointer, 1 );
#endif
            break;

        case NX_KEYDOWN:
            xe.u.u.type = KeyPress;
            xe.u.u.detail = ev.data.key.keyCode + MIN_KEYCODE;
            (darwinKeyboard->public.processInputProc)( &xe, darwinKeyboard, 1 );
            break;

        case NX_KEYUP:
            xe.u.u.type = KeyRelease;
            xe.u.u.detail = ev.data.key.keyCode + MIN_KEYCODE;
            (darwinKeyboard->public.processInputProc)(&xe, darwinKeyboard, 1);
            break;

        case NX_FLAGSCHANGED:
        {
	    // Assumes only one flag has changed. In quartz mode, 
	    // this restriction must be enforced by the quartz event feeder.
            int new_on_flags = ~old_state & ev.flags;
            int new_off_flags = old_state & ~ev.flags;
            old_state = ev.flags;
            xe.u.u.detail = ev.data.key.keyCode + MIN_KEYCODE;

            // alphalock is toggled rather than held on,
            // so we have to handle it differently
            if (new_on_flags & NX_ALPHASHIFTMASK ||
                new_off_flags & NX_ALPHASHIFTMASK) {
                xe.u.u.type = KeyPress;
                (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);
                xe.u.u.type = KeyRelease;
                (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);
                break;
            }

            if (new_on_flags) {
                xe.u.u.type = KeyPress;
            } else if (new_off_flags) {
                xe.u.u.type = KeyRelease;
            } else {
                break;
            }
            (darwinKeyboard->public.processInputProc)(&xe, darwinKeyboard, 1);
            break;
        }

        case NX_SYSDEFINED:
            if (ev.data.compound.subType == 7) {
                long hwDelta = ev.data.compound.misc.L[0];
                long hwButtons = ev.data.compound.misc.L[1];
                int i;

                for (i = 1; i < 4; i++) {
                    if (hwDelta & (1 << i)) {
                        xe.u.u.detail = i + 1;
                        if (hwButtons & (1 << i)) {
                            xe.u.u.type = ButtonPress;
                        } else {
                            xe.u.u.type = ButtonRelease;
                        }
                        (darwinPointer->public.processInputProc)
                            ( &xe, darwinPointer, 1 );
                    }
                }
            }
            break;

        case NX_SCROLLWHEELMOVED:
        {
            short count = ev.data.scrollWheel.deltaAxis1;

            if (count > 0) {
                xe.u.u.detail = SCROLLWHEELUPFAKE;
            } else {
                xe.u.u.detail = SCROLLWHEELDOWNFAKE;
                count = -count;
            }

            for (; count; --count) {
                xe.u.u.type = ButtonPress;
                (darwinPointer->public.processInputProc)
                    ( &xe, darwinPointer, 1 );
                xe.u.u.type = ButtonRelease;
                (darwinPointer->public.processInputProc)
                    ( &xe, darwinPointer, 1 );
            }
            break;
        }

	// Special events for Quartz support
        case NX_APPDEFINED:
          if (quartz) {
            switch (ev.data.compound.subType) {
              case kXServerClearModifiers:
// FIXME: We don't have ModifierKeycode(), but we probably don't need it.
// There may be better ways to do this with the XKB extension, or we may
// be able to do without this event by being smarter elsewhere.
#if 0
                xe.u.u.type = KeyRelease;
                if (old_state & NX_ALPHASHIFTMASK) {
                    xe.u.u.detail = 
                        ModifierKeycode(NX_MODIFIERKEY_ALPHALOCK, 0)
                        + MIN_KEYCODE;
                    (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);		
                }
                if (old_state & NX_COMMANDMASK) {
                    xe.u.u.detail = 
                        ModifierKeycode(NX_MODIFIERKEY_COMMAND, 0)
                        + MIN_KEYCODE;
                    (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);		
                }
                if (old_state & NX_CONTROLMASK) {
                    xe.u.u.detail = 
                        ModifierKeycode(NX_MODIFIERKEY_CONTROL, 0)
                        + MIN_KEYCODE;
                    (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);		
                }
                if (old_state & NX_ALTERNATEMASK) {
                    xe.u.u.detail = 
                        ModifierKeycode(NX_MODIFIERKEY_ALTERNATE, 0)
                        + MIN_KEYCODE;
                    (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);		
                }
                if (old_state & NX_SHIFTMASK) {
                    xe.u.u.detail = 
                        ModifierKeycode(NX_MODIFIERKEY_SHIFT, 0)
                        + MIN_KEYCODE;
                    (darwinKeyboard->public.processInputProc)
                        (&xe, darwinKeyboard, 1);		
                }
#endif
                old_state = 0;
	        break;

            case kXServerShow:
                QuartzShow();
                break;
	      
            case kXServerHide:
                QuartzHide();
                break;
	      
            case kXServerQuit:
                // FIXME: is there a better way to quit?
                FatalError("Terminated by Xmaster.\n");
            } // switch (ev.data.compound.subType)
          } // if (quartz)
          break;

        default:
            ErrorF("unknown event caught: %d\n", ev.type);
            ErrorF("\tev.type = %d\n", ev.type);
            ErrorF("\tev.location.x,y = %d,%d\n", ev.location.x, ev.location.y);
            ErrorF("\tev.time = %ld\n", ev.time);
            ErrorF("\tev.flags = 0x%x\n", ev.flags);
            ErrorF("\tev.window = %d\n", ev.window);
            ErrorF("\tev.data.key.origCharSet = %d\n", ev.data.key.origCharSet);
            ErrorF("\tev.data.key.charSet = %d\n", ev.data.key.charSet);
            ErrorF("\tev.data.key.charCode = %d\n", ev.data.key.charCode);
            ErrorF("\tev.data.key.keyCode = %d\n", ev.data.key.keyCode);
            ErrorF("\tev.data.key.origCharCode = %d\n", ev.data.key.origCharCode);
            break;
    }

    // why isn't this handled automatically by X???
    //miPointerAbsoluteCursor( ev.location.x, ev.location.y, ev.time );
    miPointerAbsoluteCursor( ev.location.x, ev.location.y,
                             tv.tv_sec * 1000 + tv.tv_usec / 1000 );
    miPointerUpdate();

}

/*
 * InitInput
 *  Register the keyboard and mouse devices
 */
void InitInput( int  argc, char **argv )
{
    if (serverGeneration == 1) {
        darwinPointer = AddInputDevice(DarwinMouseProc, TRUE);
        RegisterPointerDevice( darwinPointer );

        darwinKeyboard = AddInputDevice(DarwinKeybdProc, TRUE);
        RegisterKeyboardDevice( darwinKeyboard );
    }
}

/*
 * InitOutput
 *	Initialize screenInfo for all actually accessible framebuffers.
 */
void InitOutput( ScreenInfo *pScreenInfo, int argc, char **argv )
{
    int i;

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    // list how we want common pixmap formats to be padded
    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i = 0; i < NUMFORMATS; i++)
        pScreenInfo->formats[i] = formats[i];

    AddScreen( DarwinAddScreen, argc, argv );
}

/*
 * OsVendorFataError
 */
void OsVendorFatalError( void )
{
    ErrorF( "   OsVendorFatalError\n" );
}

/*
 * OsVendorInit
 *  One-time initialization of Darwin support.
 *  Initialize display and event handling.
 */
void OsVendorInit(void)
{
    DarwinPrintBanner();

    if (quartz) {
        QuartzOsVendorInit();
    } else {
        XFIOKitOsVendorInit();
    }
}

/*
 * ddxProcessArgument --
 *	Process device-dependent command line args. Returns 0 if argument is
 *      not device dependent, otherwise Count of number of elements of argv
 *      that are part of a device dependent commandline option.
 */
int ddxProcessArgument( int argc, char *argv[], int i )
{
#if 0
    if ( !strcmp( argv[i], "-screen" ) ) {
    	if ( i == argc-1 ) {
            FatalError( "-screen must be followed by a number" );
        }
    	darwinScreenNumber = atoi( argv[i+1] );
        ErrorF( "Attempting to use screen number %i\n", darwinScreenNumber );
        return 2;
    }
#endif

    if ( !strcmp( argv[i], "-fakebuttons" ) ) {
    	fake3Buttons = TRUE;
        ErrorF( "Faking a three button mouse\n" );
        return 1;
    }

    if ( !strcmp( argv[i], "-nofakebuttons" ) ) {
    	fake3Buttons = FALSE;
        ErrorF( "Not faking a three button mouse\n" );
        return 1;
    }

#ifdef DARWIN_WITH_QUARTZ
    if ( !strcmp( argv[i], "-quartz" ) ) {
        quartz = TRUE;
        ErrorF( "Running in parallel with Mac OS X Quartz window server.\n" );
#ifdef QUARTZ_SAFETY_DELAY
        ErrorF( "Quitting in %d seconds if no controller application is found.\n",
                QUARTZ_SAFETY_DELAY );
#endif
        return 1;
    }
#endif

    if ( !strcmp( argv[i], "-size" ) ) {
    	if ( i >= argc-2 ) {
            FatalError( "-size must be followed by two numbers" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -size option on old PowerBook G3\n");
#else
    	darwinDesiredWidth = atoi( argv[i+1] );
        darwinDesiredHeight = atoi( argv[i+2] );
        ErrorF( "Attempting to use width x height = %i x %i\n",
                darwinDesiredWidth, darwinDesiredHeight );
#endif
        return 3;
    }

    if ( !strcmp( argv[i], "-depth" ) ) {
        int     bitDepth;
    	if ( i == argc-1 ) {
            FatalError( "-depth must be followed by a number" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -depth option on old PowerBook G3\n");
#else
    	bitDepth = atoi( argv[i+1] );
        if (bitDepth == 8)
            darwinDesiredDepth = 0;
        else if (bitDepth == 15)
            darwinDesiredDepth = 1;
        else if (bitDepth == 24)
            darwinDesiredDepth = 2;
        else
            FatalError( "Unsupported pixel depth. Use 8, 15, or 24 bits" );
        ErrorF( "Attempting to use pixel depth of %i\n", bitDepth );
#endif
        return 2;
    }

    if ( !strcmp( argv[i], "-refresh" ) ) {
    	if ( i == argc-1 ) {
            FatalError( "-refresh must be followed by a number" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -refresh option on old PowerBook G3\n");
#else
    	darwinDesiredRefresh = atoi( argv[i+1] );
        ErrorF( "Attempting to use refresh rate of %i\n", darwinDesiredRefresh );
#endif
        return 2;
    }

    if (!strcmp( argv[i], "-showconfig" ) || !strcmp( argv[i], "-version" )) {
        DarwinPrintBanner();
        exit(0);
    }

    return 0;
}

/*
 * ddxUseMsg --
 *	Print out correct use of device dependent commandline options.
 *      Maybe the user now knows what really to do ...
 */
void ddxUseMsg( void )
{
    ErrorF("\n");
    ErrorF("\n");
    ErrorF("Device Dependent Usage:\n");
    ErrorF("\n");
#if 0
    ErrorF("-screen <0,1,...> : use this mac screen num.\n" );
#endif
    ErrorF("-fakebuttons : fake a three button mouse with Command and Option keys.\n");
    ErrorF("-nofakebuttons : don't fake a three button mouse.\n");
    ErrorF("-version : show the server version\n");
#ifdef DARWIN_WITH_QUARTZ
    ErrorF("-quartz : run in parallel with Mac OS X Quartz window server.\n");
    ErrorF("\n");
    ErrorF("IOKit specific options (ignored with -quartz):\n");
#endif
    ErrorF("-size <height> <width> : use a screen resolution of <height> x <width>.\n");
    ErrorF("-depth <8,15,24> : use this bit depth.\n");
    ErrorF("-refresh <rate> : use a monitor refresh rate of <rate> Hz.\n");
    ErrorF("\n");
}

/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by dix before normal server death.
 */
void ddxGiveUp( void )
{
    ErrorF( "   ddxGiveUp\n" ); 

    if (quartz) {
        QuartzGiveUp();
    } else {
        XFIOKitGiveUp();
    }
}

/*
 * AbortDDX --
 *      DDX - specific abort routine.  Called by AbortServer(). The attempt is
 *      made to restore all original setting of the displays. Also all devices
 *      are closed.
 */
void AbortDDX( void )
{
    ErrorF( "   AbortDDX\n" ); 
    /*
     * This is needed for a abnormal server exit, since the normal exit stuff
     * MUST also be performed (i.e. the vt must be left in a defined state)
     */
    ddxGiveUp();
}

Bool DPMSSupported(void)
{	return 0;
}

void DPMSSet(void)
{	return;
}
