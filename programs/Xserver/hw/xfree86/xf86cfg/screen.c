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

#include <X11/IntrinsicP.h>
#include <X11/extensions/shape.h>
#include <X11/Xaw/Simple.h>
#include "screen.h"

/*
 * Prototypes
 */
extern void DrawCables(void);
void ReshapeScreenWidget(Widget);

/*
 * Initialization
 */
extern int sxpos, sypos;
extern Widget work;

static int rows, columns;	/* number of rows/columns of monitors */

/*
 * Implementation
 */
Widget
CreateScreenWidget(void)
{
    Widget w = XtCreateWidget("screen", simpleWidgetClass,
			      XtParent(computer.cpu), NULL, 0);

    XtRealizeWidget(w);
    ReshapeScreenWidget(w);

    return (w);
}

void
ReshapeScreenWidget(Widget w)
{
    Pixmap pixmap;
    XGCValues values;
    GC gc;

    pixmap = XCreatePixmap(XtDisplay(w), XtWindow(w),
			   w->core.width, w->core.height, 1);
    values.foreground = 0;
    values.background = 1;
    gc = XCreateGC(XtDisplay(w), pixmap, GCForeground | GCBackground, &values);
    XFillRectangle(XtDisplay(w), pixmap, gc, 0, 0, w->core.width, w->core.height);
    XSetForeground(XtDisplay(w), gc, 1);

    DrawScreenMask(XtDisplay(w), pixmap, gc, 0, 0, w->core.width, w->core.height);
    XShapeCombineMask(XtDisplay(w), XtWindow(w), ShapeBounding, 
		      0, 0, pixmap, ShapeSet);
    XFreePixmap(XtDisplay(w), pixmap);
    XFreeGC(XtDisplay(w), gc);
}

void
AddScreen(xf86cfgDevice *mon, xf86cfgDevice *dev)
{
    int nscreens = 0;
    char screen_name[48];
    XF86ConfScreenPtr screen = XF86Config->conf_screen_lst;
    XF86ConfAdjacencyPtr adj;

    while (screen != NULL) {
	++nscreens;
	screen = (XF86ConfScreenPtr)(screen->list.next);
    }
    do {
	++nscreens;
	XmuSnprintf(screen_name, sizeof(screen_name), "Screen%d",
		    nscreens);
    } while (xf86FindScreen(screen_name,
	     XF86Config->conf_screen_lst) != NULL);

    screen = (XF86ConfScreenPtr)XtCalloc(1, sizeof(XF86ConfScreenRec));
    screen->scrn_identifier = XtNewString(screen_name);
    screen->scrn_device_str = XtNewString(((XF86ConfDevicePtr)(dev->config))->dev_identifier);
    screen->scrn_device = (XF86ConfDevicePtr)(dev->config);
    screen->scrn_monitor_str = XtNewString(((XF86ConfMonitorPtr)(mon->config))->mon_identifier);
    screen->scrn_monitor = (XF86ConfMonitorPtr)(mon->config);
    XF86Config->conf_screen_lst =
	xf86AddScreen(XF86Config->conf_screen_lst, screen);

    adj = (XF86ConfAdjacencyPtr)XtCalloc(1, sizeof(XF86ConfAdjacencyRec));
    adj->adj_screen = screen;
    adj->adj_screen_str = XtNewString(screen_name);
    computer.layout->lay_adjacency_lst = (XF86ConfAdjacencyPtr)
	addListItem((GenericListPtr)computer.layout->lay_adjacency_lst,
		    (GenericListPtr)adj);

    computer.screens = (xf86cfgScreen**)
	XtRealloc((XtPointer)computer.screens, sizeof(xf86cfgScreen*) *
		  (computer.num_screens + 1));
    computer.screens[computer.num_screens] = XtNew(xf86cfgScreen);
    computer.screens[computer.num_screens]->screen = screen;
    computer.screens[computer.num_screens]->card = dev;
    computer.screens[computer.num_screens]->monitor = mon;

    computer.screens[computer.num_screens]->widget = CreateScreenWidget();
    computer.screens[computer.num_screens]->type = SCREEN;
    SetTip((xf86cfgDevice*)computer.screens[computer.num_screens]);

    ++computer.num_screens;
}

void
RemoveScreen(xf86cfgDevice *mon, xf86cfgDevice *dev)
{
    XF86ConfScreenPtr screen = XF86Config->conf_screen_lst;
    int i;

    mon->state = dev->state = UNUSED;
    while (screen != NULL) {
	if ((XtPointer)screen->scrn_monitor == mon->config &&
	    (XtPointer)screen->scrn_device == dev->config)
	    break;

	screen = (XF86ConfScreenPtr)(screen->list.next);
    }

    for (i = 0; i < computer.num_screens; i++) {
	if (computer.screens[i]->screen == screen) {
	    XtDestroyWidget(computer.screens[i]->widget);
	    if (i < --computer.num_screens)
		memmove(&computer.screens[i], &computer.screens[i + 1],
			(computer.num_screens - i) * sizeof(xf86cfgScreen*));
	    break;
	}
    }

    xf86RemoveScreen(XF86Config, screen);
}

void
ChangeScreen(XF86ConfMonitorPtr mon, XF86ConfMonitorPtr oldmon,
	     XF86ConfDevicePtr dev, XF86ConfDevicePtr olddev)
{
    int ioldm, im, ioldc, ic;

    if (mon == oldmon && dev == olddev)
	return;

    if (mon != NULL) {
	for (im = 0; im < computer.num_devices; im++)
	    if (computer.devices[im]->config == (XtPointer)mon)
		break;
    }
    else
	im = -1;
    if (oldmon != NULL) {
	for (ioldm = 0; ioldm < computer.num_devices; ioldm++)
	    if (computer.devices[ioldm]->config == (XtPointer)oldmon)
		break;
    }
    else
	ioldm = -1;

    if (dev != NULL) {
	for (ic = 0; ic < computer.num_devices; ic++)
	    if (computer.devices[ic]->config == (XtPointer)dev)
		break;
    }
    else
	ic = -1;
    if (olddev != NULL) {
	for (ioldc = 0; ioldc < computer.num_devices; ioldc++)
	    if (computer.devices[ioldc]->config == (XtPointer)olddev)
		break;
    }
    else
	ioldc = -1;

    if (ioldm >= 0 && ioldc >= 0) {
	RemoveScreen(computer.devices[ioldm], computer.devices[ioldc]);
	computer.devices[ioldm]->state = UNUSED;
/*	computer.devices[ioldc]->state = UNUSED;*/
    }

    if (im >= 0 && ic >= 0) {
	AddScreen(computer.devices[im], computer.devices[ic]);
	computer.devices[im]->state = USED;
/*	computer.devices[ic]->state = USED;*/
    }
}

/*

+------------------------------------------------+
|						 |
|  +------------------------------------------+  |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  |					      |	 |
|  +------------------------------------------+  |
|						 |
+------------------------------------------------+
	    |			     |
    +-------+			     +-------+
    |					     |
    +----------------------------------------+

 */
static double oxs = 0.0, oys = 0.0, oxe = 100.0, oye = 70.0;
static double ixs = 7.0, iys = 7.0, ixe = 93.0, iye = 63.0;
static double lin[] = { 25.0, 70.0, 25.0, 75.0,  5.0, 75.0,  5.0, 80.0,
			95.0, 80.0, 95.0, 75.0, 75.0, 75.0, 75.0, 70.0 };

void
DrawScreen(Display *dpy, Drawable win, GC gc, int xs, int ys, int xe, int ye,
	   Bool active)
{
    double xfact, yfact;
    XPoint points[(sizeof(lin) / sizeof(lin[0])) >> 1];
    int i;
    static GC gray1, gray2, black, red;

    if (black == NULL) {
	XColor color, exact;
	XGCValues values;

	XAllocNamedColor(XtDisplay(toplevel), toplevel->core.colormap, "gray75",
			 &color, &exact);
	values.foreground = color.pixel;
	gray1 = XCreateGC(XtDisplay(toplevel), win, GCForeground, &values);

	XAllocNamedColor(XtDisplay(toplevel), toplevel->core.colormap, "gray60",
			 &color, &exact);
	values.foreground = color.pixel;
	gray2 = XCreateGC(XtDisplay(toplevel), win, GCForeground, &values);

	XAllocNamedColor(XtDisplay(toplevel), toplevel->core.colormap, "gray20",
			 &color, &exact);
	values.foreground = color.pixel;
	black = XCreateGC(XtDisplay(toplevel), win, GCForeground, &values);

	XAllocNamedColor(XtDisplay(toplevel), toplevel->core.colormap, "red",
			 &color, &exact);
	values.foreground = color.pixel;
	values.line_width = 4;
	values.cap_style = CapButt;
	red = XCreateGC(XtDisplay(toplevel), win,
			GCForeground | GCLineWidth | GCCapStyle, &values);
    }

    xfact = (xe - xs) / 100.0;
    yfact = (ye - ys) / 80.0;

    /* outer rectangle */
    XFillRectangle(dpy, win, gray1,
		   oxs * xfact + xs,
		   oys * yfact + ys,
		   (oxe - oxs) * xfact,
		   (oye - oys) * yfact);

    XDrawLine(dpy, win, gray2,
	      oxs * xfact + xs, oye * yfact + ys - 1,
	      oxe * xfact + xs - 1, oye * yfact + ys - 1);

    XDrawLine(dpy, win, gray2,
	      oxe * xfact + xs - 1, oys * yfact + ys,
	      oxe * xfact + xs - 1, oye * yfact + ys - 1);

    /* inner rectangle */
    XFillRectangle(dpy, win, black,
		   ixs * xfact + xs,
		   iys * yfact + ys,
		   (ixe - ixs) * xfact,
		   (iye - iys) * yfact);

    for (i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
	points[i].x = lin[i<<1] * xfact + xs;
	points[i].y = lin[(i<<1) + 1] * yfact + ys;
    }

    XFillPolygon(dpy, win, gray2, points, i, Convex, CoordModeOrigin);

    XDrawLine(dpy, win, black,
	      lin[6] * xfact + xs, lin[7] * yfact + ys - 1,
	      lin[8] * xfact + xs - 1, lin[9] * yfact + ys - 1);
    XDrawLine(dpy, win, black,
	      lin[8] * xfact + xs - 1, lin[9] * yfact + ys - 1,
	      lin[10] * xfact + xs - 1, lin[11] * yfact + ys);

    XDrawLine(dpy, win, black,
	      lin[12] * xfact + xs - 1, lin[13] * yfact + ys,
	      lin[14] * xfact + xs - 1, lin[15] * yfact + ys);

    if (!active) {
	XDrawLine(dpy, win, red,
		  ixs * xfact, iys * yfact, ixe * xfact, iye * yfact);
	XDrawLine(dpy, win, red,
		  ixe * xfact, iys * yfact, ixs * xfact, iye * yfact);
    }
}

void
DrawScreenMask(Display *dpy, Drawable win, GC gc, int xs, int ys, int xe, int ye)
{
    double xfact, yfact;
    XPoint points[(sizeof(lin) / sizeof(lin[0])) >> 1];
    int i;

    xfact = (xe - xs) / 100.0;
    yfact = (ye - ys) / 80.0;

    /* rectangle */
    XFillRectangle(dpy, win, gc,
		   oxs * xfact + xs,
		   oys * yfact + ys,
		   (oxe - oxs) * xfact,
		   (oye - oys) * yfact);

    for (i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
	points[i].x = lin[i<<1] * xfact + xs;
	points[i].y = lin[(i<<1) + 1] * yfact + ys;
    }

    XFillPolygon(dpy, win, gc, points, i, Convex, CoordModeOrigin);
}

void
AdjustScreenUI(void)
{
    XF86ConfLayoutPtr lay = computer.layout;
    XF86ConfAdjacencyPtr adj;
    int i, dx, dy, w, h;
    double xf, yf;

    if (lay == NULL)
	return;

    adj = lay->lay_adjacency_lst;

#define USED1	-USED

    while (adj) {
	xf86cfgScreen *scr, *topscr, *botscr, *lefscr, *rigscr;

	for (i = 0; i < computer.num_screens; i++)
	    if (computer.screens[i]->screen == adj->adj_screen)
		break;
	    scr = computer.screens[i];

	if (adj->adj_top != NULL) {
	    for (i = 0; i < computer.num_screens; i++)
		if (computer.screens[i]->screen == adj->adj_top)
		    break;
	    topscr = computer.screens[i];
	}
	else
	    topscr = NULL;

	if (adj->adj_bottom != NULL) {
	    for (i = 0; i < computer.num_screens; i++)
		if (computer.screens[i]->screen == adj->adj_bottom)
		    break;
	    botscr = computer.screens[i];
	}
	else
	    botscr = NULL;

	if (adj->adj_left != NULL) {
	    for (i = 0; i < computer.num_screens; i++)
		if (computer.screens[i]->screen == adj->adj_left)
		    break;
	    lefscr = computer.screens[i];
	}
	else
	    lefscr = NULL;

	if (adj->adj_right != NULL) {
	    for (i = 0; i < computer.num_screens; i++)
		if (computer.screens[i]->screen == adj->adj_right)
		    break;
	    rigscr = computer.screens[i];
	}
	else
	    rigscr = NULL;

	if (lefscr == NULL && rigscr == NULL && topscr == NULL && lefscr == NULL) {
	    XF86ConfScreenPtr s;

	    if (adj->adj_where >= CONF_ADJ_RIGHTOF < adj->adj_where <= CONF_ADJ_BELOW) {
		s = xf86FindScreen(adj->adj_refscreen, XF86Config->conf_screen_lst);
		for (i = 0; i < computer.num_screens; i++)
		    if (computer.screens[i]->screen == s)
			break;
		switch (adj->adj_where) {
		    case CONF_ADJ_RIGHTOF:
			lefscr = computer.screens[i];
			break;
		    case CONF_ADJ_LEFTOF:
			rigscr = computer.screens[i];
			break;
		    case CONF_ADJ_ABOVE:
			botscr = computer.screens[i];
			break;
		    case CONF_ADJ_BELOW:
			topscr = computer.screens[i];
			break;
		}
	    }
	}

	XtMoveWidget(scr->widget, 0, 0);
	scr->state = USED1;
	if (lefscr != NULL) {
	    if (lefscr->state == USED1)
		XtMoveWidget(scr->widget,
			     lefscr->widget->core.x + lefscr->widget->core.width,
			     lefscr->widget->core.y);
	    else
		XtMoveWidget(lefscr->widget,
			     -(int)(lefscr->widget->core.width),
			     scr->widget->core.y);
	}

	if (rigscr != NULL) {
	    if (rigscr->state == USED1) {
		dx = rigscr->widget->core.x - scr->widget->core.width - scr->widget->core.x;
		dy = rigscr->widget->core.y - scr->widget->core.y;

		XtMoveWidget(scr->widget, scr->widget->core.x + dx,
			     scr->widget->core.y + dy);
		if (lefscr != NULL && lefscr->state != USED1)
		    XtMoveWidget(lefscr->widget, lefscr->widget->core.x + dx,
				 lefscr->widget->core.y + dy);
	    }
	    else
		XtMoveWidget(rigscr->widget, scr->widget->core.width,
			     scr->widget->core.y);
	}

	if (topscr != NULL) {
	    if (topscr->state == USED1) {
		dx = topscr->widget->core.x - scr->widget->core.x;
		dy = topscr->widget->core.y + topscr->widget->core.height -
		     scr->widget->core.y;

		XtMoveWidget(scr->widget, scr->widget->core.x + dx,
			     scr->widget->core.y + dy);
		if (lefscr != NULL && lefscr->state != USED1)
		    XtMoveWidget(lefscr->widget, lefscr->widget->core.x + dx,
				 lefscr->widget->core.y + dy);
		if (rigscr != NULL && rigscr->state != USED1)
		    XtMoveWidget(rigscr->widget, rigscr->widget->core.x + dx,
				 rigscr->widget->core.y + dy);
	    }
	    else
		XtMoveWidget(topscr->widget, scr->widget->core.x,
			     scr->widget->core.y - topscr->widget->core.height);
	}

	if (botscr != NULL) {
	    if (botscr->state == USED1) {
		dx = botscr->widget->core.x - scr->widget->core.x;
		dy = botscr->widget->core.y - scr->widget->core.height - scr->widget->core.y;

		XtMoveWidget(scr->widget, scr->widget->core.x + dx,
			     scr->widget->core.y + dy);
		if (lefscr != NULL && lefscr->state != USED1)
		    XtMoveWidget(lefscr->widget, lefscr->widget->core.x + dx,
				 lefscr->widget->core.y + dy);
		if (rigscr != NULL && rigscr->state != USED1)
		    XtMoveWidget(rigscr->widget, rigscr->widget->core.x + dx,
				 rigscr->widget->core.y + dy);
		if (botscr != NULL && botscr->state != USED1)
		    XtMoveWidget(botscr->widget, botscr->widget->core.x + dx,
				 botscr->widget->core.y + dy);
	    }
	    else
		XtMoveWidget(botscr->widget, scr->widget->core.x,
			     scr->widget->core.y + scr->widget->core.height);
	}

	adj = (XF86ConfAdjacencyPtr)(adj->list.next);
    }

    for (i = 0; i < computer.num_screens; i++)
	if (computer.screens[i]->state == USED1)
	    computer.screens[i]->state = USED;

    w = work->core.width / (columns + 1) - 5;
    h = work->core.height / (rows + 1) - 5;

    /*CONSTCOND*/
    while (True) {
	int num = w * 8 - h * 10;

	num = num < 0 ? -num : num;
	if (num < 6)
	    break;

	/* factor of drawing is w = 10 and h = 8 */
	if (w * 8 > h * 10)
	    w = (double)h * 10. / 8.;
	else if (w * 8 < h * 10)
	    h = (double)w * 8. / 10.;
	else
	    break;
    }

    dx = (work->core.width - (columns * w)) >> 1;
    dy = (work->core.height - (rows * h)) >> 1;

    xf = (double)w / (double)computer.screens[0]->widget->core.width;
    yf = (double)h / (double)computer.screens[0]->widget->core.height;

    for (i = 0; i < computer.num_screens; i++) {
	Widget z = computer.screens[i]->widget;

	if (computer.screens[i]->state == USED)
	    XtConfigureWidget(z, z->core.x * xf + dx, z->core.y * yf + dy, w, h, 0);
	else
	    XtConfigureWidget(z, z->core.x, z->core.y, w, h, 0);
	ReshapeScreenWidget(z);
    }
}

int
qcmp_screen(_Xconst void *a, _Xconst void *b)
{
    xf86cfgScreen *s1, *s2;

    s1 = *(xf86cfgScreen**)a;
    s2 = *(xf86cfgScreen**)b;

    if (s1->widget->core.x > s2->widget->core.x) {
	if (s2->widget->core.y >=
	    s1->widget->core.y + (s1->widget->core.height >> 1))
	    return (-1);
	return (1);
    }
    else {
	if (s1->widget->core.y >=
	    s2->widget->core.y + (s2->widget->core.height >> 1))
	    return (1);
	return (-1);
    }
    /*NOTREACHED*/
}

void
UpdateScreenUI(void)
{
    XF86ConfLayoutPtr lay = computer.layout;
    XF86ConfAdjacencyPtr adj, prev, left, base;
    int i, p, cols;

    if (lay == NULL)
	return;

    rows = columns = cols = 1;

    qsort(computer.screens, computer.num_screens, sizeof(xf86cfgScreen*),
	  qcmp_screen);

    adj = prev, base = NULL;
    for (i = p = 0; i < computer.num_screens; p = i, i++) {
	XF86ConfScreenPtr scr = computer.screens[i]->screen;

	if (computer.screens[i]->state == UNUSED)
	    continue;

	adj = (XF86ConfAdjacencyPtr)XtCalloc(1, sizeof(XF86ConfAdjacencyRec));
	adj->adj_scrnum = i;
	adj->adj_screen = scr;
	adj->adj_screen_str = XtNewString(scr->scrn_identifier);
	if (base == NULL)
	    base = left = adj;
	else {
	    int dy = computer.screens[i]->widget->core.y -
		     computer.screens[p]->widget->core.y;

	    prev->list.next = adj;
	    if (dy > (computer.screens[i]->widget->core.height >> 1)) {
		adj->adj_where = CONF_ADJ_BELOW;
		adj->adj_refscreen = XtNewString(left->adj_screen_str);
		left = adj;
		cols = 1; 
		++rows;
	    }
	    else {
		adj->adj_where = CONF_ADJ_RIGHTOF;
		if (++cols > columns)
		    columns = cols;
		adj->adj_refscreen = XtNewString(prev->adj_screen_str);
	    }
	}
	prev = adj;
    }

    adj = lay->lay_adjacency_lst;

    while (adj != NULL) {
	prev = adj;
	adj = (XF86ConfAdjacencyPtr)(adj->list.next);
	XtFree(prev->adj_screen_str);
	XtFree(prev->adj_right_str);
	XtFree(prev->adj_left_str);
	XtFree(prev->adj_top_str);
	XtFree(prev->adj_bottom_str);
	XtFree(prev->adj_refscreen);
	XtFree((char*)prev);
    }

    lay->lay_adjacency_lst = base;
}


void
ClearScreenUI(void)
{
    int i;

    for (i = 0; i < computer.num_screens; i++)
	XClearArea(XtDisplay(toplevel), XtWindow(computer.screens[i]->widget),
		   0, 0, computer.screens[i]->widget->core.width,
		   computer.screens[i]->widget->core.height, True);
}
