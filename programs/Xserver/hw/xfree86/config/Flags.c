/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Flags.c,v 1.1.2.1 1997/07/21 10:17:43 hohndel Exp $ */
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

extern LexRec val;

static xf86ConfigSymTabRec ServerFlagsTab[] =
{
	{ENDSECTION, "endsection"},
	{NOTRAPSIGNALS, "notrapsignals"},
	{DONTZAP, "dontzap"},
	{DONTZOOM, "dontzoom"},
	{DISABLEVIDMODE, "disablevidmodeextension"},
	{ALLOWNONLOCAL, "allownonlocalxvidtune"},
	{DISABLEMODINDEV, "disablemodindev"},
	{MODINDEVALLOWNONLOCAL, "allownonlocalmodindev"},
	{ALLOWMOUSEOPENFAIL, "allowmouseopenfail"},
	{OPTION, "option"},
	{-1, ""},
};

#define CLEANUP freeFlags

XF86ConfFlagsPtr
parseFlagsSection (void)
{
	parsePrologue (XF86ConfFlagsPtr, XF86ConfFlagsRec)

		while ((token = xf86GetToken (ServerFlagsTab)) != ENDSECTION)
	{
		switch (token)
		{
			/* 
			 * these old keywords are turned into standard generic options.
			 * we fall through here on purpose
			 */
		case NOTRAPSIGNALS:
		case DONTZAP:
		case DONTZOOM:
		case DISABLEVIDMODE:
		case ALLOWNONLOCAL:
		case DISABLEMODINDEV:
		case MODINDEVALLOWNONLOCAL:
		case ALLOWMOUSEOPENFAIL:
			{
				int i = 0;
				while (ServerFlagsTab[i].token != -1)
				{
					char *tmp;

					if (ServerFlagsTab[i].token == token)
					{
						/* can't use strdup because it calls malloc */
						tmp = ConfigStrdup (ServerFlagsTab[i].name);
						ptr->flg_option_lst = addNewOption
							(ptr->flg_option_lst, tmp, NULL);
					}
					i++;
				}
			}
			break;
		case OPTION:
			{
				char *name;
				if ((token = xf86GetToken (NULL)) != STRING)
				{
					Error (BAD_OPTION_MSG, NULL);
					break;
				}

				name = val.str;
				if ((token = xf86GetToken (NULL)) == STRING)
				{
					ptr->flg_option_lst = addNewOption (ptr->flg_option_lst,
														name, val.str);
				}
				else
				{
					ptr->flg_option_lst = addNewOption (ptr->flg_option_lst,
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

#ifdef DEBUG
	printf ("Flags section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printServerFlagsSection (FILE * f, XF86ConfFlagsPtr flags)
{
	XF86OptionPtr p;

	if ((!flags) || (!flags->flg_option_lst))
		return;
	p = flags->flg_option_lst;
	fprintf (f, "Section \"ServerFlags\"\n");
	while (p)
	{
		if (p->opt_val)
			fprintf (f, "\tOption \"%s\" \"%s\"\n", p->opt_name, p->opt_val);
		else
			fprintf (f, "\tOption \"%s\"\n", p->opt_name);
		p = p->list.next;
	}
	fprintf (f, "EndSection\n\n");
}

XF86OptionPtr
addNewOption (XF86OptionPtr head, char *name, char *val)
{
	XF86OptionPtr new;

	new = (XF86OptionPtr) xf86confmalloc (sizeof (XF86OptionRec));
	new->opt_name = name;
	new->opt_val = val;
	new->list.next = NULL;

	return ((XF86OptionPtr) addListItem ((glp) head, (glp) new));
}

void
freeFlags (XF86ConfFlagsPtr flags)
{
	if (flags == NULL)
		return;
	xf86OptionListFree (flags->flg_option_lst);
	xf86conffree (flags);
}

void
xf86OptionListFree (XF86OptionPtr opt)
{
	XF86OptionPtr prev;

	while (opt)
	{
		TestFree (opt->opt_name);
		TestFree (opt->opt_val);
		prev = opt;
		opt = opt->list.next;
		xf86conffree (prev);
	}
}

/*
 * this function searches the given option list for the named option and
 * returns a pointer to the option rec if found. If not found, it returns
 * NULL
 */

XF86OptionPtr
xf86FindOption (XF86OptionPtr list, char *name)
{
	while (list)
	{
		if (StrCaseCmp (list->opt_name, name) == 0)
			return (list);
		list = list->list.next;
	}
	return (NULL);
}

/*
 * this function searches the given option list for the named option. If
 * found and the option has a parameter, a pointer to the parameter is
 * returned. If the option does not have a parameter, a pointer to the option 
 * name is returned. If the option is not found, a NULL is returned
 */

char *
xf86FindOptionValue (XF86OptionPtr list, char *name)
{
	XF86OptionPtr p = xf86FindOption (list, name);

	if (p)
	{
		if (p->opt_val)
			return (p->opt_val);
		else
			return (p->opt_name);
	}
	return (NULL);
}

XF86OptionPtr
xf86OptionListCreate (char **options, int count)
{
	XF86OptionPtr p = NULL;
	char *t1, *t2;
	int i;

	if ((count % 2) != 0)
	{
		fprintf (stderr,
			"xf86OptionListCreate: count must be an even number.\n");
		return (NULL);
	}
	for (i = 0; i < count; i += 2)
	{
		/* can't use strdup because it calls malloc */
		t1 = (char *) xf86confmalloc (sizeof (char) *
				(strlen (options[i]) + 1));
		strcpy (t1, options[i]);
		t2 = (char *) xf86confmalloc (sizeof (char) *
				(strlen (options[i + 1]) + 1));
		strcpy (t2, options[i + 1]);
		p = addNewOption (p, t1, t2);
	}

	return (p);
}

XF86OptionPtr
xf86OptionListMerge (XF86OptionPtr head, XF86OptionPtr tail)
{
	XF86OptionPtr a, b, ap = NULL, bp = NULL, f = NULL;
XF86ConfFlagsRec ff;

	a = head;
	while (a)
	{
		bp = NULL;
		b = tail;
		while (b)
		{
			if (StrCaseCmp (a->opt_name, b->opt_name) == 0)
			{
				if ((a == head) && (b == tail))
				{
					head = b;
					tail = b->list.next;
					b->list.next = a->list.next;
					bp = tail;
				}
				else if (a == head)
				{
					head = b;
					bp->list.next = b->list.next;
					b->list.next = a->list.next;
				}
				else if (b == tail)
				{
					tail = b->list.next;
					ap->list.next = b;
					b->list.next = a->list.next;
					bp = tail;
				}
				else
				{
					ap->list.next = b;
					bp->list.next = b->list.next;
					b->list.next = a->list.next;
				}
				a->list.next = f;
				f = a;
				a = b;
				b = bp;
				continue;
			}
			bp = b;
			b = b->list.next;
		}
		ap = a;
		a = a->list.next;
	}

	ap->list.next = tail;

	xf86OptionListFree (f);
	return (head);
}
