/*
 * $XFree86: xc/lib/Xft/parse.c,v 1.1 2000/10/05 18:05:27 keithp Exp $
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

static Bool dbg;

#define DBG(L, F)	if (dbg >= L) fprintf F;

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
			    DBG (2,(stderr, "\tface mismatch \"%s\"\n", s->string));
			    s->state = XftSkipping;
			}
			else
			{
			    DBG (1,(stderr, "\tface match \"%s\"\n", s->string));
			}
			break;
		    case XFT_ENCODING:
			if ((s->name->mask & XftFontNameEncoding) &&
			    !_XftStrImatch (s->string, s->name->encoding))
			{
			    DBG (2,(stderr, "\tencoding mismatch \"%s\"\n", s->string));
			    s->state = XftSkipping;
			}
			else
			{
			    DBG (1,(stderr, "\tencoding match \"%s\"\n", s->string));
			}
			break;
		    case XFT_FILE:
			if ((s->name->mask & XftFontNameFile) &&
			    !_XftStrImatch (s->string, s->name->file))
			{
			    DBG (2,(stderr, "\tfile mismatch \"%s\"\n", s->string));
			    s->state = XftSkipping;
			}
			else
			{
			    DBG (1,(stderr, "\tfile match \"%s\"\n", s->string));
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
			    DBG (1, (stderr, "\tsetting face \"%s\"\n", s->name->face));
			}
			else
			{
			    DBG (2, (stderr, "\tnot setting face \"%s\"\n", s->string));
			}
			break;
		    case XFT_ENCODING:
			if (!(s->name->mask & XftFontNameEncoding))
			{
			    s->name->encoding = _XftSaveString (s->string);
			    s->name->mask |= XftFontNameEncoding;
			    DBG (1, (stderr, "\tsetting encoding \"%s\"\n", s->name->encoding));
			}
			else
			{
			    DBG (2, (stderr, "\tnot setting face \"%s\"\n", s->string));
			}
			break;
		    case XFT_FILE:
			if (!(s->name->mask & XftFontNameFile))
			{
			    s->name->file = _XftSaveFile (s->dir, s->string);
			    s->name->mask |= XftFontNameFile;
			    DBG (1, (stderr, "\tsetting file \"%s\"\n", s->name->file));
			}
			else
			{
			    DBG (2, (stderr, "\tnot setting file \"%s\"\n", s->string));
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
			    DBG (2,(stderr, "\tsize mismatch %d\n", s->number));
			}
			else
			{
			    DBG (1,(stderr, "\tsize match %d\n", s->number));
			}
			break;
		    case XFT_ROTATION:
			if ((s->name->mask & XftFontNameRotation) &&
			    s->number != s->name->rotation)
			{
			    s->state = XftSkipping;
			    DBG (2,(stderr, "\trotation mismatch %d\n", s->number));
			}
			else
			{
			    DBG (1,(stderr, "\trotation match %d\n", s->number));
			}
			break;
		    case XFT_SPACING:
			if ((s->name->mask & XftFontNameSpacing) &&
			    s->number != s->name->spacing)
			{
			    s->state = XftSkipping;
			    DBG (2,(stderr, "\tspacing mismatch %d\n", s->number));
			}
			else
			{
			    DBG (1,(stderr, "\trotation match %d\n", s->number));
			}
			break;
		    }
		case XftSkipping:
		    break;
		case XftEditing:
		    switch (token) {
		    case XFT_SIZE:
			DBG (1,(stderr, "\t editing size %d: ", s->number));
			if (!(s->name->mask & XftFontNameSize))
			{
			    s->name->size = s->number;
			    DBG (1,(stderr, "replacing %d\n", s->number));
			    s->name->mask |= XftFontNameSize;
			}
			else
			{
			    DBG (1,(stderr, "already set\n"));
			}
			break;
		    case XFT_ROTATION:
			DBG (1,(stderr, "\t editing size %d: ", s->number));
			if (!(s->name->mask & XftFontNameRotation))
			{
			    s->name->rotation = s->number;
			    DBG (1,(stderr, "replacing %d\n", s->number));
			    s->name->mask |= XftFontNameRotation;
			}
			else
			{
			    DBG (1,(stderr, "already set\n"));
			}
			break;
		    case XFT_SPACING:
			DBG (1,(stderr, "\t editing size %d: ", s->number));
			if (!(s->name->mask & XftFontNameSpacing))
			{
			    s->name->spacing = s->number;
			    DBG (1,(stderr, "replacing %d\n", s->number));
			    s->name->mask |= XftFontNameSpacing;
			}
			else
			{
			    DBG (1,(stderr, "already set\n"));
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
		DBG (1,(stderr, "\tadding font file directory \"%s\"\n", ndir+olen));
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
    DBG (1,(stderr, "Parsing Xft config file \"%s\"\n", elt));
    state.file = fopen (elt, "r");
    if (!state.file)
    {
	DBG (1,(stderr, "Failed to open\n"));
	return ret;
    }
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
    DBG (1,(stderr, "Finished parsing \"%s\":\n"
	  "\t\tface     \"%s\"\n"
	  "\t\tencoding \"%s\"\n"
	  "\t\tfile     \"%s\"\n"
	  "\t\tsize     %d\n"
	  "\t\trotation %d\n"
	  "\t\tspacing  %d\n\n",
	  elt,
	  result->mask & XftFontNameFace ? result->face : "(no face)",
	  result->mask & XftFontNameEncoding ? result->encoding : "(no encoding)",
	  result->mask & XftFontNameFile ? result->file : "(no file)",
	  result->mask & XftFontNameSize ? result->size / 64 : -1,
	  result->mask & XftFontNameRotation ? result->rotation : -1,
	  result->mask & XftFontNameSpacing ? result->spacing : -1));
    return ret;
}

Bool
_XftParsePathList (char *path, char *dir, XftFontName *result)
{
    char    file[XFT_MAX_STRING], *home, *rest;
    char    *colon;
    char    *d;
    
    d = getenv ("XFT_DEBUG");
    if (d)
    {
	dbg = atoi(d);
	if (dbg == 0) dbg = 1;
    }
    else
	dbg = 0;
    
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
