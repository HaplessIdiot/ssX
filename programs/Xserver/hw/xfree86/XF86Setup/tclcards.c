
/*

  This file contains Tcl bindings to the Cards database routines

 */

#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include <tk.h>

#include "cards.h"

#include <X11/Intrinsic.h>
#include <X11/Xmd.h>
#include <X11/Xos.h>

int TCL_XF86GetCardList(
#if NeedNestedPrototypes
    ClientData	clientData,
    Tcl_Interp	*interp,
    int		argc,
    char	**argv
#endif
);

int TCL_XF86GetCardEntry(
#if NeedNestedPrototypes
    ClientData	clientData,
    Tcl_Interp	*interp,
    int		argc,
    char	**argv
#endif
);

/*
   Adds all the Cards database specific commands to the Tcl interpreter
*/

Cards_Init(interp)
    Tcl_Interp	*interp;
{
	if (parse_database()) {
		Tcl_AppendResult(interp, "Couldn't read card database file ",
			CARD_DATABASE_FILE, (char *) NULL);
		return TCL_ERROR;
	}

	Tcl_CreateCommand(interp, "xf86cards_getlist",
		TCL_XF86GetCardList, (ClientData) NULL,
		(void (*)()) NULL);

	Tcl_CreateCommand(interp, "xf86cards_getentry",
		TCL_XF86GetCardEntry, (ClientData) NULL,
		(void (*)()) NULL);

	return TCL_OK;
}

/*
   Implements the xf86cards_getlist command which returns
   a list of the cards in the database
*/

TCL_XF86GetCardList(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	int i;
	char **av;

	if (argc != 1) {
		Tcl_SetResult(interp, "Usage: xf86cards_getlist", TCL_STATIC);
		return TCL_ERROR;
	}

	av = (char **) XtMalloc(sizeof(char *) * (lastcard+2));

	for (i = 0; i <= lastcard; i++) {
		av[i] = card[i].name;
	}
	av[i] = (char *) NULL;
	Tcl_SetResult(interp, Tcl_Merge(i, av), TCL_DYNAMIC);
	XtFree((char *) av);
	return TCL_OK;
}

/*
   Implements the xf86cards_getentry command which returns
   the full info on the given card
*/

TCL_XF86GetCardEntry(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	int idx;
	char tmpbuf[64];
	char *av[9];

	if (argc != 2) {
		Tcl_SetResult(interp, "Usage: xf86cards_getentry cardname",
				TCL_STATIC);
		return TCL_ERROR;
	}

	if ((idx = lookupcard(argv[1])) == -1) {
		Tcl_SetResult(interp, "Cannot find card", TCL_STATIC);
		return TCL_ERROR;
	}

	av[0] = card[idx].name;
	av[1] = card[idx].chipset;
	av[2] = card[idx].server;
	av[3] = card[idx].ramdac;
	av[4] = card[idx].clockchip;
	av[5] = card[idx].dacspeed;
	av[6] = card[idx].lines;

	tmpbuf[0] = '\0';
	if (card[idx].flags & NOCLOCKPROBE)
		strcat(tmpbuf, "NOCLOCKPROBE ");
	if (card[idx].flags & UNSUPPORTED)
		strcat(tmpbuf, "UNSUPPORTED ");
	if (tmpbuf[0] != '\0')
		tmpbuf[strlen(tmpbuf)-1] = '\0';
	av[7] = tmpbuf;
	av[8] = (char *) NULL;
	
	Tcl_SetResult(interp, Tcl_Merge(8, av), TCL_DYNAMIC);
	return TCL_OK;
}

