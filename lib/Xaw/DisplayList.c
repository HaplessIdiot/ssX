/*
 * $Id: DisplayList.c,v 3.2 1998/04/28 13:33:32 robin Exp $
 *
 * Copyright (c) 1996 by The XFree86 Project, Inc.
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
 */

/* $XFree86: $ */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include "Private.h"

/*
 * Types
 */
typedef struct _XawDLProc XawDLProc;
typedef struct _XawDLData XawDLData;
typedef struct _XawDLInfo XawDLInfo;

struct _XawDLProc {
  XrmQuark qname;
  String *params;
  Cardinal num_params;
  XawDisplayListProc proc;
  XtPointer args;
  XawDLData *data;
};

struct _XawDLData {
  XawDLClass *dlclass;
  XtPointer data;
};

struct _XawDLInfo {
  String name;
  XrmQuark qname;
  XawDisplayListProc proc;
};

struct _XawDL {
  XawDLProc **procs;
  Cardinal num_procs;
  XawDLData **data;
  Cardinal num_data;
  Screen *screen;
  Colormap colormap;
  int depth;
};

struct _XawDLClass {
  String name;
  XawDLInfo **infos;
  Cardinal num_infos;
  XawDLArgsInitProc args_init;
  XawDLArgsDestructor args_destructor;
  XawDLDataInitProc data_init;
  XawDLDataDestructor data_destructor;
};

/*
 * Private Methods
 */
static XawDLClass *_XawFindDLClass(String);
static int qcmp_dlist_class(_Xconst void*, _Xconst void*);
static int bcmp_dlist_class(_Xconst void*, _Xconst void*);
static XawDLInfo *_XawFindDLInfo(XawDLClass*, String);
static int qcmp_dlist_info(_Xconst void*, _Xconst void*);
static int bcmp_dlist_info(_Xconst void*, _Xconst void*);
static void _XawDestroyDisplayList(XawDisplayList*);
static void *_Xaw_Xlib_ArgsInitProc(String, String*, Cardinal*,
				    Screen*, Colormap, int);
static void _Xaw_Xlib_ArgsDestructor(Display*, String, XtPointer,
				     String*, Cardinal*);
static void *_Xaw_Xlib_DataInitProc(String, Screen*, Colormap, int);
static void _Xaw_Xlib_DataDestructor(Display*, String, XtPointer);

/*
 * Initialization
 */
static XawDLClass **classes;
static Cardinal num_classes;
static String xlib = "xlib";

/*
 * Implementation
 */
void XawRunDisplayList(Widget w, XawDisplayList *list,
		       XEvent *event, Region region)
{
  XawDLProc *proc;
  Cardinal i;

  for (i = 0; i < list->num_procs; i++)
    {
      proc = list->procs[i];
      proc->proc(w, proc->args, proc->data->data, event, region);
    }
}

#define DLERR  -2
#define DLEOF  -1
#define DLEND  1
#define DLNAME 2
#define DLARG  3
static char *
read_token(char *src, char *dst, Cardinal size, int *status)
{
  int ch, i;
  Boolean esc;

  i = 0;
  esc = False;

  while (1)
    {
      ch = *src;
      if (ch != '\n' && isspace(ch))
	++src;
      else
	break;
    }

  for (; i < size - 1; src++)
    {
      ch = *src;
      if (ch == '\\')
	{
	  if (esc)
	    {
	      dst[i++] = ch;
	      esc = False;
	      continue;
	    }
	  esc = True;
	}
      if (ch == '\0')
	{
	  *status = DLEOF;
	  dst[i] = '\0';
	  return (src);
	}
      else if (!esc)
	{
	  if (ch == ',')
	    {
	      *status = DLARG;
	      dst[i] = '\0';
	      return (++src);
	    }
	  else if (ch == ' ' || ch == '\t')
	    {
	      *status = DLNAME;
	      dst[i] = '\0';
	      return (++src);
	    }
	  else if (ch == ';' || ch == '\n')
	    {
	      *status = DLEND;
	      dst[i] = '\0';
	      return (++src);
	    }
	}
      dst[i++] = ch;
    }

  *status = DLERR;
  dst[i] = '\0';

  return (src);
}

XawDisplayList *XawCreateDisplayList(String string, Screen *screen,
				     Colormap colormap, int depth)
{
  XawDisplayList *dlist;
  XawDLClass *lc, *xlibc;
  XawDLData *data;
  XawDLInfo *info;
  XawDLProc *proc;
  char cname[64], fname[64], aname[128];
  Cardinal i;
  char *cp, *fp, *lp;
  int status;

  xlibc = XawGetDisplayListClass(xlib);
  if (!xlibc)
    {
      XawDisplayListInitialize();
      xlibc = XawGetDisplayListClass(xlib);
    }

  dlist = (XawDisplayList *)XtMalloc(sizeof(XawDisplayList));
  dlist->procs = NULL;
  dlist->num_procs = 0;
  dlist->data = NULL;
  dlist->num_data = NULL;
  dlist->screen = screen;
  dlist->colormap = colormap;
  dlist->depth = depth;

  cp = string;

  status = 0;
  while (status != DLEOF)
    {
      lp = cp;
      cp = read_token(cp, fname, sizeof(fname), &status);
      if (status == DLEOF)
	break;
      if (status == DLEND)
	continue;
      if (status != DLNAME)
	{
	  char msg[256];

	  snprintf(msg, sizeof(msg),
		   "Error parsing displayList at \"%s\"", lp);
	  XtAppWarning(XtDisplayToApplicationContext(DisplayOfScreen(screen)),
		       msg);
	  _XawDestroyDisplayList(dlist);
	  return (NULL);
	}
      fp = fname; 
      while (1)
	{
	  fp = strchr(fp, ':');
	  if (!fp || (fp == cp || fp[-1] != '\\'))
	    break;
	  ++fp;
	}
      if (fp)
	{
	  snprintf(cname, fp - fname + 1, fname);
	  bcopy(fp + 1, fname, strlen(fp));
	  lc = cname[0] ? XawGetDisplayListClass(cname) : xlibc;
	  if (!lc)
	    {
	      char msg[256];

	      snprintf(msg, sizeof(msg),
		       "Cannot find displayList class \"%s\"", cname);
	      XtAppWarning(XtDisplayToApplicationContext
			   (DisplayOfScreen(screen)), msg);
	      _XawDestroyDisplayList(dlist);
	      return (NULL);
	    }
	}
      else
	lc = xlibc;

      if ((info = _XawFindDLInfo(lc, fname)) == NULL)
	{
	  char msg[256];

	  snprintf(msg, sizeof(msg),
		   "Cannot find displayList procedure \"%s\"", fname);
	  XtAppWarning(XtDisplayToApplicationContext(DisplayOfScreen(screen)),
		       msg);
	  _XawDestroyDisplayList(dlist);
	  return (NULL);
	}

      proc = (XawDLProc *)XtMalloc(sizeof(XawDLProc));
      proc->qname = info->qname;
      proc->params = NULL;
      proc->num_params = 0;
      proc->proc = info->proc;
      proc->args = NULL;

      if (!dlist->procs)
	{
	  dlist->num_procs = 1;
	  dlist->procs = (XawDLProc**)XtMalloc(sizeof(XawDLProc*));
	}
      else
	{
	  ++dlist->num_procs;
	  dlist->procs = (XawDLProc**)
	    XtRealloc((char *)dlist->procs, sizeof(XawDLProc*) *
		      dlist->num_procs);
	}
      dlist->procs[dlist->num_procs - 1] = proc;

      if (status == DLEND)
	continue;

      while (1)
	{
	  lp = cp;
	  cp = read_token(cp, aname, sizeof(aname), &status);

	  if (status != DLARG && status != DLEND && status != DLEOF)
	    {
	      char msg[256];

	      snprintf(msg, sizeof(msg),
		       "Error parsing displayList at \"%s\"", lp);
	      XtAppWarning(XtDisplayToApplicationContext
			   (DisplayOfScreen(screen)), msg);
	      _XawDestroyDisplayList(dlist);
	      return (NULL);
	    }

	  if (!proc->num_params)
	    {
	      proc->num_params = 1;
	      proc->params = (String *)XtMalloc(sizeof(String));
	    }
	  else
	    {
	      ++proc->num_params;
	      proc->params = (String *)XtRealloc((char *)proc->params,
						 sizeof(String) *
						 proc->num_params);
	    }
	  proc->params[proc->num_params - 1] = XtNewString(aname);

	  if (status == DLEND || status == DLEOF)
	    break;
	}

      /* verify if data is already created for lc */
      data = NULL;
      for (i = 0; i < dlist->num_data; i++)
	if (dlist->data[i]->dlclass == lc)
	  {
	    data = dlist->data[i];
	    break;
	  }

      if (!data)
	{
	  data = (XawDLData *)XtMalloc(sizeof(XawDLData));
	  data->dlclass = lc;
	  if (lc->data_init)
	    data->data = lc->data_init(lc->name, screen, colormap, depth);
	  else
	    data->data = NULL;

	  if (!dlist->data)
	    {
	      dlist->num_data = 1;
	      dlist->data = (XawDLData **)XtMalloc(sizeof(XawDLData*));
	    }
	  else
	    {
	      ++dlist->num_data;
	      dlist->data = (XawDLData **)
		XtRealloc((char *)dlist->data, sizeof(XawDLData*) *
			  dlist->num_data);
	    }
	  dlist->data[dlist->num_data - 1] = data;
	}

      if (lc->args_init)
	proc->args = lc->args_init(fname, proc->params, &proc->num_params,
				   screen, colormap, depth);
      else
	proc->args = NULL;

      proc->data = data;
    }

  return (dlist);
}

void
_XawDestroyDisplayList(XawDisplayList *dlist)
{
  Cardinal i, j;
  XawDLProc *proc;
  XawDLData *data;

  for (i = 0; i < dlist->num_procs; i++)
    {
      proc = dlist->procs[i];
      data = proc->data;

      if (data->dlclass->args_destructor)
	data->dlclass->args_destructor(DisplayOfScreen(dlist->screen),
				       XrmQuarkToString(proc->qname),
				       proc->args,
				       proc->params, &proc->num_params);
      if (data->data)
	{
	  if (data->dlclass->data_destructor)
	    {
	      data->dlclass->data_destructor(DisplayOfScreen(dlist->screen),
					     XrmQuarkToString(proc->qname),
					     data->data);
	      data->data = NULL;
	    }
	}

      for (j = 0; j < proc->num_params; j++)
	XtFree(proc->params[i]);
      XtFree((char *)proc->params);
      XtFree((char *)proc);
    }

  XtFree((char *)dlist->procs);
  XtFree((char *)dlist);
}

/**********************************************************************
 * If you want to implement your own class of procedures, look at
 * the code bellow.
 **********************************************************************/
/* Start of Implementation of class "xlib" */
typedef struct _XawXlibData {
  GC gc;
  unsigned long mask;
  XGCValues values;
  int shape;
  int mode;
} XawXlibData;

typedef struct _XawDLPosition {
  Position pos;
  short denom;
  Boolean high;
} XawDLPosition;

typedef struct _XawDLPositionPtr {
  XawDLPosition *pos;
  Cardinal num_pos;
} XawDLPositionPtr;

#define X_ARG(x) (((x).denom != 0) ?					      \
		  ((float)XtWidth(w) * ((float)(x).pos / (float)(x).denom)) : \
		  ((x).high ? XtWidth(w) - (x).pos : (x).pos))
#define Y_ARG(x) (((x).denom != 0) ?					      \
		  ((float)XtHeight(w) * ((float)(x).pos / (float)(x).denom)): \
		  ((x).high ? XtHeight(w) - (x).pos : (x).pos))
  
static void
DlLine(Widget w, XtPointer args, XtPointer data, XEvent *event, Region region)
{
  XawDLPosition *pos = (XawDLPosition *)args;
  XawXlibData *xdata = (XawXlibData *)data;
  Display *display;
  Window window;
  Position x1, y1, x2, y2;

  x1 = X_ARG(pos[0]);
  y1 = Y_ARG(pos[1]);
  x2 = X_ARG(pos[2]);
  y2 = Y_ARG(pos[3]);

  if (!XtIsWidget(w))
    {
      Position xpad, ypad;

      xpad = XtX(w) + XtBorderWidth(w);
      ypad = XtY(w) + XtBorderWidth(w);
      x1 += xpad; y1 += ypad;
      x2 += xpad; y2 += ypad;
      display = XtDisplayOfObject(w);
      window = XtWindowOfObject(w);
    }
  else
    {
      display = XtDisplay(w);
      window = XtWindow(w);
    }

  XDrawLine(display, window, xdata->gc, x1, y1, x2, y2);
}

static void
DlDrawRectangle(Widget w, XtPointer args, XtPointer data,
		XEvent *event, Region region)
{
  XawDLPosition *pos = (XawDLPosition *)args;
  XawXlibData *xdata = (XawXlibData *)data;
  Display *display;
  Window window;
  Position x1, y1, x2, y2;

  x1 = X_ARG(pos[0]);
  y1 = Y_ARG(pos[1]);
  x2 = X_ARG(pos[2]);
  y2 = Y_ARG(pos[3]);

  if (!XtIsWidget(w))
    {
      Position xpad, ypad;

      xpad = XtX(w) + XtBorderWidth(w);
      ypad = XtY(w) + XtBorderWidth(w);
      x1 += xpad; y1 += ypad;
      x2 += xpad; y2 += ypad;
      display = XtDisplayOfObject(w);
      window = XtWindowOfObject(w);
    }
  else
    {
      display = XtDisplay(w);
      window = XtWindow(w);
    }

  XDrawRectangle(display, window, xdata->gc, x1, y1, x2 - x1, y2 - y1);
}

static void
DlFillRectangle(Widget w, XtPointer args, XtPointer data,
		XEvent *event, Region region)
{
  XawDLPosition *pos = (XawDLPosition *)args;
  XawXlibData *xdata = (XawXlibData *)data;
  Display *display;
  Window window;
  Position x1, y1, x2, y2;

  x1 = X_ARG(pos[0]);
  y1 = Y_ARG(pos[1]);
  x2 = X_ARG(pos[2]);
  y2 = Y_ARG(pos[3]);

  if (!XtIsWidget(w))
    {
      Position xpad, ypad;

      xpad = XtX(w) + XtBorderWidth(w);
      ypad = XtY(w) + XtBorderWidth(w);
      x1 += xpad; y1 += ypad;
      x2 += xpad; y2 += ypad;
      display = XtDisplayOfObject(w);
      window = XtWindowOfObject(w);
    }
  else
    {
      display = XtDisplay(w);
      window = XtWindow(w);
    }

  XFillRectangle(display, window, xdata->gc, x1, y1, x2 - x1, y2 - y1);
}
static void
DlFillPolygon(Widget w, XtPointer args, XtPointer data,
	      XEvent *event, Region region)
{
  XawDLPositionPtr *pos_ptr = (XawDLPositionPtr *)args;
  XawXlibData *xdata = (XawXlibData *)data;
  XawDLPosition *pos;
  XPoint *points;
  Display *display;
  Window window;
  Position p;
  Cardinal num_points, i, j;

  num_points = pos_ptr->num_pos>>1;
  points = (XPoint *)XtMalloc(sizeof(XPoint) * num_points);

  for (i = j = 0; i < pos_ptr->num_pos; i += 2, j++)
    {
      pos = &pos_ptr->pos[i];
      p = X_ARG(pos[0]);
      if (i + 1 < pos_ptr->num_pos)
	{
	  pos = &pos_ptr->pos[i+1];
	  p = Y_ARG(pos[0]);
	  points[j].y = p;
	}
      else
	points[j].y = 0;
    }

  if (!XtIsWidget(w))
    {
      Position xpad, ypad;

      xpad = XtX(w) + XtBorderWidth(w);
      ypad = XtY(w) + XtBorderWidth(w);
      if (xdata->mode != CoordModePrevious)
	{
	  for (i = 0; i < num_points; i++)
	    {
	      points[i].x += xpad;
	      points[i].y += ypad;
	    }
	}
      else
	{
	  points[0].x += xpad;
	  points[1].y += ypad;
	}
      display = XtDisplayOfObject(w);
      window = XtWindowOfObject(w);
    }
  else
    {
      display = XtDisplay(w);
      window = XtWindow(w);
    }

  XFillPolygon(display, window, xdata->gc, points, num_points,
	       xdata->shape, xdata->mode);
}

/* ARGSUSED */
static void
DlForeground(Widget w, XtPointer args, XtPointer data,
	     XEvent *event, Region region)
{
  XawXlibData *xdata = (XawXlibData *)data;
  Pixel foreground = (Pixel)args;

  xdata->mask |= GCForeground;
  xdata->values.foreground = foreground;
  XSetForeground(XtDisplayOfObject(w), xdata->gc, foreground);
}

/* ARGSUSED */
static void
DlBackground(Widget w, XtPointer args, XtPointer data,
	     XEvent *event, Region region)
{
  XawXlibData *xdata = (XawXlibData *)data;
  Pixel background = (Pixel)args;

  xdata->mask |= GCBackground;
  xdata->values.foreground = background;
  XSetBackground(XtDisplayOfObject(w), xdata->gc, background);
}

typedef struct _Dl_init Dl_init;
struct _Dl_init {
  String name;
  XawDisplayListProc proc;
  Cardinal id;
};

#define DRECT 0
#define FRECT 1
#define LINE  2
#define GCFG  3
#define GCBG  4
#define FPOLY 5
static Dl_init dl_init[] =
{
  {"background", DlBackground,     GCBG},
  {"draw-rect",  DlDrawRectangle,  DRECT},
  {"fill-poly",  DlFillPolygon,    FPOLY},
  {"fill-rect",  DlFillRectangle,  FRECT},
  {"foreground", DlForeground,     GCFG},
  {"line",       DlLine,           LINE},
};

void
XawDisplayListInitialize()
{
  static Boolean first_time = True;
  XawDLClass *lc;
  Cardinal i;

  if (first_time == False)
    return;

  first_time = False;

  lc = XawCreateDisplayListClass(xlib,
				 _Xaw_Xlib_ArgsInitProc,
				 _Xaw_Xlib_ArgsDestructor,
				 _Xaw_Xlib_DataInitProc,
				 _Xaw_Xlib_DataDestructor);
  for (i = 0; i < sizeof(dl_init) / sizeof(dl_init[0]); i++)
    (void)XawDeclareDisplayListProc(lc, dl_init[i].name, dl_init[i].proc);
}

int
bcmp_cvt_proc(register _Xconst void *string,
	      register _Xconst void *dlinfo)
{
  return (strcmp((String)string, ((Dl_init*)dlinfo)->name));
}

static int
read_int(char *cp, char **cpp)
{
  int value = 0, sign = 1;

  if (*cp == '-')
    {
      sign = -1;
      ++cp;
    }
  else if (*cp == '+')
    ++cp;
  value = 0;
  while (*cp >= '0' && *cp <= '9')
    {
      value = value * 10 + *cp - '0';
      ++cp;
    }
  if (cpp)
    *cpp = cp;
  return (value * sign);
}

static void
read_position(char *arg, XawDLPosition *pos)
{
  int ch;

  ch = *arg;
  if (ch == '-' || ch == '+')
    {
      ++arg;
      if (ch == '-')
	pos->high = True;
      pos->pos = read_int(arg, NULL);
    }
  else if (isdigit(ch))
    {
      pos->pos = read_int(arg, &arg);
      ch = *arg++;
      if (ch == '/')
	pos->denom = read_int(arg, NULL);
    }
}

/* ARGSUSED */
void *
_Xaw_Xlib_ArgsInitProc(String proc_name, String *params, Cardinal *num_params,
		       Screen *screen, Colormap colormap, int depth)
{
  Cardinal id, i;
  Dl_init *init;
  void *retval = NULL;

  init = (Dl_init *)bsearch(proc_name, dl_init,
			    sizeof(dl_init) / sizeof(dl_init[0]),
			    sizeof(dl_init[0]),
			    bcmp_cvt_proc);

  id = init->id;

  switch (id)
    {
    case LINE:
    case DRECT:
    case FRECT:
      {
	XawDLPosition *pos;

	pos = (XawDLPosition *)XtMalloc(sizeof(XawDLPosition) * 4);
	bzero(pos, sizeof(XawDLPosition) * 4);
	for (i = 0; i < XawMin(4, *num_params); i++)
	  read_position(params[i], &pos[i]);
	retval = (void *)pos;
      } break;
      break;
    case FPOLY:
      {
	XawDLPositionPtr *pos;

	pos = (XawDLPositionPtr *)XtMalloc(sizeof(XawDLPositionPtr));
	pos->pos = (XawDLPosition *)XtMalloc(sizeof(XawDLPosition) *
					     *num_params);
	pos->num_pos = *num_params;
	for (i = 0; i < *num_params; i++)
	  read_position(params[i], &pos->pos[i]);
	retval = (void *)pos;
      } break;
    case GCFG:
    case GCBG:
      {
	XColor xcolor;

	if (*num_params)
	  (void)XAllocNamedColor(DisplayOfScreen(screen), colormap,
				 params[0], &xcolor, &xcolor);
	else
	  xcolor.pixel = 0;
	retval = (void *)xcolor.pixel;
      } break;
    }

  return (retval);
}

/* ARGSUSED */
void *
_Xaw_Xlib_DataInitProc(String class_name,
		       Screen *screen, Colormap colormap, int depth)
{
  XawXlibData *data;
  Window tmp_win;

  data = (XawXlibData *)XtMalloc(sizeof(XawXlibData));

  tmp_win = XCreateWindow(DisplayOfScreen(screen),
			  RootWindowOfScreen(screen),
			  0, 0, 1, 1, 1, depth,
			  InputOutput, CopyFromParent, 0, NULL);
  data->mask = 0;
  data->gc = XCreateGC(DisplayOfScreen(screen), tmp_win, 0, &data->values);
  XDestroyWindow(DisplayOfScreen(screen), tmp_win);
  data->shape = Complex;
  data->mode = CoordModeOrigin;

  return ((void *)data);
}

/* ARGSUSED */
void
_Xaw_Xlib_ArgsDestructor(Display *display, String proc_name, XtPointer args,
			 String *params, Cardinal *num_params)
{
  Cardinal id;
  Dl_init *init;

  init = (Dl_init *)bsearch(proc_name, dl_init,
			    sizeof(dl_init) / sizeof(dl_init[0]),
			    sizeof(dl_init[0]),
			    bcmp_cvt_proc);

  id = init->id;

  switch (id)
    {
    case LINE:
    case DRECT:
    case FRECT:
      XtFree(args);
    case FPOLY:
      {
	XawDLPositionPtr *ptr = (XawDLPositionPtr *)args;

	XtFree((char *)ptr->pos);
	XtFree(args);
      } break;
    case GCFG:
    case GCBG:
      break;
    }
}

/* ARGSUSED */
void
_Xaw_Xlib_DataDestructor(Display *display, String class_name, XtPointer data)
{
  if (data)
    {
      XFreeGC(display, (*(XawXlibData **)data)->gc);
      XtFree((char *)data);
    }
}

/* Start of DLInfo Management Functions */
int
qcmp_dlist_info(register _Xconst void *left, register _Xconst void *right)
{
  return (strcmp((*(XawDLInfo **)left)->name, (*(XawDLInfo **)right)->name));
}

Boolean XawDeclareDisplayListProc(XawDLClass *lc, String name,
				  XawDisplayListProc proc)
{
  XawDLInfo *info;

  if (!lc || !proc || !name || name[0] == '\0')
    return (False);

  if ((info = _XawFindDLInfo(lc, name)) != NULL)
    return (True);

  info = (XawDLInfo *)XtMalloc(sizeof(XawDLInfo));
  info->name = XtNewString(name);
  info->qname = XrmStringToQuark(info->name);
  info->proc = proc;

  if (!lc->num_infos)
    {
      lc->num_infos = 1;
      lc->infos = (XawDLInfo **)XtMalloc(sizeof(XawDLInfo*));
    }
  else
    {
      ++lc->num_infos;
      lc->infos = (XawDLInfo **)
	XtRealloc((char *)lc->infos, sizeof(XawDLInfo*) * lc->num_infos);
    }
  lc->infos[lc->num_infos - 1] = info;

  if (lc->num_infos > 1)
    qsort(lc->infos, lc->num_infos, sizeof(XawDLInfo*), qcmp_dlist_info);

  return (True);
}

int
bcmp_dlist_info(register _Xconst void *string,
		register _Xconst void *dlinfo)
{
  return (strcmp((String)string, (*(XawDLClass **)dlinfo)->name));
}

XawDLInfo *
_XawFindDLInfo(XawDLClass *lc, String name)
{
  XawDLInfo **info;

  if (!lc->infos)
    return (NULL);

  info = (XawDLInfo **)bsearch(name, lc->infos, lc->num_infos,
			       sizeof(XawDLInfo*), bcmp_dlist_info);

  return (info ? *info : NULL);
}

/* Start of DLClass Management Functions */
XawDLClass *
XawGetDisplayListClass(String name)
{
  return (_XawFindDLClass(name));
}

int
qcmp_dlist_class(register _Xconst void *left, register _Xconst void *right)
{
  return (strcmp((*(XawDLClass **)left)->name, (*(XawDLClass **)right)->name));
}

XawDLClass *
XawCreateDisplayListClass(String name,
			  XawDLArgsInitProc args_init,
			  XawDLArgsDestructor args_destructor,
			  XawDLDataInitProc data_init,
			  XawDLDataDestructor data_destructor)
{
  XawDLClass *lc;

  if (!name || name[0] == '\0')
    return (NULL);

  lc = (XawDLClass *)XtMalloc(sizeof(XawDLClass));
  lc->name = XtNewString(name);
  lc->infos = NULL;
  lc->num_infos = 0;
  lc->args_init = args_init;
  lc->args_destructor = args_destructor;
  lc->data_init = data_init;
  lc->data_destructor = data_destructor;

  if (!classes)
    {
      num_classes = 1;
      classes = (XawDLClass **)XtMalloc(sizeof(XawDLClass));
    }
  else
    {
      ++num_classes;
      classes = (XawDLClass **)XtRealloc((char *)classes,
					 sizeof(XawDLClass) * num_classes);
    }
  classes[num_classes - 1] = lc;

  if (num_classes > 1)
    qsort(&classes[0], num_classes, sizeof(XawDLClass*), qcmp_dlist_class);

  return (lc);
}

int
bcmp_dlist_class(register _Xconst void *string,
		 register _Xconst void *dlist)
{
  return (strcmp((String)string, (*(XawDLClass **)dlist)->name));
}

XawDLClass *
_XawFindDLClass(String name)
{
  XawDLClass **lc;

  if (!classes)
    return (NULL);

  lc = (XawDLClass **)bsearch(name, &classes[0], num_classes,
			      sizeof(XawDLClass*), bcmp_dlist_class);

  return (lc ? *lc : NULL);
}
