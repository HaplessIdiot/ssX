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
#include <ctype.h>

static struct {
    int	    token;
    char    *name;
} tokens[] = {
    { XFT_MATCH,    "match", },
    { XFT_EDIT,	    "edit", },
    { XFT_PATH,	    "path", },
    { XFT_DIR,	    "dir", },
    { XFT_FACE,	    "face", },
    { XFT_ENCODING, "encoding", },
    { XFT_FILE,	    "file", },
    { XFT_SIZE,	    "size", },
    { XFT_ROTATION, "rotation", },
    { XFT_SPACING,  "spacing", },
};

#define NUM_TOKEN   (sizeof (tokens) / sizeof (tokens[0]))

int
_XftLex (XftParseState *s)
{
    int	    c;
    int	    len;
    int	    i;

again:
    while ((c = getc(s->file)) != EOF)
    {
	if (!isspace(c))
	    break;
	if (c == '\n')
	    ++s->line;
    }
    if (c == EOF)
	return s->token = XFT_EOF;
    switch (c) {
    case '=':
	return s->token = XFT_EQUAL;
    case '#':
	while ((c = getc(s->file)) != EOF)
	{
	    if (c == '\n')
	    {
		++s->line;
		goto again;
	    }
	}
	return s->token = XFT_EOF;
    case '"':
	len = 0;
	while ((c = getc(s->file)) != EOF)
	{
	    switch (c) {
	    case '"':
		break;
	    case '\\':
		c = getc(s->file);
		if (c == EOF)
		    break;
	    default:
		if (c == '\n')
		    ++s->line;
		if (len < XFT_MAX_STRING - 1)
		    s->string[len++] = c;
		continue;
	    }
	    break;
	}
	s->string[len] = '\0';
	return s->token = XFT_STRING;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
	s->number = c - '0';
	while ((c = getc(s->file)) != EOF)
	{
	    if (!isdigit (c))
		break;
	    s->number = s->number * 10 + (c - '0');
	}
	if (c != EOF)
	    ungetc (c, s->file);
	return s->token = XFT_NUMBER;
    default:
	if (isalpha (c))
	{
	    len = 1;
	    s->string[0] = tolower (c);
	    while ((c = getc(s->file)) != EOF)
	    {
		if (!isalpha (c))
		    break;
		if (len < XFT_MAX_STRING-1)
		    s->string[len++] = tolower (c);
	    }
	    if (c != EOF)
		ungetc (c, s->file);
	    s->string[len] = '\0';
	    for (i = 0; i < NUM_TOKEN; i++)
		if (!strcmp (tokens[i].name, s->string))
		    return s->token = tokens[i].token;
	}
	return s->token = XFT_ERROR;
    }
}
