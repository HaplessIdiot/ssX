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
#include <fcntl.h>
#include <signal.h>

#define RECEIVE		1
#define SEND		2

#define CHECK		0
#define	ADD		1
#define REMOVE		2


/*
 * Types
 */
#define UNDO_DEPTH	16
typedef struct _ispell_undo {
    char *undo_str;
    int undo_count;
    XawTextPosition undo_pos;
    struct _ispell_undo *next, *prev;
} ispell_undo;

struct _ispell {
    Widget shell, form, mispelled, word, replacement, text,
	   suggestions, viewport, list, commands, replace,
	   replaceAll, undo, ignore, ignoreAll, add, suspend, cancel;

    Widget ascii, source;
    XtInputId id;
    int pid, ifd[2], ofd[2];
    XawTextPosition left, right;
    char *item;
    Bool lock;
    int stat;
    char *buf;
    int bufsiz;

    int undo_depth;
    ispell_undo *undo_head, *undo_base;

    char *wchars;
    char *cmd;
    char *skip;
    char *command;
};

typedef struct _ReplaceList {
    char *word;
    char *replace;
    struct _ReplaceList *next;
} ReplaceList;

typedef struct _IgnoreList {
    char *word;
    struct _IgnoreList *next;
} IgnoreList;

typedef struct _AddList {
    char *word;
    struct _AddList *next;
} AddList;

/*
 * Prototypes
 */
static void AddIspell(Widget, XtPointer, XtPointer);
static void IgnoreIspell(Widget, XtPointer, XtPointer);
static Bool InitIspell(void);
static Bool IspellAddedWord(char*, Bool);
static void IspellCheckUndo(void);
static Bool IspellIgnoredWord(char*, int);
static void IspellInputCallback(XtPointer, int*, XtInputId*);
static Bool IspellReceive(void);
static char *IspellReplacedWord(char*, char*);
static Bool IspellSend(void);
static void IspellSetSensitive(Bool);
static void PopdownIspell(Widget, XtPointer, XtPointer);
static void ReplaceIspell(Widget, XtPointer, XtPointer);
static void SelectIspell(Widget, XtPointer, XtPointer);
#ifndef SIGNALRETURNSINT
static void timeout_signal(int);
static void (*old_timeout)(int);
#else
static int timeout_signal(int);
static int (*old_timeout)(int);
#endif
static void UndoIspell(Widget, XtPointer, XtPointer);

Bool _XawTextSrcUndo(TextSrcObject, XawTextPosition*);

/*
 * Initialization
 */
extern Boolean international;

static struct _ispell ispell;

#define STRTBLSZ	23
static ReplaceList *replace_list[STRTBLSZ];
static IgnoreList *ignore_list[STRTBLSZ];
static AddList *add_list[STRTBLSZ];

#define Offset(field) XtOffsetOf(struct _ispell, field)
static XtResource resources[] = {
    {"wordChars", "Chars", XtRString, sizeof(char*),
	Offset(wchars), XtRString, ""},
    {"ispellCommand", "CommandLine", XtRString, sizeof(char*),
	Offset(cmd), XtRString, "/usr/local/bin/ispell"},
    {"skipLines", "Skip", XtRString, sizeof(char*),
	Offset(skip), XtRString, "#"},
};
#undef Offset

/*
 * Implementation
 */
/*ARGSUSED*/
#ifndef SIGNALRETURNSINT
static void
timeout_signal(int unused)
{
    fprintf(stderr, "Warning: Timeout waiting ispell process to die.\n");
    kill(ispell.pid, SIGTERM);
}
#else
static int
timeout_signal(int unused)
{
    fprintf(stderr, "Warning: Timeout waiting ispell process to die.\n");
    kill(ispell.pid, SIGTERM);

    return (0);
}
#endif

static void
IspellSetSensitive(Bool state)
{
    XtSetSensitive(ispell.replace, state);
    XtSetSensitive(ispell.replaceAll, state);
    XtSetSensitive(ispell.ignore, state);
    XtSetSensitive(ispell.ignoreAll, state);
    XtSetSensitive(ispell.add, state);
    XtSetSensitive(ispell.text, state);
    XtSetSensitive(ispell.list, state);
}

static void
IspellCheckUndo(void)
{
    ispell_undo *undo = XtNew(ispell_undo);

    undo->next = NULL;
    if ((undo->prev = ispell.undo_head) != NULL)
	undo->prev->next = undo;
    else
	undo->prev = NULL;
    ++ispell.undo_depth;
    if (!ispell.undo_base) {
	ispell.undo_base = undo;
	XtSetSensitive(ispell.undo, True);
    }
    else if (ispell.undo_depth > UNDO_DEPTH) {
	ispell_undo *tmp;

	if (ispell.undo_base->undo_str)
	    XtFree(ispell.undo_base->undo_str);
	tmp = ispell.undo_base->next;
	XtFree((char*)ispell.undo_base);
	tmp->prev = NULL;
	ispell.undo_base = tmp;
	ispell.undo_depth = UNDO_DEPTH;
    }
    ispell.undo_head = undo;
}

static char *
IspellReplacedWord(char *word, char *replace)
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
IspellIgnoredWord(char *word, int cmd)
{
    IgnoreList *list, *prev;
    int ii = 0;
    char *pp = word;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (prev = list = ignore_list[ii]; list; prev = list, list = list->next)
	if (strcmp(list->word, word) == 0) {
	    if (cmd == REMOVE) {
		XtFree(list->word);
		prev->next = list->next;
		XtFree((char*)list);
		if (prev == list)
		    ignore_list[ii] = NULL;
		return (True);
	    }
	    return (cmd == CHECK);
	}

    if (cmd != ADD)
	return (False);

    list = XtNew(IgnoreList);
    list->word = XtNewString(word);
    list->next = ignore_list[ii];
    ignore_list[ii] = list;

    return (True);
}

static Bool
IspellAddedWord(char *word, int cmd)
{
    AddList *list, *prev;
    int ii = 0;
    char *pp = word;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (prev = list = add_list[ii]; list; prev = list, list = list->next)
	if (strcmp(list->word, word) == 0) {
	    if (cmd == REMOVE) {
		XtFree(list->word);
		prev->next = list->next;
		XtFree((char*)list);
		if (prev == list)
		    add_list[ii] = NULL;
		return (True);
	    }
	    return (cmd == CHECK);
	}

    if (cmd != ADD)
	return (False);

    list = XtNew(AddList);
    list->word = XtNewString(word);
    list->next = add_list[ii];
    add_list[ii] = list;

    return (True);
}

/*ARGSUSED*/
static Bool
IspellReceive(void)
{
    int i, len, buflen, old_len;
    Arg args[2];
    char *str, *end, **list, **old_list;
    char *tmp, word[1024];
    int j;

    if (ispell.lock || ispell.stat != RECEIVE)
	return (False);

    buflen = 0;
    while (1) {		/* read the entire line */
	if (len >= ispell.bufsiz)
	    ispell.buf = XtRealloc(ispell.buf, ispell.bufsiz += BUFSIZ);
	if ((len = read(ispell.ifd[0], &ispell.buf[buflen],
			ispell.bufsiz - buflen - 1)) <= 0)
	    break;
	buflen += len;
    }
    while (buflen > 0 && ispell.buf[buflen - 1] == '\n')
	--buflen;
    if (buflen <= 0)
	return (False);
    ispell.buf[buflen] = '\0';

    switch (ispell.buf[0]) {
	case '&':	/* MISS */
	case '?':	/* GUESS */
	    str = strchr(&ispell.buf[2], ' ');
	    *str = '\0';
	    XtSetArg(args[0], XtNlabel, &ispell.buf[2]);
	    XtSetValues(ispell.word, args, 1);

	    ++str;
	    list = (char **)XtMalloc(sizeof(char*));
	    str = strchr(str, ':') + 1;
	    for (i = 0; ; i++) {
		end = strchr(str, ',');
		if (end)	*end = '\0';
		list = (char**)XtRealloc((char*)list, (i + 1) * sizeof(char*));
		tmp = word;
		for (j = 1; j < sizeof(word) && str[j]; j++) {
		    if (str[j] == '+')
			continue;
		    else if (str[j] == '-') {
			char *p, string[256];
			int k, l;

			for (l = 0, k = j + 1; str[k] != '+' && str[k] != '-'
			     && str[k] && l < sizeof(string) - 1; k++, l++)
			    string[l] = str[k];
			string[l] = '\0';
			*tmp = '\0';
			if ((p = strstr(word, string)) != NULL) {
			    char *sav = p;

			    while ((p = strstr(p + l, string)) != NULL)
				sav = p;
			    p = sav;
			    if (strcmp(p, string) == 0) {
				tmp = p;
				j = k - 1;
			    }
			    else
				*tmp++ = '-';
			}
			else
			    *tmp++ = '-';
		    }
		    else
			*tmp++ = str[j];
		}
		*tmp = '\0';
		list[i] = XtNewString(word);

		if (end)	str = end + 1;
		else		break;
	    }
	    len = i + 1;

	    XtSetArg(args[0], XtNlist, &old_list);
	    XtSetArg(args[1], XtNnumberStrings, &old_len);
	    XtGetValues(ispell.list, args, 2);

	    ispell.item = NULL;
	    if ((str = IspellReplacedWord(&ispell.buf[2], NULL)) != NULL)
		for (i = 0; i < len; i++) {
		    if (strcmp(list[i], str) == 0) {
			ispell.item = list[i];
			break;
		    }
		}
	    else
		ispell.item = list[i = 0];
	    if (!ispell.item) {
		list = (char**)XtRealloc((char*)list, (len + 1) * sizeof(char*));
		ispell.item = list[i] = XtNewString(str);
		++len;
	    }

	    XtSetArg(args[0], XtNlist, list);
	    XtSetArg(args[1], XtNnumberStrings, len);
	    XtSetValues(ispell.list, args, 2);

	    XawListHighlight(ispell.list, i);
	    if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
		while (--old_len)
		    XtFree(old_list[old_len]);
		XtFree((char*)old_list);
	    }

	    XtSetArg(args[0], XtNstring, ispell.item);
	    XtSetValues(ispell.text, args, 1);

	    XawTextSetInsertionPoint(ispell.ascii, ispell.left);
	    XawTextSetSelection(ispell.ascii, ispell.left, ispell.right);

	    ispell.lock = True;
	    break;
	case '#':	/* NONE */
	    str = strchr(&ispell.buf[2], ' ');
	    *str = '\0';
	    XtSetArg(args[0], XtNlabel, &ispell.buf[2]);
	    XtSetValues(ispell.word, args, 1);

	    XtSetArg(args[0], XtNlist, &old_list);
	    XtSetArg(args[1], XtNnumberStrings, &old_len);
	    XtGetValues(ispell.list, args, 2);
	    ispell.item = NULL;

	    list = (char**)XtMalloc(sizeof(char**));
	    if ((str = IspellReplacedWord(&ispell.buf[2], NULL)) == NULL)
		str = &ispell.buf[2];
	    list[0] = XtNewString(str);

	    XtSetArg(args[0], XtNlist, list);
	    XtSetArg(args[1], XtNnumberStrings, 1);
	    XtSetValues(ispell.list, args, 2);

	    if (str == &ispell.buf[2])
		XawListUnhighlight(ispell.list);
	    else
		XawListHighlight(ispell.list, 0);
	    if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
		while (--old_len)
		    XtFree(old_list[old_len]);
		XtFree((char*)old_list);
	    }

	    XtSetArg(args[0], XtNstring, str);
	    XtSetValues(ispell.text, args, 1);

	    XawTextSetInsertionPoint(ispell.ascii, ispell.left);
	    XawTextSetSelection(ispell.ascii, ispell.left, ispell.right);

	    ispell.lock = True;
	    break;
	case '-':	/* COMPOUND */
	case '+':	/* ROOT */
	case '*':	/* OK */
	    ispell.lock = False;
	    break;
	default:
	    fprintf(stderr, "Unknown ispell command '%c'\n", ispell.buf[0]);
	    return (False);
    }

    ispell.stat = SEND;
    if (!ispell.lock)
	IspellSend();

    return (True);
}

/*ARGSUSED*/
static Bool
IspellSend(void)
{
    XawTextPosition position;
    XawTextBlock block;
    int i, len;
    char buf[1024];
    Bool nl;

    if (ispell.lock || ispell.stat != SEND)
	return (False);

    len = 1;
    strcpy(buf, "^");	/* don't evaluate following characters as commands */

    /* skip non word characters */
    position = ispell.right;
    nl = position == 0;
    while (1) {
	Bool done = False;

	position = XawTextSourceRead(ispell.source, position,
				     &block, BUFSIZ);
	if (block.length == 0) {	/* end of file */
	    ispell.stat = 0;
	    XawTextSetInsertionPoint(ispell.ascii, ispell.right);
	    XawTextUnsetSelection(ispell.ascii);
	    IspellSetSensitive(False);
	    return (False);
	}
	if (international) {
	    wchar_t *wptr = (wchar_t*)block.ptr;
	    unsigned char mb[sizeof(wchar_t)];

	    for (i = 0; i < block.length; i++) {
		wctomb(mb, wptr[i]);
		if (isalpha(*mb) ||
		    (*mb && strchr(ispell.wchars, *mb))) {
		    done = True;
		    break;
		}
		else if (*mb == '\n')
		    nl = True;
		else if (nl) {
		    nl = False;
		    if (strchr(ispell.skip, *mb)) {
			position = ispell.right =
			    XawTextSourceScan(ispell.source, ispell.right + i,
					      XawstEOL, XawsdRight, 1, False);
			i = 0;
			break;
		    }
		}
	    }
	}
	else {
	    for (i = 0; i < block.length; i++) {
		if (isalpha(block.ptr[i]) ||
		    (block.ptr[i] && strchr(ispell.wchars, block.ptr[i]))) {
		    done = True;
		    break;
		}
		else if (block.ptr[i] == '\n')
		    nl = True;
		else if (nl) {
		    nl = False;
		    if (strchr(ispell.skip, block.ptr[i])) {
			position = ispell.right =
			    XawTextSourceScan(ispell.source, ispell.right + i,
					      XawstEOL, XawsdRight, 1, False);
			i = 0;
			break;
		    }
		}
	    }
	}
	ispell.right += i;
	if (done)
	    break;
    }

    /* read a word */
    position = ispell.left = ispell.right;
    while (1) {
	Bool done = False;

	position = XawTextSourceRead(ispell.source, position,
				     &block, BUFSIZ);
	if (block.length == 0 && len == 1) {	/* end of file */
	    ispell.stat = 0;
	    XawTextSetInsertionPoint(ispell.ascii, ispell.right);
	    XawTextUnsetSelection(ispell.ascii);
	    IspellSetSensitive(False);
	    return (False);
	}
	if (international) {
	    wchar_t *wptr = (wchar_t*)block.ptr;
	    unsigned char mb[sizeof(wchar_t)];

	    for (i = 0; i < block.length; i++) {
		wctomb(mb, wptr[i]);
		if (!isalpha(*mb) && (!*mb || !strchr(ispell.wchars, *mb))) {
		    done = True;
		    break;
		}
		buf[len] = *mb;
		if (++len >= sizeof(buf) - 1) {
		    done = True;
		    fprintf(stderr, "Warning: word is too large!\n");
		    break;
		}
	    }
	}
	else {
	    for (i = 0; i < block.length; i++) {
		if (!isalpha(block.ptr[i]) &&
		    (!block.ptr[i] || !strchr(ispell.wchars, block.ptr[i]))) {
		    done = True;
		    break;
		}
		buf[len] = block.ptr[i];
		if (++len >= sizeof(buf) - 1) {
		    done = True;
		    fprintf(stderr, "Warning: word is too large!\n");
		    break;
		}
	    }
	}
	ispell.right += i;
	if (done || block.length == 0)
	    break;
    }

    buf[len] = '\0';
    if (IspellIgnoredWord(&buf[1], CHECK) || IspellAddedWord(&buf[1], CHECK))
	return (IspellSend());

    buf[len++] = '\n';

    write(ispell.ofd[1], buf, len);

    ispell.stat = RECEIVE;

    return (True);
}

/*ARGSUSED*/
static void
IspellInputCallback(XtPointer closure, int *source, XtInputId *id)
{
    int len;
    char buf[1024];

    if (ispell.right < 0) {
	ispell.right = XawTextGetInsertionPoint(ispell.ascii);
	ispell.right = XawTextSourceScan(ispell.source, ispell.right,
					      XawstEOL, XawsdLeft, 1, True);
	len = read(ispell.ifd[0], buf, sizeof(buf));
	if (strncmp(buf, "@(#)", 4) == 0) {
	    Arg args[1];

	    buf[len - 1] = '\0';
	    XtSetArg(args[0], XtNtitle, &buf[5]);
	    XtSetValues(ispell.shell, args, 1);
	}
	else
	    fprintf(stderr, "Error: is ispell talking with me?\n");
	IspellSend();
    }

    if (ispell.source)
	IspellReceive();
}

/*ARGSUSED*/
void
IspellCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Cardinal zero = 0;

    IspellAction(textwindow, NULL, NULL, &zero);
}

/*ARGSUSED*/
void
IspellAction(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Arg args[3];
    Cardinal num_args;
    Dimension width, height, b_width;
    Position x, y, max_x, max_y;
    char **strs;
    int n_strs;

    if (!XtIsSubclass(w, textWidgetClass)) {
	Feep();
	return;
    }

    InitIspell();

    if (*num_params == 1 && (params[0][0] == 'e' || params[0][0] == 'E')) {
	PopdownIspell(w, NULL, NULL);
	return;
    }
    if (ispell.source) {
	Feep();
	return;
    }

    ispell.source = XawTextGetSource(ispell.ascii = w);

    x = y = -1;
    if (event) {
	switch (event->type) {
	    case ButtonPress:
	    case ButtonRelease:
		x = event->xbutton.x_root;
		y = event->xbutton.y_root;
		break;
	    case KeyPress:
	    case KeyRelease:
		x = event->xkey.x_root;
		y = event->xkey.y_root;
		break;
	}
    }
    if (x < 0 || y < 0) {
	Window r, c;
	int rx, ry, wx, wy;
	unsigned mask;

	XQueryPointer(XtDisplay(ispell.shell), XtWindow(ispell.shell),
		      &r, &c, &rx, &ry, &wx, &wy, &mask);
	x = rx;
	y = ry;
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, &width);		num_args++;
    XtSetArg(args[num_args], XtNheight, &height);	num_args++;
    XtSetArg(args[num_args], XtNborderWidth, &b_width);	num_args++;
    XtGetValues(ispell.shell, args, num_args);

    width += b_width << 1;
    height += b_width << 1;

    x -= (Position)(width >> 1);
    if (x < 0)
	x = 0;
    if (x > (max_x = (Position)(XtScreen(w)->width - width)))
	x = max_x;

    y -= (Position)(height >> 1);
    if (y < 0)
	y = 0;
    if (y > (max_y = (Position)(XtScreen(w)->height - height)))
	y = max_y;

    num_args = 0;
    XtSetArg(args[num_args], XtNx, x);	num_args++;
    XtSetArg(args[num_args], XtNy, y);	num_args++;
    XtSetValues(ispell.shell, args, num_args);

    ispell.lock = False;
    ispell.stat = SEND;

    IspellSetSensitive(True);
    XtSetSensitive(ispell.undo, False);
    ispell.undo_depth = 0;

    if (!ispell.pid) {
	pipe(ispell.ifd);
	pipe(ispell.ofd);
	if ((ispell.pid = fork()) == 0) {
	    close(0);
	    close(1);
	    dup2(ispell.ofd[0], 0);
	    dup2(ispell.ifd[1], 1);
	    close(ispell.ofd[0]);
	    close(ispell.ofd[1]);
	    close(ispell.ifd[0]);
	    close(ispell.ifd[1]);
	    execl("/bin/sh", "sh", "-c", ispell.command, NULL);
	    exit(-127);
	}
	else if (ispell.pid < 0) {
	    fprintf(stderr, "Cannot fork\n");
	    exit(1);
	}
	ispell.buf = XtMalloc(ispell.bufsiz = BUFSIZ);
	ispell.right = -1;
	ispell.id = XtAppAddInput(XtWidgetToApplicationContext(w), ispell.ifd[0],
				  (XtPointer)XtInputReadMask, IspellInputCallback,
				  NULL);
	fcntl(ispell.ifd[0], F_SETFL, O_NONBLOCK);
    }
    else {
	ispell.right = XawTextGetInsertionPoint(ispell.ascii);
	ispell.right = XawTextSourceScan(ispell.source, ispell.right,
					      XawstEOL, XawsdLeft, 1, True);
	IspellSend();
    }

    XtSetArg(args[0], XtNlabel, "");
    XtSetValues(ispell.word, args, 1);

    XtSetArg(args[0], XtNstring, "");
    XtSetValues(ispell.text, args, 1);

    XtSetArg(args[0], XtNlist, &strs);
    XtSetArg(args[1], XtNnumberStrings, &n_strs);
    XtGetValues(ispell.list, args, 2);

    XtSetArg(args[0], XtNlist, NULL);
    XtSetArg(args[1], XtNnumberStrings, 0);
    XtSetValues(ispell.list, args, 2);

    if (n_strs > 1 || (XtName(ispell.list) != strs[0])) {
	while (--n_strs)
	    XtFree(strs[n_strs]);
	XtFree((char*)strs);
    }

    XtPopup(ispell.shell, XtGrabExclusive);
    XtSetKeyboardFocus(ispell.shell, ispell.text);
}

/*ARGSUSED*/
static void
PopdownIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    ispell.source = NULL;

    if (ispell.pid) {
	ispell_undo *undo, *pundo;
	AddList *al, *pal;
	int i;

	if (client_data) {
	    ReplaceList *rl, *prl;
	    IgnoreList *il, *pil;

	    XtRemoveInput(ispell.id);

	    close(ispell.ofd[0]);
	    close(ispell.ofd[1]);
	    close(ispell.ifd[0]);
	    close(ispell.ifd[1]);

	    /* if something goes wrong, we don't want to block here forever */
	    old_timeout = signal(SIGALRM, timeout_signal);
	    alarm(10);
	    waitpid(ispell.pid, NULL, 0);
	    alarm(0);
	    signal(SIGALRM, old_timeout);

	    ispell.pid = 0;
	    if (ispell.buf)
		XtFree(ispell.buf);
	    ispell.buf = NULL;

	    for (i = 0; i < STRTBLSZ; i++) {
		prl = rl = replace_list[i];
		while (prl) {
		    rl = rl->next;
		    XtFree(prl->word);
		    XtFree(prl->replace);
		    XtFree((char*)prl);
		    prl = rl;
		}
	        replace_list[i] = NULL;
	    }
	    for (i = 0; i < STRTBLSZ; i++) {
		pil = il = ignore_list[i];
		while (pil) {
		    il = il->next;
		    XtFree(pil->word);
		    XtFree((char*)pil);
		    pil = il;
		}
	        ignore_list[i] = NULL;
	    }
	}

	/* insert added words in private dictionary */
	for (i = 0; i < STRTBLSZ; i++) {
	    pal = al = add_list[i];
	    while (pal) {
		al = al->next;
		write(ispell.ofd[1], "*", 1);
		write(ispell.ofd[1], pal->word, strlen(pal->word));
		write(ispell.ofd[1], "\n", 1);
		XtFree(pal->word);
		XtFree((char*)pal);
		pal = al;
	    }
	    add_list[i] = NULL;
	}
	write(ispell.ofd[1], "#\n", 2);		/* save dictionary */

	undo = pundo = ispell.undo_base;
	while (undo) {
	    undo = undo->next;
	    if (pundo->undo_str)
		XtFree(pundo->undo_str);
	    XtFree((char*)pundo);
	    pundo = undo;
	}
	ispell.undo_base = ispell.undo_head = NULL;
    }
    XtPopdown(ispell.shell);
}

/*ARGSUSED*/
static void
SelectIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    XawListReturnStruct *info = (XawListReturnStruct *)call_data;
    Arg args[1];

    XtSetArg(args[0], XtNstring, ispell.item = info->string);
    XtSetValues(ispell.text, args, 1);
}

/*ARGSUSED*/
void
ReplaceIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    XawTextPosition pos = XawTextGetInsertionPoint(ispell.ascii);
    XawTextBlock check, search, replace;
    Arg args[1];
    char *text;

    if (!ispell.lock)
	return;

    XtSetArg(args[0], XtNlabel, &text);
    XtGetValues(ispell.word, args, 1);
    search.ptr = text;
    search.format = XawFmt8Bit;
    search.firstPos = 0;
    search.length = ispell.right - pos;

    XtSetArg(args[0], XtNstring, &text);
    XtGetValues(ispell.text, args, 1);
    replace.ptr = text;
    replace.format = XawFmt8Bit;
    replace.firstPos = 0;
    replace.length = strlen(text);

    IspellCheckUndo();
    ispell.undo_head->undo_str = NULL;
    ispell.undo_head->undo_pos = pos;
    ispell.undo_head->undo_count = 0;
    if (strcmp(search.ptr, replace.ptr) != 0) {
	XawTextReplace(ispell.ascii, pos, pos + search.length, &replace);
	ispell.right += replace.length - search.length;
	ispell.undo_head->undo_count = 1;

	if (client_data) {
	    XawTextDisableRedisplay(ispell.ascii);
	    pos = ispell.right;
	    while ((pos = XawTextSourceSearch(ispell.source, pos, XawsdRight, &search))
		!= XawTextSearchError) {
		Bool do_replace = True;

		if (XawTextSourceRead(ispell.source, pos - 1, &check, 1) > 0)
		    do_replace = !isalpha(*check.ptr) && !strchr(ispell.wchars, *check.ptr);
		if (do_replace &&
		    XawTextSourceRead(ispell.source, pos + search.length, &check, 1) > 0)
		    do_replace = !isalpha(*check.ptr) && !strchr(ispell.wchars, *check.ptr);
		if (do_replace) {
		    XawTextReplace(ispell.ascii, pos, pos + search.length, &replace);
		    ++ispell.undo_head->undo_count;
		}
		pos += search.length;
	    }
	    XawTextEnableRedisplay(ispell.ascii);
	}
	else
	    (void)IspellReplacedWord(search.ptr, replace.ptr);
    }

    if (!ispell.item || strcmp(ispell.item, replace.ptr))
	ispell.right = ispell.left;	/* check it again! */

    ispell.lock = False;
    ispell.stat = SEND;
    IspellSend();
}

/*ARGSUSED*/
void
IgnoreIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg args[1];
    char *text;

    if (!ispell.lock)
	return;

    XtSetArg(args[0], XtNlabel, &text);
    XtGetValues(ispell.word, args, 1);

    IspellCheckUndo();
    ispell.undo_head->undo_pos = XawTextGetInsertionPoint(ispell.ascii);
    ispell.undo_head->undo_count = 0;

    if (client_data) {
	IspellIgnoredWord(text, ADD);
	ispell.undo_head->undo_str = XtNewString(text);
    }
    else
	ispell.undo_head->undo_str = NULL;

    ispell.undo_head->undo_pos = XawTextGetInsertionPoint(ispell.ascii);

    ispell.lock = False;
    ispell.stat = SEND;
    IspellSend();
}

/*ARGSUSED*/
void
AddIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg args[1];
    char *text;

    if (!ispell.lock)
	return;

    XtSetArg(args[0], XtNlabel, &text);
    XtGetValues(ispell.word, args, 1);

    IspellCheckUndo();
    ispell.undo_head->undo_str = XtNewString(text);
    ispell.undo_head->undo_pos = XawTextGetInsertionPoint(ispell.ascii);
    ispell.undo_head->undo_count = -1;

    (void)IspellAddedWord(text, ADD);

    ispell.lock = False;
    ispell.stat = SEND;
    IspellSend();
}

/*ARGSUSED*/
static void
UndoIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Bool enable_redisplay = False;
    ispell_undo *undo = ispell.undo_head;

    if ((!ispell.lock && ispell.stat) || !undo)
	return;

    if (undo->undo_count > 0) {
	XawTextPosition tmp;

	enable_redisplay = undo->undo_count > 1;
	if (enable_redisplay)
	    XawTextDisableRedisplay(ispell.ascii);
	while (undo->undo_count--)
	    if (!_XawTextSrcUndo((TextSrcObject)ispell.source, &tmp)) {
		Feep();
		break;
	    }
    }
    else if (undo->undo_count < 0) {
	if (undo->undo_str)
	    (void)IspellAddedWord(undo->undo_str, REMOVE);
    }
    else if (undo->undo_str)
	IspellIgnoredWord(undo->undo_str, REMOVE);
    XawTextSetInsertionPoint(ispell.ascii,
			     ispell.right = ispell.left = undo->undo_pos);
    if (enable_redisplay)
	XawTextEnableRedisplay(ispell.ascii);

    if (undo->prev)
	undo->prev->next = NULL;
    ispell.undo_head = undo->prev;
    if (undo == ispell.undo_base) {
	ispell.undo_base = NULL;
	XtSetSensitive(ispell.undo, False);
    }
    if (undo->undo_str)
	XtFree(undo->undo_str);
    XtFree((char*)undo);
    --ispell.undo_depth;

    if (!ispell.stat)
	IspellSetSensitive(True);

    ispell.lock = False;
    ispell.stat = SEND;
    IspellSend();
}

static Bool
InitIspell(void)
{
    int len;
    Atom delete_window;

    if (ispell.shell)
	return (False);

    ispell.shell	= XtCreatePopupShell("ispell", transientShellWidgetClass,
					     topwindow, NULL, 0);
    ispell.form		= XtCreateManagedWidget("form", formWidgetClass,
						ispell.shell, NULL, 0);
    ispell.mispelled	= XtCreateManagedWidget("mispelled", labelWidgetClass,
						ispell.form, NULL, 0);
    ispell.word		= XtCreateManagedWidget("word", labelWidgetClass,
						ispell.form, NULL, 0);
    ispell.replacement	= XtCreateManagedWidget("replacement", labelWidgetClass,
						ispell.form, NULL, 0);
    ispell.text		= XtVaCreateManagedWidget("text", asciiTextWidgetClass,
						ispell.form,
						XtNeditType, XawtextEdit,
						NULL, 0);
    ispell.suggestions	= XtCreateManagedWidget("suggestions", labelWidgetClass,
						ispell.form, NULL, 0);
    ispell.viewport	= XtCreateManagedWidget("viewport", viewportWidgetClass,
						ispell.form, NULL, 0);
    ispell.list		= XtCreateManagedWidget("list", listWidgetClass,
						ispell.viewport, NULL, 0);
    XtAddCallback(ispell.list, XtNcallback, SelectIspell, NULL);
    ispell.commands	= XtCreateManagedWidget("commands", formWidgetClass,
						ispell.form, NULL, 0);
    ispell.undo		= XtCreateManagedWidget("undo", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.undo, XtNcallback, UndoIspell, NULL);
    ispell.replace	= XtCreateManagedWidget("replace", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.replace, XtNcallback, ReplaceIspell, (XtPointer)False);
    ispell.replaceAll	= XtCreateManagedWidget("replaceAll", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.replaceAll, XtNcallback, ReplaceIspell, (XtPointer)True);
    ispell.ignore	= XtCreateManagedWidget("ignore", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.ignore, XtNcallback, IgnoreIspell, (XtPointer)False);
    ispell.ignoreAll	= XtCreateManagedWidget("ignoreAll", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.ignoreAll, XtNcallback, IgnoreIspell, (XtPointer)True);
    ispell.add		= XtCreateManagedWidget("add", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.add, XtNcallback, AddIspell, NULL);
    ispell.suspend	= XtCreateManagedWidget("suspend", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.suspend, XtNcallback, PopdownIspell, (XtPointer)False);
    ispell.cancel	= XtCreateManagedWidget("cancel", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.cancel, XtNcallback, PopdownIspell, (XtPointer)True);

    XtRealizeWidget(ispell.shell);
    delete_window = XInternAtom(XtDisplay(ispell.shell), "WM_DELETE_WINDOW", False);
    XSetWMProtocols(XtDisplay(ispell.shell), XtWindow(ispell.shell), &delete_window, 1);

    XtGetApplicationResources(ispell.shell, (XtPointer)&ispell, resources,
			      XtNumber(resources), NULL, 0);

    len = strlen(ispell.cmd) + strlen(ispell.wchars) + 12;
    ispell.command = XtMalloc(len);
    XmuSnprintf(ispell.command, len, "%s -a -w '%s'", ispell.cmd, ispell.wchars);

    return (True);
}
