/* $Xorg: Clock.c,v 1.4 2001/02/09 02:05:39 xorgcvs Exp $ */

/***********************************************************

Copyright 1987, 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


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
/* $XFree86: xc/programs/xclock/Clock.c,v 3.16 2002/05/20 17:55:37 keithp Exp $ */

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include "ClockP.h"
#include <X11/Xosdefs.h>
#include <stdio.h>
#include <X11/Xos.h>

#include <time.h>
#define Time_t time_t

#ifdef XKB
#include <X11/extensions/XKBbells.h>
#endif

	
/* Private Definitions */

#define VERTICES_IN_HANDS	6	/* to draw triangle */
#define PI			3.14159265358979
#define TWOPI			(2. * PI)

#define MINOR_TICK_FRACT	95
#define SECOND_HAND_FRACT	90
#define MINUTE_HAND_FRACT	70
#define HOUR_HAND_FRACT		40
#define HAND_WIDTH_FRACT	7
#define SECOND_WIDTH_FRACT	5
#define SECOND_HAND_TIME	30

#define ANALOG_SIZE_DEFAULT	164

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(a) ((a) < 0 ? -(a) : (a))


/* Initialization of defaults */

#define offset(field) XtOffsetOf(ClockRec, clock.field)
#define goffset(field) XtOffsetOf(WidgetRec, core.field)

static XtResource resources[] = {
    {XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
	goffset(width), XtRImmediate, (XtPointer) 0},
    {XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
	goffset(height), XtRImmediate, (XtPointer) 0},
    {XtNupdate, XtCInterval, XtRInt, sizeof(int), 
        offset(update), XtRImmediate, (XtPointer) 60 },
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(fgpixel), XtRString, XtDefaultForeground},
    {XtNhand, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(Hdpixel), XtRString, XtDefaultForeground},
    {XtNhighlight, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(Hipixel), XtRString, XtDefaultForeground},
    {XtNutime, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(utime), XtRImmediate, (XtPointer) FALSE},
    {XtNanalog, XtCBoolean, XtRBoolean, sizeof(Boolean),
        offset(analog), XtRImmediate, (XtPointer) TRUE},
    {XtNbrief, XtCBoolean, XtRBoolean, sizeof(Boolean),
        offset(brief), XtRImmediate, (XtPointer) FALSE},
    {XtNchime, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(chime), XtRImmediate, (XtPointer) FALSE },
    {XtNpadding, XtCMargin, XtRInt, sizeof(int),
        offset(padding), XtRImmediate, (XtPointer) 8},
    {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        offset(font), XtRString, XtDefaultFont},
    {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof (int),
    	offset (backing_store), XtRString, "default"},
#ifdef XRENDER
    {XtNrender, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(render), XtRImmediate, (XtPointer) FALSE },
    {XtNsharp, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(sharp), XtRImmediate, (XtPointer) FALSE },
    {XtNhourColor, XtCForeground, XtRXftColor, sizeof(XftColor),
	offset(hour_color), XtRString, "rgba:7f/00/00/c0"},
    {XtNminuteColor, XtCForeground, XtRXftColor, sizeof(XftColor),
	offset(min_color), XtRString, "rgba:00/7f/7f/c0"},
    {XtNsecondColor, XtCForeground, XtRXftColor, sizeof(XftColor),
	offset(sec_color), XtRString, "rgba:00/00/ff/80"},
    {XtNmajorColor, XtCForeground, XtRXftColor, sizeof(XftColor),
	offset(major_color), XtRString, "rgba:7f/00/00/c0"},
    {XtNminorColor, XtCForeground, XtRXftColor, sizeof(XftColor),
	offset(minor_color), XtRString, "rgba:00/7f/7f/c0"},
    {XtNface, XtCFace, XtRXftFont, sizeof (XftFont *),
	offset (face), XtRString, ""},
#endif
};

#undef offset
#undef goffset

static void ClassInitialize ( void );
static void Initialize ( Widget request, Widget new, ArgList args, 
			 Cardinal *num_args );
static void Realize ( Widget gw, XtValueMask *valueMask, 
		      XSetWindowAttributes *attrs );
static void Destroy ( Widget gw );
static void Resize ( Widget gw );
static void Redisplay ( Widget gw, XEvent *event, Region region );
static void clock_tic ( XtPointer client_data, XtIntervalId *id );
static void erase_hands ( ClockWidget w, struct tm *tm );
static void ClockAngle ( int tick_units, double *sinp, double *cosp );
static void DrawLine ( ClockWidget w, Dimension blank_length, 
		       Dimension length, int tick_units );
static void DrawHand ( ClockWidget w, Dimension length, Dimension width, 
		       int tick_units );
static void DrawSecond ( ClockWidget w, Dimension length, Dimension width, 
			 Dimension offset, int tick_units );
static void SetSeg ( ClockWidget w, int x1, int y1, int x2, int y2 );
static void DrawClockFace ( ClockWidget w );
static int clock_round ( double x );
static Boolean SetValues ( Widget gcurrent, Widget grequest, Widget gnew, 
			   ArgList args, Cardinal *num_args );

ClockClassRec clockClassRec = {
    { /* core fields */
    /* superclass		*/	(WidgetClass) &simpleClassRec,
    /* class_name		*/	"Clock",
    /* widget_size		*/	sizeof(ClockRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* resource_count		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	XtExposeCompressMaximal,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	Resize,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry           */	XtInheritQueryGeometry,
    /* display_accelerator      */	XtInheritDisplayAccelerator,
    /* extension                */	NULL
    },
    { /* simple fields */
    /* change_sensitive         */      XtInheritChangeSensitive
    },
    { /* clock fields */
    /* ignore                   */      0
    }
};

WidgetClass clockWidgetClass = (WidgetClass) &clockClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#ifdef XRENDER
XtConvertArgRec xftColorConvertArgs[] = {
    {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.screen),
     sizeof(Screen *)},
    {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.colormap),
     sizeof(Colormap)}
};

#define	donestr(type, value, tstr) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    XtDisplayStringConversionWarning(dpy, 	\
			(char*) fromVal->addr, tstr);		\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}

static void
XmuFreeXftColor (XtAppContext app, XrmValuePtr toVal, XtPointer closure,
		 XrmValuePtr args, Cardinal *num_args)
{
    Screen	*screen;
    Colormap	colormap;
    XftColor	*color;
    
    if (*num_args != 2)
    {
	XtAppErrorMsg (app,
		       "freeXftColor", "wrongParameters",
		       "XtToolkitError",
		       "Freeing an XftColor requires screen and colormap arguments",
		       (String *) NULL, (Cardinal *)NULL);
	return;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);
    color = (XftColor *) toVal->addr;
    XftColorFree (DisplayOfScreen (screen),
		  DefaultVisual (DisplayOfScreen (screen),
				 XScreenNumberOfScreen (screen)),
		  colormap, color);
}
    
static Boolean
XmuCvtStringToXftColor(Display *dpy,
		       XrmValue *args, Cardinal *num_args,
		       XrmValue *fromVal, XrmValue *toVal,
		       XtPointer *converter_data)
{
    char	    *spec;
    XRenderColor    renderColor;
    XftColor	    xftColor;
    Screen	    *screen;
    Colormap	    colormap;
    
    if (*num_args != 2)
    {
	XtAppErrorMsg (XtDisplayToApplicationContext (dpy),
		       "cvtStringToXftColor", "wrongParameters",
		       "XtToolkitError",
		       "String to render color conversion needs screen and colormap arguments",
		       (String *) NULL, (Cardinal *)NULL);
	return False;
    }

    screen = *((Screen **) args[0].addr);
    colormap = *((Colormap *) args[1].addr);

    spec = (char *) fromVal->addr;
    if (strcasecmp (spec, XtDefaultForeground) == 0)
    {
	renderColor.red = 0;
	renderColor.green = 0;
	renderColor.blue = 0;
	renderColor.alpha = 0xffff;
    }
    else if (strcasecmp (spec, XtDefaultBackground) == 0)
    {
	renderColor.red = 0xffff;
	renderColor.green = 0xffff;
	renderColor.blue = 0xffff;
	renderColor.alpha = 0xffff;
    }
    else if (!XRenderParseColor (dpy, spec, &renderColor))
	return False;
    if (!XftColorAllocValue (dpy, 
			     DefaultVisual (dpy,
					    XScreenNumberOfScreen (screen)),
			     colormap,
			     &renderColor,
			     &xftColor))
	return False;
    
    donestr (XftColor, xftColor, XtRXftColor);
}

static void
XmuFreeXftFont (XtAppContext app, XrmValuePtr toVal, XtPointer closure,
		XrmValuePtr args, Cardinal *num_args)
{
    Screen  *screen;
    XftFont *font;
    
    if (*num_args != 1)
    {
	XtAppErrorMsg (app,
		       "freeXftFont", "wrongParameters",
		       "XtToolkitError",
		       "Freeing an XftFont requires screen argument",
		       (String *) NULL, (Cardinal *)NULL);
	return;
    }

    screen = *((Screen **) args[0].addr);
    font = *((XftFont **) toVal->addr);
    if (font)
	XftFontClose (DisplayOfScreen (screen), font);
}

static Boolean
XmuCvtStringToXftFont(Display *dpy,
		      XrmValue *args, Cardinal *num_args,
		      XrmValue *fromVal, XrmValue *toVal,
		      XtPointer *converter_data)
{
    char    *name;
    XftFont *font;
    Screen  *screen;
    
    if (*num_args != 1)
    {
	XtAppErrorMsg (XtDisplayToApplicationContext (dpy),
		       "cvtStringToXftFont", "wrongParameters",
		       "XtToolkitError",
		       "String to XftFont conversion needs screen argument",
		       (String *) NULL, (Cardinal *)NULL);
	return False;
    }

    screen = *((Screen **) args[0].addr);
    name = (char *) fromVal->addr;
    
    font = XftFontOpenName (dpy,
			    XScreenNumberOfScreen (screen),
			    name);
    if (font)
    {
	donestr (XftFont *, font, XtRXftFont);
    }
    XtDisplayStringConversionWarning(dpy, (char *) fromVal->addr, XtRXftFont);
    return False;
}

XtConvertArgRec xftFontConvertArgs[] = {
    {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.screen),
     sizeof(Screen *)},
};

#endif

static void 
ClassInitialize(void)
{
    XtAddConverter( XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
		    NULL, 0 );
#ifdef XRENDER
    XtSetTypeConverter (XtRString, XtRXftColor, 
			XmuCvtStringToXftColor, 
			xftColorConvertArgs, XtNumber(xftColorConvertArgs),
			XtCacheByDisplay, XmuFreeXftColor);
    XtSetTypeConverter (XtRString, XtRXftFont,
			XmuCvtStringToXftFont,
			xftFontConvertArgs, XtNumber(xftFontConvertArgs),
			XtCacheByDisplay, XmuFreeXftFont);
#endif
}

static char *
TimeString (ClockWidget w, struct tm *tm)
{
   if (w->clock.brief)
   {
      static char brief[5];
      sprintf (brief, "%02d:%02d", tm->tm_hour, tm->tm_min);
      return brief;
   }
   else if (w->clock.utime)
   {
      static char utime[35];
      Time_t tsec;
      tsec = time(NULL);
      sprintf (utime, "%10lu seconds since Epoch", (unsigned long)tsec);
      return utime;
   }
   return asctime (tm);
}

/* ARGSUSED */
static void 
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    ClockWidget w = (ClockWidget)new;
    XtGCMask		valuemask;
    XGCValues	myXGCV;
    int min_height, min_width;

    valuemask = GCForeground | GCBackground | GCFont | GCLineWidth;
    if (w->clock.font != NULL)
        myXGCV.font = w->clock.font->fid;
    else
        valuemask &= ~GCFont;	/* use server default font */

    min_width = min_height = ANALOG_SIZE_DEFAULT;
    if(!w->clock.analog) {
       char *str;
       struct tm tm;
       Time_t time_value;

       (void) time(&time_value);
       tm = *localtime(&time_value);
       str = TimeString (w, &tm);
       if (w->clock.font == NULL)
          w->clock.font = XQueryFont( XtDisplay(w),
				      XGContextFromGC(
					   DefaultGCOfScreen(XtScreen(w))) );
#ifdef XRENDER
       if (w->clock.render)
       {
	XGlyphInfo  extents;
	XftTextExtents8 (XtDisplay (w), w->clock.face,
			 (FcChar8 *) str, strlen (str), &extents);
	min_width = extents.xOff + 2 * w->clock.padding;
	min_height = w->clock.face->ascent + w->clock.face->descent +
		     2 * w->clock.padding;
       }
       else
#endif
       {
       min_width = XTextWidth(w->clock.font, str, strlen(str)) +
	  2 * w->clock.padding;
       min_height = w->clock.font->ascent +
	  w->clock.font->descent + 2 * w->clock.padding;
       }
    }
    if (w->core.width == 0)
	w->core.width = min_width;
    if (w->core.height == 0)
	w->core.height = min_height;

    myXGCV.foreground = w->clock.fgpixel;
    myXGCV.background = w->core.background_pixel;
    if (w->clock.font != NULL)
        myXGCV.font = w->clock.font->fid;
    else
        valuemask &= ~GCFont;	/* use server default font */
    myXGCV.line_width = 0;
    w->clock.myGC = XtGetGC((Widget)w, valuemask, &myXGCV);

    valuemask = GCForeground | GCLineWidth ;
    myXGCV.foreground = w->core.background_pixel;
    w->clock.EraseGC = XtGetGC((Widget)w, valuemask, &myXGCV);

    myXGCV.foreground = w->clock.Hipixel;
    w->clock.HighGC = XtGetGC((Widget)w, valuemask, &myXGCV);

    valuemask = GCForeground;
    myXGCV.foreground = w->clock.Hdpixel;
    w->clock.HandGC = XtGetGC((Widget)w, valuemask, &myXGCV);

    if (w->clock.update <= 0)
	w->clock.update = 60;	/* make invalid update's use a default */
    w->clock.show_second_hand = (w->clock.update <= SECOND_HAND_TIME);
    w->clock.numseg = 0;
    w->clock.interval_id = 0;
#ifdef XRENDER
    if (w->clock.render)
	w->clock.mask_format = XRenderFindStandardFormat (XtDisplay (w),
							  w->clock.sharp ?
							  PictStandardA1 :
							  PictStandardA8);
    w->clock.draw = 0;
    w->clock.damage.x = 0;
    w->clock.damage.y = 0;
    w->clock.damage.height = 0;
    w->clock.damage.width = 0;
#endif
}

#if XRENDER
static void
RenderPrepare (ClockWidget  w, XftColor *color)
{
    if (!w->clock.draw)
    {
	w->clock.draw = XftDrawCreate (XtDisplay (w), XtWindow (w),
				       DefaultVisual (XtDisplay (w),
						      DefaultScreen(XtDisplay (w))),
				       w->core.colormap);
	w->clock.picture = XftDrawPicture (w->clock.draw);
    }
    if (color)
	w->clock.fill_picture = XftDrawSrcPicture (w->clock.draw, color);
}

static void
RenderClip (ClockWidget w)
{
    Region	r;
    XClearArea (XtDisplay (w),
		XtWindow (w), 
		w->clock.damage.x,
		w->clock.damage.y,
		w->clock.damage.width,
		w->clock.damage.height, False);
    RenderPrepare (w, 0);
    r = XCreateRegion ();
    XUnionRectWithRegion (&w->clock.damage,
			  r, r);
    XftDrawSetClip (w->clock.draw, r);
    XDestroyRegion (r);
}

static void
RenderTextBounds (ClockWidget w, char *str, int off, int len, 
		  XRectangle *bounds, int *xp, int *yp)
{
    XGlyphInfo  head, tail;
    int	    x, y;

    XftTextExtents8 (XtDisplay (w), w->clock.face, (FcChar8 *) str, 
		     off, &head);
    XftTextExtents8 (XtDisplay (w), w->clock.face, (FcChar8 *) str + off, 
		     len - off, &tail);
    /*
     * Compute position of tail
     */
    x = w->clock.padding + head.xOff;
    y = w->clock.face->ascent + w->clock.padding + head.yOff;
    /*
     * Compute bounds of tail, pad a bit as the bounds aren't exact
     */
    bounds->x = x - tail.x - 1;
    bounds->y = y - tail.y - 1;
    bounds->width = tail.width + 2;
    bounds->height = tail.height + 2;
    if (xp) *xp = x;
    if (yp) *yp = y;
}

static void
RenderUpdateRectBounds (XRectangle *damage, XRectangle *bounds)
{
    int	    x1 = bounds->x;
    int	    y1 = bounds->y;
    int	    x2 = bounds->x + bounds->width; 
    int	    y2 = bounds->y + bounds->height; 
    int	    d_x1 = damage->x;
    int	    d_y1 = damage->y;
    int	    d_x2 = damage->x + damage->width; 
    int	    d_y2 = damage->y + damage->height; 

    if (x1 == x2) 
    { 
	x1 = d_x1; 
	x2 = d_x2; 
    }
    else
    {
	if (d_x1 < x1) x1 = d_x1;
	if (d_x2 > x2) x2 = d_x2;
    }
    if (y1 == y2)
    {
	y1 = d_y1;
	y2 = d_y2;
    }
    else
    {
	if (d_y1 < y1) y1 = d_y1;
	if (d_y2 > y2) y2 = d_y2;
    }

    bounds->x = x1;
    bounds->y = y1;
    bounds->width = x2 - x1;
    bounds->height = y2 - y1;
}

static Boolean
RenderRectIn (XRectangle *rect, XRectangle *bounds)
{
    int	    x1 = bounds->x;
    int	    y1 = bounds->y;
    int	    x2 = bounds->x + bounds->width; 
    int	    y2 = bounds->y + bounds->height; 
    int	    r_x1 = rect->x;
    int	    r_y1 = rect->y;
    int	    r_x2 = rect->x + rect->width; 
    int	    r_y2 = rect->y + rect->height; 
    
    return r_x1 < x2 && x1 < r_x2 && r_y1 < y2 && y1 < r_y2;
}

#define LINE_WIDTH  0.01
#include <math.h>

#define XCoord(x,w) ((x) * (w)->clock.x_scale + (w)->clock.x_off)
#define YCoord(y,w) ((y) * (w)->clock.y_scale + (w)->clock.y_off)

static void
RenderUpdateBounds (XPointDouble *points, int npoints, XRectangle *bounds)
{
    int	    x1 = bounds->x;
    int	    y1 = bounds->y;
    int	    x2 = bounds->x + bounds->width; 
    int	    y2 = bounds->y + bounds->height; 
    
    while (npoints--)
    {
	int	    r_x1 = points[0].x;
	int	    r_y1 = points[0].y;
	int	    r_x2 = points[0].x + 1;
	int	    r_y2 = points[0].y + 1;

	if (x1 == x2)
	    x2 = x1 = r_x1;
	if (y1 == y2)
	    y2 = y1 = r_y1;
	if (r_x1 < x1) x1 = r_x1;
	if (r_y1 < y1) y1 = r_y1;
	if (r_x2 > x2) x2 = r_x2;
	if (r_y2 > y2) y2 = r_y2;
	points++;
    }
    bounds->x = x1;
    bounds->y = y1;
    bounds->width = x2 - x1;
    bounds->height = y2 - y1;
}

static Boolean
RenderCheckBounds (XPointDouble *points, int npoints, XRectangle *bounds)
{
    int	    x1 = bounds->x;
    int	    y1 = bounds->y;
    int	    x2 = bounds->x + bounds->width; 
    int	    y2 = bounds->y + bounds->height; 
    
    while (npoints--)
    {
	if (x1 <= points->x && points->x <= x2 &&
	    y1 <= points->y && points->y <= y2)
	    return True;
	points++;
    }
    return False;
}

static void
RenderResetBounds (XRectangle *bounds)
{
    bounds->x = 0;
    bounds->y = 0;
    bounds->width = 0;
    bounds->height = 0;
}

static void
RenderLine (ClockWidget w, XDouble x1, XDouble y1, XDouble x2, XDouble y2,
	    XftColor *color,
	    Boolean draw)
{
    XPointDouble    poly[4];
    XDouble	    dx = (x2 - x1);
    XDouble	    dy = (y2 - y1);
    XDouble	    len = sqrt (dx*dx + dy*dy);
    XDouble	    ldx = (LINE_WIDTH/2.0) * dy / len;
    XDouble	    ldy = (LINE_WIDTH/2.0) * dx / len;

    poly[0].x = XCoord (x1 + ldx, w);
    poly[0].y = YCoord (y1 - ldy, w);
    
    poly[1].x = XCoord (x2 + ldx, w);
    poly[1].y = YCoord (y2 - ldy, w);
    
    poly[2].x = XCoord (x2 - ldx, w);
    poly[2].y = YCoord (y2 + ldy, w);
    
    poly[3].x = XCoord (x1 - ldx, w);
    poly[3].y = YCoord (y1 + ldy, w);

    RenderUpdateBounds (poly, 4, &w->clock.damage);
    if (draw)
    {
	if (RenderCheckBounds (poly, 4, &w->clock.damage))
	{
	    RenderPrepare (w, color);
	    XRenderCompositeDoublePoly (XtDisplay (w),
					PictOpOver,
					w->clock.fill_picture,
					w->clock.picture,
					w->clock.mask_format,
					0, 0, 0, 0, poly, 4, EvenOddRule);
	}
    }
    else
	RenderUpdateBounds (poly, 4, &w->clock.damage);
}

static void
RenderRotate (ClockWidget w, XPointDouble *out, double x, double y, double s, double c)
{
    out->x = XCoord (x * c - y * s, w);
    out->y = YCoord (y * c + x * s, w);
}

static void
RenderHand (ClockWidget w, int tick_units, int size, XftColor *color,
	    Boolean draw)
{
    double	    c, s;
    XPointDouble    poly[3];
    double	    outer_x;
    double	    inner_y;

    ClockAngle (tick_units, &c, &s);
    s = -s;

    /* compute raw positions */
    outer_x = size / 100.0;
    inner_y = HAND_WIDTH_FRACT / 100.0;

    /* rotate them into position */
    RenderRotate (w, &poly[0], outer_x, 0.0, s, c);
    RenderRotate (w, &poly[1], -inner_y, inner_y, s, c);
    RenderRotate (w, &poly[2], -inner_y, -inner_y, s, c);
    
    if (draw)
    {
	if (RenderCheckBounds (poly, 3, &w->clock.damage))
	{
	    RenderPrepare (w, color);
	    XRenderCompositeDoublePoly (XtDisplay (w),
					PictOpOver,
					w->clock.fill_picture,
					w->clock.picture,
					w->clock.mask_format,
					0, 0, 0, 0, poly, 3, EvenOddRule);
	}
    }
    RenderUpdateBounds (poly, 3, &w->clock.damage);
}

static void
RenderHands (ClockWidget w, struct tm *tm, Boolean draw)
{
    RenderHand (w, tm->tm_min * 12, MINUTE_HAND_FRACT, &w->clock.min_color, draw);
    RenderHand (w, tm->tm_hour * 60 + tm->tm_min, HOUR_HAND_FRACT, &w->clock.hour_color, draw);
}

static void
RenderSec (ClockWidget w, struct tm *tm, Boolean draw)
{
    double	    c, s;
    XPointDouble    poly[10];
    double	    inner_x, middle_x, outer_x, far_x;
    double	    middle_y;
    double	    line_y;

    ClockAngle (tm->tm_sec * 12, &c, &s);
    
    s = -s;
    
    /*
     * Compute raw positions
     */
    line_y = LINE_WIDTH;
    inner_x = (MINUTE_HAND_FRACT / 100.0);
    middle_x = ((SECOND_HAND_FRACT + MINUTE_HAND_FRACT) / 200.0);
    outer_x = (SECOND_HAND_FRACT / 100.0);
    far_x = (MINOR_TICK_FRACT / 100.0);
    middle_y = (SECOND_WIDTH_FRACT / 100.0);
    
    /*
     * Rotate them into position
     */
    RenderRotate (w, &poly[0], -line_y, line_y, s, c);
    RenderRotate (w, &poly[1], inner_x, line_y, s, c);
    RenderRotate (w, &poly[2], middle_x, middle_y, s, c);
    RenderRotate (w, &poly[3], outer_x, line_y, s, c);
    RenderRotate (w, &poly[4], far_x, line_y, s, c);
    RenderRotate (w, &poly[5], far_x, -line_y, s, c);
    RenderRotate (w, &poly[6], outer_x, -line_y, s, c);
    RenderRotate (w, &poly[7], middle_x, -middle_y, s, c);
    RenderRotate (w, &poly[8], inner_x, -line_y, s, c);
    RenderRotate (w, &poly[9], -line_y, -line_y, s, c);

    if (draw)
    {
	if (RenderCheckBounds (poly, 10, &w->clock.damage))
	{
	    RenderPrepare (w, &w->clock.sec_color);
	    XRenderCompositeDoublePoly (XtDisplay (w),
					PictOpOver,
					w->clock.fill_picture,
					w->clock.picture,
					w->clock.mask_format,
					0, 0, 0, 0, poly, 10, EvenOddRule);
	}
    }
    else
    {
	RenderUpdateBounds (poly, 10, &w->clock.damage);
    }
}

#endif

static void 
Realize(Widget gw, XtValueMask *valueMask, XSetWindowAttributes *attrs)
{
     ClockWidget	w = (ClockWidget) gw;
#ifdef notdef
     *valueMask |= CWBitGravity;
     attrs->bit_gravity = ForgetGravity;
#endif
     switch (w->clock.backing_store) {
     case Always:
     case NotUseful:
     case WhenMapped:
     	*valueMask |=CWBackingStore;
	attrs->backing_store = w->clock.backing_store;
	break;
     }
     (*clockWidgetClass->core_class.superclass->core_class.realize)
	 (gw, valueMask, attrs);
     Resize(gw);
}

static void 
Destroy(Widget gw)
{
     ClockWidget w = (ClockWidget) gw;
     if (w->clock.interval_id) XtRemoveTimeOut (w->clock.interval_id);
#ifdef RENDER
    if (w->clock.picture)
	XRenderFreePicture (dpy, w->clock.picture);
    if (w->clock.fill_picture)
	XRenderFreePicture (dpy, w->clock.fill_picture);
#endif
     XtReleaseGC (gw, w->clock.myGC);
     XtReleaseGC (gw, w->clock.HighGC);
     XtReleaseGC (gw, w->clock.HandGC);
     XtReleaseGC (gw, w->clock.EraseGC);
}

static void 
Resize(Widget gw) 
{
    ClockWidget w = (ClockWidget) gw;
    /* don't do this computation if window hasn't been realized yet. */
    if (XtIsRealized(gw) && w->clock.analog) {
	/* need signed value since Dimension is unsigned */
	int radius = ((int) min(w->core.width, w->core.height) - (int) (2 * w->clock.padding)) / 2;
        w->clock.radius = (Dimension) max (radius, 1);

        w->clock.second_hand_length = (int)(SECOND_HAND_FRACT * w->clock.radius) / 100;
        w->clock.minute_hand_length = (int)(MINUTE_HAND_FRACT * w->clock.radius) / 100;
        w->clock.hour_hand_length = (int)(HOUR_HAND_FRACT * w->clock.radius) / 100;
        w->clock.hand_width = (int)(HAND_WIDTH_FRACT * w->clock.radius) / 100;
        w->clock.second_hand_width = (int)(SECOND_WIDTH_FRACT * w->clock.radius) / 100;

        w->clock.centerX = w->core.width / 2;
        w->clock.centerY = w->core.height / 2;
    }
#ifdef XRENDER
    w->clock.x_scale = 0.45 * w->core.width;
    w->clock.y_scale = 0.45 * w->core.height;
    w->clock.x_off = 0.5 * w->core.width;
    w->clock.y_off = 0.5 * w->core.height;
#endif
}

/* ARGSUSED */
static void 
Redisplay(Widget gw, XEvent *event, Region region)
{
    ClockWidget w = (ClockWidget) gw;
    if (w->clock.analog) {
#ifdef XRENDER
	if (w->clock.render)
	    XClipBox (region, &w->clock.damage);
	else
#endif
	{
	    if (w->clock.numseg != 0)
		erase_hands (w, (struct tm *) 0);
	    DrawClockFace(w);
	}
    } else {
	w->clock.prev_time_string[0] = '\0';
    }
    clock_tic((XtPointer)w, (XtIntervalId)0);
}

/* ARGSUSED */
static void 
clock_tic(XtPointer client_data, XtIntervalId *id)
{
        ClockWidget w = (ClockWidget)client_data;    
	struct tm tm; 
	Time_t	time_value;
	struct timeval	tv;
	char	*time_ptr;
        register Display *dpy = XtDisplay(w);
        register Window win = XtWindow(w);

	X_GETTIMEOFDAY (&tv);
	if (id || !w->clock.interval_id)
	    w->clock.interval_id =
		XtAppAddTimeOut( XtWidgetToApplicationContext( (Widget) w),
				(w->clock.update - 1) * 1000 + (1000000 - tv.tv_usec)/1000, clock_tic, (XtPointer)w );
	time_value = tv.tv_sec;
	tm = *localtime(&time_value);
	/*
	 * Beep on the half hour; double-beep on the hour.
	 */
	if (w->clock.chime == TRUE) {
	    if (w->clock.beeped && (tm.tm_min != 30) &&
		(tm.tm_min != 0))
	      w->clock.beeped = FALSE;
	    if (((tm.tm_min == 30) || (tm.tm_min == 0)) 
		&& (!w->clock.beeped)) {
		w->clock.beeped = TRUE;
#ifdef XKB
		if (tm.tm_min==0) {
		    XkbStdBell(dpy,win,50,XkbBI_ClockChimeHour);
		    XkbStdBell(dpy,win,50,XkbBI_RepeatingLastBell);
		}
		else {
		    XkbStdBell(dpy,win,50,XkbBI_ClockChimeHalf);
		}
#else
		XBell(dpy, 50);	
		if (tm.tm_min == 0)
		  XBell(dpy, 50);
#endif
	    }
	}
	if( w->clock.analog == FALSE ) {
	    int	clear_from;
	    int i, len, prev_len;

	    time_ptr = TimeString (w, &tm);
	    len = strlen (time_ptr);
	    if (time_ptr[len - 1] == '\n') time_ptr[--len] = '\0';
	    prev_len = strlen (w->clock.prev_time_string);
	    for (i = 0; ((i < len) && (i < prev_len) && 
	    		 (w->clock.prev_time_string[i] == time_ptr[i])); i++);
	    strcpy (w->clock.prev_time_string+i, time_ptr+i);

#ifdef XRENDER
	    if (w->clock.render)
	    {
		XRectangle  old_tail, new_tail, head;
		int	    x, y;

		RenderTextBounds (w, w->clock.prev_time_string, i, prev_len,
				  &old_tail, 0, 0);
		RenderUpdateRectBounds (&old_tail, &w->clock.damage);
		RenderTextBounds (w, time_ptr, i, len,
				  &new_tail, 0, 0);
		RenderUpdateRectBounds (&new_tail, &w->clock.damage);
		
		while (i)
		{
		    RenderTextBounds (w, time_ptr, 0, i, &head, 0, 0);
		    if (!RenderRectIn (&head, &w->clock.damage))
			break;
		    i--;
		}
		RenderTextBounds (w, time_ptr, i, len, &new_tail, &x, &y);
		RenderClip (w);
		RenderPrepare (w, 0);
		XftDrawString8 (w->clock.draw,
				&w->clock.hour_color,
				w->clock.face,
				x, y,
				(FcChar8 *) time_ptr + i, len - i);
		RenderResetBounds (&w->clock.damage);
	    }
	    else
#endif
	    {
	    XDrawImageString (dpy, win, w->clock.myGC,
			      (1+w->clock.padding +
			       XTextWidth (w->clock.font, time_ptr, i)),
			      w->clock.font->ascent+w->clock.padding,
			      time_ptr + i, len - i);
	    /*
	     * Clear any left over bits
	     */
	    clear_from = XTextWidth (w->clock.font, time_ptr, len)
 	    		+ 2 + w->clock.padding;
	    if (clear_from < (int)w->core.width)
		XFillRectangle (dpy, win, w->clock.EraseGC,
		    clear_from, 0, w->core.width - clear_from, w->core.height);
	    }
	} else {
			/*
			 * The second (or minute) hand is sec (or min) 
			 * sixtieths around the clock face. The hour hand is
			 * (hour + min/60) twelfths of the way around the
			 * clock-face.  The derivation is left as an excercise
			 * for the reader.
			 */

			/*
			 * 12 hour clock.
			 */
			if(tm.tm_hour >= 12)
				tm.tm_hour -= 12;

#ifdef XRENDER
			if (w->clock.render)
			{
			    /*
			     * Compute repaint area
			     */
			    if (tm.tm_min != w->clock.otm.tm_min ||
				tm.tm_hour != w->clock.otm.tm_hour)
			    {
				RenderHands (w, &w->clock.otm, False);
				RenderHands (w, &tm, False);
			    }
			    if (w->clock.show_second_hand &&
				tm.tm_sec != w->clock.otm.tm_sec)
			    {
				RenderSec (w, &w->clock.otm, False);
				RenderSec (w, &tm, False);
			    }
			    if (w->clock.damage.width && 
				w->clock.damage.height)
			    {
				RenderClip (w);
				DrawClockFace (w);
				RenderHands (w, &tm, True);
				if (w->clock.show_second_hand == TRUE)
				    RenderSec (w, &tm, True);
			    }
			    w->clock.otm = tm;
			    RenderResetBounds (&w->clock.damage);
			    return;
			}
#endif
		
			erase_hands (w, &tm);

		    if (w->clock.numseg == 0 ||
			tm.tm_min != w->clock.otm.tm_min ||
			tm.tm_hour != w->clock.otm.tm_hour) {
			    w->clock.segbuffptr = w->clock.segbuff;
			    w->clock.numseg = 0;
			    /*
			     * Calculate the hour hand, fill it in with its
			     * color and then outline it.  Next, do the same
			     * with the minute hand.  This is a cheap hidden
			     * line algorithm.
			     */
			    DrawHand(w,
				w->clock.minute_hand_length, w->clock.hand_width,
				tm.tm_min * 12
			    );
			    if(w->clock.Hdpixel != w->core.background_pixel)
				XFillPolygon( dpy,
				    win, w->clock.HandGC,
				    w->clock.segbuff, VERTICES_IN_HANDS,
				    Convex, CoordModeOrigin
				);
			    XDrawLines( dpy,
				win, w->clock.HighGC,
				w->clock.segbuff, VERTICES_IN_HANDS,
				       CoordModeOrigin);
			    w->clock.hour = w->clock.segbuffptr;
			    DrawHand(w, 
				w->clock.hour_hand_length, w->clock.hand_width,
				tm.tm_hour * 60 + tm.tm_min
			    );
			    if(w->clock.Hdpixel != w->core.background_pixel) {
			      XFillPolygon(dpy,
					   win, w->clock.HandGC,
					   w->clock.hour,
					   VERTICES_IN_HANDS,
					   Convex, CoordModeOrigin
					   );
			    }
			    XDrawLines( dpy,
				       win, w->clock.HighGC,
				       w->clock.hour, VERTICES_IN_HANDS,
				       CoordModeOrigin );

			    w->clock.sec = w->clock.segbuffptr;
		    }
		    if (w->clock.show_second_hand == TRUE) {
			    w->clock.segbuffptr = w->clock.sec;
			    DrawSecond(w,
				w->clock.second_hand_length - 2, 
				w->clock.second_hand_width,
				w->clock.minute_hand_length + 2,
				tm.tm_sec * 12
			    );
			    if(w->clock.Hdpixel != w->core.background_pixel)
				XFillPolygon( dpy,
				    win, w->clock.HandGC,
				    w->clock.sec,
				    VERTICES_IN_HANDS -2,
				    Convex, CoordModeOrigin
			    );
			    XDrawLines( dpy,
				       win, w->clock.HighGC,
				       w->clock.sec,
				       VERTICES_IN_HANDS-1,
				       CoordModeOrigin
				        );

			}
			w->clock.otm = tm;
		}
}
	
static void
erase_hands(ClockWidget w, struct tm *tm)
{
    /*
     * Erase old hands.
     */
    if(w->clock.numseg > 0) {
	Display	*dpy;
	Window	win;

	dpy = XtDisplay (w);
	win = XtWindow (w);
	if (w->clock.show_second_hand == TRUE) {
	    XDrawLines(dpy, win,
		w->clock.EraseGC,
		w->clock.sec,
		VERTICES_IN_HANDS-1,
		CoordModeOrigin);
	    if(w->clock.Hdpixel != w->core.background_pixel) {
		XFillPolygon(dpy,
			win, w->clock.EraseGC,
			w->clock.sec,
			VERTICES_IN_HANDS-2,
			Convex, CoordModeOrigin
			);
	    }
	}
	if(!tm || tm->tm_min != w->clock.otm.tm_min ||
		  tm->tm_hour != w->clock.otm.tm_hour)
 	{
	    XDrawLines( dpy, win,
			w->clock.EraseGC,
			w->clock.segbuff,
			VERTICES_IN_HANDS,
			CoordModeOrigin);
	    XDrawLines( dpy, win,
			w->clock.EraseGC,
			w->clock.hour,
			VERTICES_IN_HANDS,
			CoordModeOrigin);
	    if(w->clock.Hdpixel != w->core.background_pixel) {
		XFillPolygon( dpy, win,
 			      w->clock.EraseGC,
			      w->clock.segbuff, VERTICES_IN_HANDS,
			      Convex, CoordModeOrigin);
		XFillPolygon( dpy, win,
 			      w->clock.EraseGC,
			      w->clock.hour,
			      VERTICES_IN_HANDS,
			      Convex, CoordModeOrigin);
	    }
	}
    }
}

static float const Sines[] = {
.000000, .008727, .017452, .026177, .034899, .043619, .052336, .061049,
.069756, .078459, .087156, .095846, .104528, .113203, .121869, .130526,
.139173, .147809, .156434, .165048, .173648, .182236, .190809, .199368,
.207912, .216440, .224951, .233445, .241922, .250380, .258819, .267238,
.275637, .284015, .292372, .300706, .309017, .317305, .325568, .333807,
.342020, .350207, .358368, .366501, .374607, .382683, .390731, .398749,
.406737, .414693, .422618, .430511, .438371, .446198, .453990, .461749,
.469472, .477159, .484810, .492424, .500000, .507538, .515038, .522499,
.529919, .537300, .544639, .551937, .559193, .566406, .573576, .580703,
.587785, .594823, .601815, .608761, .615661, .622515, .629320, .636078,
.642788, .649448, .656059, .662620, .669131, .675590, .681998, .688355,
.694658, .700909, .707107
};

static float const Cosines[] = {
1.00000, .999962, .999848, .999657, .999391, .999048, .998630, .998135,
.997564, .996917, .996195, .995396, .994522, .993572, .992546, .991445,
.990268, .989016, .987688, .986286, .984808, .983255, .981627, .979925,
.978148, .976296, .974370, .972370, .970296, .968148, .965926, .963630,
.961262, .958820, .956305, .953717, .951057, .948324, .945519, .942641,
.939693, .936672, .933580, .930418, .927184, .923880, .920505, .917060,
.913545, .909961, .906308, .902585, .898794, .894934, .891007, .887011,
.882948, .878817, .874620, .870356, .866025, .861629, .857167, .852640,
.848048, .843391, .838671, .833886, .829038, .824126, .819152, .814116,
.809017, .803857, .798636, .793353, .788011, .782608, .777146, .771625,
.766044, .760406, .754710, .748956, .743145, .737277, .731354, .725374,
.719340, .713250, .707107
};

static void 
ClockAngle(int tick_units, double *sinp, double *cosp)
{
    int reduced, upper;

    reduced = tick_units % 90;
    upper = tick_units / 90;
    if (upper & 1)
	reduced = 90 - reduced;
    if ((upper + 1) & 2) {
	*sinp = Cosines[reduced];
	*cosp = Sines[reduced];
    } else {
	*sinp = Sines[reduced];
	*cosp = Cosines[reduced];
    }
    if (upper >= 2 && upper < 6)
	*cosp = -*cosp;
    if (upper >= 4)
	*sinp = -*sinp;
}

/*
 * DrawLine - Draws a line.
 *
 * blank_length is the distance from the center which the line begins.
 * length is the maximum length of the hand.
 * Tick_units is a number between zero and 12*60 indicating
 * how far around the circle (clockwise) from high noon.
 *
 * The blank_length feature is because I wanted to draw tick-marks around the
 * circle (for seconds).  The obvious means of drawing lines from the center
 * to the perimeter, then erasing all but the outside most pixels doesn't
 * work because of round-off error (sigh).
 */
static void 
DrawLine(ClockWidget w, Dimension blank_length, Dimension length, 
	 int tick_units)
{
	double dblank_length = (double)blank_length, dlength = (double)length;
	double cosangle, sinangle;
	int cx = w->clock.centerX, cy = w->clock.centerY, x1, y1, x2, y2;

	/*
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	ClockAngle(tick_units, &sinangle, &cosangle);

	/* break this out so that stupid compilers can cope */
	x1 = cx + (int)(dblank_length * sinangle);
	y1 = cy - (int)(dblank_length * cosangle);
	x2 = cx + (int)(dlength * sinangle);
	y2 = cy - (int)(dlength * cosangle);
	SetSeg(w, x1, y1, x2, y2);
}

/*
 * DrawHand - Draws a hand.
 *
 * length is the maximum length of the hand.
 * width is the half-width of the hand.
 * Tick_units is a number between zero and 12*60 indicating
 * how far around the circle (clockwise) from high noon.
 *
 */
static void 
DrawHand(ClockWidget w, Dimension length, Dimension width, int tick_units)
{

	double cosangle, sinangle;
	register double ws, wc;
	Position x, y, x1, y1, x2, y2;

	/*
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	ClockAngle(tick_units, &sinangle, &cosangle);
	/*
	 * Order of points when drawing the hand.
	 *
	 *		1,4
	 *		/ \
	 *	       /   \
	 *	      /     \
	 *	    2 ------- 3
	 */
	wc = width * cosangle;
	ws = width * sinangle;
	SetSeg(w,
	       x = w->clock.centerX + clock_round(length * sinangle),
	       y = w->clock.centerY - clock_round(length * cosangle),
	       x1 = w->clock.centerX - clock_round(ws + wc), 
	       y1 = w->clock.centerY + clock_round(wc - ws));  /* 1 ---- 2 */
	/* 2 */
	SetSeg(w, x1, y1, 
	       x2 = w->clock.centerX - clock_round(ws - wc), 
	       y2 = w->clock.centerY + clock_round(wc + ws));  /* 2 ----- 3 */

	SetSeg(w, x2, y2, x, y);	/* 3 ----- 1(4) */
}

/*
 * DrawSecond - Draws the second hand (diamond).
 *
 * length is the maximum length of the hand.
 * width is the half-width of the hand.
 * offset is direct distance from center to tail end.
 * Tick_units is a number between zero and 12*60 indicating
 * how far around the circle (clockwise) from high noon.
 *
 */
static void 
DrawSecond(ClockWidget w, Dimension length, Dimension width, 
	   Dimension offset, int tick_units)
{

	double cosangle, sinangle;
	register double ms, mc, ws, wc;
	register int mid;
	Position x, y;

	/*
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	ClockAngle(tick_units, &sinangle, &cosangle);
	/*
	 * Order of points when drawing the hand.
	 *
	 *		1,5
	 *		/ \
	 *	       /   \
	 *	      /     \
	 *	    2<       >4
	 *	      \     /
	 *	       \   /
	 *		\ /
	 *	-	 3
	 *	|
	 *	|
	 *   offset
	 *	|
	 *	|
	 *	-	 + center
	 */

	mid = (int) (length + offset) / 2;
	mc = mid * cosangle;
	ms = mid * sinangle;
	wc = width * cosangle;
	ws = width * sinangle;
	/*1 ---- 2 */
	SetSeg(w,
	       x = w->clock.centerX + clock_round(length * sinangle),
	       y = w->clock.centerY - clock_round(length * cosangle),
	       w->clock.centerX + clock_round(ms - wc),
	       w->clock.centerY - clock_round(mc + ws) );
	SetSeg(w, w->clock.centerX + clock_round(offset *sinangle),
	       w->clock.centerY - clock_round(offset * cosangle), /* 2-----3 */
	       w->clock.centerX + clock_round(ms + wc), 
	       w->clock.centerY - clock_round(mc - ws));
	w->clock.segbuffptr->x = x;
	w->clock.segbuffptr++->y = y;
	w->clock.numseg ++;
}

static void 
SetSeg(ClockWidget w, int x1, int y1, int x2, int y2)
{
	w->clock.segbuffptr->x = x1;
	w->clock.segbuffptr++->y = y1;
	w->clock.segbuffptr->x = x2;
	w->clock.segbuffptr++->y = y2;
	w->clock.numseg += 2;
}

/*
 *  Draw the clock face (every fifth tick-mark is longer
 *  than the others).
 */
static void 
DrawClockFace(ClockWidget w)
{
	register int i;
	register int delta = (int)(w->clock.radius - w->clock.second_hand_length) / 3;
	
	w->clock.segbuffptr = w->clock.segbuff;
	w->clock.numseg = 0;
	for (i = 0; i < 60; i++)
	{
#ifdef XRENDER
	    if (w->clock.render)
	    {
		double	s, c;
		XDouble	x1, y1, x2, y2;
		XftColor	*color;
		ClockAngle (i * 12, &s, &c);
		x1 = c;
		y1 = s;
		if (i % 5)
		{
		    x2 = c * (MINOR_TICK_FRACT / 100.0);
		    y2 = s * (MINOR_TICK_FRACT / 100.0);
		    color = &w->clock.minor_color;
		}
		else
		{
		    x2 = c * (SECOND_HAND_FRACT / 100.0);
		    y2 = s * (SECOND_HAND_FRACT / 100.0);
		    color = &w->clock.major_color;
		}
		RenderLine (w, x1, y1, x2, y2, color, True);
	    }
	    else
#endif
	    {
		DrawLine(w, ( (i % 5) == 0 ? 
			     w->clock.second_hand_length :
			     (w->clock.radius - delta) ),
			 w->clock.radius, i * 12);
	    }
	}
#ifdef XRENDER
	if (w->clock.render)
	    return;
#endif
	/*
	 * Go ahead and draw it.
	 */
	XDrawSegments(XtDisplay(w), XtWindow(w),
		      w->clock.myGC, (XSegment *) &(w->clock.segbuff[0]),
		      w->clock.numseg/2);
	
	w->clock.segbuffptr = w->clock.segbuff;
	w->clock.numseg = 0;
}

static int 
clock_round(double x)
{
	return(x >= 0.0 ? (int)(x + .5) : (int)(x - .5));
}

#ifdef XRENDER
static Boolean
sameColor (XftColor *old, XftColor *new)
{
    if (old->color.red != new->color.red) return False;
    if (old->color.green != new->color.green) return False;
    if (old->color.blue != new->color.blue) return False;
    if (old->color.alpha != new->color.alpha) return False;
    return True;
}
#endif

/* ARGSUSED */
static Boolean 
SetValues(Widget gcurrent, Widget grequest, Widget gnew, 
	  ArgList args, Cardinal *num_args)
{
      ClockWidget current = (ClockWidget) gcurrent;
      ClockWidget new = (ClockWidget) gnew;
      Boolean redisplay = FALSE;
      XtGCMask valuemask;
      XGCValues	myXGCV;

      /* first check for changes to clock-specific resources.  We'll accept all
         the changes, but may need to do some computations first. */

      if (new->clock.update != current->clock.update) {
	  if (current->clock.interval_id)
	      XtRemoveTimeOut (current->clock.interval_id);
	  if (XtIsRealized( (Widget) new))
	      new->clock.interval_id = XtAppAddTimeOut( 
                                         XtWidgetToApplicationContext(gnew),
					 new->clock.update*1000,
				         clock_tic, (XtPointer)gnew);

	  new->clock.show_second_hand =(new->clock.update <= SECOND_HAND_TIME);
	  if (new->clock.show_second_hand != current->clock.show_second_hand)
	    redisplay = TRUE;
      }

      if (new->clock.padding != current->clock.padding)
	   redisplay = TRUE;

      if (new->clock.analog != current->clock.analog)
	   redisplay = TRUE;

       if (new->clock.font != current->clock.font)
	   redisplay = TRUE;

      if ((new->clock.fgpixel != current->clock.fgpixel)
          || (new->core.background_pixel != current->core.background_pixel)) {
          valuemask = GCForeground | GCBackground | GCFont | GCLineWidth;
	  myXGCV.foreground = new->clock.fgpixel;
	  myXGCV.background = new->core.background_pixel;
          myXGCV.font = new->clock.font->fid;
	  myXGCV.line_width = 0;
	  XtReleaseGC (gcurrent, current->clock.myGC);
	  new->clock.myGC = XtGetGC(gcurrent, valuemask, &myXGCV);
	  redisplay = TRUE;
          }

      if (new->clock.Hipixel != current->clock.Hipixel) {
          valuemask = GCForeground | GCLineWidth;
	  myXGCV.foreground = new->clock.Hipixel;
          myXGCV.font = new->clock.font->fid;
	  myXGCV.line_width = 0;
	  XtReleaseGC (gcurrent, current->clock.HighGC);
	  new->clock.HighGC = XtGetGC((Widget)gcurrent, valuemask, &myXGCV);
	  redisplay = TRUE;
          }

      if (new->clock.Hdpixel != current->clock.Hdpixel) {
          valuemask = GCForeground;
	  myXGCV.foreground = new->clock.Hdpixel;
	  XtReleaseGC (gcurrent, current->clock.HandGC);
	  new->clock.HandGC = XtGetGC((Widget)gcurrent, valuemask, &myXGCV);
	  redisplay = TRUE;
          }

      if (new->core.background_pixel != current->core.background_pixel) {
          valuemask = GCForeground | GCLineWidth;
	  myXGCV.foreground = new->core.background_pixel;
	  myXGCV.line_width = 0;
	  XtReleaseGC (gcurrent, current->clock.EraseGC);
	  new->clock.EraseGC = XtGetGC((Widget)gcurrent, valuemask, &myXGCV);
	  redisplay = TRUE;
	  }
#ifdef XRENDER
     if (new->clock.face != current->clock.face)
	redisplay = TRUE;
     if (!sameColor (&new->clock.hour_color, &current->clock.hour_color) ||
	 !sameColor (&new->clock.min_color, &current->clock.min_color) ||
	 !sameColor (&new->clock.sec_color, &current->clock.sec_color) ||
	 !sameColor (&new->clock.major_color, &current->clock.major_color) ||
	 !sameColor (&new->clock.minor_color, &current->clock.minor_color))
	redisplay = True;
    if (new->clock.sharp != current->clock.sharp)
	redisplay = True;
    if (new->clock.render != current->clock.render)
	redisplay = True;
#endif
     return (redisplay);

}
