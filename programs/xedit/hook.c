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

/*
 * This file is intended to be used to add all the necessary hooks to xedit
 * emulate certain features of emacs (and other text editors) that are better
 * kept only in xedit, to avoid unnecessary code in the Text Widget.
 *
 * The code here is not finished, and will probably be changed frequently.
 */

#include "xedit.h"
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>

/*
 * Types
 */
typedef struct _ReplaceList {
    char *word;
    char *replace;
    struct _ReplaceList *next;
} ReplaceList;

/*
 * Prototypes
 */
static void ActionHook(Widget, XtPointer, String, XEvent*, String*, Cardinal*);
static void AutoReplaceHook(Widget, String, XEvent*);
static Bool StartAutoReplace(void);
static char *ReplacedWord(char*, char*);
static Bool AutoReplace(Widget, XEvent*, Bool);

/*
 * Initialization
 */
extern Widget textwindow;
#define STRTBLSZ	11
static ReplaceList *replace_list[STRTBLSZ];

/*
 * Implementation
 */
Bool
StartHooks(XtAppContext app)
{
    static Bool first_time = True;

    if (first_time) {
	if (StartAutoReplace())
	    (void)XtAppAddActionHook(app, ActionHook, NULL);
	first_time = False;

	return (True);
    }
    return (False);
}

/*ARGSUSED*/
static void
ActionHook(Widget w, XtPointer client_data, String action, XEvent *event,
	   String *params, Cardinal *num_params)
{
    AutoReplaceHook(w, action, event);
}

/*** auto replace ***/
static void
AutoReplaceHook(Widget w, String action, XEvent *event)
{
    static Widget widget;
    static Bool state, multiply;

    if (w != textwindow) {
	state = False;
	widget = w;
	return;
    }

    if (widget != textwindow) {
	widget = textwindow;
	state = False;
    }
    else if (strcmp(action, "multiply") == 0) {
	multiply = True;
	state = False;
	return;
    }
    else if (strcmp(action, "numeric") == 0) {
	if (multiply)
	    return;
    }
    else if (strcmp(action, "insert-char") && strcmp(action, "newline") &&
	strcmp(action, "newline-and-indent")) {
	state = False;
	return;
    }
    multiply = False;

    state = AutoReplace(w, event, state);
}

static Bool
StartAutoReplace(void)
{
    Bool esc;
    int len, llen, rlen, count = 0;
    char ch, *tmp, *left, *right, *replace = app_resources.auto_replace;

    if (!replace || !*replace)
	return (False);

    left = XtMalloc(llen = 256);
    right = XtMalloc(rlen = 256);
    while (*replace) {
	/* skip white spaces */
	while (*replace && isspace(*replace))
	    ++replace;
	if (!*replace)
	    break;

	/* read left */
	tmp = replace;
	while (*replace && !isspace(*replace))
	    ++replace;
	len = replace - tmp;
	if (len >= llen)
	    left = XtRealloc(left, llen = len + 1);
	strncpy(left, tmp, len);
	left[len] = '\0';

	/* skip white spaces */
	while (*replace && isspace(*replace))
	    ++replace;

	/* read right */
	len = 0;
	esc = False;
	while ((ch = *replace) != '\0') {
	    ++replace;
	    if (ch == '\\') {
		if (esc)
		    right[len++] = '\\';
		esc = !esc;
		continue;
	    }
	    else if (ch == '\n' && !esc)
		break;
	    else
		right[len++] = ch;
	    esc = False;
	    if (len + 2 >= rlen)
		right = XtRealloc(right, rlen += 256);
	}
	right[len] = '\0';

	(void)ReplacedWord(left, right);
	++count;
    }
    XtFree(left);
    XtFree(right);

    return (count > 0);
}

static char *
ReplacedWord(char *word, char *replace)
{
    ReplaceList *list;
    int ii = 0;
    char *pp = word;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (list = replace_list[ii]; list; list = list->next)
	if (strcmp(list->word, word) == 0) {
	    if (replace) {
		XtFree(list->replace);
		list->replace = XtNewString(replace);
	    }
	    return (list->replace);
	}

    if (!replace)
	return (NULL);

    list = XtNew(ReplaceList);
    list->word = XtNewString(word);
    list->replace = XtNewString(replace);
    list->next = replace_list[ii];
    replace_list[ii] = list;

    return (list->replace);
}

static Bool
AutoReplace(Widget w, XEvent *event, Bool state)
{
    static XComposeStatus compose = {NULL, 0};
    KeySym keysym;
    XawTextBlock block, tmp;
    XawTextPosition left, right, pos;
    Widget source;
    int i, len, size;
    char *str, buf[32], mb[sizeof(wchar_t)];

    size = XLookupString((XKeyEvent*)event, mb, sizeof(mb), &keysym, &compose);

    if (size != 1 || isalnum(*mb))
	return (True);

    source = XawTextGetSource(w);
    right = XawTextGetInsertionPoint(w);
    left = XawTextSourceScan(source, right, XawstWhiteSpace,
			     XawsdLeft, 1, False);

    if (left < 0 || left == right)
	return (!isspace(*mb));
    else if (!state)
	return (True);

    len = 0;
    str = buf;
    size = sizeof(buf);
    pos = left;
    while (pos < right) {
	pos = XawTextSourceRead(source, pos, &tmp, right - pos);
	for (i = 0; i < tmp.length; i++) {
	    if (tmp.format == FMT8BIT)
		*mb = tmp.ptr[i];
	    else
		wctomb(mb, ((wchar_t*)tmp.ptr)[i]);
	    str[len++] = *mb;
	    if (len + 2 >= size) {
		if (str == buf)
		    str = XtMalloc(size += sizeof(buf));
		else
		    str = XtRealloc(str, size += sizeof(buf));
	    }
	}
    }
    str[len] = '\0';
    if ((block.ptr = ReplacedWord(str, NULL)) != NULL) {
	block.firstPos = 0;
	block.format = FMT8BIT;
	block.length = strlen(block.ptr);
	(void)XawTextReplace(w, left, right, &block);
	XawTextSetInsertionPoint(w, left + block.length);
	state = False;
    }
    if (str != buf)
	XtFree(str);

    return (state);
}
