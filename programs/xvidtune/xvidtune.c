/* $XFree86: xc/programs/vgahelp/vgahelp.c,v 3.2 1995/03/19 10:21:26 dawes Exp $ */

/*

Copyright (c) 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY.

*/

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/extensions/VGAHelp.h>
#include <stdio.h>

int MajorVersion, MinorVersion;

typedef enum { HDisplay, HSyncStart, HSyncEnd, HTotal,
	VDisplay, VSyncStart, VSyncEnd, VTotal, Flags, fields_num } fields;

typedef struct {
    fields	me;
    fields	use;
    int		val;
    int		lastpercent;
    int		range;
    Widget	textwidget;
    Widget	scrollwidget;
} ScrollData;

static struct _AppResources {
    ScrollData	field[fields_num];
    Bool	ad_installed;
    int		orig[fields_num];
} AppRes = {
    {
	{ HDisplay, },
	{ HSyncStart, HDisplay, },
	{ HSyncEnd, HDisplay, },
	{ HTotal, HDisplay, },
	{ VDisplay, },
	{ VSyncStart, VDisplay, },
	{ VSyncEnd, VDisplay, },
	{ VTotal, VDisplay, },
	{ Flags, }
    },
};

static XtResource Resources[] = {
    { "adInstalled", "AdInstalled", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _AppResources, ad_installed),
	XtRImmediate, (XtPointer)FALSE },
    { "hSyncStartRange", "SyncStartRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[HSyncStart].range), 
	XtRImmediate, (XtPointer)200 },
    { "hSyncEndRange", "SyncEndRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[HSyncEnd].range), 
	XtRImmediate, (XtPointer)400 },
    { "hTotalRange", "TotalRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[HTotal].range), 
	XtRImmediate, (XtPointer)400 },
    { "vSyncStartRange", "SyncStartRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[VSyncStart].range), 
	XtRImmediate, (XtPointer)20 },
    { "vSyncEndRange", "SyncEndRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[VSyncEnd].range), 
	XtRImmediate, (XtPointer)40 },
    { "vTotalRange", "TotalRange", XtRInt, sizeof(int),
	XtOffsetOf(struct _AppResources, field[VTotal].range), 
	XtRImmediate, (XtPointer)80 },
};

static Atom wm_delete_window;

static Bool GetModeLine (dpy, scrn)
    Display* dpy;
    int scrn;
{
    XVGAHelpModeLine mode_line;
    int dot_clock;
    fields i;

    if (!XVGAHelpGetModeLine (dpy, scrn, &dot_clock, &mode_line))
	return FALSE;

    AppRes.field[HDisplay].val = mode_line.hdisplay;
    AppRes.field[HSyncStart].val = mode_line.hsyncstart;
    AppRes.field[HSyncEnd].val = mode_line.hsyncend;
    AppRes.field[HTotal].val = mode_line.htotal;
    AppRes.field[VDisplay].val = mode_line.vdisplay;
    AppRes.field[VSyncStart].val = mode_line.vsyncstart;
    AppRes.field[VSyncEnd].val = mode_line.vsyncend;
    AppRes.field[VTotal].val = mode_line.vtotal;
    AppRes.field[Flags].val = mode_line.flags;
    for (i = HDisplay; i < fields_num; i++) 
	AppRes.orig[i] = AppRes.field[i].val;
    return TRUE;
}

static Bool GetMonitor (dpy, scrn)
    Display* dpy;
    int scrn;
{
    XVGAHelpMonitor monitor;
    int i;

    if (!XVGAHelpGetMonitor (dpy, scrn, &monitor))
	return FALSE;

    printf("Vendor: %s, Model: %s\n", monitor.vendor, monitor.model);
    printf("Num hsync: %d, Num vsync: %d\n", monitor.nhsync, monitor.nvsync);
    for (i = 0; i < monitor.nhsync; i++) {
	printf("hsync range %d: %6.2f - %6.2f\n", i, monitor.hsync[i].lo,
	       monitor.hsync[i].hi);
    }
    for (i = 0; i < monitor.nvsync; i++) {
	printf("vsync range %d: %6.2f - %6.2f\n", i, monitor.vsync[i].lo,
	       monitor.vsync[i].hi);
    }
    return TRUE;
}

static void SetScrollbars ()
{
    fields i;

    for (i = HDisplay; i <= Flags; i++) {

	ScrollData* sdp = &AppRes.field[i];

	if (sdp->scrollwidget != (Widget) NULL) {
	    int base;
	    float percent;

	    base = AppRes.field[sdp->use].val;
	    percent = ((float)(sdp->val - base)) / ((float)sdp->range);
	    XawScrollbarSetThumb (sdp->scrollwidget, percent, 0.0);
	}
    }
}

static void QuitCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
#if XtSpecificationRelease < 6
    exit (0);
#else
    XtAppSetExitFlag (XtWidgetToApplicationContext (w));
#endif
}

static void ApplyCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    XVGAHelpModeLine mode_line;
    char* string;
    int i;

    mode_line.hdisplay = AppRes.field[HDisplay].val;
    mode_line.hsyncstart = AppRes.field[HSyncStart].val;
    mode_line.hsyncend = AppRes.field[HSyncEnd].val;
    mode_line.htotal = AppRes.field[HTotal].val;
    mode_line.vdisplay = AppRes.field[VDisplay].val;
    mode_line.vsyncstart = AppRes.field[VSyncStart].val;
    mode_line.vsyncend = AppRes.field[VSyncEnd].val;
    mode_line.vtotal = AppRes.field[VTotal].val;
    XtVaGetValues (AppRes.field[Flags].textwidget,
		XtNstring, &string, NULL);
    (void) sscanf (string, "%x", &i);
    mode_line.flags = i;

    XVGAHelpModModeLine (XtDisplay (w), DefaultScreen (XtDisplay (w)), 
		&mode_line);
}

static void FetchCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    fields i;
    (void) GetModeLine(XtDisplay (w), DefaultScreen (XtDisplay (w)));
    SetScrollbars ();
    for (i = HDisplay; i < fields_num; i++) { 
	ScrollData* sdp = &AppRes.field[i];

	if (sdp->textwidget != (Widget) NULL) {
	    char buf[6];

	    (void) sprintf (buf, i == Flags ? "%04x" : "%5d", sdp->val);
	    sdp->lastpercent = -1;
	    if (i == Flags) {
		XawTextBlock text;

		text.firstPos = 0;
		text.length = 4;
		text.ptr = buf;
		text.format = XawFmt8Bit;
		XawTextReplace (sdp->textwidget, 0, 4, &text);
	    } else 
		XtVaSetValues (sdp->textwidget, XtNlabel, buf, NULL);
	}
    }
}

static void TestTO (client, id)
    XtPointer client;
    XtIntervalId* id;
{
    fields i;
    for (i = HDisplay; i < fields_num; i++)
	AppRes.field[i].val = AppRes.orig[i];

    SetScrollbars ();

    ApplyCB ((Widget) client, NULL, NULL);

}

static void TestCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    XtAppAddTimeOut (XtWidgetToApplicationContext (w),
		5000, TestTO, (XtPointer) w);

    ApplyCB (w, client, call);
}

#if 0
static void EditCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    int base, current, i, len;
    int lower, upper;
    float percent;
    ScrollData* sdp = (ScrollData*) client;

    len = strlen (sdp->string);

    for (i = 0; i < len; i++) {
	if (!(isdigit (sdp->string[i]) || isspace (sdp->string[i]))) {
	    XBell (XtDisplay(XtParent(w)), 100);
	    return;
	}
    }
    switch (sdp->me) {
    case HSyncStart: 
	lower = atoi (AppRes.field[HDisplay].string);
	upper = atoi (AppRes.field[HSyncEnd].string);
	break;

    case HSyncEnd:
	lower = atoi (AppRes.field[HSyncStart].string);
	upper = atoi (AppRes.field[HTotal].string);
	break;

    case HTotal:
	lower = atoi (AppRes.field[HSyncEnd].string);
	upper = atoi (AppRes.field[HDisplay].string) + 
		AppRes.field[HTotal].range;
	break;

    case VSyncStart: 
	lower = atoi (AppRes.field[VDisplay].string);
	upper = atoi (AppRes.field[VSyncEnd].string);
	break;

    case VSyncEnd:
	lower = atoi (AppRes.field[VSyncStart].string);
	upper = atoi (AppRes.field[VTotal].string);
	break;

    case VTotal:
	lower = atoi (AppRes.field[VSyncEnd].string);
	upper = atoi (AppRes.field[VDisplay].string) + 
		AppRes.field[VTotal].range;
	break;
    }
    current = atoi (sdp->string);
    if (current < lower || current > upper) {
	XawTextBlock text;
	char tmp[6];

	if (current < lower) {
	    (void) sprintf (tmp, "%5d", lower);
	    current = lower;
	} else {
	    (void) sprintf (tmp, "%5d", upper);
	    current = upper;
	}
	text.firstPos = 0;
	text.length = strlen (tmp);
	text.ptr = tmp;
	text.format = XawFmt8Bit;
	XawTextReplace (sdp->textwidget, 0, text.length, &text);
    }
    base = atoi (AppRes.field[sdp->use].string);
    percent = ((float)(current - base)) / ((float)sdp->range);
    XawScrollbarSetThumb (sdp->scrollwidget, percent, 0.0);
}
#endif

static void FlagsEditCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    int i, len;
    char* string;
    fields findex = (fields) client;
    ScrollData* sdp = &AppRes.field[findex];

    XtVaGetValues (w, XtNstring, &string, NULL);
    len = strlen (string);
    if (len > 4) {
	XawTextBlock text;
	char buf[5];

	XBell (XtDisplay(XtParent(w)), 100);
	(void) strncpy (buf, string, 4);
	XtVaSetValues (sdp->textwidget, XtNstring, buf, NULL);
	XawTextSetInsertionPoint (sdp->textwidget, 4);
    }

    for (i = 0; i < len; i++) {
	if (!isxdigit (string[i])) {
	    XBell (XtDisplay(XtParent(w)), 100);
	}
    }
}

static void ScrollCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    float percent = *(float*) call;
    int ipercent = percent * 100;
    int fieldindex = (int) client;
    ScrollData* sdp = &AppRes.field[fieldindex];

    if (ipercent != sdp->lastpercent) {

	char buf[6];

	sdp->val = AppRes.field[sdp->use].val;
	sdp->val += (int) (((float)sdp->range) * percent);
	sdp->lastpercent = ipercent;
	(void) sprintf (buf, "%5d", sdp->val);
	XtVaSetValues (sdp->textwidget, XtNlabel, buf, NULL);
    }
}

static void AddCallback (w, callback_name, callback, client_data)
    Widget w;
    String  callback_name;
    XtCallbackProc callback;
    XtPointer client_data;
{
    Widget src;

    XtVaGetValues (w, XtNtextSource, &src, NULL);
    XtAddCallback (src, callback_name, callback, client_data);
}

static void CreateTyp (form, findex, w1name, w2name, w3name)
    Widget form;
    fields findex;
    String w1name;
    String w2name;
    String w3name;
{
    Widget wids[3];
    char buf[6];

    wids[0] = XtCreateWidget (w1name, labelWidgetClass, form, NULL, 0);
    (void) sprintf (buf, "%5d", AppRes.field[findex].val);
    wids[1] = XtVaCreateWidget (w2name, labelWidgetClass,
		form, XtNlabel, buf, NULL);
    if (w3name != NULL) {
	wids[2] = XtCreateWidget (w3name, scrollbarWidgetClass, form, NULL, 0);
	XtAddCallback (wids[2], XtNjumpProc, ScrollCB, (XtPointer) findex);
	XtManageChildren (wids, 3);
    } else {
	wids[2] = (Widget) NULL;
	XtManageChildren (wids, 2);
    }
    AppRes.field[findex].textwidget = wids[1];
    AppRes.field[findex].scrollwidget = wids[2];
}

static void CreateHierarchy(top)
    Widget top;
{
    char buf[5];
    Widget form, forms[10];
    Widget wids[4];
    XawTextBlock text;
    XtTranslations trans;
    int i;
    static String form_names[] = {
	"HDisplay-form",
	"HSyncStart-form",
	"HSyncEnd-form",
	"HTotal-form",
	"VDisplay-form",
	"VSyncStart-form",
	"VSyncEnd-form",
	"VTotal-form",
	"Flags-form",
	"Buttons-form" } ;

    form = XtCreateWidget ("form", formWidgetClass, top, NULL, 0);

    for (i = 0; i < 10; i++)
	forms[i] = XtCreateWidget (form_names[i], formWidgetClass, 
		form, NULL, 0);

    CreateTyp (forms[0], HDisplay, "HDisplay-label", "HDisplay-text", NULL);
    CreateTyp (forms[1], HSyncStart, "HSyncStart-label",
		"HSyncStart-text", "HSyncStart-scrollbar");
    CreateTyp (forms[2], HSyncEnd, "HSyncEnd-label", "HSyncEnd-text", 
		"HSyncEnd-scrollbar");
    CreateTyp (forms[3], HTotal, "HTotal-label", "HTotal-text", 
		"HTotal-scrollbar");
    CreateTyp (forms[4], VDisplay, "VDisplay-label", "VDisplay-text", NULL);
    CreateTyp (forms[5], VSyncStart, "VSyncStart-label",
		"VSyncStart-text", "VSyncStart-scrollbar");
    CreateTyp (forms[6], VSyncEnd, "VSyncEnd-label", "VSyncEnd-text", 
		"VSyncEnd-scrollbar");
    CreateTyp (forms[7], VTotal, "VTotal-label", "VTotal-text", 
		"VTotal-scrollbar");

    trans = XtParseTranslationTable ("\
	<Key>0: insert-char()\n<Key>1: insert-char()\n\
	<Key>2: insert-char()\n<Key>3: insert-char()\n\
	<Key>4: insert-char()\n<Key>5: insert-char()\n\
	<Key>6: insert-char()\n<Key>7: insert-char()\n\
	<Key>8: insert-char()\n<Key>9: insert-char()\n\
	<Key>a: insert-char()\n<Key>b: insert-char()\n\
	<Key>c: insert-char()\n<Key>d: insert-char()\n\
	<Key>e: insert-char()\n<Key>f: insert-char()\n\
	<Key>BackSpace: delete-previous-character()\n\
	<Key>Right: forward-character()\n<Key>KP_Right: forward-character()\n\
	<Key>Left: backward-character()\n<Key>KP_Left: backward-character()\n\
	<Key>Delete: delete-previous-character()\n\
	<Key>KP_Delete: delete-previous-character()\n\
	<EnterWindow>: enter-window()\n<LeaveWindow>: leave-window()\n\
	<FocusIn>: focus-in()\n<FocusOut>: focus-out()\n\
	<Btn1Down>: select-start()\n");
    (void) sprintf (buf, "%04x", AppRes.field[Flags].val);
    wids[0] = XtCreateWidget ("Flags-label", labelWidgetClass,
		forms[8], NULL, 0);
    wids[1] = XtVaCreateWidget ("Flags-text", asciiTextWidgetClass,
		forms[8], XtNstring, buf, XtNtranslations, trans, NULL);
    AddCallback (wids[1], XtNcallback, FlagsEditCB, (XtPointer) Flags);
    XtManageChildren (wids, 2);
    AppRes.field[Flags].textwidget = wids[1];

    wids[0] = XtCreateWidget ("Quit-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[0], XtNcallback, QuitCB, NULL);
    wids[1] = XtCreateWidget ("Fetch-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[1], XtNcallback, FetchCB, NULL);
    wids[2] = XtCreateWidget ("Apply-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[2], XtNcallback, ApplyCB, NULL);
    wids[3] = XtCreateWidget ("Test-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[3], XtNcallback, TestCB, NULL);
    XtManageChildren (wids, 4);

    XtManageChildren (forms, 10);
    XtManageChild (form);

    SetScrollbars ();
}

static void QuitAction (w, e, vector, count)
    Widget w;
    XEvent* e;
    String* vector;
    Cardinal* count;
{
    if (e->type == ClientMessage && e->xclient.data.l[0] == wm_delete_window)
	QuitCB(w, NULL, NULL);
}

int main (argc, argv)
    int argc;
    char** argv;
{
    Widget top;
    XtAppContext app;
    Display* dpy;
    static XtActionsRec actions[] = { { "vgahelp-quit", QuitAction } };

    top = XtVaOpenApplication (&app, "Vgahelp", NULL, 0, &argc, argv,
		NULL, applicationShellWidgetClass, 
		XtNmappedWhenManaged, False, NULL);

    XtGetApplicationResources (top, (XtPointer)&AppRes,
		Resources, XtNumber(Resources),
		NULL, 0);

    if (!AppRes.ad_installed) {
	(void) printf ("%s\n", "Please install the program before using");
	return 0;
    }

    if (!XVGAHelpQueryVersion(XtDisplay (top), &MajorVersion, &MinorVersion))
	return 0;

    if (MinorVersion > 0 || MajorVersion > 0) {
	if (argc > 1) {
	    int i = 0;
	    if (!strcmp(argv[1], "-next"))
		i = 1;
	    else if (!strcmp(argv[1], "-prev"))
		i = -1;
	    if (i != 0) {
		XVGAHelpSwitchMode(XtDisplay (top),
				   DefaultScreen (XtDisplay (top)), i);
		XSync(XtDisplay (top), True);
		return 0;
	    }
	}
	if (!GetMonitor(XtDisplay (top), DefaultScreen (XtDisplay (top))))
	    return 0;
    } else {
	fprintf(stderr, "Warning, Xserver is running old VGAHelp (%d.%d)\n",
		MajorVersion, MinorVersion);
    }

    if (!GetModeLine(XtDisplay (top), DefaultScreen (XtDisplay (top))))
	return 0;


    CreateHierarchy (top);

    XtAppAddActions (app, actions, XtNumber(actions));

    XtOverrideTranslations (top,
		XtParseTranslationTable ("<Message>WM_PROTOCOLS: vgahelp-quit()"));

    XtRealizeWidget (top);

    dpy = XtDisplay(top);

    wm_delete_window = XInternAtom (dpy, "WM_DELETE_WINDOW", False);

    (void) XSetWMProtocols (dpy, XtWindow (top), &wm_delete_window, 1);

    XtMapWidget (top);

    XtAppMainLoop (app);

    return 0;
}
