/* $XFree86$ */

#include "xedit.h"
#include <fcntl.h>

#define RECEIVE		1
#define SEND		2

/*
 * Types
 */
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

    int undo_count;
    XawTextPosition undo_position;

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

/*
 * Prototypes
 */
static void AddIspell(Widget, XtPointer, XtPointer);
static void IgnoreIspell(Widget, XtPointer, XtPointer);
static Bool InitIspell(void);
static void IspellInputCallback(XtPointer, int*, XtInputId*);
static Bool IspellReceive(void);
static char *IspellReplaceWord(char*, char*);
static Bool IspellSend(void);
static void PopdownIspell(Widget, XtPointer, XtPointer);
static void ReplaceIspell(Widget, XtPointer, XtPointer);
static void SelectIspell(Widget, XtPointer, XtPointer);
static void UndoIspell(Widget, XtPointer, XtPointer);

Bool _XawTextSrcUndo(TextSrcObject, XawTextPosition*);

/*
 * Initialization
 */
static struct _ispell ispell;

#define STRTBLSZ	23
static ReplaceList *replace_list[STRTBLSZ];

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
static char *
IspellReplaceWord(char *word, char *replace)
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

/*ARGSUSED*/
static Bool
IspellReceive(void)
{
    int i, len, buflen, old_len;
    Arg args[2];
    char *str, *end, **list, **old_list;

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
	    len = atoi(str);
	    list = (char **)XtMalloc(sizeof(char*));
	    str = strchr(str, ':') + 1;
	    for (i = 0; i < len || len == 0; i++) {
		end = strchr(str, ',');
		if (end)	*end = '\0';
		list = (char**)XtRealloc((char*)list, (i + 1) * sizeof(char*));
		list[i] = XtNewString(str + 1);
		if (end)	str = end + 1;
		else		break;
	    }
	    len = len == 0 ? i + 1 : len;

	    XtSetArg(args[0], XtNlist, &old_list);
	    XtSetArg(args[1], XtNnumberStrings, &old_len);
	    XtGetValues(ispell.list, args, 2);

	    ispell.item = NULL;
	    if ((str = IspellReplaceWord(&ispell.buf[2], NULL)) != NULL)
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
	    list[0] = XtNewString(&ispell.buf[2]);
	    XtSetArg(args[0], XtNlist, list);
	    XtSetArg(args[1], XtNnumberStrings, 1);
	    XtSetValues(ispell.list, args, 2);

	    XawListUnhighlight(ispell.list);
	    if (old_len > 1 || (XtName(ispell.list) != old_list[0])) {
		while (--old_len)
		    XtFree(old_list[old_len]);
		XtFree((char*)old_list);
	    }

	    XtSetArg(args[0], XtNstring, &ispell.buf[2]);
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
	    return (False);
	}
	for (i = 0; i < block.length; i++) {
	    if (isalpha(block.ptr[i]) || strchr(ispell.wchars, block.ptr[i])) {
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
	for (i = 0; i < block.length; i++) {
	    if (!isalpha(block.ptr[i]) && !strchr(ispell.wchars, block.ptr[i])) {
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
	ispell.right += i;
	if (done)
	    break;
    }

    if (len == 1)
	return (False);

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

    if (event) {
	Arg args[3];
	Cardinal num_args;
	Dimension width, height, b_width;
	Position x, y, max_x, max_y;

	x = y = 0;
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

    ispell.lock = False;
    ispell.stat = SEND;

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

    XtSetSensitive(ispell.undo, False);
    XtPopup(ispell.shell, XtGrabExclusive);
    XtSetKeyboardFocus(ispell.shell, ispell.text);
}

/*ARGSUSED*/
static void
PopdownIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    ispell.source = NULL;
    if (ispell.pid) {
	if (client_data) {
	    int i;
	    ReplaceList *rl, *prl;

	    XtRemoveInput(ispell.id);
	    /* save user's private dictionary */
	    write(ispell.ofd[1], "#\n", 2);
	    close(ispell.ofd[0]);
	    close(ispell.ofd[1]);
	    close(ispell.ifd[0]);
	    close(ispell.ifd[1]);
	    waitpid(ispell.pid, NULL, 0);
	    ispell.pid = 0;
	    if (ispell.buf)
		XtFree(ispell.buf);
	    ispell.buf = NULL;

	    for (i = 0; i < STRTBLSZ; i++) {
		prl = rl = replace_list[i];
		while (prl) {
		    rl = rl->next;
		    XtFree(prl->word);
		    XtFree((char*)prl);
		    prl = rl;
		}
	        replace_list[i] = NULL;
	    }
	}
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

    if (strcmp(search.ptr, replace.ptr) != 0) {
	ispell.undo_position = pos;
	XawTextReplace(ispell.ascii, pos, pos + search.length, &replace);
	ispell.right += replace.length - search.length;
	ispell.undo_count = 1;

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
		    ++ispell.undo_count;
		}
		pos += search.length;
	    }
	    XawTextEnableRedisplay(ispell.ascii);
	}
	else
	    IspellReplaceWord(search.ptr, replace.ptr);
	XtSetSensitive(ispell.undo, True);
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
    if (!ispell.lock)
	return;

    XtSetSensitive(ispell.undo, False);

    if (client_data) {
	Arg args[1];
	char *text;

	XtSetArg(args[0], XtNlabel, &text);
	XtGetValues(ispell.word, args, 1);

	/* tell ispell to ignore this word */
	write(ispell.ofd[1], "@", 1);
	write(ispell.ofd[1], text, strlen(text));
	write(ispell.ofd[1], "\n", 1);
    }

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

    XtSetSensitive(ispell.undo, False);

    XtSetArg(args[0], XtNlabel, &text);
    XtGetValues(ispell.word, args, 1);

    /* tell ispell to add this word to user's dictionary */
    write(ispell.ofd[1], "*", 1);
    write(ispell.ofd[1], text, strlen(text));
    write(ispell.ofd[1], "\n", 1);

    ispell.lock = False;
    ispell.stat = SEND;
    IspellSend();
}

/*ARGSUSED*/
static void
UndoIspell(Widget w, XtPointer client_data, XtPointer call_data)
{
    XawTextPosition tmp;

    if (!ispell.lock)
	return;

    XtSetSensitive(ispell.undo, False);

    XawTextDisableRedisplay(ispell.ascii);
    while (ispell.undo_count--)
	if (!_XawTextSrcUndo((TextSrcObject)ispell.source, &tmp)) {
	    Feep();
	    break;
	}
    XawTextSetInsertionPoint(ispell.ascii,
			     ispell.right = ispell.left = ispell.undo_position);
    XawTextEnableRedisplay(ispell.ascii);

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
    ispell.replace	= XtCreateManagedWidget("replace", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.replace, XtNcallback, ReplaceIspell, (XtPointer)False);
    ispell.replaceAll	= XtCreateManagedWidget("replaceAll", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.replaceAll, XtNcallback, ReplaceIspell, (XtPointer)True);
    ispell.undo		= XtCreateManagedWidget("undo", commandWidgetClass,
						ispell.commands, NULL, 0);
    XtAddCallback(ispell.undo, XtNcallback, UndoIspell, NULL);
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
