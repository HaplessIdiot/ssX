/*
 * $XFree86: xc/lib/Xft/xftname.c,v 1.1 2000/11/29 08:39:23 keithp Exp $
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

#include "xftint.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct _XftObjectType {
    const char	*object;
    XftType	type;
} XftObjectType;

const XftObjectType _XftObjectTypes[] = {
    { XFT_FAMILY,	XftTypeString, },
    { XFT_STYLE,	XftTypeString, },
    { XFT_SLANT,	XftTypeInteger, },
    { XFT_WEIGHT,	XftTypeInteger, },
    { XFT_SIZE,		XftTypeDouble, },
    { XFT_PIXEL_SIZE,	XftTypeDouble, },
    { XFT_ENCODING,	XftTypeString, },
    { XFT_SPACING,	XftTypeInteger, },
    { XFT_FOUNDRY,	XftTypeString, },
    { XFT_CORE,		XftTypeBool, },
    { XFT_ANTIALIAS,	XftTypeBool, },
    { XFT_XLFD,		XftTypeString, },
    { XFT_FILE,		XftTypeString, },
    { XFT_INDEX,	XftTypeInteger, },
    { XFT_RASTERIZER,	XftTypeString, },
    { XFT_OUTLINE,	XftTypeBool, },
    { XFT_SCALABLE,	XftTypeBool, },
    { XFT_RGBA,		XftTypeInteger, },
    { XFT_SCALE,	XftTypeDouble, },
    { XFT_RENDER,	XftTypeBool, },
};

#define NUM_OBJECT_TYPES    (sizeof _XftObjectTypes / sizeof _XftObjectTypes[0])

static const XftObjectType *
XftNameGetType (const char *object)
{
    int	    i;
    
    for (i = 0; i < NUM_OBJECT_TYPES; i++)
    {
	if (!strcmp (object, _XftObjectTypes[i].object))
	    return &_XftObjectTypes[i];
    }
    return 0;
}

typedef struct _XftConstant {
    const char  *name;
    int		value;
} XftConstant;

static XftConstant XftConstants[] = {
    { "light",		XFT_WEIGHT_LIGHT, },
    { "medium",		XFT_WEIGHT_MEDIUM, },
    { "demibold",	XFT_WEIGHT_DEMIBOLD, },
    { "bold",		XFT_WEIGHT_BOLD, },
    { "black",		XFT_WEIGHT_BLACK, },

    { "roman",		XFT_SLANT_ROMAN, },
    { "italic",		XFT_SLANT_ITALIC, },
    { "oblique",	XFT_SLANT_OBLIQUE, },

    { "proportional",	XFT_PROPORTIONAL, },
    { "mono",		XFT_MONO, },
    { "charcell",	XFT_CHARCELL, },

    { "rgb",		XFT_RGBA_RGB, },
    { "bgr",		XFT_RGBA_BGR, },
};

#define NUM_XFT_CONSTANTS   (sizeof XftConstants/sizeof XftConstants[0])

Bool
XftNameConstant (char *string, int *result)
{
    int	    i;

    for (i = 0; i < NUM_XFT_CONSTANTS; i++)
	if (!strcmp (string, XftConstants[i].name))
	{
	    *result = XftConstants[i].value;
	    return True;
	}
    return False;
}

static XftValue
_XftNameConvert (XftType type, char *string)
{
    XftValue	v;

    v.type = type;
    switch (v.type) {
    case XftTypeInteger:
	if (!XftNameConstant (string, &v.u.i))
	    v.u.i = atoi (string);
	break;
    case XftTypeString:
	v.u.s = string;
	break;
    case XftTypeBool:
	v.u.b = XftDefaultParseBool (string);
	break;
    case XftTypeDouble:
	v.u.d = strtod (string, 0);
	break;
    default:
	break;
    }
    return v;
}

XftPattern *
XftNameParse (const char *name)
{
    char		*save;
    XftPattern		*pat;
    double		d;
    char		*e;
    XftValue		v;
    const XftObjectType	*t;

    save = malloc (strlen (name) + 1);
    if (!save)
	goto bail0;
    pat = XftPatternCreate ();
    if (!pat)
	goto bail1;

    for (;;)
    {
	name = _XftSplitValue (name, save);
	if (!XftPatternAddString (pat, XFT_FAMILY, save))
	    goto bail2;
	if (*name != ',')
	    break;
    }
    if (*name)
    {
	_XftSplitStr (name, save);
	d = strtod (save, &e);
	if (e != save)
	{
	    if (!XftPatternAddDouble (pat, XFT_SIZE, d))
		goto bail2;
	}
	name = strchr (name, '-');
	if (name)
	    ++name;
	while (name && *name)
	{
	    name = _XftSplitField (name, save);
	    t = XftNameGetType (save);
	    if (*name != '=')
		goto bail2;
	    {
		do
		{
		    name++;
		    name = _XftSplitValue (name, save);
		    if (t)
		    {
			v = _XftNameConvert (t->type, save);
			XftPatternAdd (pat, t->object, v, True);
		    }
		}
		while (*name == ',');
	    }
	}
    }

    free (save);
    return pat;

bail2:
    XftPatternDestroy (pat);
bail1:
    free (save);
bail0:
    return 0;
}
