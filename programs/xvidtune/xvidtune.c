/* $XFree86: xc/programs/xvidtune/xvidtune.c,v 3.9 1995/06/10 13:33:21 dawes Exp $ */

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
#include <X11/Xaw/Box.h>
#include <X11/extensions/xf86vmode.h>
#include <stdio.h>
#include <signal.h>

int MajorVersion, MinorVersion;
int EventBase, ErrorBase;
int dot_clock, mode_flags;

/* Minimum extension version required */
#define MINMAJOR 0
#define MINMINOR 2

/* Mode flags -- ignore flags not in V_FLAG_MASK */
#define V_FLAG_MASK	0x1FF;
#define V_PHSYNC	0x001 
#define V_NHSYNC	0x002
#define V_PVSYNC	0x004
#define V_NVSYNC	0x008
#define V_INTERLACE	0x010 
#define V_DBLSCAN	0x020
#define V_CSYNC		0x040
#define V_PCSYNC	0x080
#define V_NCSYNC	0x100

typedef enum { HDisplay, HSyncStart, HSyncEnd, HTotal,
	VDisplay, VSyncStart, VSyncEnd, VTotal, Flags, 
	PixelClock, HSyncRate, VSyncRate, fields_num } fields;

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
    int		old[fields_num];
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
	{ Flags, },
	{ PixelClock, },
	{ HSyncRate, },
	{ VSyncRate, },
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
static Widget invalid_mode_popup;
static Widget testing_popup;
static Widget Top;

static void UpdateSyncRates();

static void CleanUp(dpy)
    Display *dpy;
{
    /* Make sure mode switching is not locked out at exit */
    XF86VidModeLockModeSwitch(dpy, DefaultScreen(dpy), FALSE);
    XFlush(dpy);
}

static void CatchSig(signal)
    int signal;
{
    CleanUp(XtDisplay(Top));
    exit(2);
}

static Bool GetModeLine (dpy, scrn)
    Display* dpy;
    int scrn;
{
    XF86VidModeModeLine mode_line;
    fields i;

    if (!XF86VidModeGetModeLine (dpy, scrn, &dot_clock, &mode_line))
	return FALSE;

    AppRes.field[HDisplay].val = mode_line.hdisplay;
    AppRes.field[HSyncStart].val = mode_line.hsyncstart;
    AppRes.field[HSyncEnd].val = mode_line.hsyncend;
    AppRes.field[HTotal].val = mode_line.htotal;
    AppRes.field[VDisplay].val = mode_line.vdisplay;
    AppRes.field[VSyncStart].val = mode_line.vsyncstart;
    AppRes.field[VSyncEnd].val = mode_line.vsyncend;
    AppRes.field[VTotal].val = mode_line.vtotal;
    mode_flags = mode_line.flags;
    AppRes.field[Flags].val = mode_flags & V_FLAG_MASK;
    AppRes.field[PixelClock].val = dot_clock;
    UpdateSyncRates(FALSE);

    for (i = HDisplay; i < fields_num; i++) 
	AppRes.orig[i] = AppRes.field[i].val;
    return TRUE;
}

static Bool GetMonitor (dpy, scrn)
    Display* dpy;
    int scrn;
{
    XF86VidModeMonitor monitor;
    int i;

    if (!XF86VidModeGetMonitor (dpy, scrn, &monitor))
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

static int hitError = 0;
static int (*xtErrorfunc)();

static int vidmodeError(dis, err)
Display *dis;
XErrorEvent *err;
{
  if (err->error_code >= ErrorBase &&
      err->error_code < ErrorBase + XF86VidModeNumberErrors) {
     hitError=1;
  } else {
     CleanUp(dis);
     if (xtErrorfunc) 
	(*xtErrorfunc)(dis, err);
  }
  return 0; /* ignored */
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
    CleanUp(XtDisplay(w));
#if XtSpecificationRelease < 6
    exit (0);
#else
    XtAppSetExitFlag (XtWidgetToApplicationContext (w));
#endif
}

popdownInvalid(w, client, call)
    Widget w;
    XtPointer client, call;
{
   XtPopdown((Widget)client);
}

static void ApplyCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    XF86VidModeModeLine mode_line;
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
    /* Don't read flags from widget */
#if 0
    XtVaGetValues (AppRes.field[Flags].textwidget,
		XtNstring, &string, NULL);
    (void) sscanf (string, "%x", &i);
#endif
    mode_line.flags = mode_flags;
    
   hitError = 0;

   XF86VidModeModModeLine (XtDisplay (w), DefaultScreen (XtDisplay (w)), 
		&mode_line);
   XSync(XtDisplay (w), False); /* process errors  */
   if (hitError) {
       XBell(XtDisplay (w), 80);
       XtPopup(invalid_mode_popup, XtGrabExclusive /*XtGrabNone*/);
   }
}


static void SetLabel(i)
fields i;
{
   ScrollData* sdp = &AppRes.field[i];

   if (sdp->textwidget != (Widget) NULL) {
      char buf[10];

      if (i == Flags)
	 (void) sprintf (buf, "%04x", sdp->val);
      else if (i >= PixelClock && i <= VSyncRate)
	 (void) sprintf (buf, "%6.2f", (float)sdp->val / 1000.0);
      else
	 (void) sprintf (buf, "%5d", sdp->val);
	 
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

static void UpdateSyncRates(dolabels)
    Bool dolabels;
{
    AppRes.field[HSyncRate].val = AppRes.field[PixelClock].val * 1000 /
				  AppRes.field[HTotal].val;
    AppRes.field[VSyncRate].val = AppRes.field[HSyncRate].val * 1000 /
				  AppRes.field[VTotal].val;
    if (mode_flags & V_INTERLACE)
	AppRes.field[VSyncRate].val *= 2;
    else if (mode_flags & V_DBLSCAN)
	AppRes.field[VSyncRate].val /= 2;
    if (dolabels) {
	SetLabel(HSyncRate);
	SetLabel(VSyncRate);
    }
}

static void RestoreCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    fields i;
    for (i = HDisplay; i < fields_num; i++) {
	AppRes.field[i].val = AppRes.orig[i];
	SetLabel(i);
    }
    SetScrollbars ();
}


static void FetchCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    fields i;
    (void) GetModeLine(XtDisplay (w), DefaultScreen (XtDisplay (w)));
    SetScrollbars ();
    for (i = HDisplay; i < fields_num; i++) {
        SetLabel(i);
    }   
}

static XtIntervalId TOid;

static void TestTO (client, id)
    XtPointer client;
    XtIntervalId* id;
{
    fields i;
    for (i = HDisplay; i < fields_num; i++)
	AppRes.field[i].val = AppRes.orig[i];

    ApplyCB ((Widget) client, NULL, NULL);

    for (i = HDisplay; i < fields_num; i++)
	AppRes.field[i].val = AppRes.old[i];
    SetScrollbars ();

    XtPopdown(testing_popup);
}

static void TestTOCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
  XtRemoveTimeOut(TOid);
  TestTO(w, (XtIntervalId *) NULL);
}

static void TestCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    fields i;
    for (i = HDisplay; i < fields_num; i++)
	AppRes.old[i] = AppRes.field[i].val;

    XtPopup(testing_popup, XtGrabExclusive /*XtGrabNone*/);
    XSync(XtDisplay(w), False);   
    TOid = XtAppAddTimeOut (XtWidgetToApplicationContext (w),
		5000, TestTO, (XtPointer) w);

    ApplyCB (w, client, call);
}

static void ShowCB(w, client, call)
    Widget w;
    XtPointer client, call;
{
    printf("\"%dx%d\"\t%.2f\t%d %d %d %d\t%d %d %d %d",
	   AppRes.field[HDisplay].val, AppRes.field[VDisplay].val,
	   (float)dot_clock/1000.0,
	   AppRes.field[HDisplay].val,
	   AppRes.field[HSyncStart].val,
	   AppRes.field[HSyncEnd].val,
	   AppRes.field[HTotal].val,
	   AppRes.field[VDisplay].val,
	   AppRes.field[VSyncStart].val,
	   AppRes.field[VSyncEnd].val,
	   AppRes.field[VTotal].val);
    /* Print out the flags (if any) */
    if (mode_flags & V_PHSYNC) printf(" +hsync");
    if (mode_flags & V_NHSYNC) printf(" -hsync");
    if (mode_flags & V_PVSYNC) printf(" +vsync");
    if (mode_flags & V_NVSYNC) printf(" -vsync");
    if (mode_flags & V_INTERLACE) printf(" interlace");
    if (mode_flags & V_CSYNC) printf(" composite");
    if (mode_flags & V_PCSYNC) printf(" +csync");
    if (mode_flags & V_PCSYNC) printf(" -csync");
    if (mode_flags & V_DBLSCAN) printf(" doublescan");
    printf("\n");
}


static void AdjustCB(w, client, call)
    Widget w;
    XtPointer client, call;
{
   int what = (int) client;
   
   switch (what) {
    case HSyncStart:
      if (AppRes.field[HSyncEnd].val + 4 < AppRes.field[HTotal].val) {
	 AppRes.field[HSyncEnd].val += 4;
	 AppRes.field[HSyncStart].val += 4;
	 SetLabel(HSyncStart);	 
	 SetLabel(HSyncEnd);		 
      } else
	XBell(XtDisplay(w), 80);
      break;
    case -HSyncStart:
      if (AppRes.field[HSyncStart].val - 4 > AppRes.field[HDisplay].val) {
	 AppRes.field[HSyncEnd].val -= 4;
	 AppRes.field[HSyncStart].val -= 4;
	 SetLabel(HSyncStart);	 
	 SetLabel(HSyncEnd);	 	 	 
      } else
	XBell(XtDisplay(w), 80);
      break;
    case HTotal:
      AppRes.field[HTotal].val += 4;
      SetLabel(HTotal);	       
      UpdateSyncRates(TRUE);
      break;      
    case -HTotal:
      if (AppRes.field[HTotal].val - 4 >  AppRes.field[HSyncEnd].val) {	 
	AppRes.field[HTotal].val -= 4;
	SetLabel(HTotal);	 
	UpdateSyncRates(TRUE);
      } else
	XBell(XtDisplay(w), 80);
      break;
    case VSyncStart:
      if (AppRes.field[VSyncEnd].val + 4 < AppRes.field[VTotal].val) {
	 AppRes.field[VSyncEnd].val += 4;
	 AppRes.field[VSyncStart].val += 4;
	 SetLabel(VSyncStart);	 
	 SetLabel(VSyncEnd); 	 
      } else
	XBell(XtDisplay(w), 80);
      break;
    case -VSyncStart:
      if (AppRes.field[VSyncStart].val - 4 > AppRes.field[VDisplay].val) {
	 AppRes.field[VSyncEnd].val -= 4;
	 AppRes.field[VSyncStart].val -= 4;
	 SetLabel(VSyncStart);	 
	 SetLabel(VSyncEnd);	 	 
      } else
	XBell(XtDisplay(w), 80);
      break;
    case VTotal:
      AppRes.field[VTotal].val += 4;
      SetLabel(VTotal);      
      UpdateSyncRates(TRUE);
      break;      
    case -VTotal:
      if (AppRes.field[VTotal].val - 4 >  AppRes.field[VSyncEnd].val) {	 
	AppRes.field[VTotal].val -= 4;
	SetLabel(VTotal);
	UpdateSyncRates(TRUE);
      } else
	XBell(XtDisplay(w), 80);
      break;
   }  
   SetScrollbars ();
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

static int isValid(val, field)
int val, field;
{
   switch(field) {
     case HSyncStart:
	if (val+8 > AppRes.field[HSyncEnd].val)
	   val = AppRes.field[HSyncEnd].val - 8;
        break;
     case HSyncEnd:
        if (val-8 < AppRes.field[HSyncStart].val)
	    val = AppRes.field[HSyncStart].val + 8;
        if (val > AppRes.field[HTotal].val)
	    val = AppRes.field[HTotal].val;
        break;
     case HTotal:
	if (val < AppRes.field[HSyncEnd].val)
	   val = AppRes.field[HSyncEnd].val;
         break;
     case VSyncStart:
	if (val+8 > AppRes.field[VSyncEnd].val)
	   val = AppRes.field[VSyncEnd].val - 8;
        break;
     case VSyncEnd:
        if (val-8 < AppRes.field[VSyncStart].val)
	    val = AppRes.field[VSyncStart].val + 8;
        if (val > AppRes.field[VTotal].val)
	    val = AppRes.field[VTotal].val;
        break;
     case VTotal:
	if (val < AppRes.field[VSyncEnd].val)
	   val = AppRes.field[VSyncEnd].val;
        break;
   }
   return val;
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
        int tmp_val;
	char buf[6];

	tmp_val = AppRes.field[sdp->use].val;
	tmp_val += (int) (((float)sdp->range) * percent);

        sdp->val = isValid(tmp_val, fieldindex);
        
	sdp->lastpercent = ipercent;
	(void) sprintf (buf, "%5d", sdp->val);
	XtVaSetValues (sdp->textwidget, XtNlabel, buf, NULL);
        if (sdp->val != tmp_val) {
            int base;
            float percent;

            base = AppRes.field[sdp->use].val;
            percent = ((float)(sdp->val - base)) / ((float)sdp->range);
            /* This doesn't always work, why? */
            XawScrollbarSetThumb (sdp->scrollwidget, percent, 0.0);
	}
	if (fieldindex == HTotal || fieldindex == VTotal)
	    UpdateSyncRates(TRUE);
    }
}

static void SwitchCB (w, client, call)
    Widget w;
    XtPointer client, call;
{
    XF86VidModeLockModeSwitch(XtDisplay(w), DefaultScreen (XtDisplay (w)),
			      FALSE);
    XF86VidModeSwitchMode(XtDisplay(w), DefaultScreen (XtDisplay (w)),
			  (int)client);
    XF86VidModeLockModeSwitch(XtDisplay(w), DefaultScreen (XtDisplay (w)),
			      TRUE);
    FetchCB(w, NULL, NULL);
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
    char buf[10];

    wids[0] = XtCreateWidget (w1name, labelWidgetClass, form, NULL, 0);
    if (findex >= PixelClock && findex <= VSyncRate)
	(void) sprintf(buf, "%6.2f", (float)AppRes.field[findex].val / 1000.0);
    else
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


static void AckWarn (w, client, call)
    Widget w;
    XtPointer client, call; 
{
    XtPopdown((Widget) client);
    XtDestroyWidget((Widget) client);
}

static void displayWarning(top)
    Widget top;
{
    Widget w, popup, popupBox;
    int x, y;

    x =  DisplayWidth(XtDisplay (top),DefaultScreen (XtDisplay (top))) / 3;
    y =  DisplayHeight(XtDisplay (top),DefaultScreen (XtDisplay (top))) / 3;

    popup = XtVaCreatePopupShell("Warning", 
			    transientShellWidgetClass, top,
			    XtNtitle, "WARNING",
			    XtNx, x,
			    XtNy, y,
			    NULL);

    popupBox = XtVaCreateManagedWidget(
               "WarningBox",
               boxWidgetClass,
               popup,
               NULL);

    w = XtVaCreateManagedWidget( "WarnLabel",
                                     labelWidgetClass,
				     popupBox,
                                     NULL);

    w = XtVaCreateManagedWidget( "WarnOK",
                                     commandWidgetClass,
				     popupBox,
                                     NULL);

    XtAddCallback (w, XtNcallback, AckWarn, (XtPointer)popup);

    w = XtVaCreateManagedWidget( "WarnCancel",
                                     commandWidgetClass,
				     popupBox,
                                     NULL);
    XtAddCallback (w, XtNcallback, QuitCB, (XtPointer)NULL);

    XtPopup(popup, XtGrabExclusive);
    
}



static void CreateHierarchy(top)
    Widget top;
{
    char buf[5];
    Widget form, forms[14];
    Widget wids[8];
    Widget boxW,messageW, popdownW, w;   
    XawTextBlock text;
    XtTranslations trans;
    int i;
    int x, y;
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
	"Buttons-form",
	"PixelClock-form",
	"HSyncRate-form",
	"VSyncRate-form",
	"Buttons2-form"
	};

    form = XtCreateWidget ("form", formWidgetClass, top, NULL, 0);

    for (i = 0; i < 14; i++)
	forms[i] = XtCreateWidget (form_names[i], formWidgetClass, 
		form, NULL, 0);

    CreateTyp (forms[0], HDisplay, "HDisplay-label", "HDisplay-text", NULL);
    CreateTyp (forms[1], HSyncStart, "HSyncStart-label",
		"HSyncStart-text", "HSyncStart-scrollbar");
    CreateTyp (forms[2], HSyncEnd, "HSyncEnd-label", "HSyncEnd-text", 
		"HSyncEnd-scrollbar");
    CreateTyp (forms[3], HTotal, "HTotal-label", "HTotal-text", 
		"HTotal-scrollbar");

    w = XtVaCreateManagedWidget(
                                     "Left-button",
                                     commandWidgetClass,
                                     forms[3],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)HSyncStart);
    w = XtVaCreateManagedWidget(
                                     "Right-button",
                                     commandWidgetClass,
                                     forms[3],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)-HSyncStart);
    w=  XtVaCreateManagedWidget(
                                     "Wider-button",
                                     commandWidgetClass,
                                     forms[3],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)-HTotal);
    w = XtVaCreateManagedWidget(
                                     "Narrower-button",
                                     commandWidgetClass,
                                     forms[3],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)HTotal);
    CreateTyp (forms[4], VDisplay, "VDisplay-label", "VDisplay-text", NULL);
    CreateTyp (forms[5], VSyncStart, "VSyncStart-label",
		"VSyncStart-text", "VSyncStart-scrollbar");
    CreateTyp (forms[6], VSyncEnd, "VSyncEnd-label", "VSyncEnd-text", 
		"VSyncEnd-scrollbar");
    CreateTyp (forms[7], VTotal, "VTotal-label", "VTotal-text", 
		"VTotal-scrollbar");
    w = XtVaCreateManagedWidget(
                                     "Up-button",
                                     commandWidgetClass,
                                     forms[7],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)VSyncStart);
    w = XtVaCreateManagedWidget(
                                     "Down-button",
                                     commandWidgetClass,
                                     forms[7],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)-VSyncStart);   
    w=  XtVaCreateManagedWidget(
                                     "Shorter-button",
                                     commandWidgetClass,
                                     forms[7],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)VTotal);   
    w = XtVaCreateManagedWidget(
                                     "Taller-button",
                                     commandWidgetClass,
                                     forms[7],
                                     NULL);
    XtAddCallback (w, XtNcallback, AdjustCB, (XtPointer)-VTotal);
   
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

    wids[1] = XtCreateWidget ("Apply-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[1], XtNcallback, ApplyCB, NULL);

    wids[2] = XtCreateWidget ("Test-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[2], XtNcallback, TestCB, NULL);

    wids[3] = XtCreateWidget ("Restore-button", commandWidgetClass, 
		forms[9], NULL, 0);
    XtAddCallback (wids[3], XtNcallback, RestoreCB, NULL);

    XtManageChildren (wids, 4);

    CreateTyp (forms[10], PixelClock, "PixelClock-label", "PixelClock-text",
	       NULL);
    CreateTyp (forms[11], HSyncRate, "HSyncRate-label", "HSyncRate-text",
	       NULL);
    CreateTyp (forms[12], VSyncRate, "VSyncRate-label", "VSyncRate-text",
	       NULL);

    wids[0] = XtCreateWidget ("Fetch-button", commandWidgetClass, 
		forms[13], NULL, 0);
    XtAddCallback (wids[0], XtNcallback, FetchCB, NULL);

    wids[1] = XtCreateWidget ("Show-button", commandWidgetClass, 
		forms[13], NULL, 0);
    XtAddCallback (wids[1], XtNcallback, ShowCB, NULL);

    wids[2] = XtCreateWidget ("Next-button", commandWidgetClass, 
		forms[13], NULL, 0);
    XtAddCallback (wids[2], XtNcallback, SwitchCB, (XtPointer)1);

    wids[3] = XtCreateWidget ("Prev-button", commandWidgetClass, 
		forms[13], NULL, 0);
    XtAddCallback (wids[3], XtNcallback, SwitchCB, (XtPointer)-1);

    XtManageChildren (wids, 4);

    XtManageChildren (forms, 14);
    XtManageChild (form);

    SetScrollbars ();
    x = DisplayWidth(XtDisplay (top),DefaultScreen (XtDisplay (top))) / 2;
    y = DisplayHeight(XtDisplay (top),DefaultScreen (XtDisplay (top))) / 2;

    invalid_mode_popup = XtVaCreatePopupShell("invalidMode", 
			    transientShellWidgetClass, top,
			    XtNtitle, "Invalid Mode requested",
			    XtNx, x - 20,
			    XtNy, y - 40,
			    NULL);

    testing_popup = XtVaCreatePopupShell("testing", 
			    transientShellWidgetClass, top,
			    XtNtitle, "Testing_1_2_3",
			    XtNx, x - 20,
			    XtNy, y - 40,
			    NULL);
    boxW = XtVaCreateManagedWidget(
                                   "TestingBox",
                                   boxWidgetClass,
                                   testing_popup,
                                   NULL);

    w = XtVaCreateManagedWidget(
		   "testingMessage",
                   labelWidgetClass,
                   boxW,
                   NULL);

    w = XtVaCreateManagedWidget(
                               "Abort",
                                commandWidgetClass,
                                boxW,
                                NULL);

    XtAddCallback (w, XtNcallback, (XtCallbackProc) TestTOCB,
		  (XtPointer) NULL);

    boxW = XtVaCreateManagedWidget(
                                   "invalidBox",
                                   boxWidgetClass,
                                   invalid_mode_popup,
                                   NULL);
        
    messageW = XtVaCreateManagedWidget(
		   "ErrorMessage",
                   labelWidgetClass,
                   boxW,
                   NULL);

   popdownW = XtVaCreateManagedWidget(
                                     "AckError",
                                     commandWidgetClass,
                                     boxW,
                                     NULL);

   XtAddCallback (popdownW, XtNcallback, (XtCallbackProc)popdownInvalid, 
		  (XtPointer) invalid_mode_popup);
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
    static XtActionsRec actions[] = { { "xvidtune-quit", QuitAction } };

    Top = top = XtVaOpenApplication (&app, "Xvidtune", NULL, 0, &argc, argv,
		NULL, applicationShellWidgetClass, 
		XtNmappedWhenManaged, False, NULL);

    XtGetApplicationResources (top, (XtPointer)&AppRes,
		Resources, XtNumber(Resources),
		NULL, 0);

    if (!AppRes.ad_installed) {
	(void) printf ("%s\n", "Please install the program before using");
	return 0;
    }

    if (!XF86VidModeQueryVersion(XtDisplay (top), &MajorVersion, &MinorVersion))
	return 0;

    if (!XF86VidModeQueryExtension(XtDisplay (top), &EventBase, &ErrorBase))
	return 0;

    /* Fail if the extension version in the server is too old */
    if (MajorVersion < MINMAJOR || MinorVersion < MINMINOR) {
	fprintf(stderr,
		"Xserver is running old XFree86-VidModeExtension (%d.%d)\n",
		MajorVersion, MinorVersion);
	fprintf(stderr, "Minimum required version is %d.%d\n",
		MINMAJOR, MINMINOR);
	exit(1);
    }
 
    /* This should probably be done differently */
    if (argc > 1) {
	int i = 0;
	if (!strcmp(argv[1], "-next"))
	    i = 1;
	else if (!strcmp(argv[1], "-prev"))
	    i = -1;
	else if (!strcmp(argv[1], "-unlock")) {
	    CleanUp(XtDisplay (top));
	    XSync(XtDisplay (top), True);
	    return 0;
	}
	if (i != 0) {
	    XF86VidModeSwitchMode(XtDisplay (top),
				  DefaultScreen (XtDisplay (top)), i);
	    XSync(XtDisplay (top), True);
	    return 0;
	}
    }
    if (!GetMonitor(XtDisplay (top), DefaultScreen (XtDisplay (top))))
	return 0;

    if (!XF86VidModeLockModeSwitch(XtDisplay (top),
				   DefaultScreen (XtDisplay (top)), TRUE))
	return 0;

    signal(SIGINT, CatchSig);
    signal(SIGQUIT, CatchSig);
    signal(SIGTERM, CatchSig);
    signal(SIGHUP, CatchSig);

    if (!GetModeLine(XtDisplay (top), DefaultScreen (XtDisplay (top)))) {
	CleanUp(XtDisplay (top));
	return 0;
    }

    xtErrorfunc = XSetErrorHandler(vidmodeError); 

    CreateHierarchy (top);

    XtAppAddActions (app, actions, XtNumber(actions));

    XtOverrideTranslations (top,
		XtParseTranslationTable ("<Message>WM_PROTOCOLS: xvidtune-quit()"));

    XtRealizeWidget (top);

    dpy = XtDisplay(top);

    wm_delete_window = XInternAtom (dpy, "WM_DELETE_WINDOW", False);

    (void) XSetWMProtocols (dpy, XtWindow (top), &wm_delete_window, 1);

    XtMapWidget (top);
    displayWarning(top);
    /* really we should run our on event despaching  here until the
     * warning has been read...
     */
    XtAppMainLoop (app);

    return 0;
}
