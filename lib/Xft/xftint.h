/*
 * $XFree86: xc/lib/Xft/xftint.h,v 1.5 2000/11/30 23:30:00 dawes Exp $
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

#ifndef _XFTINT_H_
#define _XFTINT_H_

#include <X11/Xlib.h>
#include "XftFreetype.h"

typedef struct _XftMatcher {
    char    *object;
    double  (*compare) (char *object, XftValue value1, XftValue value2);
} XftMatcher;

typedef struct _XftSymbolic {
    const char	*name;
    int		value;
} XftSymbolic;

struct _XftDraw {
    Display	    *dpy;
    Drawable	    drawable;
    Visual	    *visual;
    Colormap	    colormap;
    Bool	    core_set;
    Bool	    render_set;
    Bool	    render_able;
    struct {
	Picture		pict;
	Pixmap		fg_pix;
	Picture		fg_pict;
	XRenderColor	fg_color;
    } render;
    struct {
	GC		draw_gc;
	unsigned long	fg;
    } core;
};

typedef struct _XftDisplayInfo {
    struct _XftDisplayInfo  *next;
    Display		    *display;
    XExtCodes		    *codes;
    XftPattern		    *defaults;
    XftFontSet		    *coreFonts;
    Bool		    hasRender;
} XftDisplayInfo;

#ifdef FREETYPE2
extern FT_Library	_XftFTlibrary;
#endif
extern XftFontSet	*_XftGlobalFontSet;
extern XftDisplayInfo	*_XftDisplayInfo;
extern char		**XftConfigDirs;
extern XftFontSet	*_XftFontSet;

#define XFT_NMISSING	256

#ifndef XFT_DEFAULT_PATH
#define XFT_DEFAULT_PATH "/usr/X11R6/lib/X11/XftConfig"
#endif

typedef enum _XftOp {
    XftOpInteger, XftOpDouble, XftOpString, XftOpBool, XftOpNil,
    XftOpField,
    XftOpAssign, XftOpPrepend, XftOpAppend,
    XftOpQuest,
    XftOpOr, XftOpAnd, XftOpEqual, XftOpNotEqual,
    XftOpLess, XftOpLessEqual, XftOpMore, XftOpMoreEqual,
    XftOpPlus, XftOpMinus, XftOpTimes, XftOpDivide,
    XftOpNot
} XftOp;

typedef struct _XftExpr {
    XftOp   op;
    union {
	int	ival;
	double	dval;
	char	*sval;
	Bool	bval;
	char	*field;
	struct {
	    struct _XftExpr *left, *right;
	} tree;
    } u;
} XftExpr;

typedef enum _XftQual {
    XftQualAny, XftQualAll
} XftQual;

typedef struct _XftTest {
    struct _XftTest	*next;
    XftQual		qual;
    char		*field;
    XftOp		op;
    XftValue		value;
} XftTest;

typedef struct _XftEdit {
    struct _XftEdit *next;
    const char	    *field;
    XftOp	    op;
    XftExpr	    *expr;
} XftEdit;

typedef struct _XftSubst {
    struct _XftSubst	*next;
    XftTest		*test;
    XftEdit		*edit;
} XftSubst;

/* xftcfg.c */
Bool
XftConfigAddDir (char *d);

Bool
XftConfigAddEdit (XftTest *test, XftEdit *edit);

Bool
_XftConfigCompareValue (XftValue    m,
			XftOp	    op,
			XftValue    v);

/* xftcore.c */

#define XFT_CORE_N16LOCAL	256

XChar2b *
XftCoreConvert16 (unsigned short    *string,
		  int		    len,
		  XChar2b	    xcloc[XFT_CORE_N16LOCAL]);

XChar2b *
XftCoreConvert32 (unsigned long	    *string,
		  int		    len,
		  XChar2b	    xcloc[XFT_CORE_N16LOCAL]);

void
XftCoreExtents8 (Display	*dpy,
		 XFontStruct	*fs,
		 unsigned char  *string, 
		 int		len,
		 XGlyphInfo	*extents);

void
XftCoreExtents16 (Display	    *dpy,
		  XFontStruct	    *fs,
		  unsigned short    *string, 
		  int		    len,
		  XGlyphInfo	    *extents);

void
XftCoreExtents32 (Display	    *dpy,
		  XFontStruct	    *fs,
		  unsigned long	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents);

/* xftdbg.c */
void
XftOpPrint (XftOp op);

void
XftTestPrint (XftTest *test);

void
XftExprPrint (XftExpr *expr);

void
XftEditPrint (XftEdit *edit);

void
XftSubstPrint (XftSubst *subst);

/* xftdir.c */
Bool
XftDirScan (XftFontSet *set, const char *dir);

/* xftdpy.c */
int
XftDefaultParseBool (char *v);

Bool
XftDefaultGetBool (Display *dpy, const char *object, int screen, Bool def);

int
XftDefaultGetInteger (Display *dpy, const char *object, int screen, int def);

double
XftDefaultGetDouble (Display *dpy, const char *object, int screen, double def);

XftFontSet *
XftDisplayGetFontSet (Display *dpy);

/* xftdraw.c */
Bool
XftDrawRenderPrepare (XftDraw	*draw,
		      XftColor	*color,
		      XftFont	*font);

Bool
XftDrawCorePrepare (XftDraw	*draw,
		    XftColor	*color,
		    XftFont	*font);

/* xftextent.c */
/* xftfont.c */
/* xftfreetype.c */
XftPattern *
XftFreeTypeQuery (const char *file, int id, int *count);

XftFontStruct *
XftFreeTypeOpen (Display *dpy, XftPattern *pattern);

void
XftFreeTypeClose (Display *dpy, XftFontStruct *font);
    
/* xftfs.c */
/* xftglyphs.c */
/* xftgram.y */
int
XftConfigparse (void);

int
XftConfigwrap (void);
    
void
XftConfigerror (char *fmt, ...);
    
char *
XftConfigSaveField (const char *field);

XftTest *
XftTestCreate (XftQual qual, const char *field, XftOp compare, XftValue value);

XftExpr *
XftExprCreateInteger (int i);

XftExpr *
XftExprCreateDouble (double d);

XftExpr *
XftExprCreateString (const char *s);

XftExpr *
XftExprCreateBool (Bool b);

XftExpr *
XftExprCreateNil (void);

XftExpr *
XftExprCreateField (const char *field);

XftExpr *
XftExprCreateOp (XftExpr *left, XftOp op, XftExpr *right);

void
XftExprDestroy (XftExpr *e);

XftEdit *
XftEditCreate (const char *field, XftOp op, XftExpr *expr);

void
XftEditDestroy (XftEdit *e);

/* xftinit.c */
Bool
XftInitFtLibrary (void);

/* xftlex.l */
extern int	XftConfigLineno;
extern char	*XftConfigFile;

int
XftConfiglex (void);

Bool
XftConfigLexFile(char *s);

Bool
XftConfigPushInput (char *s, Bool complain);

/* xftlist.c */
XftObjectSet *
_XftObjectSetVapBuild (const char *first, va_list *vap);

Bool
XftListValueCompare (XftValue	v1,
		     XftValue	v2);

Bool
XftListValueListCompare (XftValueList	*v1orig,
			 XftValueList	*v2orig,
			 XftQual	qual);

Bool
XftListMatch (XftPattern    *p,
	      XftPattern    *font,
	      XftQual	    qual);

Bool
XftListAppend (XftFontSet   *s,
	       XftPattern   *font,
	       XftObjectSet *os);


/* xftmatch.c */

/* xftname.c */
Bool
XftNameConstant (char *string, int *result);

/* xftpat.c */
XftPattern *
_XftPatternVapBuild (XftPattern *orig, va_list *vap);

/* xftrender.c */

/* xftstr.c */
char *
_XftSaveString (const char *s);

const char *
_XftGetInt(const char *ptr, int *val);

char *
_XftSplitStr (const char *field, char *save);

char *
_XftDownStr (const char *field, char *save);

const char *
_XftSplitField (const char *field, char *save);

const char *
_XftSplitValue (const char *field, char *save);

int
_XftMatchSymbolic (XftSymbolic *s, int n, const char *name, int def);

/* xftxlfd.c */
Bool
XftCoreAddFonts (XftFontSet *set, Display *dpy, Bool ignore_scalable);

#endif /* _XFT_INT_H_ */
