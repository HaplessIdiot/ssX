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

#include "xftint.h"
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static void
_XftParseError (XftParseState *s)
{
    if (s->token != XFT_ERROR)
	fprintf (stderr, "Xft parse error line %d\n", s->line);
    s->token = XFT_ERROR;
}

static char *
_XftSaveString (char *string)
{
    char    *s = malloc (strlen (string) + 1);
    if (s)
	strcpy (s, string);
    return s;
}

static char *
_XftSaveFile (char *dir, char *string)
{
    char    file[XFT_MAX_STRING];
    char    *colon;
    
    if (*string != '/')
    {
	while (dir && *dir)
	{
	    colon = strchr (dir, ':');
	    if (colon)
	    {
		strncpy (file, dir, colon - dir);
		file[colon-dir] = '\0';
		dir = colon + 1;
	    }
	    else
	    {
		strcpy (file, dir);
		dir = 0;
	    }
	    strcat (file, "/");
	    strcat (file, string);
    	    if (access (file, R_OK) == 0)
		return _XftSaveString (file);
	}
    } else if (access (string, R_OK) == 0)
	return _XftSaveString (string);
    return 0;
}

static Bool
_XftStrImatch (char *a, char *b)
{
    char    ac, bc;
    for (;;)
    {
	ac = *a++;
	bc = *b++;
	if (!ac)
	    return !bc;
	if (isupper (ac))
	    ac = tolower (ac);
	if (isupper (bc))
	    bc = tolower (bc);
	if (ac != bc)
	    return False;
    }
}

static void
_XftParsePattern (XftParseState *s)
{
    int	token = s->token;
    switch (s->token) {
    case XFT_FACE:
    case XFT_ENCODING:
    case XFT_FILE:
	_XftLex (s);
	switch (s->token) {
	case XFT_EQUAL:
	    _XftLex (s);
	    switch (s->token) {
	    case XFT_STRING:
		switch (s->state) {
		case XftMatching:
		    switch (token) {
		    case XFT_FACE:
			if ((s->name->mask & XftFontNameFace) &&
			    !_XftStrImatch (s->string, s->name->face))
			{
			    s->state = XftSkipping;
			}
			break;
		    case XFT_ENCODING:
			if ((s->name->mask & XftFontNameEncoding) &&
			    !_XftStrImatch (s->string, s->name->encoding))
			{
			    s->state = XftSkipping;
			}
			break;
		    case XFT_FILE:
			if ((s->name->mask & XftFontNameFile) &&
			    !_XftStrImatch (s->string, s->name->file))
			{
			    s->state = XftSkipping;
			}
			break;
		    }
		    break;
		case XftSkipping:
		    break;
		case XftEditing:
		    switch (token) {
		    case XFT_FACE:
			if (!(s->name->mask & XftFontNameFace))
			{
			    s->name->face = _XftSaveString (s->string);
			    s->name->mask |= XftFontNameFace;
			}
			break;
		    case XFT_ENCODING:
			if (!(s->name->mask & XftFontNameEncoding))
			{
			    s->name->encoding = _XftSaveString (s->string);
			    s->name->mask |= XftFontNameEncoding;
			}
			break;
		    case XFT_FILE:
			if (!(s->name->mask & XftFontNameFile))
			{
			    s->name->file = _XftSaveFile (s->dir, s->string);
			    s->name->mask |= XftFontNameFile;
			}
			break;
		    }
		}
		_XftLex (s);
		break;
	    default:
		_XftParseError (s);
		break;
	    }
	    break;
	default:
	    _XftParseError (s);
	    break;
	}
	break;
    case XFT_SIZE:
    case XFT_ROTATION:
    case XFT_SPACING:
	_XftLex (s);
	switch (s->token) {
	case XFT_EQUAL:
	    _XftLex (s);
	    switch (s->token) {
	    case XFT_NUMBER:
		switch (s->state) {
		case XftMatching:
		    switch (token) {
		    case XFT_SIZE:
			if ((s->name->mask & XftFontNameSize) &&
			    s->number != s->name->size)
			{
			    s->state = XftSkipping;
			}
			break;
		    case XFT_ROTATION:
			if ((s->name->mask & XftFontNameRotation) &&
			    s->number != s->name->rotation)
			{
			    s->state = XftSkipping;
			}
			break;
		    case XFT_SPACING:
			if ((s->name->mask & XftFontNameSpacing) &&
			    s->number != s->name->spacing)
			{
			    s->state = XftSkipping;
			}
			break;
		    }
		case XftSkipping:
		    break;
		case XftEditing:
		    switch (token) {
		    case XFT_SIZE:
			if (!(s->name->mask & XftFontNameSize))
			{
			    s->name->size = s->number;
			    s->name->mask |= XftFontNameSize;
			}
			break;
		    case XFT_ROTATION:
			if (!(s->name->mask & XftFontNameRotation))
			{
			    s->name->rotation = s->number;
			    s->name->mask |= XftFontNameRotation;
			}
			break;
		    case XFT_SPACING:
			if (!(s->name->mask & XftFontNameSpacing))
			{
			    s->name->spacing = s->number;
			    s->name->mask |= XftFontNameSpacing;
			}
			break;
		    }
		}
		_XftLex (s);
		break;
	    default:
		_XftParseError (s);
		break;
	    }
	    break;
	default:
	    _XftParseError (s);
	    break;
	}
	break;
    default:
	_XftParseError (s);
	break;
    }
}

static void
_XftParsePatterns  (XftParseState *s)
{
    for (;;)
    {
	switch (s->token) {
	case XFT_FACE:
	case XFT_ENCODING:
	case XFT_FILE:
	case XFT_SIZE:
	case XFT_ROTATION:
	case XFT_SPACING:
	    _XftParsePattern (s);
	    break;
	case XFT_MATCH:
	case XFT_EDIT:
	case XFT_PATH:
	case XFT_DIR:
	case XFT_EOF:
	    return;
	default:
	    _XftParseError (s);
	    return;
	}
    }
}

static void
_XftParseEdit (XftParseState *s)
{
    switch (s->token) {
    case XFT_MATCH:
	_XftLex (s);
	s->state = XftMatching;
	switch (s->token) {
	case XFT_FACE:
	case XFT_ENCODING:
	case XFT_FILE:
	case XFT_SIZE:
	case XFT_ROTATION:
	case XFT_SPACING:
	case XFT_EDIT:
	    _XftParsePatterns (s);
	    break;
	default:
	    _XftParseError (s);
	    return;
	}
	switch (s->token) {
	case XFT_EDIT:
	    _XftLex (s);
	    if (s->state == XftMatching)
		s->state = XftEditing;
	    switch (s->token) {
	    case XFT_FACE:
	    case XFT_ENCODING:
	    case XFT_FILE:
	    case XFT_SIZE:
	    case XFT_ROTATION:
	    case XFT_SPACING:
	    case XFT_MATCH:
	    case XFT_PATH:
	    case XFT_DIR:
	    case XFT_EOF:
		_XftParsePatterns (s);
		break;
	    default:
		_XftParseError (s);
		return;
	    }
	    break;
	default:
	    _XftParseError (s);
	    return;
	}
	break;
    default:
	_XftParseError (s);
	return;
    }
}
    
static void
_XftParsePath (XftParseState *s)
{
    switch (s->token) {
    case XFT_PATH:
	_XftLex (s);
	switch (s->token) {
	case XFT_STRING:
	    if (!_XftParsePathList (s->string, s->dir, s->name))
		s->token = XFT_ERROR;
	    else
		_XftLex (s);
	    break;
	default:
	    _XftParseError (s);
	    break;
	}
	break;
    default:
	_XftParseError (s);
	return;
    }
}

static void
_XftParseDir (XftParseState *s)
{
    int	    len, olen;
    char    *ndir;
    char    *add;
    
    switch (s->token) {
    case XFT_DIR:
	_XftLex (s);
	switch (s->token) {
	case XFT_STRING:
	    if (!strcmp (s->string, "."))
	    {
		add = strrchr (s->filename, '/');
		if (add)
		{
		    len = add - s->filename;
		    add = s->filename;
		}
		else
		{
		    len = strlen (s->string);
		    add = s->string;
		}
	    }
	    else
	    {
		len = strlen (s->string);
		add = s->string;
	    }
	    if (s->dir)
	    {
		olen = strlen (s->dir);
		ndir = realloc (s->dir, olen + len + 2);
	    }
	    else
	    {
		olen = 0;
		ndir = malloc (len + 1);
	    }
	    if (ndir)
	    {
		if (olen)
		    ndir[olen++] = ':';
		strncpy (ndir + olen, add, len);
		ndir[olen+len] = '\0';
		s->dir = ndir;
	    }
	    _XftLex (s);
	    break;
	default:
	    _XftParseError (s);
	    break;
	}
	break;
    default:
	_XftParseError (s);
	return;
    }
}

static void
_XftParseEnt (XftParseState *s)
{
    switch (s->token) {
    case XFT_MATCH:
	_XftParseEdit (s);
	return;
    case XFT_PATH:
	_XftParsePath (s);
	return;
    case XFT_DIR:
	_XftParseDir (s);
	return;
    default:
	_XftParseError (s);
	return;
    }
}

static void
_XftParseEnts (XftParseState *s)
{
    for (;;)
    {
	switch (s->token) {
	case XFT_MATCH:
	case XFT_PATH:
	case XFT_DIR:
	    _XftParseEnt (s);
	    break;
	case XFT_EOF:
	    return;
	default:
	    _XftParseError (s);
	    return;
	}
    }
}

static Bool
_XftParse (XftParseState *s)
{
    _XftLex (s);
    _XftParseEnts (s);
    if (s->token == XFT_ERROR)
	return False;
    return True;
}

static Bool
_XftParsePathElt (char *elt, char *dir, XftFontName *result)
{
    XftParseState   state;
    Bool	    ret = False;
    
    if (*elt == '?')
    {
	ret = True;
	elt++;
    }
    state.file = fopen (elt, "r");
    if (!state.file)
	return ret;
    state.filename = elt;
    state.line = 1;
    state.token = XFT_EOF;
    state.number = 0;
    state.string[0] = '\0';
    state.state = XftMatching;
    state.name = result;
    if (dir)
	state.dir = _XftSaveString (dir);
    else
	state.dir = 0;
    ret = _XftParse (&state);
    fclose (state.file);
    return ret;
}

Bool
_XftParsePathList (char *path, char *dir, XftFontName *result)
{
    char    file[XFT_MAX_STRING], *home, *rest;
    char    *colon;
    
    while (path && *path)
    {
	rest = file;
	if (*path == '?')
	    *rest++ = *path++;
	if (*path == '~')
	{
	    home = getenv ("HOME");
	    if (home)
	    {
		strcpy (rest, home);
		rest = file + strlen (file);
	    }
	    else
		rest = 0;
	    path++;
	}
	colon = strchr (path, ':');
	if (!colon)
	{
	    if (rest)
		strcpy (rest, path);
	    path = colon;
	}
	else
	{
	    if (rest)
	    {
		strncpy (rest, path, colon-path);
		rest[colon-path] = '\0';
	    }
	    path = colon + 1;
	}
	if (rest)
	{
	    if (!_XftParsePathElt (file, dir, result))
		return False;
	}
    }
    return True;
}
