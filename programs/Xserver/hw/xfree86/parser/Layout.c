/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Layout.c,v 1.1.2.4 1998/06/02 14:49:20 dawes Exp $ */
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

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"
#include <string.h>

extern LexRec val;

static xf86ConfigSymTabRec LayoutTab[] =
{
	{ENDSECTION, "endsection"},
	{SCREEN, "screen"},
	{IDENTIFIER, "identifier"},
	{-1, ""},
};

#define CLEANUP freeLayoutList

XF86ConfLayoutPtr
parseLayoutSection (void)
{
	int has_ident = FALSE;
	parsePrologue (XF86ConfLayoutPtr, XF86ConfLayoutRec)

		while ((token = xf86GetToken (LayoutTab)) != ENDSECTION)
	{
		switch (token)
		{
		case IDENTIFIER:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Identifier");
			ptr->lay_identifier = val.str;
			has_ident = TRUE;
			break;
		case SCREEN:
			{
				XF86ConfAdjacencyPtr aptr;

				aptr = (XF86ConfAdjacencyPtr) xf86confmalloc
					(sizeof (XF86ConfAdjacencyRec));
				aptr->list.next = NULL;
				if (xf86GetToken (NULL) != STRING)
					Error (SCREEN_MSG, NULL);
				aptr->adj_screen_str = val.str;

				/* top */
				if ((token = xf86GetToken (NULL)) != STRING)
				{
				/* 
				 * if there are no other values after the
				 * first screen name assume "" "" "" ""
				 */
					xf86UnGetToken (token);
					aptr->adj_top_str = ConfigStrdup ("");
					aptr->adj_bottom_str = ConfigStrdup ("");
					aptr->adj_left_str = ConfigStrdup ("");
					aptr->adj_right_str = ConfigStrdup ("");
				}
				else
				{
					aptr->adj_top_str = val.str;

					/* bottom */
					if (xf86GetToken (NULL) != STRING)
						Error (SCREEN_MSG, NULL);
					aptr->adj_bottom_str = val.str;

					/* left */
					if (xf86GetToken (NULL) != STRING)
						Error (SCREEN_MSG, NULL);
					aptr->adj_left_str = val.str;

					/* right */
					if (xf86GetToken (NULL) != STRING)
						Error (SCREEN_MSG, NULL);
					aptr->adj_right_str = val.str;

				}
				ptr->lay_adjacency_lst = (XF86ConfAdjacencyPtr)
					addListItem ((glp) ptr->lay_adjacency_lst, (glp) aptr);
			}
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
			break;
		}
	}

	if (!has_ident)
		Error (NO_IDENT_MSG, NULL);

#ifdef DEBUG
	printf ("Layout section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printLayoutSection (FILE * cf, XF86ConfLayoutPtr ptr)
{
	XF86ConfAdjacencyPtr aptr;

	while (ptr)
	{
		fprintf (cf, "Section \"ServerLayout\"\n");
		if (ptr->lay_identifier)
			fprintf (cf, "\tIdentifier     \"%s\"\n", ptr->lay_identifier);

		for (aptr = ptr->lay_adjacency_lst; aptr; aptr = aptr->list.next)
		{
			fprintf (cf, "\tScreen         \"%s\"", aptr->adj_screen_str);
			fprintf (cf, " \"%s\"", aptr->adj_top_str);
			fprintf (cf, " \"%s\"", aptr->adj_bottom_str);
			fprintf (cf, " \"%s\"", aptr->adj_right_str);
			fprintf (cf, " \"%s\"\n", aptr->adj_left_str);
		}
		fprintf (cf, "EndSection\n\n");
		ptr = ptr->list.next;
	}
}

void
freeLayoutList (XF86ConfLayoutPtr ptr)
{
	XF86ConfLayoutPtr prev;

	while (ptr)
	{
		TestFree (ptr->lay_identifier);
		freeAdjacencyList (ptr->lay_adjacency_lst);
		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

void
freeAdjacencyList (XF86ConfAdjacencyPtr ptr)
{
	XF86ConfAdjacencyPtr prev;

	while (ptr)
	{
		TestFree (ptr->adj_screen_str);
		TestFree (ptr->adj_top_str);
		TestFree (ptr->adj_bottom_str);
		TestFree (ptr->adj_left_str);
		TestFree (ptr->adj_right_str);

		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}

}

#define CheckScreen(str, ptr)\
if (str[0] != '\0') \
{ \
screen = xf86FindScreen (str, p->conf_screen_lst); \
if (!screen) \
{ \
	xf86ValidationError (UNDEFINED_SCREEN_MSG, \
				   str, layout->lay_identifier); \
	return (FALSE); \
} \
else \
	ptr = screen; \
}

int
validateLayout (XF86ConfigPtr p)
{
	XF86ConfLayoutPtr layout = p->conf_layout_lst;
	XF86ConfAdjacencyPtr adj;
	XF86ConfScreenPtr screen;

	while (layout)
	{
		adj = layout->lay_adjacency_lst;
		while (adj)
		{
			/* the first one can't be "" but all others can */
			screen = xf86FindScreen (adj->adj_screen_str, p->conf_screen_lst);
			if (!screen)
			{
				xf86ValidationError (UNDEFINED_SCREEN_MSG,
							   adj->adj_screen_str, layout->lay_identifier);
				return (FALSE);
			}
			else
				adj->adj_screen = screen;

			CheckScreen (adj->adj_top_str, adj->adj_top);
			CheckScreen (adj->adj_bottom_str, adj->adj_bottom);
			CheckScreen (adj->adj_left_str, adj->adj_left);
			CheckScreen (adj->adj_right_str, adj->adj_right);

			adj = adj->list.next;
		}
		layout = layout->list.next;
	}
	return (TRUE);
}

XF86ConfLayoutPtr
xf86FindLayout (const char *name, XF86ConfLayoutPtr list)
{
    while (list)
    {
        if (NameCompare (list->lay_identifier, name) == 0)
            return (list);
        list = list->list.next;
    }
    return (NULL);
}

