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

#ifndef _XFTINT_H_
#define _XFTINT_H_

#include <X11/Xlib.h>
#include <X11/extensions/render.h>
#include <X11/extensions/Xrender.h>
#include "Xft.h"
#include <freetype.h>
#include <stdlib.h>
#include <stdio.h>

FT_Library  library;

struct _XftFont {
    GlyphSet	    glyphset;
    int		    min_char;
    int		    max_char;
    int		    size;
    XGlyphInfo	    **realized;
    int		    nrealized;
    int		    ascent;
    int		    descent;
    int		    height;
    int		    max_advance_width;
    Bool	    monospace;
    FT_Face	    face;      /* handle to face object */
};

#define XftGlyphExists(f,g) ((f)->nrealized > ((g) >> 3) ? (f)->realized[(g) >> 3] & (1 << ((g) & 7)) : 0)

#ifndef XFT_DEFAULT_PATH
#define XFT_DEFAULT_PATH "/usr/X11R6/lib/X11/XftConfig"
#endif

#define XFT_MAX_STRING	2048

typedef enum _XftMatchState { XftMatching, XftSkipping, XftEditing } XftMatchState;

typedef enum _XftMatchToken {
    XFT_ERROR,
    XFT_EOF,

    XFT_EQUAL,
    XFT_STRING,
    XFT_NUMBER,

    XFT_MATCH,
    XFT_EDIT,
    XFT_PATH,
    XFT_DIR,
    XFT_FACE,
    XFT_ENCODING,
    XFT_FILE,
    XFT_SIZE,
    XFT_ROTATION,
    XFT_SPACING
} XftMatchToken;

typedef struct _XftParseState {
    FILE	    *file;
    char	    *filename;
    int		    line;
    XftMatchToken   token;
    int		    number;
    char	    string[XFT_MAX_STRING];
    char	    *dir;
    XftMatchState   state;
    XftFontName	    *name;
} XftParseState;

/* drawstr.c */
#define XFT_NMISSING    128

void
_XftCheckGlyph (Display *dpy, XftFont *font, unsigned long glyph,
		unsigned long *missing, int *nmissing);

/* extents.c */

/* glyphs.c */
void
_XftLoadGlyphs (Display *dpy, XftFont *font, unsigned long *glyphs, int nglyph);

/* lex.c */
int
_XftLex (XftParseState *s);
 
/* load.c */
extern FT_Library  _XftFTlibrary;

/* match.c */

Bool
XftMatch (XftFontName *pattern, XftFontName *result);

/* parse.c */

Bool
_XftParsePathList (char *path, char *dir, XftFontName *result);
    
#endif /* _XFTINT_H_ */
