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

/* $XFree86: xc/programs/xedit/ispell.c,v 1.6 1999/05/23 06:33:53 dawes Exp $ */

#include "xedit.h"
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

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
    Bool repeat;	/* two misspelled words together? */
    struct _ispell_undo *next, *prev;
} ispell_undo;

struct _ispell {
    Widget shell, form, mispelled, repeated, word, replacement, text,
	   suggestions, viewport, list, commands, replace, status,
	   replaceAll, undo, ignore, ignoreAll, add, suspend, cancel, check;

    Widget ascii, source;
    XtInputId id;
    int pid, ifd[2], ofd[2];
    XawTextPosition left, right;
    char *item;
    Bool lock;
    Bool repeat;
    Bool checkit;
    int stat;
    char *buf;
    int bufsiz;
    char sendbuf[1024];
    char sentbuf[1024];

    int undo_depth;
    ispell_undo *undo_head, *undo_base;

    char *wchars;
    char *cmd;
    char *skip;
    char *command;
    Boolean terse_mode;
    char *guess_label, *miss_label, *root_label, *none_label, *eof_label,
	 *compound_label, *ok_label, *repeat_label, *working_label;
};

typedef struct _ReplaceList {
    char *word;
    char *replace;
    struct _ReplaceList *next;
} ReplaceList;

typedef struct _IgnoreList {
    char *word;
    Bool add;
    struct _IgnoreList *next;
} IgnoreList;

/*
 * Prototypes
 */
static void AddIspell(Widget, XtPointer, XtPointer);
static void CheckIspell(Widget, XtPointer, XtPointer);
static void IgnoreIspell(Widget, XtPointer, XtPointer);
static Bool InitIspell(void);
static void IspellCheckUndo(void);
static Bool IspellIgnoredWord(char*, int, Bool);
static void IspellInputCallback(XtPointer, int*, XtInputId*);
static Bool IspellReceive(void);
static char *IspellReplacedWord(char*, char*);
static int IspellSend(void);
static void IspellSetRepeated(Bool);
static void IspellSetSensitive(Bool);
static void IspellSetStatus(char*);
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
static struct _ispell ispell;

#define RSTRTBLSZ	23
#define ISTRTBLSZ	71
static ReplaceList *replace_list[RSTRTBLSZ];
static IgnoreList *ignore_list[ISTRTBLSZ];

#ifndef XtCStatus
#define XtCStatus	"Status"
#endif

#define Offset(field) XtOffsetOf(struct _ispell, field)
static XtResource resources[] = {
    {"wordChars", "Chars", XtRString, sizeof(char*),
	Offset(wchars), XtRString, ""},
    {"ispellCommand", "CommandLine", XtRString, sizeof(char*),
	Offset(cmd), XtRString, "/usr/local/bin/ispell"},
    {"skipLines", "Skip", XtRString, sizeof(char*),
	Offset(skip), XtRString, "#"},
    {"terseMode", "Terse", XtRBoolean, sizeof(Boolean),
	Offset(terse_mode), XtRImmediate, (XtPointer)False},
    {"guessLabel", XtCStatus, XtRString, sizeof(String),
	Offset(guess_label), XtRString, "Guess"},
    {"missLabel", XtCStatus, XtRString, sizeof(String),
	Offset(miss_label), XtRString, "Miss"},
    {"rootLabel", XtCStatus, XtRString, sizeof(String),
	Offset(root_label), XtRString, "Root:"},
    {"noneLabel", XtCStatus, XtRString, sizeof(String),
	Offset(none_label), XtRString, "None"},
    {"compoundLabel", XtCStatus, XtRString, sizeof(String),
	Offset(compound_label), XtRString, "Compound"},
    {"okLabel", XtCStatus, XtRString, sizeof(String),
	Offset(ok_label), XtRString, "Ok"},
    {"eofLabel", XtCStatus, XtRString, sizeof(String),
	Offset(eof_label), XtRString, "End Of File"},
    {"repeatLabel", XtCStatus, XtRString, sizeof(String),
	Offset(repeat_label), XtRString, "Repeat"},
    {"workingLabel", XtCStatus, XtRString, sizeof(String),
	Offset(working_label), XtRString, "..."},
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
IspellSetStatus(char *label)
{
    Arg args[1];

    XtSetArg(args[0], XtNlabel, label);
    XtSetValues(ispell.status, args, 1);
}

static void
IspellSetRepeated(Bool state)
{
    XtSetSensitive(ispell.replaceAll, !state);
    XtSetSensitive(ispell.ignoreAll, !state);
    XtSetSensitive(ispell.add, !state);
    if (state && XtIsManaged(ispell.mispelled)) {
	XtUnmapWidget(ispell.mispelled);
	XtUnmanageChild(ispell.mispelled);
	XtManageChild(ispell.repeated);
    }
    else if (!state && XtIsManaged(ispell.repeated)) {
	XtUnmapWidget(ispell.repeated);
	XtUnmanageChild(ispell.repeated);
	XtManageChild(ispell.mispelled);
    }
}

static void
IspellSetSensitive(Bool state)
{
    XtSetSensitive(ispell.replace, state);
    XtSetSensitive(ispell.replaceAll, state);
    XtSetSensitive(ispell.ignore, state);
    XtSetSensitive(ispell.ignoreAll, state);
    XtSetSensitive(ispell.add, state);
}

static void
IspellCheckUndo(void)
{
    ispell_undo *undo = XtNew(ispell_undo);

    undo->next = NULL;
    undo->repeat = False;
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
    ii %= RSTRTBLSZ;
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
IspellIgnoredWord(char *word, int cmd, Bool add)
{
    IgnoreList *list, *prev;
    int ii = 0;
    char *pp = word;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= ISTRTBLSZ;
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
    list->add = add;
    list->next = ignore_list[ii];
    ignore_list[ii] = list;

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
	if (buflen >= ispell.bufsiz - 1)
	    ispell.buf = XtRealloc(ispell.buf, ispell.bufsiz += BUFSIZ);
	if ((len = read(ispell.ifd[0], &ispell.buf[buflen],
			ispell.bufsiz - buflen - 1)) <= 0)
	    break;
	buflen += len;
    }
    if (buflen <= 0)
	return (False);
    while (buflen > 0 && ispell.buf[buflen - 1] == '\n')
	--buflen;
    ispell.buf[buflen] = '\0';

    if ((tmp = strchr(ispell.sendbuf, '\n')) != NULL)
	*tmp = '\0';

    switch (ispell.buf[0]) {
	case '&':	/* MISS */
	case '?':	/* GUESS */
	    str = strchr(&ispell.buf[2], ' ');
	    if (!ispell.checkit) {
		*str = '\0';
		XtSetArg(args[0], XtNlabel, &ispell.buf[2]);
		XtSetValues(ispell.word, args, 1);
	    }
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
		    else if (str[j] == '-' && str[j+1] != '-' && str[j-1] != '-') {
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

	    XtSetSensitive(ispell.list, True);
	    XawListHighlight(ispell.list, i);

	    if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
		while (--old_len)
		    XtFree(old_list[old_len]);
		XtFree((char*)old_list);
	    }

	    if (!ispell.checkit) {
		XtSetArg(args[0], XtNstring, ispell.item);
		XtSetValues(ispell.text, args, 1);
		XawTextSetInsertionPoint(ispell.ascii, ispell.left);
		XawTextSetSelection(ispell.ascii, ispell.left, ispell.right);
		if (ispell.repeat)
		    IspellSetRepeated(ispell.repeat = False);
	    }

	    IspellSetStatus(ispell.buf[0] == '?' ?
			    ispell.guess_label : ispell.miss_label);
	    ispell.lock = True;
	    break;
	case '#':	/* NONE */
	case '-':	/* COMPOUND */
	case '+':	/* ROOT */
	check_label:
	    str = &ispell.sendbuf[1];
	    if (!ispell.checkit) {
		XtSetArg(args[0], XtNlabel, str);
		XtSetValues(ispell.word, args, 1);
	    }

	    XtSetArg(args[0], XtNlist, &old_list);
	    XtSetArg(args[1], XtNnumberStrings, &old_len);
	    XtGetValues(ispell.list, args, 2);
	    ispell.item = NULL;

	    list = (char**)XtMalloc(sizeof(char**));
	    if ((tmp = IspellReplacedWord(str, NULL)) != NULL)
		str = tmp;
	    if (tmp == NULL && ispell.buf[0] == '#')
		list[0] = XtNewString("");
	    else
		list[0] = XtNewString(str);

	    XtSetArg(args[0], XtNlist, list);
	    XtSetArg(args[1], XtNnumberStrings, 1);
	    XtSetValues(ispell.list, args, 2);

	    if (tmp == NULL && ispell.buf[0] == '#') {
		XawListUnhighlight(ispell.list);
		XtSetSensitive(ispell.list, False);
	    }
	    else {
		XtSetSensitive(ispell.list, True);
		XawListHighlight(ispell.list, 0);
	    }
	    if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
		while (--old_len)
		    XtFree(old_list[old_len]);
		XtFree((char*)old_list);
	    }

	    if (!ispell.checkit) {
		XtSetArg(args[0], XtNstring, str);
		XtSetValues(ispell.text, args, 1);
		XawTextSetInsertionPoint(ispell.ascii, ispell.left);
		XawTextSetSelection(ispell.ascii, ispell.left, ispell.right);
		if (ispell.repeat)
		    IspellSetRepeated(ispell.repeat = False);
	    }

	    ispell.lock = True;
	    if (ispell.buf[0] == '+') {
		if ((tmp = strchr(&ispell.buf[2], ' ')) != NULL)
		    *tmp = '\0';
		XmuSnprintf(word, sizeof(word), "%s %s",
			    ispell.root_label, &ispell.buf[2]);
		IspellSetStatus(word);
	    }
	    else
		IspellSetStatus(ispell.buf[0] == '#' ? ispell.none_label :
				ispell.buf[0] == '-' ? ispell.compound_label :
				ispell.ok_label);
	    break;
	case '*':	/* OK */
	case '\0':	/* when running in terse mode */
	    if (!ispell.checkit)
		(void)IspellIgnoredWord(&ispell.sendbuf[1], ADD, False);
	    else
		goto check_label;
	    ispell.lock = False;
	    break;
	default:
	    fprintf(stderr, "Unknown ispell command '%c'\n", ispell.buf[0]);
	    return (False);
    }

    if (!ispell.lock && !ispell.checkit) {
	ispell.stat = SEND;
	while (IspellSend() == 0)
	    ;
    }

    return (True);
}

/*ARGSUSED*/
static int
IspellSend(void)
{
    XawTextPosition position, old_left;
    XawTextBlock block;
    int i, len, spaces;
    Bool nl;

    if (ispell.lock || ispell.stat != SEND)
	return (-1);

    len = 1;
    ispell.sendbuf[0] = '^';	/* don't evaluate following characters as commands */

    spaces = 0;

    /* skip non word characters */
    position = ispell.right;
    nl = position == 0;
    while (1) {
	Bool done = False;

	position = XawTextSourceRead(ispell.source, position,
				     &block, BUFSIZ);
	if (block.length == 0) {	/* end of file */
	    ispell.stat = 0;
	    ispell.lock = True;
	    XawTextSetInsertionPoint(ispell.ascii, ispell.right);
	    XawTextUnsetSelection(ispell.ascii);
	    IspellSetSensitive(False);
	    IspellSetStatus(ispell.eof_label);
	    return (-1);
	}
	if (international) {
	    wchar_t *wptr = (wchar_t*)block.ptr;
	    char mb[sizeof(wchar_t)];

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
		    spaces = -1;
		    if (*mb && strchr(ispell.skip, *mb)) {
			position = ispell.right =
			    XawTextSourceScan(ispell.source, ispell.right + i,
					      XawstEOL, XawsdRight, 1, False);
			i = 0;
			break;
		    }
		}
		else if (*mb == ' ' && spaces >= 0)
		    ++spaces;
		else
		    spaces = -1;
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
		    spaces = -1;
		    if (block.ptr[i] && strchr(ispell.skip, block.ptr[i])) {
			position = ispell.right =
			    XawTextSourceScan(ispell.source, ispell.right + i,
					      XawstEOL, XawsdRight, 1, False);
			i = 0;
			break;
		    }
		}
		else if (block.ptr[i] == ' ' && spaces >= 0)
		    ++spaces;
		else
		    spaces = -1;
	    }
	}
	ispell.right += i;
	if (done)
	    break;
    }

    old_left = ispell.left;

    /* read a word */
    position = ispell.left = ispell.right;
    while (1) {
	Bool done = False;

	position = XawTextSourceRead(ispell.source, position,
				     &block, BUFSIZ);
	if (block.length == 0 && len == 1) {	/* end of file */
	    ispell.stat = 0;
	    ispell.lock = True;
	    XawTextSetInsertionPoint(ispell.ascii, ispell.right);
	    XawTextUnsetSelection(ispell.ascii);
	    IspellSetSensitive(False);
	    IspellSetStatus(ispell.eof_label);
	    return (-1);
	}
	if (international) {
	    wchar_t *wptr = (wchar_t*)block.ptr;
	    char mb[sizeof(wchar_t)];

	    for (i = 0; i < block.length; i++) {
		wctomb(mb, wptr[i]);
		if (!isalpha(*mb) && (!*mb || !strchr(ispell.wchars, *mb))) {
		    done = True;
		    break;
		}
		ispell.sendbuf[len] = *mb;
		if (++len >= sizeof(ispell.sendbuf) - 1) {
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
		ispell.sendbuf[len] = block.ptr[i];
		if (++len >= sizeof(ispell.sendbuf) - 1) {
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

    ispell.sendbuf[len] = '\0';

    if (spaces > 0 && spaces <= 32 && strcmp(ispell.sendbuf, ispell.sentbuf) == 0) {
	Arg args[2];
	int old_len;	
	char **list, **old_list;
	char label[sizeof(ispell.sendbuf) + sizeof(ispell.sentbuf) + 32];

	strcpy(label, &ispell.sendbuf[1]);
	for (i = 0; i < spaces; i++)
	    label[len + i - 1] = ' ';
	strcpy(&label[len + i - 1], &ispell.sendbuf[1]);
	XtSetArg(args[0], XtNlabel, label);
	XtSetValues(ispell.word, args, 1);

	XtSetArg(args[0], XtNstring, &ispell.sendbuf[1]);
	XtSetValues(ispell.text, args, 1);

	XtSetArg(args[0], XtNlist, &old_list);
	XtSetArg(args[1], XtNnumberStrings, &old_len);
	XtGetValues(ispell.list, args, 2);
	list = (char**)XtMalloc(sizeof(char**));
	list[0] = XtNewString(&ispell.sendbuf[1]);
	XtSetArg(args[0], XtNlist, list);
	XtSetArg(args[1], XtNnumberStrings, 1);
	XtSetValues(ispell.list, args, 2);
	XtSetSensitive(ispell.list, True);
	XawListHighlight(ispell.list, 0);
	if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
	    while (--old_len)
		XtFree(old_list[old_len]);
	    XtFree((char*)old_list);
	}

	IspellSetRepeated(True);
	XawTextSetInsertionPoint(ispell.ascii, old_left);
	XawTextSetSelection(ispell.ascii, old_left, ispell.right);
	IspellSetStatus(ispell.repeat_label);
	ispell.repeat = ispell.lock = True;

	return (1);
    }
    strcpy(ispell.sentbuf, ispell.sendbuf);

    if (len <= 2 || IspellIgnoredWord(&ispell.sendbuf[1], CHECK, False))
	return (0);

    ispell.sendbuf[len++] = '\n';

    write(ispell.ofd[1], ispell.sendbuf, len);

    ispell.stat = RECEIVE;

    return (1);
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
	if (ispell.terse_mode)
	    write(ispell.ofd[1], "!\n", 2);	/* enter terse mode */
	while (IspellSend() == 0)
	    ;
    }
    else if (ispell.source)
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
    char **strs, **list;
    int n_strs;
    Bool first_time = InitIspell();

    if (*num_params == 1 && (params[0][0] == 'e' || params[0][0] == 'E')) {
	PopdownIspell(w, (XtPointer)True, NULL);
	return;
    }

    if (!XtIsSubclass(w, textWidgetClass) || ispell.source) {
	Feep();
	return;
    }

    ispell.source = XawTextGetSource(ispell.ascii = w);

    if (first_time) {
	/* let the user choose the better position for the ispell window */
	Dimension width, height, b_width;
	Position x, y, max_x, max_y;

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
	XtSetArg(args[num_args], XtNheight, &height);		num_args++;
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
    }

    if (ispell.repeat)
	IspellSetRepeated(False);
    ispell.lock = ispell.repeat = ispell.checkit = False;
    ispell.stat = SEND;

    IspellSetSensitive(True);
    XtSetSensitive(ispell.undo, False);
    ispell.undo_depth = 0;

    XtSetArg(args[0], XtNlabel, "");
    XtSetValues(ispell.word, args, 1);

    XtSetArg(args[0], XtNstring, "");
    XtSetValues(ispell.text, args, 1);

    XtSetArg(args[0], XtNlist, &strs);
    XtSetArg(args[1], XtNnumberStrings, &n_strs);
    XtGetValues(ispell.list, args, 2);

    list = (char**)XtMalloc(sizeof(char**));
    list[0] = XtNewString("");
    XtSetArg(args[0], XtNlist, list);
    XtSetArg(args[1], XtNnumberStrings, 1);
    XtSetValues(ispell.list, args, 2);

    if (n_strs > 1 || (XtName(ispell.list) != strs[0])) {
	while (--n_strs)
	    XtFree(strs[n_strs]);
	XtFree((char*)strs);
    }

    IspellSetStatus(ispell.working_label);

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
	while (IspellSend() == 0)
	    ;
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
	IgnoreList *il, *pil, *nil;
	int i;

	/* insert added words in private dictionary */
	for (i = 0; i < ISTRTBLSZ; i++) {
	    pil = il = ignore_list[i];
	    while (il) {
		if (il->add) {
		    nil = il->next;
		    if (il == pil)
			ignore_list[i] = nil;
		    else
			pil->next = nil;
		    write(ispell.ofd[1], "*", 1);
		    write(ispell.ofd[1], il->word, strlen(il->word));
		    write(ispell.ofd[1], "\n", 1);
		    XtFree(il->word);
		    XtFree((char*)il);
		    il = nil;
		}
		else
		    il = il->next;
		pil = il;
	    }
	}
	write(ispell.ofd[1], "#\n", 2);		/* save dictionary */

	if (client_data) {
	    ReplaceList *rl, *prl;

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

	    for (i = 0; i < RSTRTBLSZ; i++) {
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
	    for (i = 0; i < ISTRTBLSZ; i++) {
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

    if (strcmp(search.ptr, replace.ptr) != 0 &&
	XawTextReplace(ispell.ascii, pos, pos + search.length,
		       &replace) == XawEditDone) {
	ispell.right += replace.length - search.length;
	IspellCheckUndo();
	ispell.undo_head->undo_str = NULL;
	ispell.undo_head->undo_pos = pos;
	ispell.undo_head->undo_count = 1;

	if (client_data && !ispell.repeat) {
	    XawTextDisableRedisplay(ispell.ascii);
	    pos = ispell.right;
	    while ((pos = XawTextSourceSearch(ispell.source, pos, XawsdRight, &search))
		!= XawTextSearchError) {
		Bool do_replace = True;
		char mb[sizeof(wchar_t)];

		if (XawTextSourceRead(ispell.source, pos - 1, &check, 1) > 0) {
		    if (international)
			wctomb(mb, *(wchar_t*)check.ptr);
		    else
			*mb = *check.ptr;
		    do_replace = !isalpha(*mb) && *mb && !strchr(ispell.wchars, *mb);
		}
		if (do_replace &&
		    XawTextSourceRead(ispell.source, pos + search.length, &check, 1) > 0) {
		    if (international)
			wctomb(mb, *(wchar_t*)check.ptr);
		    else
			*mb = *check.ptr;
		    do_replace = !isalpha(*mb) && *mb && !strchr(ispell.wchars, *mb);
		}
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

	strncpy(&ispell.sentbuf[1], replace.ptr, sizeof(ispell.sentbuf) - 2);
	ispell.sentbuf[sizeof(ispell.sentbuf) - 1] = '\0';
    }
    else
	Feep();

    if (ispell.repeat)
	ispell.right = ispell.left = XawTextGetInsertionPoint(ispell.ascii);
    else if (!ispell.item || strcmp(ispell.item, replace.ptr))
	ispell.right = ispell.left;	/* check it again! */

    ispell.lock = ispell.checkit = False;

    ispell.stat = SEND;
    IspellSetStatus(ispell.working_label);
    while (IspellSend() == 0)
	;
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

    if ((ispell.undo_head->repeat = ispell.repeat) != False) {
	ispell.undo_head->undo_count = ispell.right;
	ispell.undo_head->undo_str = XtNewString(text);
    }
    else
	ispell.undo_head->undo_count = 0;

    ispell.undo_head->undo_pos = XawTextGetInsertionPoint(ispell.ascii);

    if (!ispell.repeat) {
	if (client_data) {
	    IspellIgnoredWord(text, ADD, False);
	    ispell.undo_head->undo_str = XtNewString(text);
	}
	else 
	    ispell.undo_head->undo_str = NULL;
    }

    ispell.lock = ispell.checkit = False;

    ispell.stat = SEND;
    IspellSetStatus(ispell.working_label);
    while (IspellSend() == 0)
	;
}

/*ARGSUSED*/
void
AddIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg args[1];
    char *text;

    if (!ispell.lock || ispell.repeat)
	return;

    XtSetArg(args[0], XtNlabel, &text);
    XtGetValues(ispell.word, args, 1);

    IspellCheckUndo();
    ispell.undo_head->undo_str = XtNewString(text);
    ispell.undo_head->undo_pos = XawTextGetInsertionPoint(ispell.ascii);
    ispell.undo_head->undo_count = -1;

    (void)IspellIgnoredWord(text, ADD, True);

    ispell.lock = ispell.checkit = False;
    ispell.stat = SEND;
    IspellSetStatus(ispell.working_label);
    while (IspellSend() == 0)
	;
}

/*ARGSUSED*/
static void
UndoIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Bool enable_redisplay = False;
    ispell_undo *undo = ispell.undo_head;

    if ((!ispell.lock && ispell.stat) || !undo)
	return;

    if (undo->undo_count > 0 && !undo->repeat) {
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
	    (void)IspellIgnoredWord(undo->undo_str, REMOVE, True);
    }
    else if (undo->undo_str) {
	if (!undo->repeat)
	    IspellIgnoredWord(undo->undo_str, REMOVE, False);
    }

    XawTextSetInsertionPoint(ispell.ascii,
			     ispell.right = ispell.left = undo->undo_pos);
    if (enable_redisplay)
	XawTextEnableRedisplay(ispell.ascii);

    /* need to do it because may be two misspelled words together */
    if (undo->repeat) {
	char **list, **old_list;
	int old_len;
	Arg args[2];

	ispell.right = (XawTextPosition)undo->undo_count;
	IspellSetRepeated(ispell.repeat = True);
	XtSetArg(args[0], XtNlabel, undo->undo_str);
	XtSetValues(ispell.word, args, 1);
	XmuSnprintf(ispell.sentbuf, sizeof(ispell.sentbuf), "^%s",
		    strrchr(undo->undo_str, ' ') + 1);
	strcpy(ispell.sendbuf, ispell.sentbuf);
	XtSetArg(args[0], XtNstring, &ispell.sentbuf[1]);
	XtSetValues(ispell.text, args, 1);

	XtSetArg(args[0], XtNlist, &old_list);
	XtSetArg(args[1], XtNnumberStrings, &old_len);
	XtGetValues(ispell.list, args, 2);

	list = (char **)XtMalloc(sizeof(char*));
	list[0] = XtNewString(&ispell.sentbuf[1]);
	XtSetArg(args[0], XtNlist, list);
	XtSetArg(args[1], XtNnumberStrings, 1);
	XtSetValues(ispell.list, args, 2);
	XtSetSensitive(ispell.list, True);
	XawListHighlight(ispell.list, 0);

	if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
	    while (--old_len)
		XtFree(old_list[old_len]);
	    XtFree((char*)old_list);
	}

	XawTextSetSelection(ispell.ascii, ispell.left, ispell.right);
	IspellSetStatus(ispell.repeat_label);
	ispell.lock = True;
	ispell.checkit = False;
    }
    else if (ispell.repeat) {
	*ispell.sentbuf = '\0';
	IspellSetRepeated(ispell.repeat = False);
    }

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

    if (!ispell.stat || ispell.checkit)
	IspellSetSensitive(True);

    if (!ispell.repeat) {
	ispell.lock = ispell.checkit = False;
	ispell.stat = SEND;
	IspellSetStatus(ispell.working_label);
	while (IspellSend() == 0)
	    ;
    }
}

/*ARGSUSED*/
static void
CheckIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg args[1];
    char *text;
    int len;

    if (!ispell.lock)
	return;

    XtSetArg(args[0], XtNstring, &text);
    XtGetValues(ispell.text, args, 1);
    len = XmuSnprintf(ispell.sendbuf, sizeof(ispell.sendbuf), "^%s\n", text);

    ispell.sendbuf[sizeof(ispell.sendbuf) - 1] = '\n';

    write(ispell.ofd[1], ispell.sendbuf, len);

    ispell.lock = False;
    ispell.checkit = True;
    ispell.stat = RECEIVE;
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
    ispell.repeated	= XtCreateWidget("repeated", labelWidgetClass,
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
    ispell.check	= XtCreateManagedWidget("check", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.check, XtNcallback, CheckIspell, NULL);
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
    ispell.status	= XtCreateManagedWidget("status", labelWidgetClass,
						ispell.form, NULL, 0);

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
