/*
 * Copyright (c) 1998 by The XFree86 Project, Inc.
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

/* $XFree86$ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include "Private.h"

/*
 * Types
 */
typedef struct _XawCache {
  long value;
  XtPointer *elems;
  unsigned int num_elems;
} XawCache;

typedef struct _XawPixmapLoaderInfo {
  XawPixmapLoader loader;
  String type;
  String ext;
} XawPixmapLoaderInfo;

/*
 * Private Methods
 */
static Boolean BitmapLoader(XawParams*, Screen*, Colormap, int,
			    Pixmap*, Pixmap*, Dimension*, Dimension*);
static Boolean GradientLoader(XawParams*, Screen*, Colormap, int,
			      Pixmap*, Pixmap*, Dimension*, Dimension*);
static XawPixmap *_XawFindPixmap(String, Screen*, Colormap, int);
static void _XawCachePixmap(XawPixmap*, Screen*, Colormap, int);
static int _XawFindPixmapLoaderIndex(String, String);
static int qcmp_long(register _Xconst void*, register _Xconst void *);
static int bcmp_long(register _Xconst void*, register _Xconst void *);
static int qcmp_string(register _Xconst void*, register _Xconst void *);
static int bcmp_string(register _Xconst void*, register _Xconst void *);

/*
 * Initialization
 */
static XawCache xaw_pixmaps;
static XawPixmapLoaderInfo **loader_info;
static Cardinal num_loader_info;

/*
 * Implementation
 */
Boolean
XawPixmapsInitialize()
{
  Boolean first_time = True;
  static char *bitmap = "bitmap";

  if (!first_time)
    return (False);

  (void)XawAddPixmapLoader(NULL, NULL, BitmapLoader);
  (void)XawAddPixmapLoader(NULL, "bm", BitmapLoader);
  (void)XawAddPixmapLoader(NULL, "xbm", BitmapLoader);
  (void)XawAddPixmapLoader(bitmap, NULL, BitmapLoader);
  (void)XawAddPixmapLoader(bitmap, "bm", BitmapLoader);
  (void)XawAddPixmapLoader(bitmap, "xbm", BitmapLoader);
  (void)XawAddPixmapLoader("gradient", NULL, GradientLoader);

  return (True);
}

XawParams *
XawParseParamsString(String name)
{
  XawParams *xaw_params;
  char *tok, *str, *type = NULL, *ext = NULL, *params = NULL;

  if (!name)
    return (NULL);

  xaw_params = (XawParams *)XtMalloc(sizeof(XawParams));

  str = XtNewString(name);

  /* Find type */
  tok = str;
  while (tok = strchr(tok, ':'), tok)
    {
      if (tok == str || tok[-1] != '\\')
	break;
      bcopy(tok, &tok[-1], strlen(tok) + 1);
    }
  if (tok)
    {
      *tok = '\0';
      ++tok;
      type = XtNewString(str);
      bcopy(tok, str, strlen(tok) + 1);
    }

  /* Find params */
  tok = str;
  while (tok = strchr(tok, '?'), tok)
    {
      if (tok == str || tok[-1] != '\\')
	params = tok;
      if (tok != str && tok[-1] == '\\')
	bcopy(tok, &tok[-1], strlen(tok) + 1);
      else
	break;
    }
  if (params)
    {
      *params = '\0';
      ++params;
    }

  /* Find ext */
  tok = str;
  while (tok = strchr(tok, '.'), tok)
    {
      if (tok == str || tok[-1] != '\\')
	ext = tok;
      if (tok != str && tok[-1] == '\\')
	bcopy(tok, &tok[-1], strlen(tok) + 1);
      else
	break;
    }
  if (ext)
    ++ext;

  xaw_params->name = XtNewString(str);
  xaw_params->type = type;
  xaw_params->ext = ext ? XtNewString(ext) : ext;
  xaw_params->args = NULL;
  xaw_params->num_args = 0;

  /* Parse params */
  if (params)
    {
      char *arg, *val;
      XawArgVal *xaw_arg;

      for (; (tok = strsep(&params, "&")) != NULL;)
	{
	  if (!tok[0])
	    ;
	  else
	    {
	      val = strchr(tok, '=');
	      if (val)
		{
		  *val = '\0';
		  ++val;
		  if (*val != '\0')
		    val = XtNewString(val);
		  else
		    val = NULL;
		}
	      arg = XtNewString(tok);
	      xaw_arg = (XawArgVal *)XtMalloc(sizeof(XawArgVal));
	      xaw_arg->name = arg;
	      xaw_arg->value = val;
	      if (!xaw_params->num_args)
		{
		  xaw_params->num_args = 1;
		  xaw_params->args = (XawArgVal **)
		    XtMalloc(sizeof(XawArgVal*));
		}
	      else
		{
		  ++xaw_params->num_args;
		  xaw_params->args = (XawArgVal **)
		    XtRealloc((char *)xaw_params->args,
			      sizeof(XawArgVal*) * xaw_params->num_args);
		}
	      xaw_params->args[xaw_params->num_args - 1] = xaw_arg;
	    }
	}
    }

  if (xaw_params->num_args > 1)
    qsort(xaw_params->args, xaw_params->num_args, sizeof(XtPointer),
	  qcmp_string);

  XtFree(str);

  return (xaw_params);
}

void
XawFreeParamsStruct(XawParams *params)
{
  int i;

  if (!params)
    return;

  for (i = 0; i < params->num_args; i++)
    {
      XtFree(params->args[i]->name);
      if (params->args[i]->value)
	XtFree(params->args[i]->value);
      XtFree((char *)params->args[i]);
    }

  XtFree((char *)params->args);
  XtFree((char *)params);
}

XawArgVal *
XawFindArgVal(XawParams *params, String name)
{
  XawArgVal **arg_val;

  if (!params->args)
    return (NULL);

  arg_val = (XawArgVal **)bsearch((void *)name, params->args,
				  params->num_args, sizeof(XtPointer*),
				  bcmp_string);
  if (!arg_val)
    return (NULL);

  return (*arg_val);
}

XawPixmap *
XawLoadPixmap(String name, Screen *screen, Colormap colormap, int depth)
{
  int index;
  Boolean success;
  XawPixmap *xaw_pixmap;
  Pixmap pixmap, mask;
  Dimension width, height;
  XawParams *xaw_params;

  if (!name)
    return (False);

  xaw_pixmap = _XawFindPixmap(name, screen, colormap, depth);

  if (xaw_pixmap)
    return (xaw_pixmap);

  if ((xaw_params = XawParseParamsString(name)) == NULL)
    return (NULL);

  index = _XawFindPixmapLoaderIndex(xaw_params->type, xaw_params->ext);
  if (index < 0)
    return (NULL);

  success = loader_info[index]->loader(xaw_params, screen, colormap, depth,
				       &pixmap, &mask, &width, &height);
  if (success)
    {
      xaw_pixmap = (XawPixmap *)XtMalloc(sizeof(XawPixmap));
      xaw_pixmap->name = XtNewString(name);
      xaw_pixmap->pixmap = pixmap;
      xaw_pixmap->mask = mask;
      xaw_pixmap->width = width;
      xaw_pixmap->height = height;
      _XawCachePixmap(xaw_pixmap, screen, colormap, depth);
    }

  XawFreeParamsStruct(xaw_params);

  return (success ? xaw_pixmap : NULL);
}

Boolean
XawAddPixmapLoader(String type, String ext, XawPixmapLoader loader)
{
  XawPixmapLoaderInfo *info;
  int i;

  if (!loader)
    return (False);

  i = _XawFindPixmapLoaderIndex(type, ext);

  if (i >= 0)
    {
      loader_info[i]->loader = loader;
      if (loader_info[i]->type)
	XtFree(loader_info[i]->type);
      if (loader_info[i]->ext)
	XtFree(loader_info[i]->ext);
      loader_info[i]->type = type ? XtNewString(type) : NULL;
      loader_info[i]->ext = ext ? XtNewString(ext) : NULL;
      return (True);
    }

  if ((info = (XawPixmapLoaderInfo *)XtMalloc(sizeof(XawPixmapLoaderInfo)))
      == NULL)
    return (False);

  info->loader = loader;
  info->type = type ? XtNewString(type) : NULL;
  info->ext = ext ? XtNewString(ext) : NULL;

  if (!loader_info)
    {
      num_loader_info = 1;
      loader_info = (XawPixmapLoaderInfo**)
	XtMalloc(sizeof(XawPixmapLoaderInfo*));
    }
  else
    {
      ++num_loader_info;
      loader_info = (XawPixmapLoaderInfo**)
	XtRealloc((char *)loader_info,
		  sizeof(XawPixmapLoaderInfo) * num_loader_info);
    }
  loader_info[num_loader_info - 1] = info;
  return (True);
}

int
_XawFindPixmapLoaderIndex(String type, String ext)
{
  int i;

  if (!loader_info)
    return (-1);

  for (i = 0; i < num_loader_info; i++)
    {
      if ((!type ^ !loader_info[i]->type)
	  || (!ext ^ !loader_info[i]->ext)
	  || (type && strcmp(type, loader_info[i]->type))
	  || (ext && strcmp(ext, loader_info[i]->ext)))
	continue;
      return (i);
    }

  return (-1);
}

static int
qcmp_long(register _Xconst void *left, register _Xconst void *right)
{
  return ((long)((*(XawCache **)left)->value) -
	  (long)((*(XawCache **)right)->value));
}

static int
qcmp_string(register _Xconst void *left, register _Xconst void *right)
{
  return (strcmp((String)((*(XawCache **)left)->value),
		 (String)((*(XawCache **)right)->value)));
}

static int
bcmp_long(register _Xconst void *value, register _Xconst void *cache)
{
  return ((long)value - (long)((*(XawCache **)cache)->value));
}

static int
bcmp_string(register _Xconst void *string,
	    register _Xconst void *cache)
{
  return (strcmp((String)string, (String)((*(XawCache **)cache)->value)));
}

#define FIND_ALL      0
#define FIND_SCREEN   1
#define FIND_COLORMAP 2
#define FIND_DEPTH    3
static XawCache **
_XawFindCache(Screen *screen, Colormap colormap, int depth, int flags)
{
  XawCache **cache;

  if (!xaw_pixmaps.num_elems)
    return (NULL);

  /* Screen */
  cache = (XawCache **)bsearch(screen, xaw_pixmaps.elems,
			       xaw_pixmaps.num_elems, sizeof(XtPointer),
			       bcmp_long);
  if (flags == FIND_SCREEN)
    return (cache);
  if (!cache || !(*cache)->num_elems)
    return (NULL);

  /* Colormap */
  cache = (XawCache **)bsearch((void *)colormap, (*cache)->elems,
			       (*cache)->num_elems, sizeof(XtPointer),
			       bcmp_long);
  if (flags == FIND_COLORMAP)
    return (cache);
  if (!cache || !(*cache)->num_elems)
    return (NULL);

  /* Depth */
  cache = (XawCache **)bsearch((void *)depth, (*cache)->elems,
			       (*cache)->num_elems, sizeof(XtPointer),
			       bcmp_long);

  return (cache);
}

XawPixmap *
_XawFindPixmap(String name, Screen *screen, Colormap colormap, int depth)
{
  XawCache **cache;
  XawPixmap **pixmap;

  cache = _XawFindCache(screen, colormap, depth, FIND_ALL);

  if (!cache || !(*cache)->num_elems)
    return (NULL);

  /* Name */
  pixmap = (XawPixmap **)bsearch((void *)name, (*cache)->elems,
				 (*cache)->num_elems, sizeof(XtPointer),
				 bcmp_string);
  if (!pixmap)
    return (NULL);

  return (*pixmap);
}

void
_XawCachePixmap(XawPixmap *pixmap,
		Screen *screen, Colormap colormap, int depth)
{
  XawCache **s_cache, **c_cache, **d_cache, **cache, *pcache;

  cache = _XawFindCache(screen, colormap, depth, FIND_ALL);

  if (!cache)
    {
      s_cache = _XawFindCache(screen, colormap, depth, FIND_SCREEN);
      if (!s_cache)
	{
	  pcache = (XawCache *)XtMalloc(sizeof(XawCache));
	  if (!xaw_pixmaps.num_elems)
	    {
	      xaw_pixmaps.num_elems = 1;
	      xaw_pixmaps.elems = (XtPointer*)XtMalloc(sizeof(XtPointer));
	    }
	  else
	    {
	      ++xaw_pixmaps.num_elems;
	      xaw_pixmaps.elems = (XtPointer*)
		XtRealloc((char *)xaw_pixmaps.elems,
			  sizeof(XtPointer) * xaw_pixmaps.num_elems);
	    }
	  pcache->value = (long)screen;
	  pcache->elems = (XtPointer *)NULL;
	  pcache->num_elems = 0;
	  xaw_pixmaps.elems[xaw_pixmaps.num_elems - 1] = (XtPointer)pcache;
	  s_cache = (XawCache **)
	    (&xaw_pixmaps.elems[xaw_pixmaps.num_elems - 1]);
	  if (xaw_pixmaps.num_elems > 1)
	    qsort(xaw_pixmaps.elems, xaw_pixmaps.num_elems,
		  sizeof(XtPointer), qcmp_long);
	}

      c_cache = _XawFindCache(screen, colormap, depth, FIND_COLORMAP);
      if (!c_cache)
	{
	  pcache = (XawCache *)XtMalloc(sizeof(XawCache));
	  if (!(*s_cache)->num_elems)
	    {
	      (*s_cache)->num_elems = 1;
	      (*s_cache)->elems = (XtPointer*)XtMalloc(sizeof(XtPointer));
	    }
	  else
	    {
	      ++(*s_cache)->num_elems;
	      (*s_cache)->elems = (XtPointer*)
		XtRealloc((char *)(*s_cache)->elems,
			  sizeof(XtPointer) * (*s_cache)->num_elems);
	    }
	  pcache->value = (long)colormap;
	  pcache->elems = (XtPointer *)NULL;
	  pcache->num_elems = 0;
	  (*s_cache)->elems[(*s_cache)->num_elems - 1] = (XtPointer)pcache;
	  c_cache = (XawCache **)
	    &(*s_cache)->elems[(*s_cache)->num_elems - 1];
	  if ((*s_cache)->num_elems > 1)
	    qsort((*s_cache)->elems, (*s_cache)->num_elems,
		  sizeof(XtPointer), qcmp_long);
	}

      d_cache = _XawFindCache(screen, colormap, depth, FIND_DEPTH);
      if (!d_cache)
	{
	  pcache = (XawCache *)XtMalloc(sizeof(XawCache));
	  if (!(*c_cache)->num_elems)
	    {
	      (*c_cache)->num_elems = 1;
	      (*c_cache)->elems = (XtPointer*)XtMalloc(sizeof(XtPointer));
	    }
	  else
	    {
	      ++(*c_cache)->num_elems;
	      (*c_cache)->elems = (XtPointer*)
		XtRealloc((char *)(*c_cache)->elems,
			  sizeof(XtPointer) * (*c_cache)->num_elems);
	    }
	  pcache->value = (long)depth;
	  pcache->elems = (XtPointer *)NULL;
	  pcache->num_elems = 0;
	  (*c_cache)->elems[(*c_cache)->num_elems - 1] = (XtPointer)pcache;
	  d_cache = (XawCache **)
	    &(*c_cache)->elems[(*c_cache)->num_elems - 1];
	  if ((*c_cache)->num_elems > 1)
	    qsort((*c_cache)->elems, (*c_cache)->num_elems,
		  sizeof(XtPointer), qcmp_long);
	}

      cache = d_cache;
    }

  if (!(*cache)->num_elems)
    {
      (*cache)->num_elems = 1;
      (*cache)->elems = (XtPointer*)XtMalloc(sizeof(XtPointer));
    }
  else
    {
      ++(*cache)->num_elems;
      (*cache)->elems = (XtPointer*)XtRealloc((char *)(*cache)->elems,
					      sizeof(XtPointer) *
					      (*cache)->num_elems);
    }

  (*cache)->elems[(*cache)->num_elems - 1] = (XtPointer)pixmap;
  if ((*cache)->num_elems > 1)
    qsort((*cache)->elems, (*cache)->num_elems,
	  sizeof(XtPointer), qcmp_string);
}

Boolean
BitmapLoader(XawParams *params, Screen *screen, Colormap colormap, int depth,
	     Pixmap *pixmap_return, Pixmap *mask_return,
	     Dimension *width_return, Dimension *height_return)
{
  Pixel fg, bg;
  XColor color, exact;
  Pixmap pixmap;
  unsigned int width, height;
  unsigned char *data = NULL;
  int hotX, hotY;
  XawArgVal *argval;
  Boolean retval = False;
  static char *path =
    "%H/%T/%N:/usr/X11R6/include/X11/%T/%N:/usr/include/X11/%T/%N:%N";
  static SubstitutionRec sub[] = {
    {'H',   NULL},
    {'N',   NULL},
    {'T',   "bitmaps"},
  };
  char *filename;

  fg = BlackPixelOfScreen(screen);
  bg = WhitePixelOfScreen(screen);

  if ((argval = XawFindArgVal(params, "foreground")) != NULL
      && argval->value)
    {
      if (XAllocNamedColor(DisplayOfScreen(screen), colormap, argval->value,
			   &color, &exact))
	fg = color.pixel;
    }
  if ((argval = XawFindArgVal(params, "background")) != NULL
      && argval->value)
    {
      if (XAllocNamedColor(DisplayOfScreen(screen), colormap, argval->value,
			   &color, &exact))
	bg = color.pixel;
    }

  if (params->name[0] != '/' && params->name[0] != '.')
    {
      if (!sub[0].substitution)
	sub[0].substitution = getenv("HOME");
      sub[1].substitution = params->name;
      filename = XtFindFile(path, sub, XtNumber(sub), NULL);
      if (!filename)
	return (FALSE);
    }
  else
    filename = params->name;

  if (XReadBitmapFileData(filename, &width, &height, &data,
			  &hotX, &hotY) == BitmapSuccess)
    {
      pixmap = XCreatePixmapFromBitmapData(DisplayOfScreen(screen),
					   RootWindowOfScreen(screen), data,
					   width, height, fg, bg, depth);
      if (data)
	XFree(data);
      *pixmap_return = pixmap;
      *mask_return = None;
      *width_return = width;
      *height_return = height;

      retval = True;
    }

  if (filename != params->name)
    XtFree(filename);

  return (retval);
}

#define VERTICAL   1
#define HORIZONTAL 2
Boolean
GradientLoader(XawParams *params, Screen *screen, Colormap colormap, int depth,
	       Pixmap *pixmap_return, Pixmap *mask_return,
	       Dimension *width_return, Dimension *height_return)
{
  double ired, igreen, iblue, red, green, blue;
  XColor start, end, color;
  XGCValues values;
  GC gc;
  double i, inc, x, y;
  Pixmap pixmap;
  XawArgVal *argval;
  int orientation, dimension, steps;

  if (strcasecmp(params->name, "vertical") == 0)
    orientation = VERTICAL;
  else if (strcasecmp(params->name, "horizontal") == 0)
    orientation = HORIZONTAL;
  else
    return (False);

  if ((argval = XawFindArgVal(params, "dimension")) != NULL
      && argval->value)
    {
      dimension = atoi(argval->value);
      if (dimension <= 0)
	return (False);
    }
  else
    dimension = 50;

  if ((argval = XawFindArgVal(params, "steps")) != NULL
      && argval->value)
    {
      steps = atoi(argval->value);
      if (steps <= 0)
	return (False);
    }
  else
    steps = dimension;

  steps = XawMin(steps, dimension);

  if ((pixmap = XCreatePixmap(DisplayOfScreen(screen),
			      RootWindowOfScreen(screen),
			      orientation == VERTICAL ? 1 : dimension,
			      orientation == VERTICAL ? dimension : 1, depth))
      == 0)
    return (False);

  if (!((argval = XawFindArgVal(params, "start")) != NULL
	&& argval->value
	&& XAllocNamedColor(DisplayOfScreen(screen), colormap, argval->value,
			    &start, &color)))
    {
      start.pixel = WhitePixelOfScreen(screen);
      XQueryColor(DisplayOfScreen(screen), colormap, &start);
    }
  if (!((argval = XawFindArgVal(params, "end")) != NULL
	&& argval->value
	&& XAllocNamedColor(DisplayOfScreen(screen), colormap, argval->value,
			    &end, &color)))
    {
      end.pixel = BlackPixelOfScreen(screen);
      XQueryColor(DisplayOfScreen(screen), colormap, &end);
    }

  ired   = (double)(end.red   - start.red)   / (double)steps;
  igreen = (double)(end.green - start.green) / (double)steps; 
  iblue  = (double)(end.blue  - start.blue)  / (double)steps;

  red   = color.red   = start.red;
  green = color.green = start.green;
  blue  = color.blue  = start.blue;

  inc = (double)dimension / (double)steps;

  gc = XCreateGC(DisplayOfScreen(screen), pixmap, 0, &values);

  x = y = 0.0;

  color.flags = DoRed | DoGreen | DoBlue;

  XSetForeground(DisplayOfScreen(screen), gc, start.pixel);
  for (i = 0.0; i < dimension; i += inc)
    {
      if ((int)color.red != red || (int)color.green != green
	  || (int)color.blue != blue)
	{
	  color.red   = (unsigned short)red;
	  color.green = (unsigned short)green;
	  color.blue  = (unsigned short)blue;
	  (void)XAllocColor(DisplayOfScreen(screen), colormap, &color);
	  XSetForeground(DisplayOfScreen(screen), gc, color.pixel);
	}
      XFillRectangle(DisplayOfScreen(screen), pixmap, gc, x, y,
		     x + inc, y + inc);
      red   += ired;
      green += igreen;
      blue  += iblue;
      if (orientation == VERTICAL)
	y += inc;
      else
	x += inc;
    }

  *pixmap_return = pixmap;
  *mask_return = None;
  *width_return = orientation == VERTICAL ? 1 : dimension;
  *height_return = orientation == VERTICAL ? dimension : 1;

  XFreeGC(DisplayOfScreen(screen), gc);

  return (True);
}
