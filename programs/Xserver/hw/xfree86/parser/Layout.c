/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Layout.c,v 1.4 1999/01/14 13:05:15 dawes Exp $ */
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
	{INACTIVE, "inactive"},
	{OPTION, "option"},
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
		case INACTIVE:
			{
				XF86ConfInactivePtr iptr;

				iptr = xf86confmalloc (sizeof (XF86ConfInactiveRec));
				iptr->list.next = NULL;
				if (xf86GetToken (NULL) != STRING)
					Error (INACTIVE_MSG, NULL);
				iptr->inactive_device_str = val.str;
				ptr->lay_inactive_lst = (XF86ConfInactivePtr)
					addListItem ((glp) ptr->lay_inactive_lst, (glp) iptr);
			}
			break;
		case SCREEN:
			{
				XF86ConfAdjacencyPtr aptr;

				aptr = xf86confmalloc (sizeof (XF86ConfAdjacencyRec));
				aptr->list.next = NULL;
				aptr->adj_scrnum = -1;
				if ((token = xf86GetToken (NULL)) == NUMBER)
				{
					aptr->adj_scrnum = val.num;
					if ((token = xf86GetToken (NULL)) != STRING)
						Error (SCREEN_MSG, NULL);
				}
				else if (token != STRING)
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
		case OPTION:
			{
				char *name;
				if ((token = xf86GetToken (NULL)) != STRING)
					Error (BAD_OPTION_MSG, NULL);
				name = val.str;
				if ((token = xf86GetToken (NULL)) == STRING)
				{
					ptr->lay_option_lst =
					    addNewOption (ptr->lay_option_lst,
							  name, val.str);
				}
				else
				{
					ptr->lay_option_lst =
					    addNewOption (ptr->lay_option_lst,
							  name, NULL);
					xf86UnGetToken (token);
				}
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
	XF86ConfInactivePtr iptr;
	XF86OptionPtr optr;

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
		for (iptr = ptr->lay_inactive_lst; iptr; iptr = iptr->list.next)
			fprintf (cf, "\tInactive       \"%s\"\n", iptr->inactive_device_str);
		for (optr = ptr->lay_option_lst; optr; optr = optr->list.next)
		{
			fprintf (cf, "\tOption      \"%s\"", optr->opt_name);
			if (optr->opt_val)
				fprintf (cf, " \"%s\"", optr->opt_val);
			fprintf (cf, "\n");
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
	XF86ConfInactivePtr iptr;
	XF86ConfScreenPtr screen;
    XF86ConfDevicePtr device;

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
		iptr = layout->lay_inactive_lst;
		while (iptr)
		{
			device = xf86FindDevice (iptr->inactive_device_str,
									p->conf_device_lst);
			if (!device)
			{
				xf86ValidationError (UNDEFINED_DEVICE_LAY_MSG,
						iptr->inactive_device_str, layout->lay_identifier);
				return (FALSE);
			}
			else
				iptr->inactive_device = device;
			iptr = iptr->list.next;
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

