/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.c,v 3.169 1999/04/24 07:36:17 dawes Exp $ */


/*
 * Copyright 1991-1997 by The XFree86 Project, Inc.
 * Copyright 1997 by Metro Link, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 *  <Put copyright message here>
 *
 * Author: Dirk Hohndel <hohndel@XFree86.Org>
 */

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#define NO_COMPILER_H_EXTRAS
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include "globals.h"

#ifdef XINPUT
#include "xf86Xinput.h"
extern DeviceAssocRec mouse_assoc;
#endif

#ifdef XKB
#define XKB_IN_SERVER
#include "XKBsrv.h"
#endif

static char *fontPath = NULL;
static char *logFilePath;

/* Forward declarations */
static Bool configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen,
			 int scrnum, MessageType from);
static Bool configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor);
static Bool configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device,
			 Bool active);
static Bool configInput(IDevPtr inputp, XF86ConfInputPtr conf_input);
static Bool configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display);
static Bool addDefaultModes(MonPtr monitorp);

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
static char *
xf86GetPathElem(char **pnt)
{
  char *p1;

  p1 = *pnt;
  *pnt = index(*pnt, ',');
  if (*pnt != NULL) {
    **pnt = '\0';
    *pnt += 1;
  }
  return(p1);
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */

#define DIR_FILE "/fonts.dir"
static char *
xf86ValidateFontPath(char *path)
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1, *dir_elem;
  struct stat stat_buf;
  int flag;
  int dirlen;

  tmp_path = xcalloc(1,strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = xf86GetPathElem(&next);
#ifndef __EMX__
    if (*path_elem == '/') {
      dir_elem = xnfcalloc(1, strlen(path_elem) + 1);
      if ((p1 = strchr(path_elem, ':')) != 0)
#else
    /* OS/2 must prepend X11ROOT */
    if (*path_elem == '/') {
      path_elem = (char*)__XOS2RedirRoot(path_elem);
      dir_elem = xnfcalloc(1, strlen(path_elem) + 1);
      if (p1 = strchr(path_elem+2, ':'))
#endif
	dirlen = p1 - path_elem;
      else
	dirlen = strlen(path_elem);
      strncpy(dir_elem, path_elem, dirlen);
      dir_elem[dirlen] = '\0';
      flag = stat(dir_elem, &stat_buf);
      if (flag == 0)
	if (!S_ISDIR(stat_buf.st_mode))
	  flag = -1;
      if (flag != 0) {
        ErrorF("Warning: The directory \"%s\" does not exist.\n", dir_elem);
	ErrorF("         Entry deleted from font path.\n");
	continue;
      }
      else {
	p1 = xnfalloc(strlen(dir_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, dir_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!S_ISREG(stat_buf.st_mode))
	    flag = -1;
#ifndef __EMX__
	xfree(p1);
#endif
	if (flag != 0) {
	  ErrorF("Warning: 'fonts.dir' not found (or not valid) in \"%s\".\n", 
		 dir_elem);
	  ErrorF("          Entry deleted from font path.\n");
	  ErrorF("          (Run 'mkfontdir' on \"%s\").\n", dir_elem);
	  continue;
	}
      }
      xfree(dir_elem);
    }

    /*
     * Either an OK directory, or a font server name.  So add it to
     * the path.
     */
    if (out_pnt != tmp_path)
      *out_pnt++ = ',';
    strcat(out_pnt, path_elem);
    out_pnt += strlen(path_elem);
  }
  return(tmp_path);
}


/*
 * use the datastructure that the parser provides and pick out the parts
 * that we need at this point
 */
char **
xf86ModulelistFromConfig(pointer **optlist)
{
    int count = 0;
    char **modulearray;
    pointer *optarray;
    XF86LoadPtr modp;
    
    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }
    
    if (xf86configptr->conf_modules) {
	/*
	 * Walk the list of modules in the "Module" section to determine how
	 * many we have.
	 */
	modp = xf86configptr->conf_modules->mod_load_lst;
	while (modp) {
	    count++;
	    modp = (XF86LoadPtr) modp->list.next;
	}
    }
    if (count == 0)
	return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfalloc((count + 1) * sizeof(char*));
    optarray = xnfalloc((count + 1) * sizeof(pointer));
    count = 0;
    if (xf86configptr->conf_modules) {
	modp = xf86configptr->conf_modules->mod_load_lst;
	while (modp) {
	    modulearray[count] = modp->load_name;
	    optarray[count] = modp->load_opt;
	    count++;
	    modp = (XF86LoadPtr) modp->list.next;
	}
    }
    modulearray[count] = NULL;
    optarray[count] = NULL;
    if (optlist)
	*optlist = optarray;
    return modulearray;
}


char **
xf86DriverlistFromConfig()
{
    int count = 0;
    char **modulearray;
    screenLayoutPtr slp;
    
    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }
    
    /*
     * Walk the list of driver lines in active "Device" sections to
     * determine now many implicitly loaded modules there are.
     *
     * XXX The set of inactive "Device" sections needs to be handled too,
     * when the rest of the supporting code is done.
     */
    slp = xf86ConfigLayout.screens;
    while ((slp++)->screen) {
	count++;
    }

    if (count == 0)
	return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfalloc((count + 1) * sizeof(char*));
    count = 0;
    slp = xf86ConfigLayout.screens;
    while (slp->screen) {
	modulearray[count] = slp->screen->device->driver;
	count++;
	slp++;
    }
    modulearray[count] = NULL;

    /* Remove duplicates */
    for (count = 0; modulearray[count] != NULL; count++) {
	int i;

	for (i = 0; i < count; i++)
	    if (xf86NameCmp(modulearray[i], modulearray[count]) == 0) {
		modulearray[count] = "";
		break;
	    }
    }
    return modulearray;
}


/*
 * xf86ConfigError --
 *      Print a READABLE ErrorMessage!!!  All information that is 
 *      available is printed.
 */
static void
xf86ConfigError(char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    ErrorF("\nConfig Error:\n");
    VErrorF(msg, ap);
    ErrorF("\n");
    va_end(ap);
    return;
}

static Bool
configFiles(XF86ConfFilesPtr fileconf)
{
  MessageType pathFrom = X_DEFAULT;

  /* FontPath */

  /* Try XF86Config FontPath first */
  if (!xf86fpFlag) {
    if (fileconf->file_fontpath) {
      char *f = xf86ValidateFontPath(fileconf->file_fontpath);
      pathFrom = X_CONFIG;
      if (*f)
        defaultFontPath = f;
      else {
	xf86Msg(X_WARNING, "FontPath is completely invalid.  Using "
		"compiled-in default.\n");
        fontPath = NULL;
        pathFrom = X_DEFAULT;
      }
    } else {
      xf86Msg(X_WARNING, "No FontPath specified.  Using compiled-in "
	      "default.\n");
      pathFrom = X_DEFAULT;
    }
  } else {
    /* Use fontpath specified with '-fp' */
    if (fontPath)
    {
      fontPath = NULL;
    }
    pathFrom = X_CMDLINE;
  }
  if (!fileconf->file_fontpath) {
    /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
    char *f = xnfalloc(strlen(defaultFontPath) + 1);
    f[0] = '\0';
    strcpy (f, defaultFontPath);
    defaultFontPath = xf86ValidateFontPath(f);
    xfree(f);
  }

  /* If defaultFontPath is still empty, exit here */

  if (! *defaultFontPath)
    FatalError("No valid FontPath could be found\n");

  xf86Msg(pathFrom, "FontPath set to \"%s\"\n", defaultFontPath);

  /* RgbPath */

  pathFrom = X_DEFAULT;

  if (xf86coFlag)
    pathFrom = X_CMDLINE;
  else if (fileconf->file_rgbpath) {
    rgbPath = fileconf->file_rgbpath;
    pathFrom = X_CONFIG;
  }

  xf86Msg(pathFrom, "RgbPath set to \"%s\"\n", rgbPath);

#ifdef XFree86LOADER
  /* ModulePath */

  if (xf86ModPathFrom != X_CMDLINE && fileconf->file_modulepath) {
    xf86ModulePath = fileconf->file_modulepath;
    xf86ModPathFrom = X_CONFIG;
  }

  xf86Msg(xf86ModPathFrom, "ModulePath set to \"%s\"\n", xf86ModulePath);
#endif

  /* LogFile */
  if (fileconf->file_logfile) {
    logFilePath = fileconf->file_logfile;
    xf86Msg(X_CONFIG, "LogFile set to \"%s\"\n", logFilePath);
  } else
    logFilePath = NULL;

  return TRUE;
}

typedef enum {
    FLAG_NOTRAPSIGNALS,
    FLAG_DONTZAP,
    FLAG_DONTZOOM,
    FLAG_DISABLEVIDMODE,
    FLAG_ALLOWNONLOCAL,
    FLAG_DISABLEMODINDEV,
    FLAG_MODINDEVALLOWNONLOCAL,
    FLAG_ALLOWMOUSEOPENFAIL,
    FLAG_PCIPROBE1,
    FLAG_PCIPROBE2,
    FLAG_PCIFORCECONFIG1,
    FLAG_PCIFORCECONFIG2,
    FLAG_SAVER_BLANKTIME,
    FLAG_DPMS_STANDBYTIME,
    FLAG_DPMS_SUSPENDTIME,
    FLAG_DPMS_OFFTIME,
    FLAG_PIXMAP
} FlagValues;
   
static OptionInfoRec FlagOptions[] = {
  { FLAG_NOTRAPSIGNALS,		"NoTrapSignals",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DONTZAP,		"DontZap",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DONTZOOM,		"DontZoom",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DISABLEVIDMODE,	"DisableVidModeExtension",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOWNONLOCAL,		"AllowNonLocalXvidtune",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DISABLEMODINDEV,	"DisableModInDev",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_MODINDEVALLOWNONLOCAL,	"AllowNonLocalModInDev",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOWMOUSEOPENFAIL,	"AllowMouseOpenFail",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIPROBE1,		"PciProbe1"		,	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIPROBE2,		"PciProbe2",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIFORCECONFIG1,	"PciForceConfig1",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIFORCECONFIG2,	"PciForceConfig2",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_SAVER_BLANKTIME,	"BlankTime"		,	OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_STANDBYTIME,	"StandbyTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_SUSPENDTIME,	"SuspendTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_OFFTIME,		"OffTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_PIXMAP,		"Pixmap",			OPTV_INTEGER,
	{0}, FALSE },
  { -1,				NULL,				OPTV_NONE,
	{0}, FALSE }
};

static Bool
configServerFlags(XF86ConfFlagsPtr flagsconf, XF86OptionPtr layoutopts)
{
    XF86OptionPtr optp, tmp;
    int i;
    Pix24Flags pix24 = Pix24DontCare;
    Bool value;

    if(flagsconf == NULL)
	return TRUE;
    /*
     * Merge the ServerLayout and ServerFlags options.  The former have
     * precedence over the latter.
     */
    optp = NULL;
    if (flagsconf->flg_option_lst)
	optp = OptionListDup(flagsconf->flg_option_lst);
    if (layoutopts) {
	tmp = OptionListDup(layoutopts);
	if (optp)
	    OptionListMerge(optp, tmp);
	else
	    optp = tmp;
    }

    xf86ProcessOptions(-1, optp, FlagOptions);

    xf86GetOptValBool(FlagOptions, FLAG_NOTRAPSIGNALS, &xf86Info.notrapSignals);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZAP, &xf86Info.dontZap);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZOOM, &xf86Info.dontZoom);

    /*
     * Set things up based on the config file information.  Some of these
     * settings may be overridden later when the command line options are
     * checked.
     */
#ifdef XF86VIDMODE
    if (xf86GetOptValBool(FlagOptions, FLAG_DISABLEVIDMODE, &value))
	xf86Info.vidModeEnabled = !value;
    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWNONLOCAL, &value))
	xf86Info.vidModeAllowNonLocal = value;
#endif

#ifdef XF86MISC
    if (xf86GetOptValBool(FlagOptions, FLAG_DISABLEMODINDEV, &value))
	xf86Info.miscModInDevEnabled = !value;
    if (xf86GetOptValBool(FlagOptions, FLAG_MODINDEVALLOWNONLOCAL, &value))
	xf86Info.miscModInDevAllowNonLocal = value;
#endif

    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWMOUSEOPENFAIL, &value))
	xf86Info.allowMouseOpenFail = value;

    if (xf86IsOptionSet(FlagOptions, FLAG_PCIPROBE1))
	xf86Info.pciFlags = PCIProbe1;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIPROBE2))
	xf86Info.pciFlags = PCIProbe2;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIFORCECONFIG1))
	xf86Info.pciFlags = PCIForceConfig1;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIFORCECONFIG2))
	xf86Info.pciFlags = PCIForceConfig2;

    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_SAVER_BLANKTIME, &i);
    if (i >= 0)
	ScreenSaverTime = defaultScreenSaverTime = i * MILLI_PER_MIN;

#ifdef DPMSExtension
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_STANDBYTIME, &i);
    if (i >= 0)
	DPMSStandbyTime = defaultDPMSStandbyTime = i * MILLI_PER_MIN;
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_SUSPENDTIME, &i);
    if (i >= 0)
	DPMSSuspendTime = defaultDPMSSuspendTime = i * MILLI_PER_MIN;
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_OFFTIME, &i);
    if (i >= 0)
	DPMSOffTime = defaultDPMSOffTime = i * MILLI_PER_MIN;
#endif

    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_PIXMAP, &i);
    switch (i) {
    case 24:
	pix24 = Pix24Use24;
	break;
    case 32:
	pix24 = Pix24Use32;
	break;
    case -1:
	break;
    default:
	xf86ConfigError("Pixmap option's value (%d) must be 24 or 32\n", i);
	return FALSE;
    }
    if (xf86Pix24 != Pix24DontCare) {
	xf86Info.pixmap24 = xf86Pix24;
	xf86Info.pix24From = X_CMDLINE;
    } else if (pix24 != Pix24DontCare) {
	xf86Info.pixmap24 = pix24;
	xf86Info.pix24From = X_CONFIG;
    } else {
	xf86Info.pixmap24 = Pix24DontCare;
	xf86Info.pix24From = X_DEFAULT;
    }
    return TRUE;
}

static KeymapKey
keyConv(int km)
{
  switch (km) {
  case CONF_KM_META:
    return KM_META;
  case CONF_KM_COMPOSE:
    return KM_COMPOSE;
  case CONF_KM_MODESHIFT:
    return KM_MODESHIFT;
  case CONF_KM_MODELOCK:
    return KM_MODELOCK;
  case CONF_KM_SCROLLLOCK:
    return KM_SCROLLLOCK;
  case CONF_KM_CONTROL:
    return KM_CONTROL;
  default:
    return KM_META;
  }
}

static Bool
configKeyboard(XF86ConfKeyboardPtr keybconf)
{
#ifdef XKB
  MessageType from = X_DEFAULT;
#endif

  /* Initialize defaults */
  xf86Info.serverNumLock = FALSE;
  xf86Info.xleds         = 0L;
  xf86Info.kbdDelay      = 500;
  xf86Info.kbdRate       = 30;
  xf86Info.kbdProc       = NULL;
  xf86Info.vtinit        = NULL;
  xf86Info.vtSysreq      = VT_SYSREQ_DEFAULT;
  xf86Info.specialKeyMap = xnfalloc(NUM_KEYMAP_TYPES * sizeof(KeymapKey));
  xf86Info.specialKeyMap[K_INDEX_LEFTALT] = KM_META;
  xf86Info.specialKeyMap[K_INDEX_RIGHTALT] = KM_META;
  xf86Info.specialKeyMap[K_INDEX_SCROLLLOCK] = KM_COMPOSE;
  xf86Info.specialKeyMap[K_INDEX_RIGHTCTL] = KM_CONTROL;
#if defined(SVR4) && defined(i386) && !defined(PC98)
  xf86Info.panix106      = FALSE;
#endif
  /* XXX Change this so that rules/model/layout are set by default */
#ifdef XKB
  xf86Info.xkbkeymap     = NULL;
  xf86Info.xkbtypes      = "default";
#ifndef PC98
  xf86Info.xkbcompat     = "default";
  xf86Info.xkbkeycodes   = "xfree86";
  xf86Info.xkbsymbols    = "us(pc101)";
  xf86Info.xkbgeometry   = "pc";
#else
  xf86Info.xkbcompat     = "pc98";
  xf86Info.xkbkeycodes   = "xfree98";
  xf86Info.xkbsymbols    = "nec/jp(pc98)";
  xf86Info.xkbgeometry   = "nec(pc98)";
#endif
  xf86Info.xkbcomponents_specified = FALSE;
  xf86Info.xkbrules      = "xfree86";
  xf86Info.xkbmodel      = NULL;
  xf86Info.xkblayout     = NULL;
  xf86Info.xkbvariant    = NULL;
  xf86Info.xkboptions    = NULL;
#endif

  if ( NameCompare(keybconf->keyb_protocol,"standard") == 0 ) {
     xf86Info.kbdProc    = xf86KbdProc;
#ifdef AMOEBA
     xf86Info.kbdEvents  = NULL;
#else
     xf86Info.kbdEvents  = xf86KbdEvents;
#endif
  } else if ( NameCompare(keybconf->keyb_protocol,"xqueue") == 0 ) {
#ifdef XQUEUE
    xf86Info.kbdProc = xf86XqueKbdProc;
    xf86Info.kbdEvents = xf86XqueEvents;
    xf86Info.mouseDev->xqueSema  = 0;
    xf86Msg(X_CONFIG, "Xqueue selected for keyboard input\n");
#endif
  } else {
    xf86ConfigError("\"%s\" is not a valid keyboard protocol name",
		    keybconf->keyb_protocol);
    return 0;
  }

  if ( keybconf->keyb_kbdDelay > 0 ) {
    xf86Info.kbdDelay = keybconf->keyb_kbdDelay;
  }

  if ( keybconf->keyb_kbdRate > 0 ) {
    xf86Info.kbdRate = keybconf->keyb_kbdRate;
  }

  if ( keybconf->keyb_serverNumLock ) {
    xf86Info.serverNumLock = TRUE;
  }

  if ( keybconf->keyb_xleds ) {
    xf86Info.xleds = keybconf->keyb_xleds;
  }

  if ( keybconf->keyb_specialKeyMap[CONF_K_INDEX_LEFTALT] ) {
    xf86Info.specialKeyMap[K_INDEX_LEFTALT] =
		keyConv(keybconf->keyb_specialKeyMap[CONF_K_INDEX_LEFTALT]);
  }

  if ( keybconf->keyb_specialKeyMap[CONF_K_INDEX_RIGHTALT] ) {
    xf86Info.specialKeyMap[K_INDEX_RIGHTALT] =
		keyConv(keybconf->keyb_specialKeyMap[CONF_K_INDEX_RIGHTALT]);
  }

  if ( keybconf->keyb_specialKeyMap[CONF_K_INDEX_SCROLLLOCK] ) {
    xf86Info.specialKeyMap[K_INDEX_SCROLLLOCK] =
		keyConv(keybconf->keyb_specialKeyMap[CONF_K_INDEX_SCROLLLOCK]);
  }

  if ( keybconf->keyb_specialKeyMap[CONF_K_INDEX_RIGHTCTL] ) {
    xf86Info.specialKeyMap[K_INDEX_RIGHTCTL] =
		keyConv(keybconf->keyb_specialKeyMap[CONF_K_INDEX_RIGHTCTL]);
  }

  if ( keybconf->keyb_vtinit ) {
    xf86Info.vtinit = keybconf->keyb_vtinit;
  }

  if ( keybconf->keyb_vtSysreq ) {
#ifdef USE_VT_SYSREQ
    xf86Info.vtSysreq = keybconf->keyb_vtSysreq;
    xf86Msg(X_CONFIG, "VTSysReq enabled\n");
#else
  xf86ConfigError("VTSysReq not supported on this OS");
#endif
  }

#ifdef XKB
  if ( keybconf->keyb_xkbDisable ) {
    noXkbExtension = TRUE;
    from = X_CONFIG;
    xf86Msg(X_CONFIG, "XKB: disabled\n");
  } else if (noXkbExtension)
    from = X_CMDLINE;
  if (noXkbExtension)
    xf86Msg(from, "XKB: disabled\n");

  if (!noXkbExtension && !XkbInitialMap) {
    if ( keybconf->keyb_xkbkeymap ) {
      xf86Info.xkbkeymap = keybconf->keyb_xkbkeymap;
      xf86Msg(X_CONFIG, "XKB: keymap: \"%s\" "
	        "(overrides other XKB settings)\n", xf86Info.xkbkeymap);
    } else {
      if ( keybconf->keyb_xkbcompat ) {
        xf86Info.xkbcompat = keybconf->keyb_xkbcompat;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: compat: \"%s\"\n", xf86Info.xkbcompat);
      }

      if ( keybconf->keyb_xkbtypes ) {
        xf86Info.xkbtypes = keybconf->keyb_xkbtypes;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: types: \"%s\"\n", xf86Info.xkbtypes);
      }

      if ( keybconf->keyb_xkbkeycodes ) {
        xf86Info.xkbkeycodes = keybconf->keyb_xkbkeycodes;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: keycodes: \"%s\"\n", xf86Info.xkbkeycodes);
      }

      if ( keybconf->keyb_xkbgeometry ) {
        xf86Info.xkbgeometry = keybconf->keyb_xkbgeometry;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: geometry: \"%s\"\n", xf86Info.xkbgeometry);
      }

      if ( keybconf->keyb_xkbsymbols ) {
        xf86Info.xkbsymbols = keybconf->keyb_xkbsymbols;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: symbols: \"%s\"\n", xf86Info.xkbsymbols);
      }

      if ( keybconf->keyb_xkbrules ) {
        xf86Info.xkbrules = keybconf->keyb_xkbrules;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: rules: \"%s\"\n", xf86Info.xkbrules);
      }

      if ( keybconf->keyb_xkbmodel ) {
        xf86Info.xkbmodel = keybconf->keyb_xkbmodel;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: model: \"%s\"\n", xf86Info.xkbmodel);
      }

      if ( keybconf->keyb_xkblayout ) {
        xf86Info.xkblayout = keybconf->keyb_xkblayout;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: layout: \"%s\"\n", xf86Info.xkblayout);
      }

      if ( keybconf->keyb_xkbvariant ) {
        xf86Info.xkbvariant = keybconf->keyb_xkbvariant;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: variant: \"%s\"\n", xf86Info.xkbvariant);
      }

      if ( keybconf->keyb_xkboptions ) {
        xf86Info.xkboptions = keybconf->keyb_xkboptions;
        xf86Info.xkbcomponents_specified = TRUE;
        xf86Msg(X_CONFIG, "XKB: options: \"%s\"\n", xf86Info.xkboptions);
      }
    }
  }
#endif
#if defined(SVR4) && defined(i386) && !defined(PC98)
  if ( keybconf->keyb_panix106 ) {
    xf86Info.panix106 = TRUE;
    xf86Msg(X_CONFIG, "PANIX106: enabled\n");
  }
#endif

  return TRUE;
}

static SymTabRec MouseTab[] = {
  { PROT_MS,			"microsoft" },
  { PROT_MSC,			"mousesystems" },
  { PROT_MM,			"mmseries" },
  { PROT_LOGI,			"logitech" },
  { PROT_BM,			"busmouse" },
  { PROT_LOGIMAN,		"mouseman" },
  { PROT_PS2,			"ps/2" },
  { PROT_MMHIT,			"mmhittab" },
  { PROT_GLIDEPOINT,		"glidepoint" },
  { PROT_IMSERIAL,		"intellimouse" },
  { PROT_THINKING,		"thinkingmouse" },
  { PROT_IMPS2,			"imps/2" },
  { PROT_THINKINGPS2,		"thinkingmouseps/2" },
  { PROT_MMANPLUSPS2,		"mousemanplusps/2" },
  { PROT_GLIDEPOINTPS2,		"glidepointps/2" },
  { PROT_NETPS2,		"netmouseps/2" },
  { PROT_NETSCROLLPS2,		"netscrollps/2" },
  { PROT_SYSMOUSE,		"sysmouse" },
  { PROT_WSMOUSE,		"wsmouse" },
  { PROT_SUN,			"sun" },
  { PROT_AUTO,			"auto" },
  { PROT_ACECAD,		"acecad" },
  { -1,				"" },
};

#define ISSERIALPROT(m) \
	((m) == PROT_MS || (m) == PROT_MSC || (m) == PROT_MM ||	\
	 (m) == PROT_LOGI || (m) == PROT_LOGIMAN || (m) == PROT_MMHIT || \
	 (m) == PROT_GLIDEPOINT || (m) == PROT_IMSERIAL || \
	 (m) == PROT_THINKING || (m) == PROT_AUTO || (m) == PROT_ACECAD)

/* XXXSTU The interface in xf86XInput.c also needs to change to match this.
 * This function is also used for the (*device_config)() function */

static Bool
configPointer(MouseDevPtr mouse_dev, XF86ConfPointerPtr pointerconf)
{
  int mtoken = -1;

  /* Set defaults */
  mouse_dev->baudRate        = 1200;
  mouse_dev->oldBaudRate     = -1;
  mouse_dev->sampleRate      = 0;
  mouse_dev->emulate3Buttons = FALSE;
  mouse_dev->emulate3Timeout = 50;
  mouse_dev->chordMiddle     = FALSE;
  mouse_dev->mouseFlags      = 0;
  mouse_dev->mseProc         = NULL;
  mouse_dev->mseDevice       = NULL;
  mouse_dev->mseType         = -1;
  mouse_dev->mseModel        = 0;
  mouse_dev->resolution      = 0;
  mouse_dev->buttons         = MSE_DFLTBUTTONS;
  mouse_dev->negativeZ       = 0;
  mouse_dev->positiveZ       = 0;

  if ( pointerconf->pntr_protocol ) {
#if defined(USE_OSMOUSE) || defined(OSMOUSE_ONLY)
    if ( NameCompare(pointerconf->pntr_protocol,"osmouse") == 0 ) {
      xf86Msg(X_CONFIG, "OsMouse selected for mouse input\n");
      mouse_dev->mseProc   = xf86OsMouseProc;
      mouse_dev->mseEvents = (void(*)(MouseDevPtr))xf86OsMouseEvents;
    }
#endif
#ifdef XQUEUE
    if ( NameCompare(pointerconf->pntr_protocol,"xqueue") == 0 ) {
      mouse_dev->mseProc   = xf86XqueMseProc;
      mouse_dev->mseEvents = (void(*)(MouseDevPtr))xf86XqueEvents;
      mouse_dev->xqueSema  = 0;
      xf86Msg(X_CONFIG, "Xqueue selected for mouse input\n");
    }
#endif
#ifndef OSMOUSE_ONLY
    if( !mouse_dev->mseProc ) {
      mtoken = xf86StringToToken(MouseTab, pointerconf->pntr_protocol);
#ifdef AMOEBA
      mouse_dev->mseProc    = xf86MseProc;
      mouse_dev->mseEvents  = NULL;
#else
      mouse_dev->mseProc    = xf86MseProc;
      mouse_dev->mseEvents  = xf86MseEvents;
#endif
      if (mtoken == -1)
      {
        xf86ConfigError("Pointer Protocol \"%s\" not recognized",
			pointerconf->pntr_protocol);
        return FALSE;
      }
      mouse_dev->mseType    = mtoken;
      if (!xf86MouseSupported(mouse_dev->mseType))
      {
        xf86ConfigError("Mouse type \"%s\"not supported by this OS",
			pointerconf->pntr_protocol);
        return FALSE;
      }
    }
#endif /* !OSMOUSE_ONLY */

    if( !mouse_dev->mseProc ) {
      xf86ConfigError("Mouse type \"%s\" not supported by this OS",
		      pointerconf->pntr_protocol);
      return FALSE;
     }
  } else {
    xf86ConfigError("No mouse protocol given\n");
    return FALSE;
  }

#ifndef OSMOUSE_ONLY
#ifdef MACH386
  mouse_dev->mseDevice = "/dev/mouse";
#else
  if( pointerconf->pntr_device ) {
    mouse_dev->mseDevice = pointerconf->pntr_device;
  }
#endif

  if( pointerconf->pntr_baudrate ) {
    if (mouse_dev->mseType == PROT_LOGIMAN) {
      /*
       * XXXX This should be extended to other mouse types -- most
       * support only 1200.  Should also disallow baudrate for bus mice
       */
      /* Moan if illegal baud rate!  [CHRIS-211092] */
      if ((pointerconf->pntr_baudrate != 1200)
         && (pointerconf->pntr_baudrate != 9600)) {
        xf86ConfigError("Only 1200 or 9600 Baud are supported by MouseMan");
        return FALSE;
      }
    }
    else {
      if (pointerconf->pntr_baudrate%1200 != 0
         || pointerconf->pntr_baudrate < 1200
         || pointerconf->pntr_baudrate > 9600) {
        xf86ConfigError("Baud rate must be one of 1200, 2400, 4800, or 9600");
        return FALSE;
      }
    }
    mouse_dev->baudRate = pointerconf->pntr_baudrate;
  }

  if( pointerconf->pntr_samplerate ) {
    if (mouse_dev->mseType == PROT_LOGIMAN) {
      /* XXXX Most mice don't allow this */
      /* Moan about illegal sample rate!  [CHRIS-211092] */
      xf86ConfigError("Selection of sample rate is not supported by MouseMan");
      return FALSE;
    }
    mouse_dev->sampleRate = pointerconf->pntr_samplerate;
  }

  if (pointerconf->pntr_resolution) {
    if (pointerconf->pntr_resolution <= 0) {
      xf86ConfigError("Resolution must be a positive value");
    } else {
      mouse_dev->resolution = pointerconf->pntr_resolution;
    }
  }

#endif /* !OSMOUSE_ONLY */
  if( pointerconf->pntr_emulate3Buttons ) {
    if( pointerconf->pntr_chordMiddle ) {
      xf86ConfigError("Can't use Emulate3Buttons with ChordMiddle");
      return FALSE;
    }
    mouse_dev->emulate3Buttons = pointerconf->pntr_emulate3Buttons;
  }
  if( pointerconf->pntr_emulate3Timeout ) {
    mouse_dev->emulate3Timeout = pointerconf->pntr_emulate3Timeout;
  }
#ifndef OSMOUSE_ONLY
  if( pointerconf->pntr_chordMiddle ) {
    if (mouse_dev->mseType == PROT_MS ||
        mouse_dev->mseType == PROT_LOGIMAN) {
      if (mouse_dev->emulate3Buttons) {
        xf86ConfigError("Can't use ChordMiddle with Emulate3Buttons");
        return FALSE;
      }
      mouse_dev->chordMiddle = pointerconf->pntr_chordMiddle;
    }
    else {
      xf86ConfigError("ChordMiddle is only supported for "
		      "Microsoft and Logiman");
      return 0;
    }
  }
  if( pointerconf->pntr_clearDtr ) {
#ifdef CLEARDTR_SUPPORT
    if (mouse_dev->mseType == PROT_MSC)
      mouse_dev->mouseFlags |= MF_CLEAR_DTR;
    else
      xf86ConfigError("ClearDTR only supported for MouseSystems mouse");
#else
    xf86ConfigError("ClearDTR not supported on this OS");
#endif
  }

  if( pointerconf->pntr_clearRts ) {
#ifdef CLEARDTR_SUPPORT
    if (mouse_dev->mseType == PROT_MSC)
      mouse_dev->mouseFlags |= MF_CLEAR_RTS;
    else
      xf86ConfigError("ClearRTS only supported for MouseSystems mouse");
#else
    xf86ConfigError("ClearRTS not supported on this OS");
#endif
  }

  if( pointerconf->pntr_alwaysCore ) {
#ifdef XINPUT
    xf86AlwaysCore(mouse_dev->local, TRUE);
#else
    xf86ConfigError("AlwaysCore not supported in this Server");
#endif
  }
#endif /* OSMOUSE_ONLY */

  if (pointerconf->pntr_buttons) {
    if (pointerconf->pntr_buttons <= 0
        || pointerconf->pntr_buttons > MSE_MAXBUTTONS) {
      xf86ConfigError("Number of buttons (1..12) expected");
    } else {
      mouse_dev->buttons = pointerconf->pntr_buttons;
    }
  }

  if (pointerconf->pntr_positiveZ) {
    switch (pointerconf->pntr_positiveZ) {
    case CONF_ZAXIS_MAPTOX:
      mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOX;
      break;
    case CONF_ZAXIS_MAPTOY:
      mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOY;
      break;
    default:	/* number */
      if (pointerconf->pntr_negativeZ <= 0
	  || pointerconf->pntr_negativeZ > MSE_MAXBUTTONS
	  || pointerconf->pntr_positiveZ <= 0
	  || pointerconf->pntr_positiveZ > MSE_MAXBUTTONS) {
	xf86ConfigError("Button number (1..12) expected");
      } else {
	mouse_dev->negativeZ = 1 << (pointerconf->pntr_negativeZ - 1);
	mouse_dev->positiveZ = 1 << (pointerconf->pntr_positiveZ - 1);
	if (pointerconf->pntr_negativeZ > mouse_dev->buttons)
	  mouse_dev->buttons = pointerconf->pntr_negativeZ;
	if (pointerconf->pntr_positiveZ > mouse_dev->buttons)
	  mouse_dev->buttons = pointerconf->pntr_positiveZ;
      }
      break;
    }
  }
    
  /*
   * If mseProc is set and mseType isn't, then using Xqueue or OSmouse.
   * Otherwise, a mouse device is required.
   */
  if (mouse_dev->mseType >= 0 && !mouse_dev->mseDevice) {
    xf86ConfigError("No mouse device given");
  }

  if (mouse_dev->mseType >= 0) {
    Bool formatFlag = FALSE;
    xf86Msg(X_CONFIG, "Mouse: type: %s, device: %s",
	    pointerconf->pntr_protocol, mouse_dev->mseDevice);
    if (ISSERIALPROT(mouse_dev->mseType)) {
      formatFlag = TRUE;
      xf86ErrorF(", baudrate: %d", mouse_dev->baudRate);
    }
    if (mouse_dev->sampleRate) {
      xf86ErrorF("%ssamplerate: %d", formatFlag ? ",\n\t" : ", ",
                 mouse_dev->sampleRate);
      formatFlag = !formatFlag;
    }
    if (mouse_dev->resolution) {
      xf86ErrorF("%sresolution: %d", formatFlag ? ",\n\t" : ", ",
                 mouse_dev->resolution);
      if (formatFlag)
	formatFlag = FALSE;
    }
    if (mouse_dev->emulate3Buttons)
      xf86ErrorF("%s3 button emulation (timeout: %dms)",
                 formatFlag ? ",\n\t" : ", ", mouse_dev->emulate3Timeout);
    if (mouse_dev->chordMiddle)
      xf86ErrorF("%sChorded middle button", formatFlag ? ",\n\t" : ", ");
    switch (mouse_dev->negativeZ) {
    case 0: /* none */
      break;
    case MSE_MAPTOX:
      xf86ErrorF("%szaxis mapping: X", formatFlag ? ",\n\t" : ", ");
      break;
    case MSE_MAPTOY:
      xf86ErrorF("%szaxis mapping: Y", formatFlag ? ",\n\t" : ", ");
      break;
    default: /* buttons */
      xf86ErrorF("%szaxis mapping: (-)%d (+)%d", formatFlag ? ",\n\t" : ", ",
		 pointerconf->pntr_negativeZ, pointerconf->pntr_positiveZ);
      break;
    }
    xf86ErrorF("\n");
  }
  return TRUE;
}

/*
 * figure out which layout is active, which screens are used in that layout,
 * which drivers and monitors are used in these screens
 */
static Bool
configLayout(serverLayoutPtr servlayoutp, XF86ConfLayoutPtr conf_layout)
{
    XF86ConfAdjacencyPtr adjp;
    XF86ConfInactivePtr idp;
    XF86ConfInputrefPtr irp;
    int count = 0;
    int scrnum;
    XF86ConfLayoutPtr l;
    MessageType from;
    screenLayoutPtr slp;
    GDevPtr gdp;
    IDevPtr indp;

    if (!servlayoutp)
	return FALSE;

    /*
     * which layout section is the active one?
     *
     * If there is a -layout command line option, use that one, otherwise
     * pick the first one.
     */
    from = X_CONFIG;
    if (xf86LayoutName != NULL) {
	if ((l = xf86FindLayout(xf86LayoutName, conf_layout)) == NULL) {
	    xf86Msg(X_ERROR, "No ServerLayout section called \"%s\"\n",
		    xf86LayoutName);
	    return FALSE;
	}
	conf_layout = l;
	from = X_CMDLINE;
    }
    xf86Msg(from, "ServerLayout \"%s\"\n", conf_layout->lay_identifier);
    adjp = conf_layout->lay_adjacency_lst;

    /*
     * we know that each screen is referenced exactly once on the left side
     * of a layout statement in the Layout section. So to allocate the right
     * size for the array we do a quick walk of the list to figure out how
     * many sections we have
     */
    while (adjp) {
        count++;
        adjp = (XF86ConfAdjacencyPtr)adjp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d screens in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    slp = xnfalloc((count + 1) * sizeof(screenLayoutRec));
    slp[count].screen = NULL;
    /*
     * now that we have storage, loop over the list again and fill in our
     * data structure; at this point we do not fill in the adjacency
     * information as it is not clear if we need it at all
     */
    adjp = conf_layout->lay_adjacency_lst;
    count = 0;
    while (adjp) {
        slp[count].screen = xnfalloc(sizeof(confScreenRec));
	if (adjp->adj_scrnum < 0)
	    scrnum = count;
	else
	    scrnum = adjp->adj_scrnum;
	if (!configScreen(slp[count].screen, adjp->adj_screen, scrnum,
			  X_CONFIG))
	    return FALSE;
        count++;
        adjp = (XF86ConfAdjacencyPtr)adjp->list.next;
    }
    /*
     * Count the number of inactive devices.
     */
    count = 0;
    idp = conf_layout->lay_inactive_lst;
    while (idp) {
        count++;
        idp = (XF86ConfInactivePtr)idp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d inactive devices in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    gdp = xnfalloc((count + 1) * sizeof(GDevRec));
    gdp[count].identifier = NULL;
    idp = conf_layout->lay_inactive_lst;
    count = 0;
    while (idp) {
	if (!configDevice(&gdp[count], idp->inactive_device, FALSE))
	    return FALSE;
        count++;
        idp = (XF86ConfInactivePtr)idp->list.next;
    }
    /*
     * Count the number of input devices.
     */
    count = 0;
    irp = conf_layout->lay_input_lst;
    while (irp) {
        count++;
        irp = (XF86ConfInputrefPtr)irp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d input devices in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    indp = xnfalloc((count + 1) * sizeof(IDevRec));
    indp[count].identifier = NULL;
    irp = conf_layout->lay_input_lst;
    count = 0;
    while (irp) {
	if (!configInput(&indp[count], irp->iref_inputdev))
	    return FALSE;
	indp[count].extraOptions = irp->iref_option_lst;
        count++;
        irp = (XF86ConfInputrefPtr)irp->list.next;
    }
    servlayoutp->id = conf_layout->lay_identifier;
    servlayoutp->screens = slp;
    servlayoutp->inactives = gdp;
    servlayoutp->inputs = indp;
    servlayoutp->options = conf_layout->lay_option_lst;
    return TRUE;
}

/*
 * No layout section, so find the first Screen section and set that up as
 * the only active screen.
 */
static Bool
configImpliedLayout(serverLayoutPtr servlayoutp, XF86ConfScreenPtr conf_screen)
{
    MessageType from;
    XF86ConfScreenPtr s;
    screenLayoutPtr slp;

    if (!servlayoutp)
	return FALSE;

    if (conf_screen == NULL) {
	xf86ConfigError("No Screen sections present\n");
	return FALSE;
    }

    /*
     * which screen section is the active one?
     *
     * If there is a -screen option, use that one, otherwise use the first
     * one.
     */

    from = X_CONFIG;
    if (xf86ScreenName != NULL) {
	if ((s = xf86FindScreen(xf86ScreenName, conf_screen)) == NULL) {
	    xf86Msg(X_ERROR, "No Screen section called \"%s\"\n",
		    xf86ScreenName);
	    return FALSE;
	}
	conf_screen = s;
	from = X_CMDLINE;
    }

    /* We have exactly one screen */

    slp = xnfalloc(2 * sizeof(screenLayoutRec));
    slp[0].screen = xnfalloc(sizeof(confScreenRec));
    slp[1].screen = NULL;
    if (!configScreen(slp[0].screen, conf_screen, 0, from))
	return FALSE;
    servlayoutp->id = "(implicit)";
    servlayoutp->screens = slp;
    servlayoutp->inactives = NULL;
    servlayoutp->options = NULL;
    return TRUE;
}

static Bool
configXvAdaptor(confXvAdaptorPtr adaptor, XF86ConfVideoAdaptorPtr conf_adaptor)
{
    int count = 0;
    XF86ConfVideoPortPtr conf_port;

    xf86Msg(X_CONFIG, "|   |-->VideoAdaptor \"%s\"\n",
	    conf_adaptor->va_identifier);
    adaptor->identifier = conf_adaptor->va_identifier;
    adaptor->options = conf_adaptor->va_option_lst;
    if (conf_adaptor->va_busid || conf_adaptor->va_driver) {
	xf86Msg(X_CONFIG, "|   | Unsupported device type, skipping entry\n");
	return FALSE;
    }

    /*
     * figure out how many videoport subsections there are and fill them in
     */
    conf_port = conf_adaptor->va_port_lst;
    while(conf_port) {
        count++;
        conf_port = (XF86ConfVideoPortPtr)conf_port->list.next;
    }
    adaptor->ports = xnfalloc((count) * sizeof(confXvPortRec));
    adaptor->numports = count;
    count = 0;
    conf_port = conf_adaptor->va_port_lst;
    while(conf_port) {
	adaptor->ports[count].identifier = conf_port->vp_identifier;
	adaptor->ports[count].options = conf_port->vp_option_lst;
        count++;
        conf_port = (XF86ConfVideoPortPtr)conf_port->list.next;
    }

    return TRUE;
}

static Bool
configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen, int scrnum,
	     MessageType from)
{
    int count = 0;
    XF86ConfDisplayPtr dispptr;
    XF86ConfAdaptorLinkPtr conf_adaptor;

    xf86Msg(from, "|-->Screen \"%s\" (%d)\n", conf_screen->scrn_identifier,
	    scrnum);
    /*
     * now we fill in the elements of the screen
     */
    screenp->id         = conf_screen->scrn_identifier;
    screenp->screennum  = scrnum;
    screenp->defaultdepth = conf_screen->scrn_defaultdepth;
    screenp->defaultbpp = conf_screen->scrn_defaultbpp;
    screenp->defaultfbbpp = conf_screen->scrn_defaultfbbpp;
    screenp->monitor    = xnfalloc(sizeof(MonRec));
    if (!configMonitor(screenp->monitor,conf_screen->scrn_monitor))
	return FALSE;
    screenp->device     = xnfalloc(sizeof(GDevRec));
    configDevice(screenp->device,conf_screen->scrn_device, TRUE);
    screenp->device->myScreenSection = screenp;
    screenp->options = conf_screen->scrn_option_lst;
    
    /*
     * figure out how many display subsections there are and fill them in
     */
    dispptr = conf_screen->scrn_display_lst;
    while(dispptr) {
        count++;
        dispptr = (XF86ConfDisplayPtr)dispptr->list.next;
    }
    screenp->displays   = xnfalloc((count) * sizeof(DispRec));
    screenp->numdisplays = count;
    count = 0;
    dispptr = conf_screen->scrn_display_lst;
    while(dispptr) {
        configDisplay(&(screenp->displays[count]),dispptr);
        count++;
        dispptr = (XF86ConfDisplayPtr)dispptr->list.next;
    }

    /*
     * figure out how many videoadaptor references there are and fill them in
     */
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while(conf_adaptor) {
        count++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr)conf_adaptor->list.next;
    }
    screenp->xvadaptors = xnfalloc((count) * sizeof(confXvAdaptorRec));
    screenp->numxvadaptors = 0;
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while(conf_adaptor) {
        if (configXvAdaptor(&(screenp->xvadaptors[screenp->numxvadaptors]),
			    conf_adaptor->al_adaptor))
    	    screenp->numxvadaptors++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr)conf_adaptor->list.next;
    }

    return TRUE;
}

static Bool
configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor)
{
    int count;
    DisplayModePtr mode,last = NULL;
    XF86ConfModeLinePtr cmodep;
    XF86ConfModesPtr modes;
    XF86ConfModesLinkPtr modeslnk = conf_monitor->mon_modes_sect_lst;
    Gamma zeros = {0.0, 0.0, 0.0};
    float badgamma = 0.0;
    
    xf86Msg(X_CONFIG, "|   |-->Monitor \"%s\"\n",
	    conf_monitor->mon_identifier);
    monitorp->id = conf_monitor->mon_identifier;
    monitorp->vendor = conf_monitor->mon_vendor;
    monitorp->model = conf_monitor->mon_modelname;
    monitorp->Modes = NULL;
    monitorp->Last = NULL;
    monitorp->gamma = zeros;
    monitorp->widthmm = conf_monitor->mon_width;
    monitorp->heightmm = conf_monitor->mon_height;
    monitorp->options = conf_monitor->mon_option_lst;

    /*
     * fill in the monitor structure
     */    
    for( count = 0 ; count < conf_monitor->mon_n_hsync; count++) {
        monitorp->hsync[count].hi = conf_monitor->mon_hsync[count].hi;
        monitorp->hsync[count].lo = conf_monitor->mon_hsync[count].lo;
    }
    monitorp->nHsync = conf_monitor->mon_n_hsync;
    for( count = 0 ; count < conf_monitor->mon_n_vrefresh; count++) {
        monitorp->vrefresh[count].hi = conf_monitor->mon_vrefresh[count].hi;
        monitorp->vrefresh[count].lo = conf_monitor->mon_vrefresh[count].lo;
    }
    monitorp->nVrefresh = conf_monitor->mon_n_vrefresh;

    /*
     * first we collect the mode lines from the UseModes directive
     */
    while(modeslnk)
    {
	modes = xf86FindModes (modeslnk->ml_modes_str, 
			       xf86configptr->conf_modes_lst);
	modeslnk->ml_modes = modes;
	    
	/* now add the modes found in the modes
	   section to the list of modes for this
	   monitor */
	conf_monitor->mon_modeline_lst = (XF86ConfModeLinePtr)
	    addListItem((GenericListPtr)conf_monitor->mon_modeline_lst,
			(GenericListPtr)modes->mon_modeline_lst);
	modeslnk = modeslnk->list.next;
    }

    /*
     * we need to hook in the mode lines now
     * here both data structures use lists, only our internal one
     * is double linked
     */
    cmodep = conf_monitor->mon_modeline_lst;
    while( cmodep ) {
        mode = xnfalloc(sizeof(DisplayModeRec));
        memset(mode,'\0',sizeof(DisplayModeRec));
	mode->type       = 0;
        mode->Clock      = cmodep->ml_clock;
        mode->HDisplay   = cmodep->ml_hdisplay;
        mode->HSyncStart = cmodep->ml_hsyncstart;
        mode->HSyncEnd   = cmodep->ml_hsyncend;
        mode->HTotal     = cmodep->ml_htotal;
        mode->VDisplay   = cmodep->ml_vdisplay;
        mode->VSyncStart = cmodep->ml_vsyncstart;
        mode->VSyncEnd   = cmodep->ml_vsyncend;
        mode->VTotal     = cmodep->ml_vtotal;
        mode->Flags      = cmodep->ml_flags;
        mode->HSkew      = cmodep->ml_hskew;
        mode->VScan      = cmodep->ml_vscan;
        mode->name       = cmodep->ml_identifier;
        if( last ) {
            mode->prev = last;
            last->next = mode;
        }
        else {
            /*
             * this is the first mode
             */
            monitorp->Modes = mode;
            mode->prev = NULL;
        }
        last = mode;
        cmodep = (XF86ConfModeLinePtr)cmodep->list.next;
    }
    if(last){
      last->next = NULL;
    }
    monitorp->Last = last;

    /* add the (VESA) default modes */
    if (! addDefaultModes(monitorp) )
	return FALSE;

    if (conf_monitor->mon_gamma_red > GAMMA_ZERO)
	monitorp->gamma.red = conf_monitor->mon_gamma_red;
    if (conf_monitor->mon_gamma_green > GAMMA_ZERO)
	monitorp->gamma.green = conf_monitor->mon_gamma_green;
    if (conf_monitor->mon_gamma_blue > GAMMA_ZERO)
	monitorp->gamma.blue = conf_monitor->mon_gamma_blue;
    
    /* Check that the gamma values are within range */
    if (monitorp->gamma.red > GAMMA_ZERO &&
	(monitorp->gamma.red < GAMMA_MIN ||
	 monitorp->gamma.red > GAMMA_MAX)) {
	badgamma = monitorp->gamma.red;
    } else if (monitorp->gamma.green > GAMMA_ZERO &&
	(monitorp->gamma.green < GAMMA_MIN ||
	 monitorp->gamma.green > GAMMA_MAX)) {
	badgamma = monitorp->gamma.green;
    } else if (monitorp->gamma.blue > GAMMA_ZERO &&
	(monitorp->gamma.blue < GAMMA_MIN ||
	 monitorp->gamma.blue > GAMMA_MAX)) {
	badgamma = monitorp->gamma.blue;
    }
    if (badgamma > GAMMA_ZERO) {
	xf86ConfigError("Gamma value %.f is out of range (%.2f - %.1f)\n",
			badgamma, GAMMA_MIN, GAMMA_MAX);
	    return FALSE;
    }

    return TRUE;
}

static int
lookupVisual(const char *visname)
{
    int i;

    if (!visname || !*visname)
	return -1;

    for (i = 0; i <= DirectColor; i++) {
	if (!NameCompare(visname, xf86VisualNames[i]))
	    break;
    }

    if (i <= DirectColor)
	return i;

    return -1;
}


static Bool
configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display)
{
    int count = 0;
    XF86ModePtr modep;
    
    displayp->frameX0           = conf_display->disp_frameX0;
    displayp->frameY0           = conf_display->disp_frameY0;
    displayp->virtualX          = conf_display->disp_virtualX;
    displayp->virtualY          = conf_display->disp_virtualY;
    displayp->depth             = conf_display->disp_depth;
    displayp->fbbpp             = conf_display->disp_bpp;
    displayp->weight.red        = conf_display->disp_weight.red;
    displayp->weight.green      = conf_display->disp_weight.green;
    displayp->weight.blue       = conf_display->disp_weight.blue;
    displayp->blackColour.red   = conf_display->disp_black.red;
    displayp->blackColour.green = conf_display->disp_black.green;
    displayp->blackColour.blue  = conf_display->disp_black.blue;
    displayp->whiteColour.red   = conf_display->disp_white.red;
    displayp->whiteColour.green = conf_display->disp_white.green;
    displayp->whiteColour.blue  = conf_display->disp_white.blue;
    displayp->options           = conf_display->disp_option_lst;
    if (conf_display->disp_visual) {
	displayp->defaultVisual = lookupVisual(conf_display->disp_visual);
	if (displayp->defaultVisual == -1) {
	    xf86ConfigError("Invalid visual name: \"%s\"",
			    conf_display->disp_visual);
	    return FALSE;
	}
    } else {
	displayp->defaultVisual = -1;
    }
	
    /*
     * now hook in the modes
     */
    modep = conf_display->disp_mode_lst;
    while(modep) {
        count++;
        modep = (XF86ModePtr)modep->list.next;
    }
    displayp->modes = xnfalloc((count+1) * sizeof(char*));
    modep = conf_display->disp_mode_lst;
    count = 0;
    while(modep) {
        displayp->modes[count] = modep->mode_name;
        count++;
        modep = (XF86ModePtr)modep->list.next;
    }
    displayp->modes[count] = NULL;
    
    return TRUE;
}

static Bool
configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device, Bool active)
{
    int i;

    if (active)
	xf86Msg(X_CONFIG, "|   |-->Device \"%s\"\n",
		conf_device->dev_identifier);
    else
	xf86Msg(X_CONFIG, "|-->Inactive Device \"%s\"\n",
		conf_device->dev_identifier);
	
    /*
     * some things (like clocks) are missing here
     */
    devicep->identifier = conf_device->dev_identifier;
    devicep->vendor = conf_device->dev_vendor;
    devicep->board = conf_device->dev_board;
    devicep->chipset = conf_device->dev_chipset;
    devicep->ramdac = conf_device->dev_ramdac;
    devicep->driver = conf_device->dev_driver;
    devicep->videoRam = conf_device->dev_videoram;
    devicep->BiosBase = conf_device->dev_bios_base;
    devicep->MemBase = conf_device->dev_mem_base;
    devicep->IOBase = conf_device->dev_io_base;
    devicep->clockchip = conf_device->dev_clockchip;
    devicep->busID = conf_device->dev_busid;
    devicep->textClockFreq = conf_device->dev_textclockfreq;
    devicep->MemClk = conf_device->dev_memclk;
    devicep->chipID = conf_device->dev_chipid;
    devicep->chipRev = conf_device->dev_chiprev;
    devicep->options = conf_device->dev_option_lst;

    for (i = 0; i < MAXDACSPEEDS; i++) {
	if (i < CONF_MAXDACSPEEDS)
            devicep->dacSpeeds[i] = conf_device->dev_dacSpeeds[i];
	else
	    devicep->dacSpeeds[i] = 0;
    }
    devicep->numclocks = conf_device->dev_clocks;
    if (devicep->numclocks > MAXCLOCKS)
	devicep->numclocks = MAXCLOCKS;
    for (i = 0; i < devicep->numclocks; i++) {
	devicep->clock[i] = conf_device->dev_clock[i];
    }
    devicep->claimed = FALSE;

    return TRUE;
}

static Bool
configInput(IDevPtr inputp, XF86ConfInputPtr conf_input)
{
    xf86Msg(X_CONFIG, "|-->Input Device \"%s\"\n",
		conf_input->inp_identifier);
    inputp->identifier = conf_input->inp_identifier;
    inputp->driver = conf_input->inp_driver;
    inputp->commonOptions = conf_input->inp_option_lst;

    return TRUE;
}
	
static Bool
modeIsPresent(char * modename,MonPtr monitorp)
{
    DisplayModePtr knownmodes = monitorp->Modes;

    /* all I can think of is a linear search... */
    while(knownmodes != NULL)
    {
	if(strcmp(modename,knownmodes->name) == 0)
	    return TRUE;
	knownmodes = knownmodes->next;
    }
    return FALSE;
}

static Bool
addDefaultModes(MonPtr monitorp)
{
    DisplayModePtr mode;
    DisplayModePtr last = monitorp->Last;
    int i = 0;

    while (xf86DefaultModes[i].name != NULL)
    {
	if ( ! modeIsPresent(xf86DefaultModes[i].name,monitorp) )
	    do
	    {
		mode = xnfalloc(sizeof(DisplayModeRec));
		memcpy(mode,&xf86DefaultModes[i],sizeof(DisplayModeRec));
		if( last ) {
		    mode->prev = last;
		    last->next = mode;
		}
		else {
		    /* this is the first mode */
		    monitorp->Modes = mode;
		    mode->prev = NULL;
		}
		last = mode;
		i++;
	    }
	    while((xf86DefaultModes[i].name != NULL) &&
		  (!strcmp(xf86DefaultModes[i].name,xf86DefaultModes[i-1].name)));
	else
	    i++;
    }
    monitorp->Last = last;

    return TRUE;
}

/*
 * load the config file and fill the global data structure
 */
Bool
xf86HandleConfigFile(void)
{
    char filename[PATH_MAX];

    if (xf86OpenConfigFile (filename)) {
        ErrorF ("Opened %s for the config file\n", filename);
    }
    else {
        ErrorF ("Unable to open config file\n");
        return FALSE;
    }
    if ((xf86configptr = xf86ReadConfigFile ()) == NULL) {
        ErrorF ("Problem when parsing config file\n");
        return FALSE;
        
    }
    xf86CloseConfigFile ();

    /* Initialise a few things. */

#if defined(XINPUT)
    xf86Info.mouseLocal = mouse_assoc.device_allocate();
    xf86Info.mouseDev = (MouseDevPtr)
			((LocalDevicePtr)xf86Info.mouseLocal)->private;
    xf86Info.mouseDev->mseProc = NULL;
#else
    xf86Info.mouseDev = (MouseDevPtr)xnfcalloc(1, sizeof(MouseDevRec));
#endif

    /* Show what the marker symbols mean */
    xf86ErrorF("Markers: " X_PROBE_STRING " probed, "
			   X_CONFIG_STRING " from config file, "
			   X_DEFAULT_STRING " default setting,\n"
	       "         " X_CMDLINE_STRING " from command line, "
			   X_NOTICE_STRING " notice, "
			   X_INFO_STRING " informational,\n"
	       "         " X_WARNING_STRING " warning, "
			   X_ERROR_STRING " error, "
			   X_UNKNOWN_STRING " unknown.\n");

    /*
     * now we convert part of the information contained in the parser
     * structures into our own structures.
     * The important part here is to figure out which Screen Sections
     * in the XF86Config file are active so that we can piece together
     * the modes that we need later down the road.
     * And while we are at it, we'll decode the rest of the stuff as well
     */

    /* First check if a layout section is present, and if it is valid. */

    if (xf86configptr->conf_layout_lst == NULL || xf86ScreenName != NULL) {
	if (xf86ScreenName == NULL) {
	    xf86Msg(X_WARNING,
		    "No Layout section.  Using the first Screen section.\n");
	}
	if (!configImpliedLayout(&xf86ConfigLayout,
				 xf86configptr->conf_screen_lst)) {
            xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
	    return FALSE;
	}
    } else {
	if (!configLayout(&xf86ConfigLayout, xf86configptr->conf_layout_lst)) {
	    xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
	    return FALSE;
	}
    }

    /* Now process everything else */

    if (!configFiles(xf86configptr->conf_files) ||
        !configServerFlags(xf86configptr->conf_flags,
			   xf86ConfigLayout.options) ||
        !configKeyboard(xf86configptr->conf_keyboard) ||
        !configPointer(xf86Info.mouseDev,xf86configptr->conf_pointer)) {
             ErrorF ("Problem when converting the config data structures\n");
             return FALSE;
    }

    /*
     * Handle some command line options that can override some of the
     * ServerFlags settings.
     */
#ifdef XF86VIDMODE
    if (xf86VidModeDisabled)
	xf86Info.vidModeEnabled = FALSE;
    if (xf86VidModeAllowNonLocal)
	xf86Info.vidModeAllowNonLocal = TRUE;
#endif

#ifdef XF86MISC
    if (xf86MiscModInDevDisabled)
	xf86Info.miscModInDevEnabled = FALSE;
    if (xf86MiscModInDevAllowNonLocal)
	xf86Info.miscModInDevAllowNonLocal = TRUE;
#endif

    if (xf86AllowMouseOpenFail)
	xf86Info.allowMouseOpenFail = TRUE;

    return TRUE;
}


