/* $XConsortium: tclxfconf.c /main/3 1996/10/23 11:44:07 kaleb $ */






/* $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/tclxfconf.c,v 3.22 1998/04/05 16:15:53 robin Exp $ */
/*
 * Copyright 1996 by Joseph V. Moss <joe@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Joseph Moss not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Joseph Moss makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * JOSEPH MOSS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL JOSEPH MOSS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


/*

  This file contains Tcl bindings to the XF86Config file reading routines

 */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"

#define INIT_OPTIONS
#include "xf86_Option.h"

#include "tcl.h"
#define XF86SETUP_NO_FUNC_RENAME
#include "xfsconf.h"

#if NeedVarargsPrototypes
#include <stdarg.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#define NO_COMPILER_H_EXTRAS
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

#define INIT_SYMTABS
#include "xf86Symtabs.h"

int TCL_XF86FindXF86Config(
#if NeedNestedPrototypes
    ClientData	clientData,
    Tcl_Interp	*interp,
    int		argc,
    char	**argv
#endif
);

int TCL_XF86ReadXF86Config(
#if NeedNestedPrototypes
    ClientData	clientData,
    Tcl_Interp	*interp,
    int		argc,
    char	**argv
#endif
);

static void init_config_vars(
#if NeedNestedPrototypes
    char *path
#endif
);

static int read_config_file(
#if NeedNestedPrototypes
    void
#endif
);

static int getsection_files(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static int getsection_server(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static int getsection_keyboard(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static int getsection_pointer(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname,
    XF86ConfPointerPtr mouse
#endif
);

static int getsection_monitor(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static int getsection_device(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static int getsection_screen(
#if NeedNestedPrototypes
    Tcl_Interp *interp,
    char *varname
#endif
);

static char *NonZeroStr(
#if NeedNestedPrototypes
    unsigned long val,
    int base
#endif
);

static char *get_path_elem(
#if NeedNestedPrototypes
     char **pnt
#endif
);

static char *validate_font_path(
#if NeedNestedPrototypes
     char *path
#endif
);

static char * token_to_string(
#if NeedNestedPrototypes
     SymTabPtr table,
     int token
#endif
);

static int string_to_token(
#if NeedNestedPrototypes
     SymTabPtr table,
     char *string
#endif
);

Tcl_Interp *errinterp;

/* Error handling functions */

#if NeedVarargsPrototypes
void
VErrorF(f, args)
    char *f;
    va_list args;
{
    char tmpbuf[1024];
    vsprintf(tmpbuf, f, args);
    Tcl_AppendResult(errinterp, tmpbuf, (char *) NULL);
}
#endif

/*VARARGS1*/
void
ErrorF(
#if NeedVarargsPrototypes
    char * f, ...)
#else
 f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    char tmpbuf[1024];
#ifdef AMOEBA
    mu_lock(&print_lock);
#endif
    sprintf( tmpbuf, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
    Tcl_AppendResult(errinterp, tmpbuf, (char *) NULL);
#ifdef AMOEBA
    mu_unlock(&print_lock);
#endif
#endif
}

/*VARARGS1*/
void
XF86SetupXF86ConfigError(
#if NeedVarargsPrototypes
    char * f, ...)
#else
f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
  va_list args;
#endif
  ErrorF("\nConfig Error:\n");
#if NeedVarargsPrototypes
  va_start(args, f);
  VErrorF(f, args);
  va_end(args);
#else
  ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
  ErrorF("\n");
}

/*VARARGS1*/
int
XF86SetupFatalError(
#if NeedVarargsPrototypes
    char *f, ...)
#else
f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
#endif
    ErrorF("\nFatal server error:\n");
#if NeedVarargsPrototypes
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
    ErrorF("\n");
    return TCL_ERROR;
}

/* Configuration variables */
char		xf86ConfigFile[PATH_MAX];
Bool		xf86fpFlag, xf86coFlag, xf86sFlag;
char		*configPath = NULL;
char		*rgbPath = NULL, *defaultFontPath = NULL;
int		xf86Verbose = 2;
XF86ConfigPtr	config_list;

ScrnInfoRec	monoInfoRec, vga2InfoRec, vga16InfoRec,
		svgaInfoRec, accelInfoRec;

/* Note that the order is important. They must be ordered numerically */
int		xf86MaxScreens, xf86ScreenNames[] = {
					SVGA, VGA2, MONO,
					VGA16, ACCEL, -1
				};
ScrnInfoPtr	xf86Screens[] = {
			&svgaInfoRec, &vga2InfoRec, &monoInfoRec,
			&vga16InfoRec, &accelInfoRec
		};

#define XF86MAXSCREENS	sizeof(xf86Screens) / sizeof(ScrnInfoPtr)

/*
  Set the various config file related variables to their defaults
*/

static void
init_config_vars(xwinhome)
  char *xwinhome;
{
	int i, pathlen;

	xf86Verbose = 2;
	xf86ConfigFile[0] = '\0';
	xf86fpFlag = xf86coFlag = xf86sFlag       = FALSE;
	xf86MaxScreens = XF86MAXSCREENS;

	pathlen = strlen(xwinhome);
	if (rgbPath == NULL)
		rgbPath = XtMalloc(pathlen + 13);
	else
		rgbPath = XtRealloc(rgbPath, pathlen + 13);
	strcpy(rgbPath, xwinhome);
	strcat(rgbPath, "/lib/X11/rgb");

	if (defaultFontPath == NULL)
		defaultFontPath = XtMalloc(pathlen*2 + 42);
	else
		defaultFontPath = XtRealloc(defaultFontPath, pathlen*2 + 42);
	sprintf(defaultFontPath,
		"%s/lib/X11/fonts/misc,%s/lib/X11/fonts/75dpi",
		xwinhome, xwinhome);

	for (i = 0; i < XF86MAXSCREENS; i++) {
		bzero(xf86Screens[i], sizeof(ScrnInfoRec));
		xf86Screens[i]->configured	= FALSE;
		xf86Screens[i]->tmpIndex	= -1;
		xf86Screens[i]->scrnIndex	= -1;
		xf86Screens[i]->BIOSbase	= 0xC0000;
	}
	monoInfoRec.depth	= 1;
	vga2InfoRec.depth	= 1;
	vga16InfoRec.depth	= 4;
	svgaInfoRec.depth	= 8;
	accelInfoRec.depth	= 8;

	vga16InfoRec.weight.red		= 5;
	svgaInfoRec.weight.red		= 5;
	accelInfoRec.weight.red		= 5;
	vga16InfoRec.weight.green	= 6;
	svgaInfoRec.weight.green	= 6;
	accelInfoRec.weight.green	= 6;
	vga16InfoRec.weight.blue	= 5;
	svgaInfoRec.weight.blue		= 5;
	accelInfoRec.weight.blue	= 5;

	monoInfoRec.bitsPerPixel	= 1;
	vga2InfoRec.bitsPerPixel	= 1;
	vga16InfoRec.bitsPerPixel	= 8;
	svgaInfoRec.bitsPerPixel	= 8;
	accelInfoRec.bitsPerPixel	= 8;

	monoInfoRec.defaultVisual	= StaticGray;
	vga2InfoRec.defaultVisual	= StaticGray;
	vga16InfoRec.defaultVisual	= PseudoColor;
	svgaInfoRec.defaultVisual	= PseudoColor;
	accelInfoRec.defaultVisual	= PseudoColor;

	monoInfoRec.name	= "MONO";
	vga2InfoRec.name	= "VGA2";
	vga16InfoRec.name	= "VGA16";
	svgaInfoRec.name	= "SVGA";
	accelInfoRec.name	= "Accel";

}

static int
read_config_file(
#if NeedNestedPrototypes
    void
#endif
)
{
	XF86ConfigPtr config;

	configPath = (char *)XtMalloc(PATH_MAX);

	if (!xf86OpenConfigFile(configPath))
	    return XF86SetupFatalError("Unable to Open Config file");

	if (xf86Verbose) {
	    ErrorF("XF86Config: %s\n", configPath);
	    ErrorF("%s stands for supplied, %s stands for probed/default values\n",
		   XCONFIG_GIVEN, XCONFIG_PROBED);
	}

	if ((config = xf86ReadConfigFile()) == NULL)
	    return XF86SetupFatalError("Error Parsing Config file");

	config_list = config;

	return TCL_OK;
}

char *KeyMapType(keyboard, key)
  XF86ConfKeyboardPtr keyboard;
  int key;
{
	switch(keyboard->keyb_specialKeyMap[key-LEFTALT]) {
		case CONF_KM_META:		return "Meta";
		case CONF_KM_COMPOSE:		return "Compose";
		case CONF_KM_MODESHIFT:		return "ModeShift";
		case CONF_KM_MODELOCK:		return "ModeLock";
		case CONF_KM_SCROLLLOCK:	return "ScrollLock";
		case CONF_KM_CONTROL:		return "Control";
	}
	return "";
}

/*
   Adds all the Cards database specific commands to the Tcl interpreter
*/

int
XF86Config_Init(interp)
    Tcl_Interp	*interp;
{
	Tcl_CreateCommand(interp, "xf86config_findfile",
		TCL_XF86FindXF86Config, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xf86config_readfile",
		TCL_XF86ReadXF86Config, (ClientData) NULL,
		(void (*)()) NULL);

	return TCL_OK;
}

/*
  Locate the XF86Config that would be used by the server
*/

int
TCL_XF86FindXF86Config(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	int	retval;
	char	*tmpbuf;

	if (argc != 1) {
		Tcl_SetResult(interp, "Usage: xf86config_findfile", TCL_STATIC);
		return TCL_ERROR;
	}

	errinterp = interp;
	tmpbuf = XtMalloc(PATH_MAX);
	if (!xf86OpenConfigFile(tmpbuf))
		Tcl_SetResult(interp, "", TCL_STATIC);
	else
		Tcl_SetResult(interp, tmpbuf, TCL_VOLATILE);
	XtFree(tmpbuf);
	return TCL_OK;
}

/*
   Implements the xf86config_readfile command which reads
   in the XF86Config file and set the values from it
*/

static char readconfig_usage[] = "Usage: xf86config_readfile " \
	"<xwinhome> <file_varname> <server_varname> " \
	"<kbd_varname> <pointer_varname> <monitor_varname> " \
	"<device_varname> <scrn_varname>";

#define StrOrNull(xx)	((xx)==NULL? "": (xx))

int
TCL_XF86ReadXF86Config(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	int retval;

	if (argc != 9) {
		Tcl_SetResult(interp, readconfig_usage, TCL_STATIC);
		return TCL_ERROR;
	}

	errinterp = interp;
	init_config_vars(argv[1]);
	if ((retval = read_config_file()) != TCL_OK)
		return retval;
	getsection_files   (interp, argv[2]);
	getsection_server  (interp, argv[3]);
	getsection_keyboard(interp, argv[4]);
	getsection_pointer (interp, argv[5], config_list->conf_pointer);
	getsection_monitor (interp, argv[6]);
	getsection_device  (interp, argv[7]);
	getsection_screen  (interp, argv[8]);
#ifdef NOT_YET
#ifdef XINPUT
	getsection_xinput  (interp, argv[9]);
#endif
	getsection_module  (interp, argv[10]);
#endif
	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the Files section
*/

static int
getsection_files(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
	char *f;

	Tcl_SetVar2(interp, "files", "LogFile",
		StrOrNull(config_list->conf_files->file_logfile), 0);
	if (config_list->conf_files->file_fontpath) {
		f = validate_font_path(config_list->conf_files->file_fontpath);
		Tcl_SetVar2(interp, "files", "FontPath", StrOrNull(f), 0);
	} else {
		Tcl_SetVar2(interp, "files", "FontPath", defaultFontPath, 0);
	}
	if (config_list->conf_files->file_rgbpath)
		Tcl_SetVar2(interp, "files", "RGBPath",
			config_list->conf_files->file_rgbpath, 0);
	else
		Tcl_SetVar2(interp, "files", "RGBPath", rgbPath, 0);
	Tcl_SetVar2(interp, "files", "ModulePath",
		StrOrNull(config_list->conf_files->file_modulepath), 0);
	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the ServerFlags section
*/

static int
getsection_server(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
	static char *opts[] = {
		"NoTrapSignals",
		"DontZap",
		"DontZoom",
#ifdef XF86VIDMODE
		"DisableVidModeExtension",
		"AllowNonLocalXvidtune",
#endif
#ifdef XF86MISC
		"DisableModInDev",
		"AllowNonLocalModInDev",
#endif
		"AllowMouseOpenFile",
#if 0 /* KY XXX */
		"PCIProbe1",
		"PCIProbe2",
		"PCIForceConfig1",
		"PCIForceConfig2",
#endif
		NULL,
	};
	XF86OptionPtr optr;
	int i;

	if (!config_list->conf_flags)
		return TRUE;

	for (optr = config_list->conf_flags->flg_option_lst; optr; optr = optr->list.next) {
		for (i = 0; opts[i]; ++i) {
		    Tcl_SetVar2(interp, "server", opts[i],
				StrCaseCmp(optr->opt_name, opts[i]) ? "" : opts[i], 0);
		}
	}
	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the Keyboard section
*/

static int
getsection_keyboard(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
	XF86ConfKeyboardPtr keyboard;
	char	tmpbuf[128], tmpbuf2[16];
	int	i;

	keyboard = config_list->conf_keyboard;
#ifdef XQUEUE
	if (!StrCaseCmp(keyboard->keyb_protocol, "Xqueue"))
		Tcl_SetVar2(interp, "keyboard", "Protocol", "Xqueue", 0);
#endif
	if (!StrCaseCmp(keyboard->keyb_protocol, "Standard"))
		Tcl_SetVar2(interp, "keyboard", "Protocol", "Standard", 0);

	Tcl_SetVar2(interp, "keyboard", "ServerNumLock",
		keyboard->keyb_serverNumLock ? "ServerNumLock": "", 0);
	sprintf(tmpbuf, "%d %d", 
		(keyboard->keyb_kbdDelay) ? keyboard->keyb_kbdDelay : 500,
		(keyboard->keyb_kbdRate) ? keyboard->keyb_kbdRate : 30);
	Tcl_SetVar2(interp, "keyboard", "AutoRepeat", tmpbuf, 0);

	tmpbuf[0] = '\0';
	for (i = 1; i < 8; i++) {
		if (keyboard->keyb_xleds & (1L << (i-1))) {
			sprintf(tmpbuf2, "%d ", i);
			strcat(tmpbuf, tmpbuf2);
		}
	}
	Tcl_SetVar2(interp, "keyboard", "XLeds", tmpbuf, 0);

	Tcl_SetVar2(interp, "keyboard", "VTInit",
		StrOrNull(keyboard->keyb_vtinit), 0);
#ifdef USE_VT_SYSREQ
	Tcl_SetVar2(interp, "keyboard", "VTSysReq", 
		keyboard->keyb_vtSysreq ? "VTSysReq" : "", 0);
#endif

        Tcl_SetVar2(interp, "keyboard", "LeftAlt", 
		KeyMapType(keyboard, LEFTALT), 0);
	Tcl_SetVar2(interp, "keyboard", "RightAlt", 
		KeyMapType(keyboard, RIGHTALT), 0);
	Tcl_SetVar2(interp, "keyboard", "ScrollLock", 
		KeyMapType(keyboard, SCROLLLOCK), 0);
	Tcl_SetVar2(interp, "keyboard", "RightCtl", 
		KeyMapType(keyboard, RIGHTCTL), 0);

#ifdef XKB
	Tcl_SetVar2(interp, "keyboard", "XkbDisable",
		keyboard->keyb_xkbDisable ? "XkbDisable": "", 0);
	Tcl_SetVar2(interp, "keyboard", "XkbKeycodes",
		StrOrNull(keyboard->keyb_xkbkeycodes), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbTypes",
		StrOrNull(keyboard->keyb_xkbtypes), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbCompat",
		StrOrNull(keyboard->keyb_xkbcompat), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbSymbols",
		StrOrNull(keyboard->keyb_xkbsymbols), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbGeometry",
		StrOrNull(keyboard->keyb_xkbgeometry), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbKeymap",
		StrOrNull(keyboard->keyb_xkbkeymap), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbRules",
		StrOrNull(keyboard->keyb_xkbrules), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbModel",
		StrOrNull(keyboard->keyb_xkbmodel), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbLayout",
		StrOrNull(keyboard->keyb_xkblayout), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbVariant",
		StrOrNull(keyboard->keyb_xkbvariant), 0);
	Tcl_SetVar2(interp, "keyboard", "XkbOptions",
		StrOrNull(keyboard->keyb_xkboptions), 0);
#endif /* XKB */

#if defined(SVR4) && defined(i386) && defined(PC98)
	Tcl_SetVar2(interp, "keyboard", "Panix106",
		keyboard->keyb_panix106 ? "Panix106": "", 0);
#endif

	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the Pointer section
*/

static int
getsection_pointer(interp, varname, pntr)
  Tcl_Interp *interp;
  char *varname;
  XF86ConfPointerPtr pntr;
{
	char tmpbuf[16];
	char *name;
	int mtoken = -1;
	int i;

	name = NULL;
	if (pntr->pntr_protocol) {
		if (!StrCaseCmp(pntr->pntr_protocol, "Xqueue"))
#ifdef XQUEUE
			name = "Xqueue";
#else
			name = NULL;
#endif
		if (!StrCaseCmp(pntr->pntr_protocol, "OSMouse"))
#if defined(USE_OSMOUSE) || defined(OSMOUSE_ONLY)
			name = "OSMouse";
#else
			name = NULL;
#endif
#ifndef OSMOUSE_ONLY
		if (name == NULL) {
			mtoken = string_to_token(xf86MouseTab, 
				pntr->pntr_protocol);
			if (mtoken >= 0)
				name = pntr->pntr_protocol;
		}
#endif
	}
	Tcl_SetVar2(interp, "mouse", "Protocol", StrOrNull(name), 0);

#ifndef OSMOUSE_ONLY
	Tcl_SetVar2(interp, "mouse", "Device",
		StrOrNull(pntr->pntr_device), 0);
	i = pntr->pntr_baudrate;
	if (i) {
		if (mtoken == LOGIMAN && i != 1200 && i != 9600)
			i = 0;
		else if (i % 1200 != 0 || i < 1200 || i > 9600)
			i = 0;
	}
	sprintf(tmpbuf, "%d", i);
	Tcl_SetVar2(interp, "mouse", "BaudRate", tmpbuf, 0);
	sprintf(tmpbuf, "%d", pntr->pntr_samplerate);
	Tcl_SetVar2(interp, "mouse", "SampleRate", tmpbuf, 0);
	sprintf(tmpbuf, "%d", pntr->pntr_resolution);
	Tcl_SetVar2(interp, "mouse", "Resolution", tmpbuf, 0);
	Tcl_SetVar2(interp, "mouse", "ChordMiddle",
		pntr->pntr_chordMiddle ? "ChordMiddle": "", 0);

#ifdef XINPUT
	Tcl_SetVar2(interp, "mouse", "AlwaysCore",
		pntr->pntr_alwaysCore ? "AlwaysCore": "", 0);
#endif

#ifdef CLEARDTR_SUPPORT
	Tcl_SetVar2(interp, "mouse", "ClearDTR",
		pntr->pntr_clearDtr ? "ClearDTR": "", 0);
	Tcl_SetVar2(interp, "mouse", "ClearRTS",
		pntr->pntr_clearRts ? "ClearRTS": "", 0);
#endif
#endif /* OSMOUSE_ONLY */

	Tcl_SetVar2(interp, "mouse", "Emulate3Buttons",
		pntr->pntr_emulate3Buttons ? "Emulate3Buttons": "", 0);
	sprintf(tmpbuf, "%d", 
		pntr->pntr_emulate3Timeout ? pntr->pntr_emulate3Timeout : 50);
	Tcl_SetVar2(interp, "mouse", "Emulate3Timeout", tmpbuf, 0);

	i = pntr->pntr_buttons;
	switch (pntr->pntr_positiveZ) {
	case MSE_MAPTOX:
	case MSE_MAPTOY:
		break;
	default:
		if (pntr->pntr_positiveZ > 0
			&& pntr->pntr_positiveZ <= MSE_MAXBUTTONS
			&& pntr->pntr_negativeZ > 0
			&& pntr->pntr_negativeZ <= MSE_MAXBUTTONS) {
			if (pntr->pntr_positiveZ > i)
				i = pntr->pntr_positiveZ;
			if (pntr->pntr_negativeZ > i)
				i = pntr->pntr_negativeZ;
		}
		break;
	}
	sprintf(tmpbuf, "%d", i);
	Tcl_SetVar2(interp, "mouse", "Buttons", tmpbuf, 0);

	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the Monitor sections
*/

static int
getsection_monitor(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
	int		i, j;
	char		*namebuf, *tmpptr, *tmpbuf;
	XF86ConfMonitorPtr 	mptr;
	XF86ConfModeLinePtr	dptr;

	for (mptr = config_list->conf_monitor_lst; mptr; mptr = mptr->list.next) {
		namebuf = (char *)XtMalloc(strlen(varname)
					   + strlen(mptr->mon_identifier) + 2);
		sprintf(namebuf, "%s_%s", varname, mptr->mon_identifier);

		Tcl_SetVar2(interp, namebuf, "VendorName",
			StrOrNull(mptr->mon_vendor), 0);
		Tcl_SetVar2(interp, namebuf, "ModelName",
			StrOrNull(mptr->mon_modelname), 0);

		tmpptr = tmpbuf = XtMalloc(mptr->mon_n_hsync*14);
		for (j = 0; j < mptr->mon_n_hsync; j++) {
		    sprintf(tmpptr, "%s%.5g-%.5g", (j? ",": ""),
			mptr->mon_hsync[j].lo, mptr->mon_hsync[j].hi);
		    tmpptr += strlen(tmpptr);
		}
		Tcl_SetVar2(interp, namebuf, "HorizSync", tmpbuf, 0);

		tmpptr = tmpbuf = XtRealloc(tmpbuf, mptr->mon_n_vrefresh*14);
		for (j = 0; j < mptr->mon_n_vrefresh; j++) {
		    sprintf(tmpptr, "%s%.5g-%.5g", (j? ",": ""),
			mptr->mon_vrefresh[j].lo, mptr->mon_vrefresh[j].hi);
		    tmpptr += strlen(tmpptr);
		}
		Tcl_SetVar2(interp, namebuf, "VertRefresh", tmpbuf, 0);

		tmpbuf = XtRealloc(tmpbuf, 256);
		if ((mptr->mon_gamma_red == mptr->mon_gamma_green)
			&& (mptr->mon_gamma_red == mptr->mon_gamma_blue)) {
		    if ((mptr->mon_gamma_red == 0.0) 
			|| (mptr->mon_gamma_red == 1.0))
			Tcl_SetVar2(interp, namebuf, "Gamma", "", 0);
		    else {
			sprintf(tmpbuf, "%.3g", 1.0/mptr->mon_gamma_red);
			Tcl_SetVar2(interp, namebuf, "Gamma", tmpbuf, 0);
		    }
		} else {
		    sprintf(tmpbuf, "%.3g %.3g %.3g",
			    1.0/mptr->mon_gamma_red, 
			    1.0/mptr->mon_gamma_green, 
			    1.0/mptr->mon_gamma_blue);
		    Tcl_SetVar2(interp, namebuf, "Gamma", tmpbuf, 0);
		}

		Tcl_SetVar2(interp, namebuf, "Modelines", "", 0);
		for (dptr = mptr->mon_modeline_lst; dptr; dptr = dptr->list.next) {
		    sprintf(tmpbuf, "\"%s\" %.4g %d %d %d %d %d %d %d %d",
			dptr->ml_identifier, dptr->ml_clock/1000.0,
			dptr->ml_hdisplay, dptr->ml_hsyncstart,
			dptr->ml_hsyncend, dptr->ml_htotal,
			dptr->ml_vdisplay, dptr->ml_vsyncstart,
			dptr->ml_vsyncend, dptr->ml_vtotal);
		    if (dptr->ml_flags & XF86CONF_INTERLACE)
			strcat(tmpbuf, " Interlace");
		    if (dptr->ml_flags & XF86CONF_PHSYNC)
			strcat(tmpbuf, " +HSync");
		    if (dptr->ml_flags & XF86CONF_NHSYNC)
			strcat(tmpbuf, " -HSync");
		    if (dptr->ml_flags & XF86CONF_PVSYNC)
			strcat(tmpbuf, " +VSync");
		    if (dptr->ml_flags & XF86CONF_NVSYNC)
			strcat(tmpbuf, " -VSync");
		    if (dptr->ml_flags & XF86CONF_CSYNC)
			strcat(tmpbuf, " Composite");
		    if (dptr->ml_flags & XF86CONF_PCSYNC)
			strcat(tmpbuf, " +CSync");
		    if (dptr->ml_flags & XF86CONF_NCSYNC)
			strcat(tmpbuf, " -CSync");
		    if (dptr->ml_flags & XF86CONF_DBLSCAN)
			strcat(tmpbuf, " DoubleScan");
		    if (dptr->ml_flags & XF86CONF_HSKEW) {
			char buf[10];
		        sprintf(buf, " HSkew %d", dptr->ml_hskew);
		        strcat(tmpbuf, buf);
		    }
		    Tcl_SetVar2(interp, namebuf, "Modelines", tmpbuf,
		        TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
		}
		XtFree(tmpbuf);
		XtFree(namebuf);
	}
	return TCL_OK;
}

/*
  Set the Tcl variables for the config from the Device sections
*/

static int
getsection_device(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
	int	i, j;
	char	*namebuf, tmpbuf[128];
	XF86ConfDevicePtr	dptr;
	XF86OptionPtr		optr;

	for (dptr = config_list->conf_device_lst; dptr; dptr = dptr->list.next) {
		namebuf = XtMalloc(strlen(varname)
				   + strlen(dptr->dev_identifier) + 2);
		sprintf(namebuf, "%s_%s", varname, dptr->dev_identifier);
		Tcl_SetVar2(interp, namebuf, "VendorName",
			StrOrNull(dptr->dev_vendor), 0);
		Tcl_SetVar2(interp, namebuf, "BoardName",
			StrOrNull(dptr->dev_board), 0);
		Tcl_SetVar2(interp, namebuf, "Chipset",
			StrOrNull(dptr->dev_chipset), 0);
		Tcl_SetVar2(interp, namebuf, "Ramdac",
			StrOrNull(dptr->dev_ramdac), 0);
		Tcl_SetVar2(interp, namebuf, "DacSpeed",
			NonZeroStr(dptr->dev_dacSpeeds[0]/1000,10), 0);
		if (dptr->dev_dacSpeeds[0]/1000 > 0)
		   for (j = 1; j < MAXDACSPEEDS; j++) {
		      if (dptr->dev_dacSpeeds[j]/1000 <= 0)
			 break;
		      sprintf(tmpbuf, " %d", dptr->dev_dacSpeeds[j]/1000);
		      Tcl_SetVar2(interp, namebuf, "DacSpeed",
				  tmpbuf, TCL_APPEND_VALUE);
		   }
		Tcl_SetVar2(interp, namebuf, "Clocks", "", 0);
		for (j = 0; j < dptr->dev_clocks; j++) {
			sprintf(tmpbuf, "%.5g ", dptr->dev_clock[j]/1000.0);
			Tcl_SetVar2(interp, namebuf, "Clocks",
				tmpbuf, TCL_APPEND_VALUE);
		}
		Tcl_SetVar2(interp, namebuf, "ClockProg",
			StrOrNull(dptr->dev_clockprog), 0);
		if (dptr->dev_clockchip && dptr->dev_clocks == 0) {
		    for (j = 0; xf86_ClockOptionTab[j].token != -1; ++j) {
			if (!StrCaseCmp(dptr->dev_clockchip,
				xf86_ClockOptionTab[j].name)) {
			    Tcl_SetVar2(interp, namebuf, "ClockChip",
				xf86_ClockOptionTab[j].name, 0);
			    break;
			}
		    }
		    if (xf86_ClockOptionTab[j].token == -1)
			/* this shouldn't happen */
			Tcl_SetVar2(interp, namebuf, "ClockChip", "", 0);
		} else
			Tcl_SetVar2(interp, namebuf, "ClockChip", "", 0);
		if (dptr->dev_speedup) { 
		    if (!StrCaseCmp(dptr->dev_speedup, "none"))
			Tcl_SetVar2(interp, namebuf, "SpeedUp", "none", 0);
		    else if (!StrCaseCmp(dptr->dev_speedup, "best"))
			Tcl_SetVar2(interp, namebuf, "SpeedUp", "best", 0);
		    else if (!StrCaseCmp(dptr->dev_speedup, "all"))
			Tcl_SetVar2(interp, namebuf, "SpeedUp", "all", 0);
		    else /* KY XXX */
			Tcl_SetVar2(interp, namebuf, "SpeedUp", "", 0);
		} else
			Tcl_SetVar2(interp, namebuf, "SpeedUp", "", 0);

		Tcl_SetVar2(interp, namebuf, "VideoRam",
			NonZeroStr(dptr->dev_videoram,10), 0);
		Tcl_SetVar2(interp, namebuf, "BIOSBase",
			NonZeroStr(dptr->dev_bios_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "Membase",
			NonZeroStr(dptr->dev_mem_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "IOBase",
			NonZeroStr(dptr->dev_io_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "DACBase",
			NonZeroStr(dptr->dev_dac_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "POSBase",
			NonZeroStr(dptr->dev_pos_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "COPBase",
			NonZeroStr(dptr->dev_cop_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "VGABase",
			NonZeroStr(dptr->dev_vga_base,16), 0);
		Tcl_SetVar2(interp, namebuf, "Instance",
			NonZeroStr(dptr->dev_instance,10), 0);
		Tcl_UnsetVar2(interp, namebuf, "Option", 0);
		for(optr = dptr->dev_option_lst; optr; optr = optr->list.next) {
		    for (j = 0; xf86_OptionTab[j].token != -1; j++) {
			if (!StrCaseCmp(optr->opt_name,
				xf86_OptionTab[j].name)) {
			    Tcl_SetVar2(interp, namebuf, "Option",
			        xf86_OptionTab[j].name,
			        TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
			}
		    }
		}
		if (Tcl_GetVar2(interp, namebuf, "Option", 0) == NULL)
			Tcl_SetVar2(interp, namebuf, "Option", "", 0);
		XtFree(namebuf);
	}
	return TCL_OK;
}

/*
  Convert the given value to a string representation in the specified
  base.  If the value is zero, return an empty string
*/

static char *NonZeroStr(val, base)
  unsigned long val;
  int base;
{
	static char tmpbuf[16];

	if (val) {
		if (base == 16)
			sprintf(tmpbuf, "%#lx", val);
		else
			sprintf(tmpbuf, "%ld", val);
		return tmpbuf;
	} else
		return "";
}

/*
  Set the Tcl variables for the config from the Screen sections
*/

static int
getsection_screen(interp, varname)
  Tcl_Interp *interp;
  char *varname;
{
    XF86ConfScreenPtr	screen;
    XF86ConfDisplayPtr	disp;
    XF86ModePtr		mode;
    XF86OptionPtr	optr;
    char		*namebuf, tmpbuf[128], tmpbuf2[32];
    int			depth;
    int			i;

    for (screen = config_list->conf_screen_lst; screen; screen = screen->list.next) {
	namebuf = (char *) XtMalloc(strlen(varname) 
		+ strlen(screen->scrn_identifier) + 5);
	sprintf(namebuf, "%s_%s", varname, screen->scrn_identifier);
	for (disp = screen->scrn_display_lst; disp; disp = disp->list.next) {
	    depth = disp->disp_depth;
	    if (depth <= 0)
		depth = screen->scrn_defaultcolordepth;
	    if (depth <= 0)
		depth = screen->scrn_display_lst->disp_depth;
	    sprintf(tmpbuf, "%d", depth);
	    sprintf(tmpbuf2, "Depth,%d", depth);
	    Tcl_SetVar2(interp, namebuf, tmpbuf2, tmpbuf, 0);

	    if (disp->disp_frameX0 > 0 || disp->disp_frameX0 > 0) {
		sprintf(tmpbuf, "%d %d",
                        disp->disp_frameX0, disp->disp_frameY0);
		sprintf(tmpbuf2, "ViewPort,%d", depth);
		Tcl_SetVar2(interp, namebuf, tmpbuf2, tmpbuf, 0);
	    }

	    if (disp->disp_virtualX > 0 || disp->disp_virtualY > 0) {
		sprintf(tmpbuf, "%d %d",
			disp->disp_virtualX, disp->disp_virtualY);
		sprintf(tmpbuf2, "Virtual,%d", depth);
		Tcl_SetVar2(interp, namebuf, tmpbuf2, tmpbuf, 0);
	    }

	    if (disp->disp_depth == 16 && disp->disp_weight.red > 0) {
		sprintf(tmpbuf, "%d%d%d",
			disp->disp_weight.red,
			disp->disp_weight.green,
			disp->disp_weight.blue);
		sprintf(tmpbuf2, "Weight,%d", depth);
		Tcl_SetVar2(interp, namebuf, tmpbuf2, tmpbuf, 0);
	    }

	    if (disp->disp_visual) {
		if (!StrCaseCmp(disp->disp_visual, "StaticGray") ||
		    !StrCaseCmp(disp->disp_visual, "GrayScale") ||
		    !StrCaseCmp(disp->disp_visual, "StaticColor") ||
		    !StrCaseCmp(disp->disp_visual, "PseudoColor") ||
		    !StrCaseCmp(disp->disp_visual, "TrueColor") ||
		    !StrCaseCmp(disp->disp_visual, "DirectColor")) {
		    sprintf(tmpbuf2, "Visual,%d", depth);
		    Tcl_SetVar2(interp, namebuf, tmpbuf2, disp->disp_visual, 0);
		}
	    }

	    sprintf(tmpbuf2, "Modes,%d", depth);
	    Tcl_SetVar2(interp, namebuf, tmpbuf2, "", 0);
	    for (mode = disp->disp_mode_lst; mode; mode = mode->list.next) {
		Tcl_SetVar2(interp, namebuf, tmpbuf2, mode->mode_name,
			TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
	    }

	    Tcl_UnsetVar2(interp, namebuf, "Option", 0);
	    for(optr = disp->disp_option_lst; optr; optr = optr->list.next) {
		for (i = 0; xf86_OptionTab[i].token != -1; i++) {
		    if (!StrCaseCmp(optr->opt_name, xf86_OptionTab[i].name)) {
			Tcl_SetVar2(interp, namebuf, "Option",
				xf86_OptionTab[i].name,
				TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
		    }
		}
	    }
	    if (Tcl_GetVar2(interp, namebuf, "Option", 0) == NULL)
		Tcl_SetVar2(interp, namebuf, "Option", "", 0);
	}

	Tcl_SetVar2(interp, namebuf, "Monitor",
		StrOrNull(screen->scrn_monitor->mon_identifier), 0);
	Tcl_SetVar2(interp, namebuf, "Device",
		StrOrNull(screen->scrn_device->dev_identifier), 0);

	if (screen->scrn_blanktime) {
	    sprintf(tmpbuf, "%ld", screen->scrn_blanktime);
	    Tcl_SetVar2(interp, namebuf, "BlankTime", tmpbuf, 0);
	}
#ifdef DPMSExtension
	if (screen->scrn_standbytime) {
	    sprintf(tmpbuf, "%ld", screen->scrn_standbytime);
	    Tcl_SetVar2(interp, namebuf, "StandbyTime", tmpbuf, 0);
	}
	if (screen->scrn_suspendtime) {
	    sprintf(tmpbuf, "%ld", screen->scrn_suspendtime);
	    Tcl_SetVar2(interp, namebuf, "SuspendTime", tmpbuf, 0);
	}
	if (screen->scrn_offtime) {
	    sprintf(tmpbuf, "%ld", screen->scrn_offtime);
	    Tcl_SetVar2(interp, namebuf, "OffTime", tmpbuf, 0);
	}
#endif
	/* XXX screen no */
	XtFree(namebuf);
    }

    return TCL_OK;
}

/* other subroutines */

#define DIR_FILE	"/fonts.dir"

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 *
 * Taken from xf86Config.c.
 */
static char *
get_path_elem(pnt)
     char **pnt;
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
 *
 * Taken from xf86Config.c.
 */
#define CHECK_TYPE(mode, type) ((S_IFMT & (mode)) == (type))
static char *
validate_font_path(path)
     char *path;
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1, *dir_elem;
  struct stat stat_buf;
  int flag;
  int dirlen;

  tmp_path = (char *)XtCalloc(1,strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = get_path_elem(&next);
#ifndef __EMX__
    if (*path_elem == '/') {
      dir_elem = (char *)XtCalloc(1, strlen(path_elem) + 1);
      if ((p1 = strchr(path_elem, ':')) != 0)
#else
    /* OS/2 must prepend X11ROOT */
    if (*path_elem == '/') {
      path_elem = (char*)__XOS2RedirRoot(path_elem);
      dir_elem = (char*)XtCalloc(1, strlen(path_elem) + 1);
      if (p1 = strchr(path_elem+2, ':'))
#endif
	dirlen = p1 - path_elem;
      else
	dirlen = strlen(path_elem);
      strncpy(dir_elem, path_elem, dirlen);
      dir_elem[dirlen] = '\0';
      flag = stat(dir_elem, &stat_buf);
      if (flag == 0)
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFDIR))
	  flag = -1;
      if (flag != 0) {
        ErrorF("Warning: The directory \"%s\" does not exist.\n", dir_elem);
	ErrorF("         Entry deleted from font path.\n");
	continue;
      }
      else {
	p1 = (char *)XtMalloc(strlen(dir_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, dir_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
	    flag = -1;
#ifndef __EMX__
	XtFree(p1);
#endif
	if (flag != 0) {
	  ErrorF("Warning: 'fonts.dir' not found (or not valid) in \"%s\".\n", 
		 dir_elem);
	  ErrorF("          Entry deleted from font path.\n");
	  ErrorF("          (Run 'mkfontdir' on \"%s\").\n", dir_elem);
	  continue;
	}
      }
      XtFree(dir_elem);
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
 * xf86TokenToString --
 *	returns the string corresponding to token
 */
static char *
token_to_string(table, token)
     SymTabPtr table;
     int token;
{
  int i;

  for (i = 0; table[i].token >= 0 && table[i].token != token; i++)
    ;
  if (table[i].token < 0)
    return("unknown");
  else
    return(table[i].name);
}
 
/*
 * xf86StringToToken --
 *	returns the string corresponding to token
 */
static int
string_to_token(table, string)
     SymTabPtr table;
     char *string;
{
  int i;

  for (i = 0; table[i].token >= 0 && StrCaseCmp(string, table[i].name); i++)
    ;
  return(table[i].token);
}
