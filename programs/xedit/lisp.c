/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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

/* $XFree86: xc/programs/xedit/lisp.c,v 1.14 2002/11/04 04:15:51 paulo Exp $ */

#include "xedit.h"
#include "lisp/lisp.h"
#include "lisp/xedit.h"
#include <unistd.h>
#include <locale.h>
#include <ctype.h>

/*
 * Prototypes
 */
static void XeditDoLispEval(Widget);

/*
 * Initialization
 */
static int lisp_initialized;
extern Widget scratch;

/*
 * Implementation
 */
void
XeditLispInitialize(void)
{
    setlocale(LC_NUMERIC, "C");
    lisp_initialized = 1;
    LispBegin();
    LispXeditInitialize();
}

void
XeditLispCleanUp(void)
{
    LispEnd();
}

void
XeditLispEval(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    XeditDoLispEval(messwidget);
}

void
XeditPrintLispEval(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (XawTextGetSource(w) == scratch) {
	XtCallActionProc(w, "newline", event, params, *num_params);
	XeditDoLispEval(w);
    }
    else
	XtCallActionProc(w, "newline-and-indent", event, params, *num_params);
}

void
XeditKeyboardReset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    /* XXX Need a different way, the interpreter runs in the same process */
    XtCallActionProc(w, "keyboard-reset", event, params, *num_params);
}

void
SetTextProperties(xedit_flist_item *item)
{
    if (lisp_initialized) {
	Widget source = XawTextGetSource(textwindow);
	XawTextPosition top = XawTextTopPosition(textwindow);

	if (source != item->source)
	    XawTextSetSource(textwindow, item->source, 0);
	XeditLispSetEditMode(item);
	if (source != item->source)
	    XawTextSetSource(textwindow, source, top);
    }
}

void
UnsetTextProperties(xedit_flist_item *item)
{
    XeditLispUnsetEditMode(item);
}

static void
XeditDoLispEval(Widget output)
{
    Widget src;
    XawTextBlock block;
    XawTextPosition position, end;
    int gotchars = 0;

    /* get lisp expression */
    if ((position = XawTextGetInsertionPoint(textwindow)) == 0) {
	Feep();
	return;
    }
    end = position;
    --position;

    src = XawTextGetSource(textwindow);
    while (position >= 0) {
	(void)XawTextSourceRead(src, position, &block, 1);
	if (!isspace(block.ptr[0])) {
	    ++gotchars;
	    break;
	}
	--gotchars;
	--position;
    }

    if (block.ptr[0] != ')') {
	while (position >= 0) {
	    (void)XawTextSourceRead(src, position, &block, 1);
	    if (isspace(block.ptr[0]) ||
		block.ptr[0] == '(' ||
		block.ptr[0] == ')' ||
		block.ptr[0] == '\'' ||
		block.ptr[0] == '`')
		break;
	    ++gotchars;
	    --position;
	}
	++position;
	if (position == end || gotchars <= 0) {
	    Feep();
	    return;
	}
	if ((block.ptr[0] == '\'' || block.ptr[0] == '`') && position > 0)
	    --position;
    }
    else {
	/* XXX note that embedded '(' and ')' will confuse this code */
	XawTextPosition last, tmp;
	int level = 0;
	char ptr[2];

	last = position;
	ptr[1] = '\0';
	block.ptr = ptr;
	do {
	    block.ptr[0] = '(';
	    position = XawTextSourceSearch(src, last, XawsdLeft, &block);
	    if (position == XawTextSearchError) {
		Feep();
		return;
	    }
	    block.ptr[0] = ')';
	    tmp = position;
	    do {
		tmp = XawTextSourceSearch(src, tmp, XawsdRight, &block);
		if (tmp == XawTextSearchError) {
		    Feep();
		    return;
		}
		if (tmp <= last)
		    ++level;
	    } while (++tmp <= last);
	    --level;
	    last = position;
	} while (level);
	/* check for quoted expression */
	if (position) {
	    (void)XawTextSourceRead(src, position - 1, &block, 1);
	    if (block.ptr[0] == '\'' || block.ptr[0] == '`')
		--position;
	}
    }

    XeditLispExecute(output, position, end);
}
