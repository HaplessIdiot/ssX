/* $TOG: rxPushB.c /main/4 1997/08/29 18:07:14 kaleb $ */

/***********************************************************

Copyright (c) 1987, 1988, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * rxPushB.c - pushbutton widget
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "rxPushBP.h"

#define DEFAULT_HIGHLIGHT_THICKNESS 2

#define Min(a,b) (a > b) ? b : a

/* Private Data */

static char defaultTranslations[] =
    "<EnterWindow>:	highlight()		\n\
     <LeaveWindow>:	reset()			\n\
     <Btn1Down>:	set()			\n\
     <Btn1Up>:		notify() unset()	";

#define offset(field) XtOffsetOf(RxPushBRec, field)
static XtResource resources[] = { 
   {XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer), 
      offset(pushb.callbacks), XtRCallback, (XtPointer)NULL},
   {XtNhighlightThickness, XtCThickness, XtRDimension, sizeof(Dimension),
      offset(pushb.highlight_thickness), XtRImmediate,
      (XtPointer) DEFAULT_HIGHLIGHT_THICKNESS},
};
#undef offset

static Boolean SetValues();
static void Initialize(), Redisplay(), Set(), Reset(), Notify(), Unset();
static void Highlight(), Unhighlight(), Destroy(), PaintWidget();

static XtActionsRec actionsList[] = {
  {"set",		Set},
  {"notify",		Notify},
  {"highlight",		Highlight},
  {"reset",		Reset},
  {"unset",		Unset},
  {"unhighlight",	Unhighlight}
};

#define SuperClass ((RxLabelWidgetClass) &rxLabelClassRec)

RxPushBClassRec rxPushBClassRec = {
  {
    (WidgetClass) SuperClass,		/* superclass		  */	
    "RxPushB",				/* class_name		  */
    sizeof(RxPushBRec),			/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    XtInheritRealize,			/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    XtInheritResize,			/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },
  {
    0,                                     /* field not used    */
  },
  {
    0,                                     /* field not used    */
  },
};

  /* for public consumption */
WidgetClass rxPushBWidgetClass = (WidgetClass) &rxPushBClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static GC 
Get_GC(pbw, fg, bg)
  RxPushBWidget pbw;
  Pixel fg, bg;
{
  XGCValues	values;
  
  values.foreground	= fg;
  values.background	= bg;
  values.cap_style	= CapProjecting;
  
  if (pbw->pushb.highlight_thickness > 1 )
    values.line_width   = pbw->pushb.highlight_thickness;
  else 
    values.line_width   = 0;
  
  return XtAllocateGC((Widget)pbw, 0, 
		      (GCForeground|GCBackground|GCLineWidth|GCCapStyle),
		      &values, GCFont, 0);
}


/* ARGSUSED */
static void 
Initialize(request, new, args, num_args)
  Widget request, new;
  ArgList args;			/* unused */
  Cardinal *num_args;		/* unused */
{
  RxPushBWidget pbw = (RxPushBWidget) new;

  pbw->pushb.normal_GC = pbw->label.normal_GC;

  pbw->pushb.inverse_GC = Get_GC(pbw, pbw->core.background_pixel, 
				   pbw->label.foreground);

  pbw->pushb.set = FALSE;
  pbw->pushb.highlighted = FALSE;
}

/* ARGSUSED */
static void 
Set(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		/* unused */
  Cardinal *num_params;	/* unused */
{
  RxPushBWidget pbw = (RxPushBWidget)w;

  if (pbw->pushb.set)
    return;

  pbw->pushb.set= TRUE;
  if (XtIsRealized(w))
    PaintWidget(w, event, (Region) NULL, TRUE);
}

/* ARGSUSED */
static void
Unset(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		/* unused */
  Cardinal *num_params;
{
  RxPushBWidget pbw = (RxPushBWidget)w;

  if (!pbw->pushb.set)
    return;

  pbw->pushb.set = FALSE;
  if (XtIsRealized(w)) {
    XClearWindow(XtDisplay(w), XtWindow(w));
    PaintWidget(w, event, (Region) NULL, TRUE);
  }
}

/* ARGSUSED */
static void 
Reset(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		/* unused */
  Cardinal *num_params;   /* unused */
{
  RxPushBWidget pbw = (RxPushBWidget)w;

  if (pbw->pushb.set) {
    Unset(w, event, params, num_params);
  } else
    Unhighlight(w, event, params, num_params);
}

/* ARGSUSED */
static void 
Highlight(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		
  Cardinal *num_params;	
{
  RxPushBWidget pbw = (RxPushBWidget)w;

  pbw->pushb.highlighted = TRUE;

  if (XtIsRealized(w))
    PaintWidget(w, event, (Region) NULL, TRUE);
}

/* ARGSUSED */
static void 
Unhighlight(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		/* unused */
  Cardinal *num_params;	/* unused */
{
  RxPushBWidget pbw = (RxPushBWidget)w;

  pbw->pushb.highlighted = FALSE;

  if (XtIsRealized(w))
    PaintWidget(w, event, (Region) NULL, TRUE);
}

/* ARGSUSED */
static void 
Notify(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;		/* unused */
  Cardinal *num_params;	/* unused */
{
  RxPushBWidget pbw = (RxPushBWidget)w; 

  /* check to be sure state is still Set so that user can cancel
     the action (e.g. by moving outside the window, in the default
     bindings.
  */
  if (pbw->pushb.set)
    XtCallCallbackList(w, pbw->pushb.callbacks, (XtPointer) NULL);
}

/* ARGSUSED */
static void 
Redisplay(w, event, region)
  Widget w;
  XEvent *event;
  Region region;
{
  PaintWidget(w, event, region, FALSE);
}

static void 
PaintWidget(w, event, region, change)
  Widget w;
  XEvent *event;
  Region region;
  Boolean change;
{
  RxPushBWidget pbw = (RxPushBWidget) w;
  Boolean very_thick;
  GC norm_gc, rev_gc;
   
  very_thick = pbw->pushb.highlight_thickness >
               (Dimension)((Dimension) Min(pbw->core.width, pbw->core.height)/2);

  if (pbw->pushb.set) {
    pbw->label.normal_GC = pbw->pushb.inverse_GC;
    XFillRectangle(XtDisplay(w), XtWindow(w), pbw->pushb.normal_GC,
		   0, 0, pbw->core.width, pbw->core.height);
    region = NULL;		/* Force label to repaint text. */
  } else
    pbw->label.normal_GC = pbw->pushb.normal_GC;

  if (pbw->pushb.highlight_thickness <= 0) {
    (*SuperClass->core_class.expose) (w, event, region);
    return;
  }

/*
 * If we are set then use the same colors as if we are not highlighted. 
 */

  if (!pbw->pushb.set == (pbw->pushb.highlighted == TRUE)) {
    norm_gc = pbw->pushb.inverse_GC;
    rev_gc = pbw->pushb.normal_GC;
  } else {
    norm_gc = pbw->pushb.normal_GC;
    rev_gc = pbw->pushb.inverse_GC;
  }

  if ( !( (!change && (pbw->pushb.highlighted == TRUE) || pbw->pushb.set ))) {
    if (very_thick) {
      pbw->label.normal_GC = norm_gc; /* Give the label the right GC. */
      XFillRectangle(XtDisplay(w),XtWindow(w), rev_gc,
		     0, 0, pbw->core.width, pbw->core.height);
    } else {
      /* wide lines are centered on the path, so indent it */
      int offset = pbw->pushb.highlight_thickness/2;
      XDrawRectangle(XtDisplay(w),XtWindow(w), rev_gc, offset, offset, 
		     pbw->core.width - pbw->pushb.highlight_thickness,
		     pbw->core.height - pbw->pushb.highlight_thickness);
    }
  }
  (*SuperClass->core_class.expose) (w, event, region);
}

static void 
Destroy(w)
  Widget w;
{
  RxPushBWidget pbw = (RxPushBWidget) w;

  /* so Label can release it */
  if (pbw->label.normal_GC == pbw->pushb.normal_GC)
    XtReleaseGC( w, pbw->pushb.inverse_GC );
  else
    XtReleaseGC( w, pbw->pushb.normal_GC );
}

/*
 * Set specified arguments into widget
 */

/* ARGSUSED */
static Boolean 
SetValues (current, request, new, args, num_args)
  Widget current, request, new;
  ArgList args;
  Cardinal *num_args;
{
  RxPushBWidget oldpbw = (RxPushBWidget) current;
  RxPushBWidget pbw = (RxPushBWidget) new;
  Boolean redisplay = False;

  if ( oldpbw->core.sensitive != pbw->core.sensitive && !pbw->core.sensitive) {
    /* about to become insensitive */
    pbw->pushb.set = FALSE;
    redisplay = TRUE;
  }
  
  if ( (oldpbw->label.foreground != pbw->label.foreground)           ||
       (oldpbw->core.background_pixel != pbw->core.background_pixel) ||
       (oldpbw->pushb.highlight_thickness != 
                                   pbw->pushb.highlight_thickness) ) {
    if (oldpbw->label.normal_GC == oldpbw->pushb.normal_GC)
	/* Label has release one of these */
      XtReleaseGC(new, pbw->pushb.inverse_GC);
    else
      XtReleaseGC(new, pbw->pushb.normal_GC);

    pbw->pushb.normal_GC = Get_GC(pbw, pbw->label.foreground, 
				    pbw->core.background_pixel);
    pbw->pushb.inverse_GC = Get_GC(pbw, pbw->core.background_pixel, 
				     pbw->label.foreground);
    XtReleaseGC(new, pbw->label.normal_GC);
    pbw->label.normal_GC = (pbw->pushb.set
			    ? pbw->pushb.inverse_GC
			    : pbw->pushb.normal_GC);
    
    redisplay = True;
  }

  return (redisplay);
}

