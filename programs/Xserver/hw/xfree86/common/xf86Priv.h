/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Priv.h,v 3.45 1999/05/23 14:38:03 dawes Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains declarations for private XFree86 functions and variables,
 * and definitions of private macros.
 *
 * "private" means not available to video drivers.
 */

#ifndef _XF86PRIV_H
#define _XF86PRIV_H

#include "xf86Privstr.h"

/*
 * Parameters set ONLY from the command line options
 * The global state of these things is held in xf86InfoRec (when appropriate).
 */
extern const char *xf86ConfigFile;
extern Bool xf86AllowMouseOpenFail;
#ifdef XF86VIDMODE
extern Bool xf86VidModeDisabled;
extern Bool xf86VidModeAllowNonLocal; 
#endif 
#ifdef XF86MISC
extern Bool xf86MiscModInDevDisabled;
extern Bool xf86MiscModInDevAllowNonLocal;
#endif 
extern Bool xf86fpFlag;
extern Bool xf86coFlag;
extern Bool xf86sFlag;
extern Bool xf86bsEnableFlag;
extern Bool xf86bsDisableFlag;
extern char *xf86LayoutName;
extern char *xf86ScreenName;
extern char *xf86PointerName;
extern char *xf86KeyboardName;
#ifdef KEEPBPP
extern int xf86Bpp;
#endif
extern int xf86FbBpp;
extern int xf86Depth;
extern Pix24Flags xf86Pix24;
extern rgb xf86Weight;
extern Bool xf86FlipPixels;
extern Bool xf86BestRefresh;
extern Gamma xf86Gamma;
extern char *xf86ServerName;
extern Bool xf86ShowUnresolved;

/* Other parameters */

extern xf86InfoRec xf86Info;
extern const char *xf86ModulePath;
extern MessageType xf86ModPathFrom;
extern const char *xf86LogFile;
extern MessageType xf86LogFileFrom;
extern serverLayoutRec xf86ConfigLayout;
extern Pix24Flags xf86ConfigPix24;

extern unsigned short xf86MouseCflags[];
extern Bool xf86SupportedMouseTypes[];
extern int xf86NumMouseTypes;

#ifdef XFree86LOADER
extern DriverPtr *xf86DriverList;
#else
extern DriverPtr xf86DriverList[];
#endif
extern int xf86NumDrivers;
extern Bool xf86Exiting;
extern Bool xf86Resetting;
extern Bool xf86Initialising;
extern Bool xf86ProbeFailed;
extern int xf86NumScreens;
extern pciVideoPtr *xf86PciVideoInfo;
extern const char *xf86VisualNames[];
extern int xf86Verbose;                 /* verbosity level */
extern int xf86LogVerbose;		/* log file verbosity level */
extern Bool xf86ProbeOnly;

#ifndef DEFAULT_VERBOSE
#define DEFAULT_VERBOSE		1
#endif
#ifndef DEFAULT_LOG_VERBOSE
#define DEFAULT_LOG_VERBOSE	3
#endif
#ifndef DEFAULT_DPI
#define DEFAULT_DPI		75
#endif

#define DEFAULT_UNRESOLVED	TRUE
#define DEFAULT_BEST_REFRESH	FALSE

/* Function Prototypes */
#ifndef _NO_XF86_PROTOTYPES

/* xf86Beta.c */
extern void xf86CheckBeta(int extraDays, char *key);

/* xf86Bus.c */

void xf86BusProbe(void);
void xf86ChangeBusIndex(int oldIndex, int newIndex);
void xf86AccessEnter(void);
void xf86AccessLeave(void);
void xf86AccessSetup(void);
void xf86FindPrimaryDevice(void);
/* new RAC */
void xf86ResourceBrokerInit(void);

/* xf86Config.c */

Bool xf86PathIsAbsolute(const char *path);
Bool xf86PathIsSafe(const char *path);

/* xf86DefaultModes */

extern DisplayModeRec xf86DefaultModes [];

/* xf86Dl.c */

void* xf86LoadModule(const char *file, const char *path);

/* xf86Events.c */

void xf86PostKbdEvent(unsigned key);
void xf86PostMseEvent(DeviceIntPtr device, int buttons, int dx, int dy);
#ifndef NEW_INPUT
void xf86Block(pointer blockData, OSTimePtr pTimeout, pointer pReadmask);
#endif
void xf86Wakeup(pointer blockData, int err, pointer pReadmask);
void xf86SigHandler(int signo);

/* xf86Helper.c */
void xf86LogInit(void);
void xf86CloseLog(void);

/* xf86Init.c */
Bool xf86LoadModules(char **list, pointer *optlist);

/* xf86Io.c */

void xf86KbdBell(int percent, DeviceIntPtr pKeyboard, pointer ctrl,
		 int unused);
void xf86KbdLeds(void);
void xf86KbdCtrl(DevicePtr pKeyboard, KeybdCtrl *ctrl); 
void xf86InitKBD(Bool init);  
int xf86KbdProc(DeviceIntPtr pKeyboard, int what);
#ifndef NEW_INPUT
void xf86MseCtrl(DevicePtr pPointer, PtrCtrl *ctrl);
int xf86MseProc(DeviceIntPtr pPointer, int what);
int xf86MseProcAux(DeviceIntPtr pPointer, int what, MouseDevPtr mouse,
		   int *fd, PtrCtrlProcPtr ctrl);
void xf86MseEvents(MouseDevPtr mouse);

/* xf86Mouse.c */

Bool xf86MouseSupported(int mousetype);
void xf86SetupMouse(MouseDevPtr mouse);  
void xf86MouseProtocol(DeviceIntPtr device, unsigned char *rBuf,  int nBytes);  
#ifdef XINPUT
void xf86MouseCtrl(DeviceIntPtr device, PtrCtrl *ctrl);
#endif

/* xf86PnPMouse.c */
int xf86GetPnPMouseProtocol(MouseDevPtr mouse);
#endif

/* xf86Kbd.c */ 

void xf86KbdGetMapping(KeySymsPtr pKeySyms, CARD8 *pModMap);

/* xf86Lock.c */

#ifdef USE_XF86_SERVERLOCK
void xf86UnlockServer(void);
#endif

/* xf86XKB.c */

void xf86InitXkb(void);

#endif /* _NO_XF86_PROTOTYPES */


#endif /* _XF86PRIV_H */
