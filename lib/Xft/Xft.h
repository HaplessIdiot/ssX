/*
 * $XFree86$
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XFT_H_
#define _XFT_H_

#include <X11/extensions/Xrender.h>
#include <stdarg.h>

#define XFT_FAMILY	    "family"	/* String */
#define XFT_STYLE	    "style"	/* String */
#define XFT_SLANT	    "slant"	/* Int */
#define XFT_WEIGHT	    "weight"	/* Int */
#define XFT_SIZE	    "size"	/* Double */
#define XFT_PIXEL_SIZE	    "pixelsize"	/* Double */
#define XFT_ENCODING	    "encoding"	/* String */
#define XFT_SPACING	    "spacing"	/* Int */
#define XFT_FOUNDRY	    "foundry"	/* String */
#define XFT_CORE	    "core"	/* Bool */
#define XFT_ANTIALIAS	    "antialias"	/* Bool */
#define XFT_XLFD	    "xlfd"	/* String */
#define XFT_FILE	    "file"	/* String */
#define XFT_INDEX	    "index"	/* Int */
#define XFT_RASTERIZER	    "rasterizer"/* String */
#define XFT_OUTLINE	    "outline"	/* Bool */
#define XFT_SCALABLE	    "scalable"	/* Bool */
#define XFT_RGBA	    "rgba"	/* Int */

/* defaults from resources */
#define XFT_SCALE	    "scale"	/* double */
#define XFT_RENDER	    "render"	/* Bool */

#define XFT_WEIGHT_LIGHT	0
#define XFT_WEIGHT_MEDIUM	100
#define XFT_WEIGHT_DEMIBOLD	180
#define XFT_WEIGHT_BOLD		200
#define XFT_WEIGHT_BLACK	210

#define XFT_SLANT_ROMAN		0
#define XFT_SLANT_ITALIC	100
#define XFT_SLANT_OBLIQUE	110

#define XFT_PROPORTIONAL    0
#define XFT_MONO	    100
#define XFT_CHARCELL	    110

#define XFT_RGBA_NONE	    0
#define XFT_RGBA_RGB	    1
#define XFT_RGBA_BGR	    2

typedef enum _XftType {
    XftTypeVoid, 
    XftTypeInteger, 
    XftTypeDouble, 
    XftTypeString, 
    XftTypeBool
} XftType;

typedef enum _XftResult {
    XftResultMatch, XftResultNoMatch, XftResultTypeMismatch, XftResultNoId
} XftResult;

typedef struct _XftValue {
    XftType	type;
    union {
	char    *s;
	int	i;
	Bool	b;
	double	d;
    } u;
} XftValue;

typedef struct _XftValueList {
    struct _XftValueList    *next;
    XftValue		    value;
} XftValueList;

typedef struct _XftPatternElt {
    const char	    *object;
    XftValueList    *values;
} XftPatternElt;

typedef struct _XftPattern {
    int		    num;
    int		    size;
    XftPatternElt   *elts;
} XftPattern;

typedef struct _XftFontSet {
    int		nfont;
    int		sfont;
    XftPattern	**fonts;
} XftFontSet;

XftFontSet *
XftFontSetCreate (void);

void
XftFontSetDestroy (XftFontSet *s);

Bool
XftFontSetAdd (XftFontSet *s, XftPattern *font);

void
XftPatternPrint (XftPattern *p);

XftPattern *
XftMatchFont (Display *dpy, int screen, XftPattern *pattern, XftResult *result);
    
typedef struct _XftFontStruct	XftFontStruct;

typedef struct _XftFont {
    int		ascent;
    int		descent;
    int		height;
    int		max_advance_width;
    Bool	core;
    XftPattern	*pattern;
    union {
	struct {
	    XFontStruct	    *font;
	} core;
	struct {
	    XftFontStruct   *font;
	} ft;
    } u;
} XftFont;

typedef struct _XftDraw XftDraw;

XftFont *
XftOpenFont (Display *dpy, XftPattern *pattern);

XftFont *
XftPatternOpenFont (Display *dpy, int screen, ...);
    
Bool
XftDefaultSet (Display *dpy, XftPattern *defaults);

XftPattern *
XftNameParse (const char *name);

XftFont *
XftNameOpenFont (Display *dpy, int screen, const char *name);

/* xftdbg.c */
void
XftValuePrint (XftValue v);

void
XftValueListPrint (XftValueList *l);

void
XftPatternPrint (XftPattern *p);

void
XftFontSetPrint (XftFontSet *s);

/* xftdir.c */
/* xftdpy.c */
/* xftdraw.c */

XftDraw *
XftDrawCreate (Display   *dpy,
	       Drawable  drawable,
	       Visual    *visual,
	       Colormap  colormap);

void
XftDrawDestroy (XftDraw		*d);

void
XftDrawString8 (XftDraw		*d,
		XRenderColor	*color,
		XftFont		*font,
		int		x, 
		int		y,
		unsigned char	*string,
		int		len);

void
XftDrawString16 (XftDraw	*draw,
		 XRenderColor	*color,
		 XftFont	*font,
		 int		x,
		 int		y,
		 unsigned short	*string,
		 int		len);

void
XftDrawString32 (XftDraw	*draw,
		 XRenderColor	*color,
		 XftFont	*font,
		 int		x,
		 int		y,
		 unsigned long	*string,
		 int		len);
void
XftDrawRect (XftDraw	    *d,
	     XRenderColor   *color,
	     int	    x, 
	     int	    y,
	     unsigned int   width,
	     unsigned int   height);


/* xftextent.c */

void
XftTextExtents8 (Display	*dpy,
		 XftFont	*font,
		 unsigned char  *string, 
		 int		len,
		 XGlyphInfo	*extents);

void
XftTextExtents16 (Display	    *dpy,
		  XftFont	    *font,
		  unsigned short    *string, 
		  int		    len,
		  XGlyphInfo	    *extents);

void
XftTextExtents32 (Display	*dpy,
		  XftFont	*font,
		  unsigned long *string, 
		  int		len,
		  XGlyphInfo	*extents);
    
/* xftfont.c */
XftPattern *
XftFontMatch (Display *dpy, int screen, XftPattern *pattern, XftResult *result);

XftFont *
XftFontOpenPattern (Display *dpy, XftPattern *pattern);

XftFont *
XftFontOpen (Display *dpy, int screen, ...);

XftFont *
XftFontOpenName (Display *dpy, int screen, const char *name);

void
XftFontClose (Display *dpy, XftFont *font);

/* xftfreetype.c */
/* xftfs.c */

/* xftpat.c */
XftPattern *
XftPatternCreate (void);

XftPattern *
XftPatternDuplicate (XftPattern *p);

void
XftPatternDestroy (XftPattern *p);

Bool
XftPatternAdd (XftPattern *p, const char *object, XftValue value, Bool append);
    
XftResult
XftPatternGet (XftPattern *p, const char *object, int id, XftValue *v);
    
Bool
XftPatternDel (XftPattern *p, const char *object);

Bool
XftPatternAddInteger (XftPattern *p, const char *object, int i);

Bool
XftPatternAddDouble (XftPattern *p, const char *object, double d);

Bool
XftPatternAddString (XftPattern *p, const char *object, const char *s);

Bool
XftPatternAddBool (XftPattern *p, const char *object, Bool b);

XftResult
XftPatternGetInteger (XftPattern *p, const char *object, int n, int *i);

XftResult
XftPatternGetDouble (XftPattern *p, const char *object, int n, double *d);

XftResult
XftPatternGetString (XftPattern *p, const char *object, int n, char **s);

XftResult
XftPatternGetBool (XftPattern *p, const char *object, int n, Bool *b);

XftPattern *
XftPatternVaBuild (XftPattern *orig, va_list va);
    
XftPattern *
XftPatternBuild (XftPattern *orig, ...);

#endif /* _XFT_H_ */
