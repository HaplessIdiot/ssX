
/*

  This file contains routines to add a few misc commands to Tcl

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xproto.h>
#include <X11/Xfuncs.h>
#include <tcl.h>
#include <tk.h>
#include <sys/stat.h>

static int	TCL_XF86GetUID(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86ServerRunning(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86ProcessRunning(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86HasSymlinks(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86Link(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86UnLink(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

static int	TCL_XF86Sleep(
#if NeedNestedProtoTypes
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
#endif
);

/*
   Adds all the new commands to the Tcl interpreter
*/

XF86Other_Init(interp)
    Tcl_Interp	*interp;
{
	Tcl_CreateCommand(interp, "getuid",
		TCL_XF86GetUID, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "server_running",
		TCL_XF86ServerRunning, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "process_running",
		TCL_XF86ProcessRunning, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "has_symlinks",
		TCL_XF86HasSymlinks, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "link",
		TCL_XF86Link, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "unlink",
		TCL_XF86UnLink, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "sleep",
		TCL_XF86Sleep, (ClientData) NULL,
		(void (*)()) NULL);

	return TCL_OK;
}

TCL_XF86GetUID(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	int idx;

	if (argc != 1) {
		Tcl_SetResult(interp, "Usage: getuid", TCL_STATIC);
		return TCL_ERROR;
	}

	/* This is short, so we can write directly into the
	   pre-allocated buffer */
	sprintf(interp->result, "%d", getuid());
	return TCL_OK;
}

TCL_XF86ServerRunning(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Display *display;

	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: server_running <display>", TCL_STATIC);
		return TCL_ERROR;
	}

	if ((display = XOpenDisplay(argv[1])) == (Display *) NULL) {
		Tcl_SetResult(interp, "0", TCL_STATIC);
	} else {
		XCloseDisplay(display);
		Tcl_SetResult(interp, "1", TCL_STATIC);
	}
	return TCL_OK;
}

TCL_XF86ProcessRunning(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	Display *display;

	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: process_running <pid>", TCL_STATIC);
		return TCL_ERROR;
	}

	if (kill(atoi(argv[1]), 0) == 0) {
		Tcl_SetResult(interp, "0", TCL_STATIC);
	} else {
		Tcl_SetResult(interp, "1", TCL_STATIC);
	}
	return TCL_OK;
}

TCL_XF86HasSymlinks(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	if (argc != 1) {
		Tcl_SetResult(interp, "Usage: has_symlinks", TCL_STATIC);
		return TCL_ERROR;
	}

#ifdef S_IFLNK
	Tcl_SetResult(interp, "1", TCL_STATIC);
#else
	Tcl_SetResult(interp, "0", TCL_STATIC);
#endif
	return TCL_OK;
}

TCL_XF86Link(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	if (argc != 3) {
		Tcl_SetResult(interp, "Usage: link <oldfilename> <newfilename>", TCL_STATIC);
		return TCL_ERROR;
	}

#ifdef S_IFLNK
	if (symlink(argv[1], argv[2]) == -1)
#else
	if (link(argv[1], argv[2]) == -1)
#endif
		Tcl_SetResult(interp, "0", TCL_STATIC);
	else
		Tcl_SetResult(interp, "1", TCL_STATIC);
	return TCL_OK;
}

TCL_XF86UnLink(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: unlink <filename>", TCL_STATIC);
		return TCL_ERROR;
	}

	if (unlink(argv[1]) == -1)
		Tcl_SetResult(interp, "0", TCL_STATIC);
	else
		Tcl_SetResult(interp, "1", TCL_STATIC);
	return TCL_OK;
}

TCL_XF86Sleep(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: sleep <seconds>", TCL_STATIC);
		return TCL_ERROR;
	}

	sleep(atoi(argv[1]));
	return TCL_OK;
}

