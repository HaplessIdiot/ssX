/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/lisp/modules/x11.c,v 1.5 2002/01/30 21:01:00 paulo Exp $ */

#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "private.h"
#include <X11/Xlib.h>

/*
 * Prototypes
 */
int x11LoadModule(LispMac*);

LispObj *Lisp_XOpenDisplay(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XCloseDisplay(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDefaultRootWindow(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDefaultScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDefaultScreenOfDisplay(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XBlackPixel(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XBlackPixelOfScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XWidthOfScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XHeightOfScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XWhitePixel(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XWhitePixelOfScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDefaultGC(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDefaultGCOfScreen(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XCreateSimpleWindow(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XMapWindow(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XDestroyWindow(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XFlush(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XRaiseWindow(LispMac*, LispBuiltin *builtin);
LispObj *Lisp_XBell(LispMac*, LispBuiltin *builtin);

LispObj *Lisp_XDrawLine(LispMac*, LispBuiltin *builtin);

/*
 * Initialization
 */
static LispBuiltin lispbuiltins[] = {
    {LispFunction, Lisp_XOpenDisplay, "x-open-display &optional display-name"},
    {LispFunction, Lisp_XCloseDisplay, "x-close-display display"},
    {LispFunction, Lisp_XDefaultRootWindow, "x-default-root-window display"},
    {LispFunction, Lisp_XDefaultScreen, "x-default-screen display"},
    {LispFunction, Lisp_XDefaultScreenOfDisplay, "x-default-screen-of-display display"},
    {LispFunction, Lisp_XBlackPixel, "x-black-pixel display &optional screen"},
    {LispFunction, Lisp_XBlackPixelOfScreen, "x-black-pixel-of-screen screen"},
    {LispFunction, Lisp_XWhitePixel, "x-white-pixel display &optional screen"},
    {LispFunction, Lisp_XWhitePixelOfScreen, "x-white-pixel-of-screen screen"},
    {LispFunction, Lisp_XDefaultGC, "x-default-gc display &optional screen"},
    {LispFunction, Lisp_XDefaultGCOfScreen, "x-default-gc-of-screen screen"},
    {LispFunction, Lisp_XCreateSimpleWindow, "x-create-simple-window display parent x y width height &optional border-width border background"},
    {LispFunction, Lisp_XMapWindow, "x-map-window display window"},
    {LispFunction, Lisp_XDestroyWindow, "X-DESTROY-WINDOW"},
    {LispFunction, Lisp_XFlush, "x-flush display"},
    {LispFunction, Lisp_XDrawLine, "x-draw-line display drawable gc x1 y1 x2 y2"},
    {LispFunction, Lisp_XBell, "x-bell display &optional percent"},
    {LispFunction, Lisp_XRaiseWindow, "x-raise-window display window"},
    {LispFunction, Lisp_XWidthOfScreen, "x-width-of-screen screen"},
    {LispFunction, Lisp_XHeightOfScreen, "x-height-of-screen screen"},
};

LispModuleData x11LispModuleData = {
    LISP_MODULE_VERSION,
    x11LoadModule
};

static int x11Display_t, x11Screen_t, x11Window_t, x11GC_t;

/*
 * Implementation
 */
int
x11LoadModule(LispMac *mac)
{
    int i;

    x11Display_t = LispRegisterOpaqueType(mac, "Display*");
    x11Screen_t = LispRegisterOpaqueType(mac, "Screen*");
    x11Window_t = LispRegisterOpaqueType(mac, "Window");
    x11GC_t = LispRegisterOpaqueType(mac, "GC");

    for (i = 0; i < sizeof(lispbuiltins) / sizeof(lispbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &lispbuiltins[i]);

    return (1);
}

LispObj *
Lisp_XOpenDisplay(LispMac *mac, LispBuiltin *builtin)
/*
x-open-display &optional display-name
 */
{
    LispObj *display_name;
    char *dname;

    display_name = ARGUMENT(0);

    if (display_name == NIL)	dname = NULL;
    else			ERROR_CHECK_STRING(display_name);
    else			dname = THESTR(display_name);

    return (OPAQUE(XOpenDisplay(dname), x11Display_t));
}

LispObj *
Lisp_XCloseDisplay(LispMac *mac, LispBuiltin *builtin)
/*
 x-close-display display
 */
{
    LispObj *display;

    display = ARGUMENT(0);

    if (!CHECKO(display, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(display));

    XCloseDisplay((Display*)(display->data.opaque.data));

    return (NIL);
}

LispObj *
Lisp_XDefaultRootWindow(LispMac *mac, LispBuiltin *builtin)
/*
 x-default-root-window display
 */
{
    LispObj *display;

    display = ARGUMENT(0);

    if (!CHECKO(display, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(display));

    return (OPAQUE(DefaultRootWindow((Display*)(display->data.opaque.data)),
		   x11Window_t));
}

LispObj *
Lisp_XDefaultScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-default-screen display
 */
{
    LispObj *display;

    display = ARGUMENT(0);

    if (!CHECKO(display, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(display));

    return (INTEGER(DefaultScreen((Display*)(display->data.opaque.data))));
}

LispObj *
Lisp_XDefaultScreenOfDisplay(LispMac *mac, LispBuiltin *builtin)
/*
 x-default-screen-of-display display
 */
{
    LispObj *display;

    display = ARGUMENT(0);

    if (!CHECKO(display, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(display));

    return (OPAQUE(DefaultScreenOfDisplay((Display*)(display->data.opaque.data)),
		   x11Screen_t));
}

LispObj *
Lisp_XBlackPixel(LispMac *mac, LispBuiltin *builtin)
/*
 x-black-pixel display &optional screen
 */
{
    Display *display;
    int screen;

    LispObj *odisplay, *oscreen;

    oscreen = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (oscreen == NIL)	screen = DefaultScreen(display);
    else		ERROR_CHECK_INDEX(oscreen);
    else		screen = oscreen->data.integer;

    if (screen >= ScreenCount(display))
	LispDestroy(mac, "%s: screen index %d too large, %d screens available",
		    STRFUN(builtin), screen, ScreenCount(display));

    return (INTEGER(BlackPixel(display, screen)));
}

LispObj *
Lisp_XBlackPixelOfScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-black-pixel-of-screen screen
 */
{
    LispObj *screen;

    screen = ARGUMENT(0);

    if (!CHECKO(screen, x11Screen_t))
	LispDestroy(mac, "%s: cannot convert %s to Screen*",
		    STRFUN(builtin), STROBJ(screen));

    return (INTEGER(XBlackPixelOfScreen((Screen*)(screen->data.opaque.data))));
}

LispObj *
Lisp_XWhitePixel(LispMac *mac, LispBuiltin *builtin)
/*
 x-white-pixel display &optional screen
 */
{
    Display *display;
    int screen;

    LispObj *odisplay, *oscreen;

    oscreen = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (oscreen == NIL)	screen = DefaultScreen(display);
    else		ERROR_CHECK_FIXNUM(oscreen);
    else		screen = oscreen->data.integer;

    if (screen >= ScreenCount(display))
	LispDestroy(mac, "%s: screen index %d too large, %d screens available",
		    STRFUN(builtin), screen, ScreenCount(display));

    return (INTEGER(WhitePixel(display, screen)));
}

LispObj *
Lisp_XWhitePixelOfScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-white-pixel-of-screen screen
 */
{
    LispObj *screen;

    screen = ARGUMENT(0);

    if (!CHECKO(screen, x11Screen_t))
	LispDestroy(mac, "%s: cannot convert %s to Screen*",
		    STRFUN(builtin), STROBJ(screen));

    return (INTEGER(WhitePixelOfScreen((Screen*)(screen->data.opaque.data))));
}

LispObj *
Lisp_XDefaultGC(LispMac *mac, LispBuiltin *builtin)
/*
 x-default-gc display &optional screen
 */
{
    Display *display;
    int screen;

    LispObj *odisplay, *oscreen;

    oscreen = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (oscreen == NIL)	screen = DefaultScreen(display);
    else		ERROR_CHECK_FIXNUM(oscreen);
    else		screen = oscreen->data.integer;

    if (screen >= ScreenCount(display))
	LispDestroy(mac, "%s: screen index %d too large, %d screens available",
		    STRFUN(builtin), screen, ScreenCount(display));

    return (OPAQUE(DefaultGC(display, screen), x11GC_t));
}

LispObj *
Lisp_XDefaultGCOfScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-default-gc-of-screen screen
 */
{
    LispObj *screen;

    screen = ARGUMENT(0);

    if (!CHECKO(screen, x11Screen_t))
	LispDestroy(mac, "%s: cannot convert %s to Screen*",
		    STRFUN(builtin), STROBJ(screen));

    return (OPAQUE(DefaultGCOfScreen((Screen*)(screen->data.opaque.data)),
		   x11GC_t));
}

LispObj *
Lisp_XCreateSimpleWindow(LispMac *mac, LispBuiltin *builtin)
/*
 x-create-simple-window display parent x y width height &optional border-width border background
 */
{
    Display *display;
    Window parent;
    int x, y;
    unsigned int width, height, border_width;
    unsigned long border, background;

    LispObj *odisplay, *oparent, *ox, *oy, *owidth, *oheight,
	    *oborder_width, *oborder, *obackground;

    obackground = ARGUMENT(8);
    oborder = ARGUMENT(7);
    oborder_width = ARGUMENT(6);
    oheight = ARGUMENT(5);
    owidth = ARGUMENT(4);
    oy = ARGUMENT(3);
    ox = ARGUMENT(2);
    oparent = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (!CHECKO(oparent, x11Window_t))
	LispDestroy(mac, "%s: cannot convert %s to Window",
		    STRFUN(builtin), STROBJ(oparent));
    parent = (Window)(oparent->data.opaque.data);

    ERROR_CHECK_FIXNUM(ox);
    x = ox->data.integer;

    ERROR_CHECK_FIXNUM(oy);
    y = oy->data.integer;

    ERROR_CHECK_INDEX(owidth);
    width = owidth->data.integer;

    ERROR_CHECK_INDEX(oheight);
    height = oheight->data.integer;

    /* check &OPTIONAL parameters */
    if (oborder_width == NIL)	border_width = 1;
    else			ERROR_CHECK_INDEX(oborder_width);
    else			border_width = oborder_width->data.integer;

    if (oborder == NIL)	border = BlackPixel(display, DefaultScreen(display));
    else		ERROR_CHECK_FIXNUM(oborder);
    else		border = oborder->data.integer;

    if (obackground == NIL)
	background = WhitePixel(display, DefaultScreen(display));
    else
	ERROR_CHECK_FIXNUM(obackground);
    else
	background = obackground->data.integer;

    return (OPAQUE(
	    XCreateSimpleWindow(display, parent, x, y, width, height,
				border_width, border, background),
	    x11Window_t));
}

LispObj *
Lisp_XMapWindow(LispMac *mac, LispBuiltin *builtin)
/*
 x-map-window display window
 */
{
    Display *display;
    Window window;

    LispObj *odisplay, *owindow;

    owindow = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (!CHECKO(owindow, x11Window_t))
	LispDestroy(mac, "%s: cannot convert %s to Window",
		    STRFUN(builtin), STROBJ(owindow));
    window = (Window)(owindow->data.opaque.data);

    XMapWindow(display, window);

    return (owindow);
}

LispObj *
Lisp_XDestroyWindow(LispMac *mac, LispBuiltin *builtin)
/*
 x-destroy-window display window
 */
{
    Display *display;
    Window window;

    LispObj *odisplay, *owindow;

    owindow = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (!CHECKO(owindow, x11Window_t))
	LispDestroy(mac, "%s: cannot convert %s to Window",
		    STRFUN(builtin), STROBJ(owindow));
    window = (Window)(owindow->data.opaque.data);

    XDestroyWindow(display, window);

    return (NIL);
}

LispObj *
Lisp_XFlush(LispMac *mac, LispBuiltin *builtin)
/*
 x-flush display
 */
{
    Display *display;

    LispObj *odisplay;

    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    XFlush(display);

    return (odisplay);
}

LispObj *
Lisp_XDrawLine(LispMac *mac, LispBuiltin *builtin)
/*
 x-draw-line display drawable gc x1 y1 x2 y2
 */
{
    Display *display;
    Drawable drawable;
    GC gc;
    int x1, y1, x2, y2;

    LispObj *odisplay, *odrawable, *ogc, *ox1, *oy1, *ox2, *oy2;

    oy2 = ARGUMENT(6);
    ox2 = ARGUMENT(5);
    oy1 = ARGUMENT(4);
    ox1 = ARGUMENT(3);
    ogc = ARGUMENT(2);
    odrawable = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    /* XXX correct check when drawing to pixmaps implemented */
    if (!CHECKO(odrawable, x11Window_t))
	LispDestroy(mac, "%s: cannot convert %s to Drawable",
		    STRFUN(builtin), STROBJ(odrawable));
    drawable = (Drawable)(odrawable->data.opaque.data);

    if (!CHECKO(ogc, x11GC_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(ogc));
    gc = (GC)(ogc->data.opaque.data);

    ERROR_CHECK_FIXNUM(ox1);
    x1 = ox1->data.integer;

    ERROR_CHECK_FIXNUM(oy1);
    y1 = oy1->data.integer;

    ERROR_CHECK_FIXNUM(ox2);
    x2 = ox2->data.integer;

    ERROR_CHECK_FIXNUM(oy2);
    y2 = oy2->data.integer;

    XDrawLine(display, drawable, gc, x1, y1, x2, y2);

    return (odrawable);
}

LispObj *
Lisp_XBell(LispMac *mac, LispBuiltin *builtin)
/*
 x-bell &optional percent
 */
{
    Display *display;
    int percent;

    LispObj *odisplay, *opercent;

    opercent = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (opercent == NIL)	percent = 0;
    else			ERROR_CHECK_FIXNUM(opercent);
    else			percent = opercent->data.integer;

    if (percent < -100 || percent > 100)
	LispDestroy(mac, "%s: percent value %d out of range -100 to 100",
		    STRFUN(builtin), percent);

    XBell(display, percent);

    return (odisplay);
}

LispObj *
Lisp_XRaiseWindow(LispMac *mac, LispBuiltin *builtin)
/*
 x-raise-window display window
 */
{
    Display *display;
    Window window;

    LispObj *odisplay, *owindow;

    owindow = ARGUMENT(1);
    odisplay = ARGUMENT(0);

    if (!CHECKO(odisplay, x11Display_t))
	LispDestroy(mac, "%s: cannot convert %s to Display*",
		    STRFUN(builtin), STROBJ(odisplay));
    display = (Display*)(odisplay->data.opaque.data);

    if (!CHECKO(owindow, x11Window_t))
	LispDestroy(mac, "%s: cannot convert %s to Window",
		    STRFUN(builtin), STROBJ(owindow));
    window = (Window)(owindow->data.opaque.data);

    XRaiseWindow(display, window);

    return (owindow);
}

LispObj *
Lisp_XWidthOfScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-width-of-screen screen
 */
{
    LispObj *screen;

    screen = ARGUMENT(0);

    if (!CHECKO(screen, x11Screen_t))
	LispDestroy(mac, "%s: cannot convert %s to Screen*",
		    STRFUN(builtin), STROBJ(screen));

    return (INTEGER(WidthOfScreen((Screen*)(screen->data.opaque.data))));
}

LispObj *
Lisp_XHeightOfScreen(LispMac *mac, LispBuiltin *builtin)
/*
 x-height-of-screen screen
 */
{
    LispObj *screen;

    screen = ARGUMENT(0);

    if (!CHECKO(screen, x11Screen_t))
	LispDestroy(mac, "%s: cannot convert %s to Screen*",
		    STRFUN(builtin), STROBJ(screen));

    return (INTEGER(HeightOfScreen((Screen*)(screen->data.opaque.data))));
}
