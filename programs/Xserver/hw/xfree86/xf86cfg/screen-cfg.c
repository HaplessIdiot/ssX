/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86$
 */

#include "xf86config.h"
#include "screen-cfg.h"
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Toggle.h>

/*
 * Prototypes
 */
static void DepthCallback(Widget, XtPointer, XtPointer);
static void SelectIndexCallback(Widget, XtPointer, XtPointer);
static void UnselectIndexCallback(Widget, XtPointer, XtPointer);
static void SelectCallback(Widget, XtPointer, XtPointer);
static void UnselectCallback(Widget, XtPointer, XtPointer);

/*
 * Initialization
 */
static char *modes[] = {
    "640x400",
    "640x480",
    "800x600",
    "1024x768",
    "1280x1024",
    "320x200",
    "320x240",
    "400x300",
    "1152x864",
    "1600x1200",
    "1800x1400",
    "512x384",
};

static int default_depth, sel_index, unsel_index;
static Widget listL, listR;
static char **defmodes;
int ndefmodes;

/*
 * Implementation
 */
XtPointer
ScreenConfig(XtPointer conf)
{
    XF86ConfScreenPtr screen = (XF86ConfScreenPtr)conf;
    XF86ConfDisplayPtr disp;
    Arg args[2];
    int i;

    if (screen == NULL)
	return (NULL);

    XtSetArg(args[0], XtNstring, screen->scrn_identifier);
    XtSetValues(ident_widget, args, 1);
    if ((default_depth = screen->scrn_defaultdepth) <= 0)
	default_depth = 8;
    sel_index = unsel_index = -1;

    ndefmodes = 0;
    disp = screen->scrn_display_lst;
    while (disp != NULL) {
	if (disp->disp_depth == default_depth) {
	    XF86ModePtr mod = disp->disp_mode_lst;

	    while (mod != NULL) {
		if (ndefmodes % 16 == 0)
		    defmodes = (char**)
			XtRealloc((XtPointer)defmodes,
				  (ndefmodes + 16) * sizeof(char*));
		defmodes[ndefmodes++] = XtNewString(mod->mode_name);
		mod = (XF86ModePtr)(mod->list.next);
	    }
	    break;
	}
	disp = (XF86ConfDisplayPtr)(disp->list.next);
    }
    if (ndefmodes == 0) {
	defmodes = (char**)XtMalloc(sizeof(char*));
	defmodes[0] = XtNewString("640x480");
    }

    if (listL != NULL) {
	XawListUnhighlight(listL);
	XawListUnhighlight(listR);
    }

    xf86info.cur_list = SCREEN;
    XtSetSensitive(back, xf86info.lists[SCREEN].cur_function > 0);
    XtSetSensitive(next, xf86info.lists[SCREEN].cur_function <
			 xf86info.lists[SCREEN].num_functions - 1);
    (xf86info.lists[SCREEN].functions[xf86info.lists[SCREEN].cur_function])
	(&xf86info);

    if (ConfigLoop(NULL) == True) {
	XF86ModePtr prev, mod;

	if (disp == NULL) {
	    disp = (XF86ConfDisplayPtr)XtCalloc(1, sizeof(XF86ConfDisplayRec));
	    screen->scrn_display_lst = (XF86ConfDisplayPtr)
		addListItem((GenericListPtr)(screen->scrn_display_lst),
			    (GenericListPtr)(disp));
	}

	if (strcasecmp(screen->scrn_identifier, ident_string))
	    xf86RenameScreen(XF86Config, screen, ident_string);

	screen->scrn_defaultdepth = default_depth;

	XtSetArg(args[0], XtNlist, NULL);
	XtSetArg(args[1], XtNnumberStrings, 0);
	XtSetValues(listR, args, 2);

	mod = disp->disp_mode_lst;
	/* free all modes */
	while (mod != NULL) {
	    prev = mod;
	    mod = (XF86ModePtr)(mod->list.next);
	    XtFree(prev->mode_name);
	    XtFree((XtPointer)prev);
	}
	/* readd modes */
	for (i = 0; i < ndefmodes; i++) {
	    mod = XtNew(XF86ModeRec);
	    mod->mode_name = XtNewString(defmodes[i]);
	    XtFree(defmodes[i]);
	    if (i == 0)
		disp->disp_mode_lst = mod;
	    else
		prev->list.next = mod;
	    prev = mod;
	}
	if (i == 0)
	    disp->disp_mode_lst = NULL;
	else
	    mod->list.next = NULL;

	XtFree((XtPointer)defmodes);
	defmodes = NULL;
	ndefmodes = 0;

	return ((XtPointer)screen);
    }

    XtSetArg(args[0], XtNlist, NULL);
    XtSetArg(args[1], XtNnumberStrings, 0);
    XtSetValues(listR, args, 2);

    for (i = 0; i < ndefmodes; i++)
	XtFree(defmodes[i]);
    XtFree((XtPointer)defmodes);
    defmodes = NULL;
    ndefmodes = 0;

    return (NULL);
}

/*ARGSUSED*/
static void
DepthCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    default_depth = (int)user_data;
}

/*ARGSUSED*/
static void
SelectIndexCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    XawListReturnStruct *info = (XawListReturnStruct *)call_data;

    sel_index = info->list_index;
}

/*ARGSUSED*/
static void
UnselectIndexCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    XawListReturnStruct *info = (XawListReturnStruct *)call_data;

    unsel_index = info->list_index;
}

/*ARGSUSED*/
static void
SelectCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    Arg args[2];

    if (sel_index < 0 || sel_index >= sizeof(modes) / sizeof(modes[0]))
	return;

    if (ndefmodes == 1 && *defmodes[0] == '\0') {
	/* make sure tmp and defentries are not the same pointer */
	char **tmp = defmodes;

	XtFree(defmodes[0]);
	defmodes = (char**)XtMalloc(sizeof(char*));
	--ndefmodes;
	XtFree((char*)tmp);
    }
    else
	defmodes = (char**)XtRealloc((XtPointer)defmodes,
				     sizeof(char*) * (ndefmodes + 1));
    defmodes[ndefmodes++] = XtNewString(modes[sel_index]);

    XtSetArg(args[0], XtNlist, defmodes);
    XtSetArg(args[1], XtNnumberStrings, ndefmodes);
    XtSetValues(listR, args, 2);
    unsel_index = -1;
}

/*ARGSUSED*/
static void
UnselectCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    Arg args[2];

    if (unsel_index < 0 || unsel_index >= ndefmodes)
	return;

    XtFree(defmodes[unsel_index]);
    if (--ndefmodes > unsel_index)
	memmove(&defmodes[unsel_index], &defmodes[unsel_index + 1],
		(ndefmodes - unsel_index) * sizeof(char*));
    if (ndefmodes == 0) {
	char **tmp = defmodes;

	defmodes = (char**)XtMalloc(sizeof(char*));
	defmodes[0] = XtNewString("");
	ndefmodes = 1;
	XtFree((char*)tmp);
    }

    XtSetArg(args[0], XtNlist, defmodes);
    XtSetArg(args[1], XtNnumberStrings, ndefmodes);
    XtSetValues(listR, args, 2);
    unsel_index = -1;
}

void
ScreenDialog(XF86SetupInfo *info)
{
    static Widget dialog, d1, d4, d8, d16, d24;
    Arg args[2];

    if (dialog == NULL) {
	Widget command;

	dialog = XtCreateWidget("screenD", formWidgetClass,
				configp, NULL, 0);
	XtCreateManagedWidget("depthL", labelWidgetClass,
			      dialog, NULL, 0);
	d1 = XtCreateManagedWidget("1", toggleWidgetClass, dialog, NULL, 0);
	XtAddCallback(d1, XtNcallback, DepthCallback, (XtPointer)1);
	d4 = XtVaCreateManagedWidget("4", toggleWidgetClass, dialog,
				     XtNradioGroup, d1, NULL, 0);
	XtAddCallback(d1, XtNcallback, DepthCallback, (XtPointer)4);
	d8 = XtVaCreateManagedWidget("8", toggleWidgetClass, dialog,
				      XtNradioGroup, d4, NULL, 0);
	XtAddCallback(d1, XtNcallback, DepthCallback, (XtPointer)8);
	d16 = XtVaCreateManagedWidget("16", toggleWidgetClass, dialog,
				      XtNradioGroup, d8, NULL, 0);
	XtAddCallback(d1, XtNcallback, DepthCallback, (XtPointer)16);
	d24 = XtVaCreateManagedWidget("24", toggleWidgetClass, dialog,
				      XtNradioGroup, d16, NULL, 0);
	XtAddCallback(d1, XtNcallback, DepthCallback, (XtPointer)24);

	XtCreateManagedWidget("modeL", labelWidgetClass, dialog, NULL, 0);
	listL = XtVaCreateManagedWidget("listLeft", listWidgetClass, dialog,
					XtNlist, modes,
					XtNnumberStrings, XtNumber(modes),
					NULL, 0);
	XtAddCallback(listL, XtNcallback, SelectIndexCallback, NULL);
	command = XtCreateManagedWidget("select", commandWidgetClass,
					dialog, NULL, 0);
	XtAddCallback(command, XtNcallback, SelectCallback, NULL);
	command = XtCreateManagedWidget("unselect", commandWidgetClass,
					dialog, NULL, 0);
	XtAddCallback(command, XtNcallback, UnselectCallback, NULL);
	listR = XtCreateManagedWidget("listRight", listWidgetClass,
				      dialog, NULL, 0);
	XtAddCallback(listR, XtNcallback, UnselectIndexCallback, NULL);
	XtRealizeWidget(dialog);
    }

    XtSetArg(args[0], XtNlist, defmodes);
    XtSetArg(args[1], XtNnumberStrings, ndefmodes);
    XtSetValues(listR, args, 2);

    XtSetArg(args[0], XtNstate, True);
    XtSetValues(default_depth == 1 ? d1 :
		default_depth == 4 ? d4 :
		default_depth == 16 ? d16 :
		default_depth == 24 ? d24 : d8, args, 1);

    XtChangeManagedSet(&current, 1, NULL, NULL, &dialog, 1);
    current = dialog;
}
