/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Flags.c,v 1.2 1998/07/25 16:57:12 dawes Exp $ */
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
	{BLANKTIME, "blanktime"},
	{STANDBYTIME, "standbytime"},
	{SUSPENDTIME, "suspendtime"},
	{OFFTIME, "offtime"},
	{-1, ""},
};

#define CLEANUP freeFlags

XF86ConfFlagsPtr
parseFlagsSection (void)
{
	parsePrologue (XF86ConfFlagsPtr, XF86ConfFlagsRec)

	while ((token = xf86GetToken (ServerFlagsTab)) != ENDSECTION)
	{
		int hasvalue = FALSE;
		switch (token)
		{
			/* 
			 * these old keywords are turned into standard generic options.
			 * we fall through here on purpose
			 */
		case BLANKTIME:
		case STANDBYTIME:
		case SUSPENDTIME:
		case OFFTIME:
			hasvalue = TRUE;
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
						char *valstr = NULL;
						/* can't use strdup because it calls malloc */
						tmp = ConfigStrdup (ServerFlagsTab[i].name);
						if (hasvalue)
						{
							if (xf86GetToken (NULL) != NUMBER)
								Error (NUMBER_MSG, tmp);
							valstr = xf86confmalloc(16);
							if (valstr)
								sprintf(valstr, "%d", val.num);
						}
						ptr->flg_option_lst = addNewOption
							(ptr->flg_option_lst, tmp, valstr);
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
	XF86OptionPtr new, old = NULL;

	/* Don't allow duplicates */
	if (head != NULL && (old = FindOption(head, name)) != NULL)
		new = old;
	else
		new = (XF86OptionPtr) xf86confmalloc (sizeof (XF86OptionRec));
	new->opt_name = name;
	new->opt_val = val;
	new->opt_used = 0;
	new->list.next = NULL;

	if (old == NULL)
		return ((XF86OptionPtr) addListItem ((glp) head, (glp) new));
	else
		return head;
}

void
freeFlags (XF86ConfFlagsPtr flags)
{
	if (flags == NULL)
		return;
	OptionListFree (flags->flg_option_lst);
	xf86conffree (flags);
}

XF86OptionPtr
OptionListDup (XF86OptionPtr opt)
{
	XF86OptionPtr newopt = NULL;

	while (opt)
	{
		newopt = addNewOption(newopt, opt->opt_name, opt->opt_val);
		newopt->opt_used = opt->opt_used;
		opt = opt->list.next;
	}
	return newopt;
}

void
OptionListFree (XF86OptionPtr opt)
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
FindOption (XF86OptionPtr list, const char *name)
{
	while (list)
	{
		if (NameCompare (list->opt_name, name) == 0)
			return (list);
		list = list->list.next;
	}
	return (NULL);
}

/*
 * this function searches the given option list for the named option. If
 * found and the option has a parameter, a pointer to the parameter is
 * returned.  If the option does not have a parameter an empty string is
 * returned.  If the option is not found, a NULL is returned.
 */

char *
FindOptionValue (XF86OptionPtr list, const char *name)
{
	XF86OptionPtr p = FindOption (list, name);

	if (p)
	{
		if (p->opt_val)
			return (p->opt_val);
		else
			return "";
	}
	return (NULL);
}

XF86OptionPtr
OptionListCreate (char **options, int count)
{
	XF86OptionPtr p = NULL;
	char *t1, *t2;
	int i;

	if ((count % 2) != 0)
	{
		fprintf (stderr,
			"OptionListCreate: count must be an even number.\n");
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
OptionListMerge (XF86OptionPtr head, XF86OptionPtr tail)
{
	XF86OptionPtr a, b, ap = NULL, bp = NULL, f = NULL;

	a = head;
	while (a)
	{
		bp = NULL;
		b = tail;
		while (b)
		{
			if (NameCompare (a->opt_name, b->opt_name) == 0)
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

	OptionListFree (f);
	return (head);
}
