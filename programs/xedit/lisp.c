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

/* $XFree86: xc/programs/xedit/lisp.c,v 1.10tsi Exp $ */

#include "xedit.h"
#include "lisp/lisp.h"
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

/*
 * Prototypes
 */
static void XeditLispInitialize(void);
static void XeditRunLisp(void);
static void XeditLispTextReplace(char*, int);
static void LispInputCallback(XtPointer, int*, XtInputId*);
static void LispErrorInputCallback(XtPointer, int*, XtInputId*);
static void XeditDoLispEval(Widget, XEvent*, String*, Cardinal*);
static void XeditCheckLispChild(void);

#ifndef SIGNALRETURNSINT
static void child_signal(int);
static void (*old_child)(int);
#else
static int child_signal(int);
static int (*old_child)(int);
#endif

/*
 * Initialization
 */
static struct {
    int pid;
    int ifd[2];
    int ofd[2];
    int efd[2];
    XtInputId id;
    XtInputId eid;
    XtAppContext appcon;
    Widget output;
    Bool sigchld;

    char buffer[8192];		/* input read and not yet processed */
    char *input;		/* for incomplete xedit-proto reads */
} lisp;

extern XtAppContext appcon;
extern Widget scratch;

/*
 * Implementation
 */
/*ARGSUSED*/
#ifndef SIGNALRETURNSINT
static void
child_signal(int unused)
{
    lisp.sigchld = True;
}
#else
static int
child_signal(int unused)
{
    lisp.sigchld = True;

    return (0);
}
#endif

void
XeditLispEval(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    lisp.output = messwidget;
    XeditDoLispEval(w, event, params, num_params);
}

void
XeditPrintLispEval(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (XawTextGetSource(textwindow) == scratch) {
	lisp.output = textwindow;
	XtCallActionProc(w, "newline", event, params, *num_params);
	XeditDoLispEval(w, event, params, num_params);
    }
    else
	XtCallActionProc(w, "newline-and-indent", event, params, *num_params);
}

void
XeditKeyboardReset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (lisp.pid) {
	XeditCheckLispChild();
	kill(lisp.pid, SIGINT);
    }
    XtCallActionProc(w, "keyboard-reset", event, params, *num_params);
}

static void
XeditDoLispEval(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Widget src;
    XawTextBlock block;
    XawTextPosition position, end;
    int gotchars = 0;

    XeditLispInitialize();

    if (XtClass(w) != asciiTextWidgetClass) {
	Feep();
	return;
    }

    /* get lisp expression */
    if ((position = XawTextGetInsertionPoint(w)) == 0) {
	Feep();
	return;
    }
    end = position;
    --position;

    src = XawTextGetSource(w);
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

    while (position < end) {
	(void)XawTextSourceRead(src, position, &block, end - position);
	write(lisp.ofd[1], block.ptr, block.length);
	position += block.length;
    }
    write(lisp.ofd[1], "\n", 1);
}

void
XeditLispCleanUp(void)
{
    if (lisp.pid) {
	kill(lisp.pid, SIGTERM);
	lisp.pid = 0;
    }
}

static void
XeditLispInitialize(void)
{
    XeditCheckLispChild();

    if (lisp.pid)
	return;

    XeditPrintf("Notice: starting lisp process.\n");

    lisp.input = lisp.buffer;

    lisp.sigchld = False;
    pipe(lisp.ifd);
    pipe(lisp.ofd);
    pipe(lisp.efd);

    if ((lisp.pid = fork()) == 0) {
	setlocale(LC_NUMERIC, "C");
	close(0);
	close(1);
	close(2);
	dup2(lisp.ofd[0], 0);
	dup2(lisp.ifd[1], 1);
	dup2(lisp.efd[1], 2);
	close(lisp.ifd[0]);
	close(lisp.ifd[1]);
	close(lisp.ofd[0]);
	close(lisp.ofd[1]);
	close(lisp.efd[0]);
	close(lisp.efd[1]);
	XeditRunLisp();
	exit(0);
    }
    else if (lisp.pid < 0) {
	fprintf(stderr, "Cannot fork\n");
	exit(1);
    }
    close(lisp.ifd[1]);
    close(lisp.ofd[0]);
    close(lisp.efd[1]);

    old_child = signal(SIGCHLD, child_signal);
    lisp.appcon = XtWidgetToApplicationContext(topwindow);
    lisp.id = XtAppAddInput(lisp.appcon, lisp.ifd[0],
			    (XtPointer)XtInputReadMask,
			    LispInputCallback, NULL);
    fcntl(lisp.ifd[0], F_SETFL, O_NONBLOCK);
    lisp.eid = XtAppAddInput(lisp.appcon, lisp.efd[0],
			     (XtPointer)XtInputReadMask,
			     LispErrorInputCallback, NULL);
    fcntl(lisp.efd[0], F_SETFL, O_NONBLOCK);
}

static void
XeditLispTextReplace(char *string, int length)
{
    if (XawTextGetSource(lisp.output) == scratch) {
	XawTextBlock block;
	XawTextPosition position = XawTextGetInsertionPoint(lisp.output);

	block.firstPos = 0;
	block.format = FMT8BIT;
	block.length = length;
	block.ptr = string;
	XawTextReplace(lisp.output, position, position, &block);
	position += length;
	XawTextSetInsertionPoint(lisp.output, position);
    }
}

static void
LispInputCallback(XtPointer closure, int *source, XtInputId *id)
{
    char *ptr, *end;
    char *start;	/* start of text or xedit-proto call */
    int length;
    int proto;		/* parsing a xedit-proto call */ 
    int quote;		/* reading a string */

    /* read data */
    length = sizeof(lisp.buffer) - (lisp.input - lisp.buffer);
    length = read(lisp.ifd[0], lisp.input, length);

    /* if nothing new, just return */
    if (length <= 0)
	return;

    /* initialize */
    proto = quote = 0;

    /* scan data read */
    for (ptr = start = lisp.buffer, end = lisp.input + length; ptr < end; ptr++) {
	if (proto) {
	    /* check for string quote */
	    if (*ptr == '"' && ptr > start && ptr[-1] != '\\')
		quote = !quote;

	    /* newlines can be embedded in strings */
	    else if (*ptr == '\n' && !quote) {
		/* not in a string, this is a xedit-proto call */

		char *result = NULL;

		/* finish string */
		*ptr = '\0';

		if (XeditProto(start + 1, &result) == False) {
		    /* error parsing xedit-proto call */

		    if (result)
			XeditPrintf(result);
		    write(lisp.ofd[1], "NIL", 3);
		}
		else
		    write(lisp.ofd[1], result, strlen(result));

		/* write any blank character to mark end of input */
		write(lisp.ofd[1], "\n", 1);

		/* not parsing a xedit-proto anymore */
		proto = 0;

		/* pointer to new data */
		start = ptr + 1;
	    }
	}
	else if (*ptr == PROTOPREFFIX) {

	    /* remember to be parsing a xedit-proto */
	    proto = 1;

	    if (ptr > start)
		/* this data goes directly to the xedit buffer */

		XeditLispTextReplace(start, ptr - start);

	    /* start points to the beginning of the xedit-proto command */
	    start = ptr;
	}
    }

    if (proto) {
	if (start == lisp.buffer) {
	    if (end == lisp.buffer + sizeof(lisp.buffer)) {
		/* xedit-proto command too large */
		XeditPrintf("Error: xedit-proto command too large.\n");
		/* buffer could be increased, but always there must be a limit */
		XeditLispTextReplace(start, ptr - start);
		lisp.input = lisp.buffer;
	    }
	    else
		/* just did not read the entire text */
		lisp.input = end;
	}
	else {
	    /* needs more space */
	    length = sizeof(lisp.buffer) - (start - lisp.buffer);
	    memmove(lisp.buffer, start, length);
	    lisp.input = lisp.buffer + length;
	}
    }
    else {
	if (ptr > start)
	    /* flush output */
	    XeditLispTextReplace(start, ptr - start);
	lisp.input = lisp.buffer;
    }
}

static void
LispErrorInputCallback(XtPointer closure, int *source, XtInputId *id)
{
    int len;
    char str[1024];

    len = read(lisp.efd[0], str, sizeof(str) - 1);
    if (len > 0) {
	str[len] = '\0';
	XeditPrintf(str);
    }
}

static void
XeditCheckLispChild(void)
{
    int status;

    if (lisp.pid) {
	waitpid(lisp.pid, &status, WNOHANG);
	if (WIFEXITED(status) || errno == ECHILD) {
	    lisp.pid = 0;
	    XtRemoveInput(lisp.id);
	    XtRemoveInput(lisp.eid);
	    close(lisp.ifd[0]);
	    close(lisp.ofd[1]);
	    close(lisp.efd[0]);
	    signal(SIGCHLD, old_child);
	}
    }
    lisp.sigchld = False;
}

static void
XeditRunLisp(void)
{
    LispMac *mac = LispBegin();

    LispSetPrompt(mac, NULL);
    LispExecute(mac, "(require \"xedit\")");
    LispMachine(mac);

    LispEnd(mac);
}
