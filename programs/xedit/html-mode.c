/*
 * Copyright (c) 1999 by The XFree86 Project, Inc.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86$ */

#include "xedit.h"
#include <X11/IntrinsicP.h>
#include <X11/Xaw/TextSinkP.h>
#include <X11/Xaw/TextSrcP.h>
#include <X11/Xmu/Xmu.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>		/* for bsearch() */
#endif
#include <ctype.h>

#define Html_Peek(parser)	((parser)->next)

/*
 * Types
 */
typedef struct _Html_Parser Html_Parser;
typedef struct _Html_Item Html_Item;

typedef struct _Html_TagInfo {
    char *name;
    int entity	: 1;	/* it changes the type of the text */
    int nest	: 1;	/* does not close tags automatically, like in <font><font></font></font> */
    int end	: 1;	/* need a close markup? <br> and <hr> don't need */
    int snl	: 1;	/* insert newline when starting tag? */
    int enl	: 1;	/* insert newline when ending tag? */
    unsigned long mask;	/* enforce use of attributes of this tag-info */
    unsigned long xlfd_mask;
    void (*parse_args)(Html_Parser*, Html_Item*);
    XawTextProperty *override;
    XrmQuark ident;
} Html_TagInfo;

struct _Html_Item {
    XrmQuark entity;
    XawTextPosition start, end;
    Html_TagInfo *info;

    XawTextProperty *combine;
    Bool override;		/* combine has high precedence? */

    XawTextBlock *replace;	/* can be a union, with previous two itens */
    int mask;

    Html_Item *parent, *child, *prev, *next;
};

struct _Html_Parser {
    Widget source;
    XawTextBlock block;
    XawTextPosition position, offset, start, end, last;
    XrmQuark quark;
    int i, ch, next, space;
    Html_Item *item, *head;
    XmuScanline *mask;
    int pre;		/* inside a pre formated text block? */
    int snl;		/* flag to prevent adding more new lines than what is
			 * required, like in </h1><p>. Shoud add only 2
			 * new-lines, not 3, or, in a worst case, find something
			 * like </h1><p><p>.
			 */
    Bool spc;		/* flag to prevent adding consecutive spaces */
    Pixel alink;
    XawTextBlock *replace;	/* latin? character entity */
};

/*
 * Prototypes
 */
void Html_ModeStart(Widget);
void Html_ModeEnd(Widget);
static void Html_ModeInit(void);
static int Html_Get(Html_Parser*);
static void Html_Commit(Html_Parser*);
static void Html_ParseTag(Html_Parser*);
static int Html_Parse1(Html_Parser*);
static int Html_Parse2(Html_Parser*);
static int Html_Parse3(Html_Parser*);
static void Html_ParseCallback(Widget, XtPointer, XtPointer);
static Html_TagInfo *Html_GetInfo(char*);
static void Html_AddEntities(Html_Parser*, Html_Item*);

static void Html_AArgs(Html_Parser*, Html_Item*);
static void Html_FontArgs(Html_Parser*, Html_Item*);

/*
 * Initialization
 */
static XrmQuark
	Qbr,
	Qdefault,
	Qentity,
	Qhide,
	Qp,
	Qpre,
	Qspace,
	Qetag,
	Qtag;

static Html_TagInfo tag_info[] = {
    {"a",	1, 0, 1, 0, 0,
     0, 0,
     Html_AArgs},
    {"address",	1, 0, 1, 0, 0,
     0, XAW_TPROP_SLANT,
     },
    {"b",	1, 0, 1, 0, 0,
     0, XAW_TPROP_WEIGHT,
     },
    {"body",	0, 0, 1, 0, 0,
     0, 0,
     },
    {"br",	0, 0, 0, 0, 0,
     },
    {"em",	1, 0, 1, 0, 0,
     0, XAW_TPROP_SLANT,
     },
    {"font",	1, 1, 1, 0, 0,
     0, 0,
     Html_FontArgs},
    {"h1",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"h2",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"h3",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"h4",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"h5",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"h6",	1, 0, 1, 1, 1,
     0, XAW_TPROP_WEIGHT | XAW_TPROP_PIXELSIZE,
     },
    {"head",	0, 0, 1, 0, 0,
     0, 0,
     },
    {"html",	0, 0, 1, 0, 0,
     0, 0,
     },
    {"i",	1, 0, 1, 0, 0,
     0, XAW_TPROP_SLANT,
     },
    {"kbd",	1, 0, 1, 0, 0,
     0, XAW_TPROP_FAMILY | XAW_TPROP_PIXELSIZE,
     },
    {"p",	0, 0, 0, 0, 0,
     },
    {"pre",	1, 0, 1, 1, 1,
     0, XAW_TPROP_FAMILY | XAW_TPROP_PIXELSIZE,
     },
    {"strong",	1, 0, 1, 0, 0,
     0, XAW_TPROP_WEIGHT,
     },
};

/*
 * Implementation
 */
void
Html_ModeStart(Widget src)
{
    Html_Parser parser;
    Html_Item *next, *item;
    XColor color, exact;

    if (XAllocNamedColor(XtDisplay(topwindow), topwindow->core.colormap,
			 "blue", &color, &exact))
	parser.alink = color.pixel;
    else
	parser.alink = 0L;

    Html_ModeInit();

    /* initialize parser state */
    parser.source = src;
    parser.position = XawTextSourceRead(parser.source, 0,
					&parser.block, 4096);
    parser.offset = -1;
    parser.quark = NULLQUARK;
    parser.i = 0;
    parser.last = XawTextSourceScan(src, 0, XawstAll, XawsdRight, 1, 1);
    parser.mask = XmuNewScanline(0, 0, 0);
    parser.snl = 0;
    parser.spc = True;
    parser.pre = 0;
    parser.item = parser.head = NULL;
    if (parser.block.length == 0)
	parser.ch = parser.next = EOF;
    else
	(void)Html_Get(&parser);

    while (Html_Parse1(&parser) != EOF)
	;

    /* create top level entity mask */
    (void)XmuScanlineNot(parser.mask, 0, parser.last);

    item = parser.item;
    while (item) {
	next = item->next;
	Html_AddEntities(&parser, item);
	if (item->combine)
	    XtFree((XtPointer)item->combine);
	XtFree((XtPointer)item);
	item = next;
    }
    XmuDestroyScanline(parser.mask);

    /* add callbacks for interactive changes */
    XtAddCallback(src, XtNpropertyCallback, Html_ParseCallback, NULL);
}

void
Html_ModeEnd(Widget src)
{
    XtRemoveCallback(src, XtNpropertyCallback, Html_ParseCallback, NULL);
}

static void
Html_ParseCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
}

static void
Html_AddEntities(Html_Parser *parser, Html_Item *item)
{
    Html_Item *parent, *next, *child = item->child;
    XmuSegment segment, *ent;
    XmuScanline *mask = XmuNewScanline(0, 0, 0);
    XawTextProperty *tprop, *property = NULL;
    Widget sink;

    /* combine properties */
    if (item->info && item->info->entity) {
	sink = XawTextGetSink(textwindow);
	parent = item->parent;
	property = XawTextSinkCopyProperty(sink, item->entity);
	property->mask = item->info->mask;
	property->xlfd_mask = item->info->xlfd_mask;
	if (parent) {
	    (void)XawTextSinkCombineProperty(sink, property,
			XawTextSinkGetProperty(sink, parent->entity), False);
	    if (item->combine && parent->combine)
		(void)XawTextSinkCombineProperty(sink, item->combine,
					 	 parent->combine,
						 item->override);
	}
	if (item->combine)
	    XawTextSinkCombineProperty(sink, property, item->combine, True);
	tprop = property;
	property = XawTextSinkAddProperty(sink, property);
	XtFree((XtPointer)tprop);
	if (property)
	    item->entity = property->identifier;
    }

    if (item->end < 0) {
	if (item->next)
	    item->end = item->next->start;
	else if (item->parent)
	    item->end = item->parent->end;
	else
	    item->end = parser->last;
    }

    while (child) {
	next = child->next;
	segment.x1 = child->start;
	segment.x2 = child->end;
	(void)XmuScanlineOrSegment(mask, &segment);
	Html_AddEntities(parser, child);
	if (child->combine)
	    XtFree((XtPointer)child->combine);
	XtFree((XtPointer)child);
	child = next;
    }

    /* build entity mask */
    (void)XmuScanlineNot(mask, item->start, item->end);
    (void)XmuScanlineAnd(mask, parser->mask);

    /* add entities */
    if (item->info && item->info->entity) {
	for (ent = mask->segment; ent; ent = ent->next)
	    (void)XawTextSourceAddEntity(parser->source, 0, 0, ent->x1,
					 ent->x2 - ent->x1, item->entity);
    }
    else if (item->info == NULL) {
	XrmQuark quark;

	parent = item->parent;
	if (parent->info->entity)
	    quark = parent->entity;
	else
	    quark = item->entity;

	(void)XawTextSourceAddEntity(parser->source, (int)item->replace,
				     item->mask, item->start,
				     item->end - item->start, quark);
    }

    /* set mask for parent entities */
    (void)XmuScanlineOr(parser->mask, mask);
    XmuDestroyScanline(mask);
}

static int
bcmp_tag_info(_Xconst void *left, _Xconst void *right)
{
    return (strcmp((char*)left, ((Html_TagInfo*)right)->name));
}

static Html_TagInfo *
Html_GetInfo(char *name)
{
    return (bsearch(name, tag_info, sizeof(tag_info) / sizeof(tag_info[0]),
		    sizeof(Html_TagInfo), bcmp_tag_info));
}

static int
Html_Get(Html_Parser *parser)
{
    if (parser->ch == EOF)
	return (EOF);
    if (parser->i >= parser->block.length) {
	parser->i = 0;
	parser->position = XawTextSourceRead(parser->source, parser->position,
					     &parser->block, 4096);
    }
    parser->ch = parser->next;
    if (parser->block.length == 0)
	parser->next = EOF;
    else
	parser->next = (unsigned char)parser->block.ptr[parser->i++];
    parser->offset++;

    return (parser->ch);
}

static void
Html_Commit(Html_Parser *parser)
{
    static XawTextBlock space = {0, 1, " "};
    static XawTextBlock br = {0, 1, "\n"};
    static XawTextBlock p = {0, 2, "\n\n"};

    XawTextPosition position;
    int length;

    position = parser->start;
    length = parser->end - parser->start;
    if (position < 0) {
	length += position;
	position = 0;
    }
    if (position + length > parser->last + 1)
	length -= (position + length) - parser->last + 1;

    if (parser->quark != Qdefault && parser->quark != NULLQUARK && length > 0) {
	XrmQuark quark = parser->quark;
	Bool clip = False;
	Html_Item *head = parser->head;
	XawTextBlock *entity = NULL;
	int mask = 0;

	parser->quark = Qdefault;
	if ((quark == Qspace && parser->spc) ||
	    (quark == Qp && parser->snl >= 2))
	    quark = Qhide;

	if (quark == Qbr || quark == Qetag) {
	    parser->snl = 1;
	    parser->spc = True;
	    XawTextSourceAddEntity(parser->source, (int)&br,
				   XAW_TENTF_READ | XAW_TENTF_REPLACE,
				   position, length, quark);
	    clip = True;
	}
	else if (quark == Qp || quark == Qtag) {
	    parser->spc = True;
	    if (quark == Qtag)
		parser->snl = 0;
	    else
		++parser->snl;
	    if (parser->snl > 1) {
		XawTextSourceAddEntity(parser->source, (int)&br,
				       XAW_TENTF_READ | XAW_TENTF_REPLACE,
				       position, length, quark);
		clip = True;
	    }
	    else {
		entity = &p;
		mask = XAW_TENTF_READ | XAW_TENTF_REPLACE;
		head = head && quark == Qtag ? head->parent : head;
	    }
	}
	else if (quark == Qspace) {
	    if (length == 1 && parser->space == ' ')
		return;
	    entity = &space;
	    mask = XAW_TENTF_READ | XAW_TENTF_REPLACE;
	}
	else if (quark == Qentity) {
	    entity = parser->replace;
	    mask = XAW_TENTF_READ | XAW_TENTF_REPLACE;
	}
	else if (quark == Qhide) {
	    XawTextSourceAddEntity(parser->source, 0, XAW_TENTF_HIDE,
				   position, length, quark);
	    clip = True;
	}

	if (mask || entity) {
	    if (head == NULL || head->end != -1) {
		XawTextSourceAddEntity(parser->source, (int)entity, mask,
				       position, length, quark);
		clip = True;
	    }
	    else {
		Html_Item *item, *it;

		item = XtNew(Html_Item);
		item->entity = quark;
		item->start = position;
		item->end = position + length;
		item->info = NULL;
		item->combine = NULL;
		item->override = False;
		item->replace = entity;
		item->mask = mask;
		item->child = item->next = NULL;

		it = head->child;

		item->parent = head;
		if (it == NULL) {
		    head->child = item;
		    item->prev = NULL;
		}
		else {
		    while (it->next)
			it = it->next;
		    it->next = item;
		    item->prev = it;
		}
	    }
	}

	if (clip) {
	    XmuSegment segment;

	    segment.x1 = position;
	    segment.x2 = position + length;
	    (void)XmuScanlineOrSegment(parser->mask, &segment);
	}
    }
}

static void
Html_AArgs(Html_Parser *parser, Html_Item *item)
{
    int ch, sz;
    char buf[32];

    /*CONSTCOND*/
    while (True) {
	sz = 0;
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF) {
	    if (isalnum(ch))
		break;
	    else
		(void)Html_Get(parser);
	}

	if (ch == '>' || ch == EOF)
	    return;
	buf[sz++] = tolower(Html_Get(parser));
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF)
	    if (isalnum(ch))
		buf[sz++] = tolower(Html_Get(parser));
	    else
		break;
	buf[sz] = '\0';
	if (strcmp(buf, "href") == 0) {
	    item->combine = XawTextSinkCopyProperty(XawTextGetSink(textwindow),
						    item->info->ident);
	    item->override = True;
	    item->combine->xlfd_mask = 0L;
	    item->combine->mask = XAW_TPROP_UNDERLINE | XAW_TPROP_FOREGROUND;
	    item->combine->foreground = parser->alink;
	    return;
	}
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF) {
	    if (isspace(ch))
		break;
	    else
		(void)Html_Get(parser);
	}
    }
}

static void
Html_FontArgs(Html_Parser *parser, Html_Item *item)
{
    int ch, sz;
    char name[32], value[256], xlfd[128];

    item->combine = XawTextSinkCopyProperty(XawTextGetSink(textwindow),
					    Qdefault);
    item->override = True;
    item->combine->mask = item->combine->xlfd_mask = 0L;

    /*CONSTCOND*/
    while (True) {
	/* skip white spaces */
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF) {
	    if (isalnum(ch))
		break;
	    else
		(void)Html_Get(parser);
	}

	if (ch == '>' || ch == EOF)
	    return;

	/* read option name */
	sz = 0;
	name[sz++] = tolower(Html_Get(parser));
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF)
	    if (isalnum(ch) && sz + 1 <= sizeof(name))
		name[sz++] = tolower(Html_Get(parser));
	    else
		break;
	name[sz] = '\0';

	if (ch != '=')
	    continue;
	(void)Html_Get(parser);	/* skip `=' */
	if (Html_Peek(parser) == '"')
	    (void)Html_Get(parser);

	sz = 0;
	while ((ch = Html_Peek(parser)) != '>' && ch != EOF) {
	    if (!isspace(ch) && sz + 1 <= sizeof(value))
		value[sz++] = Html_Get(parser);
	    else
		break;
	}
	value[sz] = '\0';
	if (sz > 0 && value[sz - 1] == '"')
	    value[--sz] = '\0';

	if (strcmp(name, "color") == 0) {
	    XColor  color, exact;

	    if (XAllocNamedColor(XtDisplay(topwindow), topwindow->core.colormap,
				 value, &color, &exact)) {
		item->combine->mask |= XAW_TPROP_FOREGROUND;
		item->combine->foreground = color.pixel;
	    }
	}
	else if (strcmp(name, "face") == 0) {
	    int count = 0;
	    char *ptr, *family, **font_list;

	    ptr = value;
	    do {
		family = ptr;
		ptr = strchr(ptr, ',');
		if (ptr)
		    *ptr++ = '\0';
		XmuSnprintf(xlfd, sizeof(xlfd), "-*-%s-*-*-*-*-*-*-*-*-*-*-*-*",
			    family);
		font_list = XListFonts(XtDisplay(topwindow), xlfd, 1, &count);
		if (font_list)
		    XFreeFontNames(font_list);
		if (count)
		    break;
	    } while (+	    if (count) {
		item->combine->xlfd_mask |= XAW_TPROP_FAMILY;
		item->combine->family = XrmStringToQuark(family);
	    }
	}
	else if (strcmp(name, "size") == 0) {
	    int size, sign;

	    if (isalnum(*value)) {
		size = atoi(value);
		sign = 0;
	    }
	    else {
		char *str = XrmQuarkToString(item->combine->pixel_size);

		size = str ? atoi(str) : 12;
		if (*value == '+') {
		    size += atoi(value + 1);
		    sign = 1;
		}
		else if (*value == '-') {
		    size -= atoi(value + 1);
		    sign = -1;
		}
	    }

	    if (item->combine->xlfd != NULLQUARK) {
		int count, ucount, dcount, usize, dsize;
		char **current, **result, **up, **down;

		current = result = up = down = NULL;
		/* try to load an appropriate font */
		XmuSnprintf(value, sizeof(value),
			    "-*-%s-%s-%s-*--%%d-*-*-*-*-*-%s-%s",
			    XrmQuarkToString(item->combine->family),
			    XrmQuarkToString(item->combine->weight),
			    XrmQuarkToString(item->combine->slant),
			    XrmQuarkToString(item->combine->registry),
			    XrmQuarkToString(item->combine->encoding));
		XmuSnprintf(xlfd, sizeof(xlfd), value,
			    atoi(XrmQuarkToString(item->combine->pixel_size)));
		current = XListFonts(XtDisplay(topwindow), xlfd, 1, &count);
		if (count) {
		    ucount = dcount = usize = dsize = 0;

		    XmuSnprintf(xlfd, sizeof(xlfd), value, size);
		    result = XListFonts(XtDisplay(topwindow), xlfd, 1, &count);
		    if (count == 0 || strstr(*result, "-0-")) {
			if (sign <= 0) {
			    sz = dsize = size;
			    while (dcount == 0 && --sz > size - 8 && sz > 1) {
				XmuSnprintf(xlfd, sizeof(xlfd), value, sz);
				down = XListFonts(XtDisplay(topwindow), xlfd,
						  1, &dcount);
				if (dcount && strstr(*down, "-0-") != NULL) {
				    XFreeFontNames(down);
				    down = NULL;
				    dcount = 0;
				}
			    }
			    if (dcount)
				dsize = sz;
			}
			if (sign >= 0) {
			    sz = usize = size;
			    while (ucount == 0 && ++sz < size + 8) {
				XmuSnprintf(xlfd, sizeof(xlfd), value, sz);
				up = XListFonts(XtDisplay(topwindow), xlfd,
						1, &ucount);
				if (ucount && strstr(*up, "-0-") != NULL) {
				    XFreeFontNames(up);
				    up = NULL;
				    ucount = 0;
				}
			    }
			    if (ucount)
				usize = sz;
			}
			if (ucount && dcount)
			    size = size - dsize < usize - size ? dsize : usize;
			else if (ucount)
			    size = usize;
			else if (dcount)
			    size = dsize;
		    }
		    if (current)
			XFreeFontNames(current);
		    if (result)
			XFreeFontNames(result);
		    if (up)
			XFreeFontNames(up);
		    if (down)
			XFreeFontNames(down);
		}
	    }

	    XmuSnprintf(value, sizeof(value), "%d", size);
	    item->combine->xlfd_mask |= XAW_TPROP_PIXELSIZE;
	    item->combine->pixel_size = XrmStringToQuark(value);
	}

	while ((ch = Html_Peek(parser)) != '>' && ch != EOF) {
	    if (isspace(ch))
		break;
	    else
		(void)Html_Get(parser);
	}
    }
}

static void
Html_ParseTag(Html_Parser *parser)
{
    int ch, sz;
    char buf[32];
    Html_TagInfo *info;
    Html_Item *item = NULL;
    XawTextPosition offset = parser->offset - 1;

    switch (Html_Peek(parser)) {
	case '!':
	    (void)Html_Get(parser);		/* eat `!' */
	    if (Html_Peek(parser) == '-') {
	    /* comment */
		(void)Html_Get(parser);		/* eat `-' */
		if (Html_Peek(parser) == '-') {
		    int count = 0;

		    (void)Html_Get(parser);
		    while ((ch = Html_Peek(parser)) != EOF) {
			if (ch == '>' && count >= 2)
			    break;
			else if (ch == '-')
			    ++count;
			else
			    count = 0;
			(void)Html_Get(parser);
		    }
		}
	    }
	    break;
	case '?':
	    break;
	case '/':
	    (void)Html_Get(parser);	/* eat `/' */
	    sz = 0;
	    while (isalnum(Html_Peek(parser)) &&
		   sz <= sizeof(buf) + 1)
		buf[sz++] = tolower(Html_Get(parser));
	    buf[sz] = '\0';
	    if ((info = Html_GetInfo(buf)) != NULL) {
		if (info->ident == Qpre)
		    --parser->pre;
		if (info->enl)
		    parser->quark = Qetag;
		if (parser->head) {
		    Html_Item *it = parser->head;

		    while (it) {
			if (it->info == info)
			    break;
			it = it->parent;
		    }

		    if (it) {
			if (it == parser->head)
			    parser->head->end = offset;
			else {
			    it->end = offset;
			    do {
				if (parser->head->info->ident == Qpre)
				    --parser->pre;
				parser->head->end = offset;
				parser->head = parser->head->parent;
			    } while (parser->head != it);
			}
			if (parser->head->parent)
			    parser->head = parser->head->parent;
			else
			    parser->head = parser->item;
		    }
		}
	    }
	    break;
	default:
	    sz = 0;
	    while (isalnum(Html_Peek(parser)) &&
		   sz <= sizeof(buf) + 1)
		buf[sz++] = tolower(Html_Get(parser));
	    buf[sz] = '\0';
	    if ((info = Html_GetInfo(buf)) != NULL) {
		if (info->end == False) {
		    parser->quark = info->ident;
		    break;	/* no more processing required */
		}
		if (info->snl)
		    parser->quark = Qtag;
		if (info->ident == Qpre)
		    ++parser->pre;
		item = XtNew(Html_Item);
		item->info = info;
		item->entity = item->info->ident;
		item->combine = NULL;
		item->override = False;
		item->replace = NULL;
		item->mask = 0;
		item->start = item->end = -1;
		item->parent = item->child = item->prev = item->next = NULL;
		if (parser->item == NULL)
		    parser->item = parser->head = item;
		else if (parser->head->end == -1) {
		    if (parser->head->info != item->info || parser->head->info->nest) {
		    /* this item is a child */
			Html_Item *it = parser->head->child;

			item->parent = parser->head;
			if (it == NULL)
			    parser->head->child = item;
			else {
			    while (it->next)
				it = it->next;
			    it->next = item;
			    item->prev = it;
			}
			parser->head = item;
		    }
		    else {
		    /* close the `head' item and start a new one */
			Html_Item *it;

			if (parser->head->parent)
			    parser->head = parser->head->parent;
			else
			    parser->head = parser->item;

			it = parser->head->child;
			item->parent = parser->head;
			while (it->next)
			    it = it->next;
			it->next = item;
			item->prev = it;
			parser->head = item;
		    }
		}
		else {
		/* this is not common, but handle it */
		    Html_Item *it = parser->item;

		    while (it->next)
			it = it->next;
		    it->next = item;
		    item->prev = it;
		    parser->head = item;
		}
		if (info->parse_args)
		    (info->parse_args)(parser, item);
	    }
	    break;
    }
    /* skip anything not processed */
    while ((ch = Html_Peek(parser)) != '>' && ch != EOF)
	(void)Html_Get(parser);
    if (item && item->start == -1)
	item->start = parser->offset + 1;
}

/* tags */
static int
Html_Parse3(Html_Parser *parser)
{
    int ch;

    for (;;) {
	if ((ch = Html_Get(parser)) == '<') {
	    parser->end = parser->offset - 1;
	    Html_Commit(parser);
	    parser->quark = Qhide;
	    parser->start = parser->end;

	    Html_ParseTag(parser);

	    (void)Html_Get(parser);	/* eat `>' */
	    parser->end = parser->offset;
	    Html_Commit(parser);
	}
	else
	    return (ch);
    }
    /*NOTREACHED*/
}

/* entities */
static int
Html_Parse2(Html_Parser *parser)
{
    static XawTextBlock *entities[256];
    static char chars[256];
    int ch;

    for (;;) {
	if ((ch = Html_Parse3(parser)) == '&') {
	    unsigned char idx = '?';
	    char buf[32];
	    int sz = 0;

	    parser->end = parser->offset - 1;
	    Html_Commit(parser);
	    parser->start = parser->end;
	    while ((ch = Html_Peek(parser)) != ';'
		   && ch != EOF && !isspace(ch)) {
		if (sz + 1 <= sizeof(buf))
		    buf[sz++] = Html_Get(parser);
	    }
	    buf[sz] = '\0';
	    if (ch == ';')
		(void)Html_Get(parser);
	    if (sz == 0)
		idx = '&';
	    else if (strcasecmp(buf, "lt") == 0)
		idx = '<';
	    else if (strcasecmp(buf, "gt") == 0)
		idx = '>';
	    else if (strcasecmp(buf, "nbsp") == 0)
		idx = ' ';
	    else if (strcasecmp(buf, "amp") == 0)
		idx = '&';
	    else if (strcasecmp(buf, "quot") == 0)
		idx = '"';
	    else if (*buf == '#') {
		if (sz == 1)
		    idx = '#';
		else {
		    char *tmp;

		    idx = strtol(buf + 1, &tmp, 10);
		    if (*tmp)
			idx = '?';
		}
	    }
	    else if (strcmp(buf + 1, "acute") == 0) {
		switch (*buf) {
		    case 'a': idx = 'á'; break;	case 'e': idx = 'é'; break;
		    case 'i': idx = 'í'; break;	case 'o': idx = 'ó'; break;
		    case 'u': idx = 'ú'; break;	case 'A': idx = 'Á'; break;
		    case 'E': idx = 'É'; break;	case 'I': idx = 'Í'; break;
		    case 'O': idx = 'Ó'; break;	case 'U': idx = 'Ú'; break;
		}
	    }
	    else if (strcmp(buf + 1, "grave") == 0) {
		switch (*buf) {
		    case 'a': idx = 'à'; break;	case 'e': idx = 'è'; break;
		    case 'i': idx = 'ì'; break;	case 'o': idx = 'ò'; break;
		    case 'u': idx = 'ù'; break;	case 'A': idx = 'À'; break;
		    case 'E': idx = 'È'; break;	case 'I': idx = 'Ì'; break;
		    case 'O': idx = 'Ò'; break;	case 'U': idx = 'Ù'; break;
		}
	    }
	    else if (strcmp(buf + 1, "tilde") == 0) {
		switch (*buf) {
		    case 'a': idx = 'ã'; break;	case 'o': idx = 'õ'; break;
		    case 'n': idx = 'ñ'; break;	case 'A': idx = 'Ã'; break;
		    case 'O': idx = 'Õ'; break;	case 'N': idx = 'Ñ'; break;
		}
	    }
	    else if (strcmp(buf + 1, "circ") == 0) {
		switch (*buf) {
		    case 'a': idx = 'â'; break;	case 'e': idx = 'ê'; break;
		    case 'i': idx = 'î'; break;	case 'o': idx = 'ô'; break;
		    case 'u': idx = 'û'; break;	case 'A': idx = 'Â'; break;
		    case 'E': idx = 'Ê'; break;	case 'I': idx = 'Î'; break;
		    case 'O': idx = 'Ô'; break;	case 'U': idx = 'Û'; break;
		}
	    }
	    else if (strcmp(buf + 1, "cedil") == 0) {
		switch (*buf) {
		    case 'c': idx = 'ç'; break;	case 'C': idx = 'Ç'; break;
		}
	    }

	    parser->quark = Qentity;
	    if (entities[idx] == NULL) {
		entities[idx] = XtNew(XawTextBlock);
		entities[idx]->firstPos = 0;
		entities[idx]->length = 1;
		entities[idx]->ptr = chars + idx;
		entities[idx]->format = FMT8BIT;
		chars[idx] = idx;
	    }
	    parser->replace = entities[idx];
	    parser->end = parser->offset;
	    Html_Commit(parser);
	    parser->start = parser->end;
	}
	else
	    return (ch);
    }
    /*NOTREACHED*/
}

/* spaces */
static int
Html_Parse1(Html_Parser *parser)
{
    int ch;

    for (;;) {
	if ((ch = Html_Parse2(parser)) == EOF)
	    return (ch);

	if (!parser->pre && isspace(ch)) {
	    if (parser->quark != Qspace) {
		parser->end = parser->offset - 1;
		Html_Commit(parser);
		parser->quark = Qspace;
		parser->start = parser->end;
		parser->space = ch;
	    }
	}
	else {
	    if (parser->quark != Qdefault) {
		parser->end = parser->offset - 1;
		Html_Commit(parser);
		parser->quark = Qdefault;
		parser->start = parser->end;
	    }
	    parser->spc = False;
	    parser->snl = 0;
	}
    }
    /*NOTREACHED*/
}

static void
Html_ModeInit(void)
{
    int i;
    static int initialized;

    if (initialized)
	return;

    Qbr			= XrmPermStringToQuark("br");
    Qdefault		= XrmPermStringToQuark("default");
    Qentity		= XrmPermStringToQuark("entity");
    Qhide		= XrmPermStringToQuark("hide");
    Qp			= XrmPermStringToQuark("p");
    Qpre		= XrmPermStringToQuark("pre");
    Qspace		= XrmPermStringToQuark("space");
    Qetag		= XrmPermStringToQuark("/tag");
    Qtag		= XrmPermStringToQuark("tag");

    for (i = 0; i < sizeof(tag_info) / sizeof(tag_info[0]); i++)
	tag_info[i].ident = XrmPermStringToQuark(tag_info[i].name);

    initialized = True;
}
