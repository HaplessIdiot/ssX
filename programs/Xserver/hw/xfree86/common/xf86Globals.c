/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Globals.c,v 1.1.2.27 1998/07/18 17:53:24 dawes Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains all the XFree86 global variables.
 */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Parser.h"

/* Globals that video drivers may access */

int xf86ScreenIndex = -1;	/* Index of ScrnInfo in pScreen.devPrivates */
ScrnInfoPtr *xf86Screens = NULL;	/* List of ScrnInfos */
int xf86PixmapIndex = 0;



/* Globals that video drivers may not access */

xf86InfoRec xf86Info = {
	NULL,		/* pKeyboard */
	NULL,		/* kbdProc */
	NULL,		/* kbdEvents */
	-1,		/* consoleFd */
	-1,		/* kbdFd */
	-1,		/* vtno */
	-1,		/* kbdType */
	-1,		/* kbdRate */
	-1, 		/* kbdDelay */
	-1,		/* bell_pitch */
	-1,		/* bell_duration */
	TRUE,		/* autoRepeat */
	0,		/* leds */
	0,		/* xleds */
	NULL,		/* vtinit */
	NULL,		/* specialKeyMap */
	0,		/* scanPrefix */
	FALSE,		/* capsLock */
	FALSE,		/* numLock */
	FALSE,		/* scrollLock */
	FALSE,		/* modeSwitchLock */
	FALSE,		/* serverNumLock */
	FALSE,		/* composeLock */
	FALSE,		/* vtSysreq */
#if defined(SVR4) && defined(i386)
	FALSE,		/* panix106 */
#endif
	NULL,		/* pMouse */
#ifdef XINPUT
	NULL,		/* mouseLocal */
#endif
	NULL,		/* mouseDev */
	-1,		/* lastEventTime */
	FALSE,		/* vtRequestsPending */
	FALSE,		/* inputPending */
	FALSE,		/* dontZap */
	FALSE,		/* dontZoom */
	FALSE,		/* notrapSignals */
	FALSE,		/* caughtSignal */
	FALSE,		/* sharedMonitor */
	NULL,		/* currentScreen */
#ifdef CSRG_BASED
	-1,		/* screenFd */
	-1,		/* consType */
#endif
#ifdef AMOEBA
	NULL,		/* screenPtr */
#endif
#ifdef XKB
	NULL,		/* xkbkeymap */
	NULL,		/* xkbkeycodes */
	NULL,		/* xkbtypes */
	NULL,		/* xkbcompat */
	NULL,		/* xkbsymbols */
	NULL,		/* xkbgeometry */
	FALSE,		/* xkbcomponents_specified */
	NULL,		/* xkbrules */
	NULL,		/* xkbmodel */
	NULL,		/* xkblayout */
	NULL,		/* xkbvariant */
	NULL,		/* xkboptions */
#endif
	FALSE,		/* allowMouseOpenFail */
	FALSE,		/* vidModeEnabled */
	FALSE,		/* vidModeAllowNonLocal */
	FALSE,		/* miscModInDevEnabled */
	FALSE,		/* miscModInDevAllowNonLocal */
	PCIProbe1	/* pciFlags */
};
char *xf86ModulePath = NULL;
screenLayoutPtr xf86ConfigLayout = NULL;
XF86ConfigPtr xf86configptr = NULL;
Bool xf86Exiting = FALSE;
Bool xf86Resetting = FALSE;
Bool xf86ProbeFailed = FALSE;
#ifdef XFree86LOADER
DriverPtr *xf86DriverList = NULL;
int xf86NumDrivers = 0;
#endif
PciProbeType xf86PCIFlags = PCIProbe1;
int xf86NumScreens = 0;

const char *xf86VisualNames[] = {
	"StaticGray",
	"GrayScale",
	"StaticColor",
	"PseudoColor",
	"TrueColor",
	"DirectColor"
};

/* Parameters set only from the command line */
char *xf86ServerName = "no-name";
Bool xf86fpFlag = FALSE;
Bool xf86coFlag = FALSE;
Bool xf86sFlag = FALSE;
char *xf86LayoutName = NULL;
char *xf86ScreenName = NULL;
Bool xf86ProbeOnly = FALSE;
int xf86Verbose = DEFAULT_VERBOSE;
int xf86Bpp = -1;
int xf86FbBpp = -1;
int xf86Depth = -1;
rgb xf86Weight = {0, 0, 0};
Bool xf86FlipPixels = FALSE;
Gamma xf86Gamma = {0.0, 0.0, 0.0};
Bool xf86ShowUnresolved = DEFAULT_UNRESOLVED;
Bool xf86BestRefresh = DEFAULT_BEST_REFRESH;

/* Parameters set either from the command line or the config file */
Bool xf86AllowMouseOpenFail = FALSE;
#ifdef XF86VIDMODE
Bool xf86VidModeEnabled = TRUE;
Bool xf86VidModeAllowNonLocal = FALSE;
#endif
#ifdef XF86MISC
Bool xf86MiscModInDevEnabled = TRUE;
Bool xf86MiscModInDevAllowNonLocal = FALSE;
#endif

#ifdef DLOPEN_HACK
/*
 * This stuff is a hack to allow dlopen() modules to work.  It is intended
 * only to be used when using dlopen() modules for debugging purposes.
 */
#endif
