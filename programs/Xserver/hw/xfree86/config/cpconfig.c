/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/cpconfig.c,v 1.1.2.1 1997/07/21 10:17:49 hohndel Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "xf86Parser.h"

char xf86ConfigFile[128] = "";

#ifdef MALLOC_FUNCTIONS
void
xfree (void *p)
{
	free (p);
	return;
}

void *
xalloc (int size)
{
	return malloc (size);
}

void *
xrealloc (void *p, int size)
{
	return realloc (p, size);
}

#endif

/* 
 * A portable hack at implementing strcasecmp()
 * The characters '_', ' ', and '\t' are ignored in the comparison
 */
int
StrCaseCmp (const char *s1, const char *s2)
{
	char c1, c2;

	if (*s1 == 0)
		if (*s2 == 0)
			return (0);
		else
			return (1);

	while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
		s1++;
	while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
		s2++;
	c1 = (isupper (*s1) ? tolower (*s1) : *s1);
	c2 = (isupper (*s2) ? tolower (*s2) : *s2);
	while (c1 == c2)
	{
		if (c1 == '\0')
			return (0);
		s1++;
		s2++;
		while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
			s1++;
		while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
			s2++;
		c1 = (isupper (*s1) ? tolower (*s1) : *s1);
		c2 = (isupper (*s2) ? tolower (*s2) : *s2);
	}
	return (c1 - c2);
}

void
main (void)
{
	char filename[128];
	XF86ConfigPtr conf;

	if (xf86OpenConfigFile (filename))
	{
		fprintf (stderr, "Opened %s for the config file\n", filename);
	}
	else
	{
		fprintf (stderr, "Unable to open config file\n");
		exit (1);
	}

	if ((conf = xf86ReadConfigFile ()) == NULL)
	{
		fprintf (stderr, "Problem when parsing config file\n");
	}
	else
	{
		fprintf (stderr, "Config file parsed OK\n");
	}
	xf86CloseConfigFile ();

#if 0
	strcpy (xf86ConfigFile, "XF86Config.2");
	if (xf86OpenConfigFile (filename))
	{
		fprintf (stderr, "Opened %s for the config file\n", filename);
	}
	else
	{
		fprintf (stderr, "Unable to open config file\n");
		exit (1);
	}

	if ((conf = xf86ReadConfigFile ()) == NULL)
	{
		fprintf (stderr, "Problem when parsing config file\n");
	}
	else
	{
		fprintf (stderr, "Config file parsed OK\n");
	}
	xf86CloseConfigFile ();
#endif
/**
	xf86WriteConfigFile ("XF86Config.new", conf);
**/
}
