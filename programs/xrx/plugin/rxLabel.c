/* $TOG: rxLabel.c /main/2 1997/08/29 15:53:27 kaleb $ */

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
 * rxLabel.c - Label widget
 *
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include "rxLabelP.h"
#include <stdio.h>
#include <ctype.h>

/* needed for abs() */
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
int abs();
#endif

#define streq(a,b) (strcmp( (a), (b) ) == 0)

#define MULTI_LINE_LABEL 32767

#ifdef CRAY
#define WORD64
#endif

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(RxLabelRec, label.field)
static XtResource resources[] = {
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    offset(foreground), XtRString, XtDefaultForeground},
  {XtNfontSet,  XtCFontSet, XtRFontSet, sizeof(XFontSet ),
    offset(fontset),XtRString, XtDefaultFontSet},
  {XtNlabel,  XtCLabel, XtRString, sizeof(String),
    offset(label), XtRString, NULL},
  {XtNinternalWidth, XtCWidth, XtRDimension,  sizeof(Dimension),
    offset(internal_width), XtRImmediate, (XtPointer)4},
  {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    offset(internal_height), XtRImmediate, (XtPointer)2},
  {XtNresize, XtCResize, XtRBoolean, sizeof(Boolean),
    offset(resize), XtRImmediate, (XtPointer)True},
};
#undef offset

static void Initialize();
static void Resize();
static void Redisplay();
static Boolean SetValues();
static void Destroy();
static XtGeometryResult QueryGeometry();

RxLabelClassRec rxLabelClassRec = {
  {
/* core_class fields */	
    /* superclass	  	*/	(WidgetClass) &coreClassRec,
    /* class_name	  	*/	"RxLabel",
    /* widget_size	  	*/	sizeof(RxLabelRec),
    /* class_initialize   	*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize		  	*/	XtInheritRealize,
    /* actions		  	*/	NULL,
    /* num_actions	  	*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* compress_motion	  	*/	TRUE,
    /* compress_exposure  	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* resize		  	*/	Resize,
    /* expose		  	*/	Redisplay,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus	 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* tm_table		   	*/	NULL,
    /* query_geometry		*/	QueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
/* Label class fields initialization */
  {
    /* not used			*/	0
  }
};
WidgetClass rxLabelWidgetClass = (WidgetClass)&rxLabelClassRec;
/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/*
 * Calculate width and height of displayed text in pixels
 */

static void SetTextWidthAndHeight(lw)
    RxLabelWidget lw;
{
    char *nl;

    XFontSet	fset = lw->label.fontset;
    XFontSetExtents *ext = XExtentsOfFontSet(fset);

    lw->label.label_height = ext->max_ink_extent.height;
    if (lw->label.label == NULL) {
	lw->label.label_len = 0;
	lw->label.label_width = 0;
    } else if ((nl = index(lw->label.label, '\n')) != NULL) {
	char *label;
	lw->label.label_len = MULTI_LINE_LABEL;
	lw->label.label_width = 0;
	for (label = lw->label.label; nl != NULL; nl = index(label, '\n')) {
	    int width = XmbTextEscapement(fset, label, (int)(nl - label));

	    if (width > (int)lw->label.label_width)
		lw->label.label_width = width;
	    label = nl + 1;
	    if (*label)
		lw->label.label_height += ext->max_ink_extent.height;
	}
	if (*label) {
	    int width = XmbTextEscapement(fset, label, strlen(label));

	    if (width > (int) lw->label.label_width)
		lw->label.label_width = width;
	}
    } else {
	lw->label.label_len = strlen(lw->label.label);
	lw->label.label_width =
	    XmbTextEscapement(fset, lw->label.label, (int) lw->label.label_len);
    }
}

static void GetnormalGC(lw)
    RxLabelWidget lw;
{
    XGCValues	values;
    XtGCMask	mask = GCForeground|GCBackground|GCGraphicsExposures;

    values.foreground	= lw->label.foreground;
    values.background	= lw->core.background_pixel;
    values.graphics_exposures = False;

    lw->label.normal_GC = 
	XtAllocateGC((Widget)lw, 0, mask, &values, GCFont, 0 );
}


/* ARGSUSED */
static void Initialize(request, new, args, num_args)
    Widget request, new;
    ArgList args;
    Cardinal *num_args;
{
    RxLabelWidget lw = (RxLabelWidget) new;

    if (lw->label.label == NULL) 
        lw->label.label = XtNewString(lw->core.name);
    else {
        lw->label.label = XtNewString(lw->label.label);
    }

    GetnormalGC(lw);

    SetTextWidthAndHeight(lw);

    if (lw->core.height == 0)
        lw->core.height = lw->label.label_height +
			    2 * lw->label.internal_height;

    if (lw->core.width == 0)		/* need label.lbm_width */
        lw->core.width = (lw->label.label_width +
			    2 * lw->label.internal_width);

    lw->label.label_x = lw->label.label_y = 0;
    (*XtClass(new)->core_class.resize) ((Widget)lw);

} /* Initialize */

/*
 * Repaint the widget window
 */

/* ARGSUSED */
static void Redisplay(gw, event, region)
    Widget gw;
    XEvent *event;
    Region region;
{
    extern WidgetClass commandWidgetClass;
    RxLabelWidget w = (RxLabelWidget) gw;
    GC gc;
    int len = w->label.label_len;
    char *label = w->label.label;
    Position ksy = w->label.label_y;
    XFontSetExtents *ext = XExtentsOfFontSet(w->label.fontset);

    /*
     * now we'll see if we need to draw the rest of the label
     */
    if (region != NULL) {
	int x = w->label.label_x;
	unsigned int width = w->label.label_width;
	if (XRectInRegion(region, x, w->label.label_y,
			 width, w->label.label_height) == RectangleOut){
	    return;
	}
    }

    gc = w->label.normal_GC;

    ksy += abs(ext->max_ink_extent.y);

    if (len == MULTI_LINE_LABEL) {
	char *nl;
	while ((nl = index(label, '\n')) != NULL) {
	    XmbDrawString(XtDisplay(w), XtWindow(w), w->label.fontset, gc,
	  		  w->label.label_x, ksy, label, (int)(nl - label));
	    ksy += ext->max_ink_extent.height;
	    label = nl + 1;
	}
	len = strlen(label);
    }
    if (len)
	XmbDrawString(XtDisplay(w), XtWindow(w), w->label.fontset, gc,
		      w->label.label_x, ksy, label, len);
}

static void Reposition(lw, width, height, dx, dy)
    RxLabelWidget lw;
    Dimension width, height;
    Position *dx, *dy;
{
    Position newPos;
    Position leftedge = lw->label.internal_width;

    newPos = (int)(width - lw->label.label_width) / 2;
    if (newPos < (Position)leftedge)
	newPos = leftedge;
    *dx = newPos - lw->label.label_x;
    lw->label.label_x = newPos;
    *dy = (newPos = (int)(height - lw->label.label_height) / 2)
	  - lw->label.label_y;
    lw->label.label_y = newPos;
    return;
}

static void Resize(w)
    Widget w;
{
    RxLabelWidget lw = (RxLabelWidget)w;
    Position dx, dy;

    Reposition(lw, w->core.width, w->core.height, &dx, &dy);
}

/*
 * Set specified arguments into widget
 */

#define PIXMAP 0
#define WIDTH 1
#define HEIGHT 2
#define NUM_CHECKS 3

static Boolean SetValues(current, request, new, args, num_args)
    Widget current, request, new;
    ArgList args;
    Cardinal *num_args;
{
    RxLabelWidget curlw = (RxLabelWidget) current;
    RxLabelWidget reqlw = (RxLabelWidget) request;
    RxLabelWidget newlw = (RxLabelWidget) new;
    int i;
    Boolean was_resized = False, redisplay = False, checks[NUM_CHECKS];

    for (i = 0; i < NUM_CHECKS; i++)
	checks[i] = FALSE;

    for (i = 0; i < *num_args; i++) {
	if (streq(XtNbitmap, args[i].name))
	    checks[PIXMAP] = TRUE;
	if (streq(XtNwidth, args[i].name))
	    checks[WIDTH] = TRUE;
	if (streq(XtNheight, args[i].name))
	    checks[HEIGHT] = TRUE;
    }

    if (newlw->label.label == NULL) {
	newlw->label.label = newlw->core.name;
    }

    if (curlw->label.fontset != newlw->label.fontset) {
	was_resized = True;
    }
    if (curlw->label.label != newlw->label.label) {
        if (curlw->label.label != curlw->core.name)
	    XtFree( (char *)curlw->label.label );

	if (newlw->label.label != newlw->core.name) {
	    newlw->label.label = XtNewString( newlw->label.label );
	}
	was_resized = True;
    }

    if (was_resized || checks[PIXMAP]) {

	SetTextWidthAndHeight(newlw);
	was_resized = True;
    }

    /* recalculate the window size if something has changed. */
    if (newlw->label.resize && was_resized) {
	if ((curlw->core.height == reqlw->core.height) && !checks[HEIGHT])
	    newlw->core.height = (newlw->label.label_height +
				    2 * newlw->label.internal_height);

	if ((curlw->core.width == reqlw->core.width) && !checks[WIDTH])
	    newlw->core.width = (newlw->label.label_width +
				    2 * newlw->label.internal_width);
    }

    if (curlw->label.foreground		!= newlw->label.foreground
	|| curlw->core.background_pixel != newlw->core.background_pixel) {

        /* The Fontset is not in the GC - don't make a new GC if FS changes! */

	XtReleaseGC(new, curlw->label.normal_GC);
	GetnormalGC(newlw);
	redisplay = True;
    }

    if ((curlw->label.internal_width != newlw->label.internal_width)
        || (curlw->label.internal_height != newlw->label.internal_height)
	|| was_resized) {
	/* Resize() will be called if geometry changes succeed */
	Position dx, dy;
	Reposition(newlw, curlw->core.width, curlw->core.height, &dx, &dy);
    }

    return was_resized || redisplay ||
	   XtIsSensitive(current) != XtIsSensitive(new);
}

static void Destroy(w)
    Widget w;
{
    RxLabelWidget lw = (RxLabelWidget)w;

    if ( lw->label.label != lw->core.name )
	XtFree( lw->label.label );
    XtReleaseGC( w, lw->label.normal_GC );
}


static XtGeometryResult QueryGeometry(w, intended, preferred)
    Widget w;
    XtWidgetGeometry *intended, *preferred;
{
    RxLabelWidget lw = (RxLabelWidget)w;

    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = lw->label.label_width + 2 * lw->label.internal_width;
    preferred->height = lw->label.label_height + 2 * lw->label.internal_height;

    if (  ((intended->request_mode & (CWWidth | CWHeight))
	   	== (CWWidth | CWHeight)) &&
	  intended->width == preferred->width &&
	  intended->height == preferred->height)
	return XtGeometryYes;
    else if (preferred->width == w->core.width &&
	     preferred->height == w->core.height)
	return XtGeometryNo;
    else
	return XtGeometryAlmost;
}

