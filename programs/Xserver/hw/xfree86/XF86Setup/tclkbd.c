/* $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/tclkbd.c,v 3.1 1996/06/30 10:44:10 dawes Exp $ */

/*

  This file contains routines to add commands to interface with XKB

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/Xproto.h>
#include <X11/Xfuncs.h>
#include <tcl.h>
#include <tk.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBui.h>

static int	TCL_XF86GetKBD(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86ShowKBD(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86ListKBD(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86LoadKBD(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86ResolveKBDComponents(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int init_xkb(
#if NeedNestedProtoTypes
    Tcl_Interp	*interp,
    Display	*dpy
#endif
);

XkbFileInfo		result;
XkbDescPtr		xkb = (XkbDescPtr) 0;

/*
   Adds all the new commands to the Tcl interpreter
*/

int
XF86Kbd_Init(interp)
    Tcl_Interp	*interp;
{
	Tcl_CreateCommand(interp, "xkb_read",
		TCL_XF86GetKBD, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xkb_show",
		TCL_XF86ShowKBD, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xkb_list",
		TCL_XF86ListKBD, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xkb_load",
		TCL_XF86LoadKBD, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xkb_resolvecomponents",
		TCL_XF86ResolveKBDComponents, (ClientData) NULL,
		(void (*)()) NULL);

	return TCL_OK;
}

static int
init_xkb(interp, dpy)
    Tcl_Interp	*interp;
    Display	*dpy;
{
	static Bool	been_here = False;
	int		major, minor, op, event, error;

	if (been_here == True)
		return TCL_OK;

	major = XkbMajorVersion;
	minor = XkbMinorVersion;
	if (!XkbQueryExtension(dpy, &op, &event, &error, &major, &minor)) {
		Tcl_SetResult(interp,
			"Unable to initialize XKEYBOARD extension",
			TCL_STATIC);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int
TCL_XF86GetKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Tk_Window topwin;
	Display *disp;

	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: xkb_read from_server|<filename>", TCL_STATIC);
		return TCL_ERROR;
	}

	if (strcmp(argv[1], "from_server") == 0) {
		if ((topwin = Tk_MainWindow(interp)) == (Tk_Window) NULL)
			return TCL_ERROR;
		disp = Tk_Display(topwin);
		if (init_xkb(interp, disp) != TCL_OK)
			return TCL_ERROR;
		xkb=XkbGetKeyboard(disp, XkbGBN_AllComponentsMask,XkbUseCoreKbd);
	} else {
#if 0
		unsigned tmp;
		FILE *fd;
		int	major, minor;

		XkbInitAtoms(NULL);
		major = XkbMajorVersion;
		minor = XkbMinorVersion;
		XkbLibraryVersion(&major, &minor);
		bzero((char *) &result, sizeof(result));
		if ((result.xkb=xkb=XkbAllocKeyboard()) == NULL) {
			Tcl_SetResult(interp, "Couldn't allocate keyboard", TCL_STATIC);
			return TCL_ERROR;
		}
		fd = fopen(argv[1], "r");
		tmp = XkmReadFile(fd,XkmGeometryMask,XkmKeymapLegal,&result);
		fclose(fd);
#else
		Tcl_SetResult(interp,
			"Reading from a file is not currently supported",
			TCL_STATIC);
		return TCL_ERROR;
#endif
	}
	

	if ((xkb==NULL)||(xkb->geom==NULL)) {
		Tcl_SetResult(interp, "Couldn't get keyboard", TCL_STATIC);
		return TCL_ERROR;
	}
	if (xkb->names->geometry == 0)
		xkb->names->geometry = xkb->geom->name;
	Tcl_AppendElement(interp,
		XkbAtomText(disp, xkb->names->keycodes, XkbMessage));
	Tcl_AppendElement(interp,
		XkbAtomText(disp, xkb->names->types, XkbMessage));
	Tcl_AppendElement(interp,
		XkbAtomText(disp, xkb->names->compat, XkbMessage));
	Tcl_AppendElement(interp,
		XkbAtomText(disp, xkb->names->symbols, XkbMessage));
	Tcl_AppendElement(interp,
		XkbAtomText(disp, xkb->geom->name, XkbMessage));

	return TCL_OK;
}

int
TCL_XF86ShowKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Tk_Window topwin, tkwin;
	Window win;
	Display *disp;
	XkbUI_ViewOptsRec       opts;
	XkbUI_ViewPtr		view;
	int                     scale,width,height;

	scale=10;

	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: xkb_show <window>", TCL_STATIC);
		return TCL_ERROR;
	}

	if ((topwin = Tk_MainWindow(interp)) == (Tk_Window) NULL)
		return TCL_ERROR;
	tkwin = Tk_NameToWindow(interp, argv[1], topwin);
	win = Tk_WindowId(tkwin);
	disp = Tk_Display(tkwin);

	if ((xkb==NULL)||(xkb->geom==NULL)) {
		Tcl_SetResult(interp, "keyboard not loaded", TCL_STATIC);
		return TCL_ERROR;
	}
	width= xkb->geom->width_mm/scale;
	height= xkb->geom->height_mm/scale;
	bzero((char *)&opts,sizeof(opts));
	opts.present= XkbUI_SizeMask|XkbUI_ColormapMask;
	width = opts.viewport.width = Tk_Width(tkwin);
	height = opts.viewport.height = Tk_Height(tkwin);
	opts.cmap = Tk_Colormap(tkwin);
	view= XkbUI_Init(disp,win,width,height,xkb,&opts);

	XClearWindow(disp,win);
	XkbUI_DrawRegion(view,NULL);
	return TCL_OK;
}

#define MAX_COMPONENTS		400	/* Max # components of one type */
#define MAX_TTL_COMPONENTS	1000	/* Max # components of all types */
#define flag2char(flag)		((flag & XkbLC_Default)? \
				((flag & XkbLC_Partial)? '*': '#'): \
				((flag & XkbLC_Partial)? '+': ' '))

int
TCL_XF86ListKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Tk_Window topwin;
	Display *disp;
	XkbComponentNamesRec getcomps;
	XkbComponentListPtr comps;
	char *av[MAX_COMPONENTS], bufs[MAX_COMPONENTS][32], *names;
	int max, i, ncomps;

	if (argc != 7) {
		Tcl_SetResult(interp, "Usage: xkb_list <keymap_pat> "
			"<keycodes_pat> <compat_pat> <types_pat> "
			"<symbols_pat> <geometry_pat>", TCL_STATIC);
		return TCL_ERROR;
	}

	if ((topwin = Tk_MainWindow(interp)) == (Tk_Window) NULL)
		return TCL_ERROR;
	disp = Tk_Display(topwin);
	max = MAX_TTL_COMPONENTS;
	getcomps.keymap   = argv[1];
	getcomps.keycodes = argv[2];
	getcomps.compat   = argv[3];
	getcomps.types    = argv[4];
	getcomps.symbols  = argv[5];
	getcomps.geometry = argv[6];
	comps = XkbListComponents(disp, XkbUseCoreKbd, &getcomps, &max);

	for (i = 0; i < MAX_COMPONENTS; i++)
		av[i] = bufs[i];

	for (i = 0, ncomps = 0; i < comps->num_keymaps; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->keymaps[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->keymaps[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->keymaps[i].name, 30);
	    bufs[ncomps++][31] = '\0';
	}
	names = Tcl_Merge(ncomps, av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0, ncomps = 0; i < comps->num_keycodes; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->keycodes[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->keycodes[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->keycodes[i].name, 30);
	    bufs[ncomps++][31] = '\0';
	}
	names = Tcl_Merge(ncomps,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0, ncomps = 0; i < comps->num_compat; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->compat[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->compat[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->compat[i].name, 30);
	    bufs[ncomps++][31] = '\0';
	}
	names = Tcl_Merge(ncomps,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0, ncomps = 0; i < comps->num_types; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->types[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->types[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->types[i].name, 30);
	    bufs[ncomps++][31] = '\0';
	}
	names = Tcl_Merge(ncomps,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0, ncomps = 0; i < comps->num_symbols; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->symbols[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->symbols[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->symbols[i].name, 30);
	    bufs[ncomps][31] = '\0';
	    ncomps++;
	}
	names = Tcl_Merge(ncomps,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0, ncomps = 0; i < comps->num_geometry; i++) {
	    if (ncomps == MAX_COMPONENTS)
	        break;
	    if (comps->geometry[i].flags & XkbLC_Hidden)
	        continue;
	    bufs[ncomps][0] = flag2char(comps->geometry[i].flags);
	    strncpy(&(bufs[ncomps][1]), comps->geometry[i].name, 30);
	    bufs[ncomps++][31] = '\0';
	}
	names = Tcl_Merge(ncomps,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	XkbFreeComponentList(comps);
	return TCL_OK;
}

static char *usage_LoadKBD = "Usage: xkb_load <keymap_pat> "
			"<keycodes_pat> <compat_pat> <types_pat> "
			"<symbols_pat> <geometry_pat> [load|noload]";

int
TCL_XF86LoadKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Tk_Window topwin;
	Display *disp;
	XkbComponentNamesRec getcomps;
	Bool loadit = True;

	if (argc < 7 || argc > 8) {
		Tcl_SetResult(interp, usage_LoadKBD, TCL_STATIC);
		return TCL_ERROR;
	}

	if (argc == 8) {
		if (!strcmp(argv[7], "load"))
			loadit = True;
		else if (!strcmp(argv[7], "noload"))
			loadit = False;
		else {
			Tcl_SetResult(interp, usage_LoadKBD, TCL_STATIC);
			return TCL_ERROR;
		}
	}

	if ((topwin = Tk_MainWindow(interp)) == (Tk_Window) NULL)
		return TCL_ERROR;
	disp = Tk_Display(topwin);

	getcomps.keymap   = argv[1];
	getcomps.keycodes = argv[2];
	getcomps.compat   = argv[3];
	getcomps.types    = argv[4];
	getcomps.symbols  = argv[5];
	getcomps.geometry = argv[6];
	xkb = XkbGetKeyboardByName(disp, XkbUseCoreKbd, &getcomps,
			XkbGBN_AllComponentsMask, 0, loadit);
	if (!xkb) {
		Tcl_SetResult(interp, "Load failed", TCL_STATIC);
		return TCL_ERROR;
	}
	if (xkb->names->geometry == 0)
		xkb->names->geometry = xkb->geom->name;

	return TCL_OK;
}

int
TCL_XF86ResolveKBDComponents(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
#if XKBLIB_HAS_RULES_SUPPORT
	XkbComponentNamesRec comps;
	FILE *fp;
#endif

	if (argc != 4) {
		Tcl_SetResult(interp, "Usage: xkb_resolvecomponents "
			"<rulesfilehandle> <model> <layout>", TCL_STATIC);
		return TCL_ERROR;
	}

#if XKBLIB_HAS_RULES_SUPPORT
	if (Tcl_GetOpenFile(interp, argv[1], 0, 1, &fp) != TCL_OK) {
		return TCL_ERROR;
	}

	XkbComponentNamesFromMapFile(fp, argv[2], argv[3], &comps);
	Tcl_AppendElement(interp,(comps.keymap?  comps.keymap:  ""));
	Tcl_AppendElement(interp,(comps.keycodes?comps.keycodes:""));
	Tcl_AppendElement(interp,(comps.compat?  comps.compat:  ""));
	Tcl_AppendElement(interp,(comps.types?   comps.types:   ""));
	Tcl_AppendElement(interp,(comps.symbols? comps.symbols: ""));
	Tcl_AppendElement(interp,(comps.geometry?comps.geometry:""));
#endif

	return TCL_OK;
}
 
