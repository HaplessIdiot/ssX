
/*

  This file contains routines to add commands to interface with XKB

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

TCL_XF86GetKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{	
	Tk_Window topwin, tkwin;
	Window win;
	Display *disp;
	int			op, event, err, major, minor;

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
		unsigned tmp;
		FILE *fd;

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
	}

	if ((xkb==NULL)||(xkb->geom==NULL)) {
		Tcl_SetResult(interp, "Couldn't get keyboard", TCL_STATIC);
		return TCL_ERROR;
	}
	Tcl_AppendElement(interp,
		XkbAtomGetString(disp, xkb->names->keycodes));
	Tcl_AppendElement(interp,
		XkbAtomGetString(disp, xkb->names->types));
	Tcl_AppendElement(interp,
		XkbAtomGetString(disp, xkb->names->compat));
	Tcl_AppendElement(interp,
		XkbAtomGetString(disp, xkb->names->symbols));
	Tcl_AppendElement(interp,
		XkbAtomGetString(disp, xkb->geom->name));

	return TCL_OK;
}

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
	int                     scale,width,height,cw,ch;

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

TCL_XF86ListKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{	
	Tk_Window topwin, tkwin;
	Window win;
	Display *disp;
	XkbComponentNamesRec getcomps;
	XkbComponentListPtr comps;
	char *av[1000], *names;
	int max, i;

	if (argc != 7) {
		Tcl_SetResult(interp, "Usage: xkb_list <keymap_pat> "
			"<keycodes_pat> <compat_pat> <types_pat> "
			"<symbols_pat> <geometry_pat>", TCL_STATIC);
		return TCL_ERROR;
	}

	if ((topwin = Tk_MainWindow(interp)) == (Tk_Window) NULL)
		return TCL_ERROR;
	disp = Tk_Display(topwin);
	max = 1000;
	getcomps.keymap   = argv[1];
	getcomps.keycodes = argv[2];
	getcomps.compat   = argv[3];
	getcomps.types    = argv[4];
	getcomps.symbols  = argv[5];
	getcomps.geometry = argv[6];
	comps = XkbListComponents(disp, XkbUseCoreKbd, &getcomps, &max);

	for (i = 0; i < comps->num_keymaps; i++)
	    av[i] = comps->keymaps[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	for (i = 0; i < comps->num_keycodes; i++)
	    av[i] = comps->keycodes[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);

	for (i = 0; i < comps->num_compat; i++)
	    av[i] = comps->compat[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);

	for (i = 0; i < comps->num_types; i++)
	    av[i] = comps->types[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);

	for (i = 0; i < comps->num_symbols; i++)
	    av[i] = comps->symbols[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);

	for (i = 0; i < comps->num_geometry; i++)
	    av[i] = comps->geometry[i].name;
	names = Tcl_Merge(i,av);
	Tcl_AppendElement(interp, names);
	XtFree(names);

	XkbFreeComponentList(comps);
	return TCL_OK;
}

/* NOT FINISHED */

TCL_XF86LoadKBD(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{	
	Tk_Window topwin, tkwin;
	Window win;
	Display *disp;
	XkbComponentNamesRec getcomps;
	XkbComponentListPtr comps;
	char *av[1000], *names;
	int max, i;

	if (argc != 7) {
		Tcl_SetResult(interp, "Usage: xkb_load <keymap_pat> "
			"<keycodes_pat> <compat_pat> <types_pat> "
			"<symbols_pat> <geometry_pat>", TCL_STATIC);
		return TCL_ERROR;
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
	/*
	XkbGetKeyboardByName(disp, XkbUseCoreKbd, &getcomps,
			XkbGBN_AllComponentsMask, 0, True);
	XkbGetNamedGeometry(disp, xkb, argv[6]);
	*/

	return TCL_OK;
}

