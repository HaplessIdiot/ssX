/* $XFree86$ */

#include "xedit.h"
#include <stdlib.h>
#include <limits.h>		/* LONG_MIN and LONG_MAX */
#include <ctype.h>
#include <errno.h>

#define BUFFERFMT	"\"#<XEDIT-BUFFER@0x%lx>\""

/*
 * Types
 */
typedef struct _XeditReqTrans XeditReqTrans;
typedef struct _XeditReqArgs XeditReqArgs;
typedef union _XeditReqArg XeditReqArg;
typedef struct _XeditReqInfo XeditReqInfo;
typedef Bool (*XeditReqFun)(XeditReqInfo*, XeditReqArgs*, char**);

struct _XeditReqTrans {
    char *req;
    XeditReqFun fun;
    char *args_desc;
};

union _XeditReqArg {
    long integer;
    char *string;
    xedit_flist_item *item;
};

/* XXX update if a function with more arguments is defined */
#define REQUEST_MAX_ARGUMENTS	3

struct _XeditReqArgs {
    XeditReqArg args[REQUEST_MAX_ARGUMENTS];
    int num_args;
};

struct _XeditReqInfo {
    Widget text;
    Widget source;
    Widget sink;
};

/*
 * Prototypes
 */
static xedit_flist_item *StringToFlistItem(char*);
static Bool Search(XeditReqInfo*, XeditReqArgs*, char**, XawTextScanDirection);

static Bool PointMin(XeditReqInfo*, XeditReqArgs*, char**);
static Bool PointMax(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetPoint(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetPoint(XeditReqInfo*, XeditReqArgs*, char**);
static Bool Insert(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetForeground(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetForeground(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetBackground(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetBackground(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetFont(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetFont(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetWrapMode(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetWrapMode(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetAutoFill(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetAutoFill(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetLeftColumn(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetLeftColumn(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetRightColumn(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetRightColumn(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetJustification(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetJustification(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetVertScrollbar(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetVertScrollbar(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetHorizScrollbar(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetHorizScrollbar(XeditReqInfo*, XeditReqArgs*, char**);
static Bool CreateBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool RemoveBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetCurrentBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetCurrentBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetOtherBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetOtherBuffer(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetBufferName(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetBufferName(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetBufferFileName(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetBufferFileName(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SearchForward(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SearchBackward(XeditReqInfo*, XeditReqArgs*, char**);
static Bool ReplaceText(XeditReqInfo*, XeditReqArgs*, char**);

/* todo */
#if 0
static Bool GetForegroundPixmap(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetForegroundPixmap(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetBackgroundPixmap(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetBackgroundPixmap(XeditReqInfo*, XeditReqArgs*, char**);
static Bool GetFontSet(XeditReqInfo*, XeditReqArgs*, char**);
static Bool SetFontSet(XeditReqInfo*, XeditReqArgs*, char**);
#endif

/*
 * Initialization
 */
static XeditReqTrans trans[] = {
/* nil:			means nothing, i.e. NULL
 * (b)uffer		buffer representation
 * (i)nteger:		number
 * (s)tring:		any text, enclosed in "'s
 */

    /* input  = string: buffer-name
     * output = string: buffer-identifier */
    {"create-buffer",		CreateBuffer,		"s"},

    /* input  = nil
     * output = string: converter value, one of "true" and "false" */
    {"get-auto-fill",		GetAutoFill,		""},

    /* input  = nil
     * output = string: background color */
    {"get-background",		GetBackground,		""},

    /* input  = string: buffer-identifier
     * output = string: buffer-file-name */
    {"get-buffer-filename",	GetBufferFileName,	"b"},

    /* input  = string: buffer-identifier
     * output = string: buffer-name */
    {"get-buffer-name",		GetBufferName,		"b"},

    /* input  = nil
     * output = string: buffer-identifier */
    {"get-current-buffer",	GetCurrentBuffer,	""},

    /* input  = nil
     * output = string: font name */
    {"get-font",		GetFont,		""},

    /* input  = nil
     * output = string: foreground color */
    {"get-foreground",		GetForeground,		""},

    /* input  = nil
     * output = string: one of "always" and "never" */
    {"get-horiz-scrollbar",	GetHorizScrollbar,	""},

    /* input  = nil
     * output = string: one of "left", "right", "center" and "full" */
    {"get-justification",	GetJustification,	""},

    /* input  = nil
     * output = integer: left colum, used only if AutoFill is true */
    {"get-left-column",		GetLeftColumn,		""},

    /* input  = nil
     * output = string: buffer-identifier */
    {"get-other-buffer",	GetOtherBuffer,		""},

    /* input  = nil
     * output = integer: current cursor position */
    {"get-point",		GetPoint,		""},

    /* input  = nil
     * output = integer: right colum, used only if AutoFill is true */
    {"get-right-column",	GetRightColumn,		""},

    /* input  = nil
     * output = string: one of "always" and "never" */
    {"get-vert-scrollbar",	GetVertScrollbar,	""},

    /* input  = nil
     * output = string: converter value, one of "never", "line" and "word" */
    {"get-wrap-mode",		GetWrapMode,		""},

    /* input  = string: text to be inserted
     * output = nil */
    {"insert",			Insert,			"s"},

    /* input  = nil
     * output = integer: largest visible cursor position */
    {"point-max",		PointMax,		""},

    /* input  = nil
     * output = integer: smallest visible cursor position */
    {"point-min",		PointMin,		""},

    /* input  = string: buffer-identifier
     * output = nil
     */
    {"remove-buffer",		RemoveBuffer,		"b"},

    /* input  = string: buffer-identifier
     * output = nil
     */
    {"replace-text",		ReplaceText,		"iis"},

    /* input  = string: string to be searched
     * output = integer: position of text or nil if no match */
    {"search-backward",		SearchBackward,		"s"},

    /* input  = string: string to be searched
     * output = integer: position of text or nil if no match */
    {"search-forward",		SearchForward,		"s"},

    /* input  = string: converter value, one of "true" and "false"
     * output = nil */
    {"set-auto-fill",		SetAutoFill,		"s"},

    /* input  = string: background color
     * output = nil */
    {"set-background",		SetBackground,		"s"},

    /* input  = string list: buffer-identifier and new buffer-name
     * output = nil */
    {"set-buffer-name",		SetBufferName,		"bs"},

    /* input  = string list: buffer-identifier and new buffer-file-name
     * output = nil */
    {"set-buffer-filename",	SetBufferFileName,	"bs"},

    /* input  = string: buffer-identifier
     * output = nil */
    {"set-current-buffer",	SetCurrentBuffer,	"b"},

    /* input  = string: font name
     * output = nil */
    {"set-font",		SetFont,		"s"},

    /* input  = string: foreground color
     * output = nil */
    {"set-foreground",		SetForeground,		"s"},

    /* input  = string: one of "always" and "never"
     * output = nil */
    {"set-horiz-scrollbar",	SetHorizScrollbar,	"s"},

    /* input  = string: one of "left", "right", "center" and "full"
     * output = nil */
    {"set-justification",	SetJustification,	"s"},

    /* input  = integer: left colum, used only if AutoFill is true
     * output = nil */
    {"set-left-column",		SetLeftColumn,		"i"},

    /* input  = string: buffer-identifier
     * output = nil */
    {"set-other-buffer",	SetOtherBuffer,		"b"},

    /* input  = integer: cursor position
     * output = nil */
    {"set-point",		SetPoint,		"i"},

    /* input  = integer: right colum, used only if AutoFill is true
     * output = nil */
    {"set-right-column",	SetRightColumn,		"i"},

    /* input  = string: one of "always" and "never"
     * output = nil */
    {"set-vert-scrollbar",	SetVertScrollbar,	"s"},

    /* input  = string: converter value, one of "never", "line" and "word"
     * output = nil */
    {"set-wrap-mode",		SetWrapMode,		"s"},
};

static char buffer[512];
static char errstr[80];

/*
 * Implementation
 */
static int
compar(_Xconst void *left, _Xconst void *right)
{
    return (strcmp((char*)left, ((XeditReqTrans*)right)->req));
}

Bool
XeditProto(char *input, char **result)
{
    static XeditReqInfo info;
    Bool status;
    XeditReqTrans *request;
    XeditReqArgs arguments;
    char *function, *string, *description, *value;

    /* unless overriden, always return NIL */
    *result = "NIL\n";

    /* get function name */
    for (function = input; *function && isspace(*function); function++)
	;
    for (string = function; *string && !isspace(*string); string++)
	;

    /* input argument is a writable string, and only useful here */

    if (*string) {
	/* skip spaces after function name */
	for (*string = '\0', ++string; *string && isspace(*string); string++)
	    ;
    }
    else
	*string = '\0';

    request = bsearch(function, trans, sizeof(trans) / sizeof(trans[0]),
		      sizeof(XeditReqTrans), compar);

    if (request == NULL) {
	XmuSnprintf(buffer, sizeof(buffer), "unknown request %s\n", function);
	*result = buffer;

	return (False);
    }

    /* initialize */
    arguments.num_args = 0;
    description = request->args_desc;

    /* parse arguments */
    for (; *string && *description; description++) {
	value = string;
	/* argument is a string */
	if (*description == 's') {
	    int length;

	    if (*string != '"') {
		XmuSnprintf(buffer, sizeof(buffer),
			    "%s: expecting string\n", function);
		*result = buffer;

		return (False);
	    }

	    /* skip '"' */
	    ++value;
	    ++string;

	    /* read string */
	    length = 0;
	    while (*string != '"') {
		if (*string == '\0') {
		    XmuSnprintf(buffer, sizeof(buffer),
				"%s: unfinished string\n", function);
		    *result = buffer;

		    return (False);
		}
		else if (*string == '\\') {
		    /* this should be uncommon, just to allow escaping
		     * quotes and backslashes */
		    if (length)
			memmove(value + 1, value, length);
		    ++value;
		    /* if not quoting a nul character */
		    if (*++string)
			++string;
		}
		else
		    ++string;
		++length;
	    }

	    /* check for incomplete string already done */
	    *string++ = '\0';
	    arguments.args[arguments.num_args++].string = value;
	}

	/* argument is an integer */
	else if (*description == 'i') {
	    char *end;
	    long number;

	    /* read number */
	    value = string;
	    for (; *string && !isspace(*string); string++)
		;
	    if (*string)
		*string++ = '\0';
	    else
		*string = '\0';

	    errno = 0;
	    number = strtol(value, &end, 10);

	    if (*end || (errno == ERANGE &&
		((*value == '-' && number == LONG_MIN) ||
		 (*value != '-' && number == LONG_MAX)))) {
		XmuSnprintf(buffer, sizeof(buffer),
			    "%s: expecting integer\n", function);
		*result = buffer;

		return (False);
	    }

	    arguments.args[arguments.num_args++].integer = number;
	}

	/* argument is a buffer name */
	else if (*description == 'b') {
	    xedit_flist_item *item;

	    /* read buffer name */
	    value = string;
	    for (; *string && !isspace(*string); string++)
		;
	    if (*string)
		*string++ = '\0';
	    else
		*string = '\0';

	    item = StringToFlistItem(value);
	    if (item == NULL) {
		XmuSnprintf(buffer, sizeof(buffer),
			    "%s: expecting buffer name\n", function);
		*result = buffer;

		return (False);
	    }

	    arguments.args[arguments.num_args++].item = item;
	}

	/* skip white-spaces before parsing next argument */
	for (; *string && isspace(*string); string++)
	    ;
    }

    if (*description) {
	XmuSnprintf(buffer, sizeof(buffer), "%s: too few arguments\n", function);
	*result = buffer;

	return (False);
    }
    else if (*string) {
	XmuSnprintf(buffer, sizeof(buffer), "%s: too many arguments\n", function);
	*result = buffer;

	return (False);
    }

    /* if running on another text buffer */
    if (info.text != textwindow) {
	info.text = textwindow;
	info.source = XawTextGetSource(textwindow);
	info.sink = XawTextGetSink(textwindow);
    }

    /* call the xedit-proto function */
    status = (request->fun)(&info, &arguments, result);

    if (status == False) {
	XmuSnprintf(buffer, sizeof(buffer), "%s: %s\n", function, errstr);
	*result = buffer;
    }

    return (status);
}

static xedit_flist_item *
StringToFlistItem(char *string)
{
    int i;
    xedit_flist_item *item;

    if (sscanf(string, BUFFERFMT, (long*)(&item)) != 1)
	return (NULL);

    for (i = 0; i < flist.num_itens; i++)
	if (flist.itens[i] == item)
	    return (item);

    return (NULL);
}

/*ARGSUSED*/
static Bool
PointMin(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition point = XawTextSourceScan(info->source, 0,
					      XawstAll, XawsdLeft, 1, True);

    XmuSnprintf(buffer, sizeof(buffer), "%ld\n", point);
    *result = buffer;

    return (True);
}

/*ARGSUSED*/
static Bool
PointMax(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition point = XawTextSourceScan(info->source, 0,
					      XawstAll, XawsdRight, 1, True);

    XmuSnprintf(buffer, sizeof(buffer), "%ld\n", point);
    *result = buffer;

    return (True);
}

/*ARGSUSED*/
static Bool
GetPoint(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition point = XawTextGetInsertionPoint(info->text);

    XmuSnprintf(buffer, sizeof(buffer), "%ld\n", point);
    *result = buffer;

    return (True);
}

static Bool
SetPoint(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition point = args->args[0].integer;

    XawTextSetInsertionPoint(info->text, point);

    return (True);
}

static Bool
Insert(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition point = XawTextGetInsertionPoint(info->text);
    XawTextBlock block;

    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = strlen(args->args[0].string);
    block.ptr = args->args[0].string;
    XawTextReplace(info->text, point, point, &block);
    XawTextSetInsertionPoint(info->text, point + block.length);

    return (True);
}

/*ARGSUSED*/
static Bool
GetForeground(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    Pixel pixel;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(pixel);
    from.addr = (XtPointer)&pixel;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNforeground, &pixel);
    XtGetValues(info->sink, arg, 1);
    XtConvertAndStore(info->sink, XtRPixel, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetForeground(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    Pixel pixel;
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(pixel);
    to.addr = (XtPointer)&pixel;

    if (!XtConvertAndStore(info->sink, XtRString, &from, XtRPixel, &to)) {
	XmuSnprintf(errstr, sizeof(errstr), "cannot convert \"%s\" to Pixel",
		    from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNforeground, pixel);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetBackground(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    Pixel pixel;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(pixel);
    from.addr = (XtPointer)&pixel;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNbackground, &pixel);
    XtGetValues(info->sink, arg, 1);
    XtConvertAndStore(info->text, XtRPixel, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetBackground(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Pixel pixel;
    Arg arg[1];
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(pixel);
    to.addr = (XtPointer)&pixel;

    if (!XtConvertAndStore(info->sink, XtRString, &from, XtRPixel, &to)) {
	XmuSnprintf(errstr, sizeof(errstr), "cannot convert \"%s\" to Pixel",
		    from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNbackground, pixel);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetFont(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    XFontStruct *font;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(font);
    from.addr = (XtPointer)&font;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNfont, &font);
    XtGetValues(info->sink, arg, 1);
    XtConvertAndStore(info->text, XtRFontStruct, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetFont(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XFontStruct *font;
    Arg arg[1];
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(font);
    to.addr = (XtPointer)&font;

    if (!XtConvertAndStore(info->sink, XtRString, &from, XtRFontStruct, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to FontStruct", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNfont, font);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetWrapMode(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    XawTextWrapMode wrap;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(wrap);
    from.addr = (XtPointer)&wrap;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNwrap, &wrap);
    XtGetValues(info->text, arg, 1);
    XtConvertAndStore(info->text, XtRWrapMode, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetWrapMode(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextWrapMode wrap;
    Arg arg[1];
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(wrap);
    to.addr = (XtPointer)&wrap;

    if (!XtConvertAndStore(info->text, XtRString, &from, XtRWrapMode, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to WrapMode", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNwrap, wrap);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetAutoFill(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    Boolean fill;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(fill);
    from.addr = (XtPointer)&fill;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNautoFill, &fill);
    XtGetValues(info->text, arg, 1);
    XtConvertAndStore(info->text, XtRBoolean, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetAutoFill(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Boolean fill;
    Arg arg[1];
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(fill);
    to.addr = (XtPointer)&fill;

    if (!XtConvertAndStore(info->text, XtRString, &from, XtRBoolean, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to Boolean", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNautoFill, fill);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetLeftColumn(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    short left;

    XtSetArg(arg[0], XtNleftColumn, &left);
    XtGetValues(info->text, arg, 1);
    XmuSnprintf(buffer, sizeof(buffer), "%d\n", left);
    *result = buffer;

    return (True);
}

static Bool
SetLeftColumn(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    int left;

    left = args->args[0].integer;
    XtSetArg(arg[0], XtNleftColumn, left);
    XtSetValues(info->text, arg, 1);

    return (True);
}

/*ARGSUSED*/
static Bool
GetRightColumn(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    short right;

    XtSetArg(arg[0], XtNrightColumn, &right);
    XtGetValues(info->text, arg, 1);
    XmuSnprintf(buffer, sizeof(buffer), "%d\n", right);
    *result = buffer;

    return (True);
}

static Bool
SetRightColumn(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    int right;

    right = args->args[0].integer;
    XtSetArg(arg[0], XtNrightColumn, right);
    XtSetValues(info->text, arg, 1);

    return (True);
}

static Bool
GetJustification(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    XawTextJustifyMode justify;
    Arg arg[1];
    XrmValue from, to;

    from.size = sizeof(justify);
    from.addr = (XtPointer)&justify;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNjustifyMode, &justify);
    XtGetValues(info->text, arg, 1);
    XtConvertAndStore(info->text, XtRJustifyMode, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetJustification(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextJustifyMode justify;
    Arg arg[1];
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(justify);
    to.addr = (XtPointer)&justify;

    if (!XtConvertAndStore(info->text, XtRString, &from, XtRJustifyMode, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to JustifyMode", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNjustifyMode, justify);
    XtVaSetValues(info->text, arg, 1);

    return (True);
}

static Bool
GetVertScrollbar(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    Arg arg[1];
    XawTextScrollMode scroll;
    XrmValue from, to;

    from.size = sizeof(scroll);
    from.addr = (XtPointer)&scroll;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNscrollVertical, &scroll);
    XtGetValues(info->text, arg, 1);
    XtConvertAndStore(info->text, XtRScrollMode, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetVertScrollbar(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    XawTextScrollMode scroll;
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(scroll);
    to.addr = (XtPointer)&scroll;

    if (!XtConvertAndStore(info->text, XtRString, &from, XtRScrollMode, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to ScrollMode", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNscrollVertical, scroll);
    XtSetValues(info->text, arg, 1);

    return (True);
}

static Bool
GetHorizScrollbar(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    char *str;
    Arg arg[1];
    XawTextScrollMode scroll;
    XrmValue from, to;

    from.size = sizeof(scroll);
    from.addr = (XtPointer)&scroll;
    to.size = 0;
    to.addr = NULL;

    XtSetArg(arg[0], XtNscrollHorizontal, &scroll);
    XtGetValues(info->text, arg, 1);
    XtConvertAndStore(info->text, XtRScrollMode, &from, XtRString, &to);
    str = to.addr;
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", str);
    *result = buffer;

    return (True);
}

static Bool
SetHorizScrollbar(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Arg arg[1];
    XawTextScrollMode scroll;
    XrmValue from, to;

    from.size = strlen(args->args[0].string) + 1;
    from.addr = (XtPointer)args->args[0].string;
    to.size = sizeof(scroll);
    to.addr = (XtPointer)&scroll;

    if (!XtConvertAndStore(info->text, XtRString, &from, XtRScrollMode, &to)) {
	XmuSnprintf(errstr, sizeof(errstr),
		    "cannot convert \"%s\" to ScrollMode", from.addr);
	return (False);
    }
    XtSetArg(arg[0], XtNscrollHorizontal, scroll);
    XtSetValues(info->text, arg, 1);

    return (True);
}

static Bool
CreateBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    Widget source;
    xedit_flist_item *item;

    source =  XtVaCreateWidget("textSource", international ?
			       multiSrcObjectClass : asciiSrcObjectClass,
			       topwindow,
			       XtNeditType, XawtextEdit,
			       NULL, NULL);

    item = AddTextSource(source, args->args[0].string, NULL, 0, 0);
    XmuSnprintf(buffer, sizeof(buffer), BUFFERFMT "\n", item);
    *result = buffer;

    return (True);
}

static Bool
RemoveBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    KillTextSource(args->args[0].item);

    return (True);
}

static Bool
GetCurrentBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    xedit_flist_item *item = flist.current;

    if (item == NULL)
	/* this is probably an error */
	return (True);

    XmuSnprintf(buffer, sizeof(buffer), BUFFERFMT "\n", item);
    *result = buffer;

    return (True);
}

static Bool
SetCurrentBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    SwitchTextSource(args->args[0].item);

    return (True);
}

static Bool
GetOtherBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    xedit_flist_item *item = flist.other;

    if (item == NULL)
	/* this is not an error */
	return (True);

    XmuSnprintf(buffer, sizeof(buffer), BUFFERFMT "\n", item);
    *result = buffer;
    return (True);
}

static Bool
SetOtherBuffer(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    flist.other = args->args[0].item;

    return (True);
}

static Bool
GetBufferName(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", args->args[0].item->name);
    *result = buffer;

    return (True);
}

static Bool
SetBufferName(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XtFree(args->args[0].item->name);
    args->args[0].item->name = XtNewString(args->args[1].string);

    return (True);
}

static Bool
GetBufferFileName(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XmuSnprintf(buffer, sizeof(buffer), "\"%s\"\n", args->args[0].item->filename);
    *result = buffer;

    return (True);
}

/* this probably should not be allowed */
static Bool
SetBufferFileName(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XtFree(args->args[0].item->filename);
    args->args[0].item->filename = XtNewString(args->args[1].string);

    return (True);
}

static Bool
Search(XeditReqInfo *info, XeditReqArgs *args, char **result,
       XawTextScanDirection direction)
{
    XawTextPosition point, position;
    XawTextBlock block;

    position = XawTextGetInsertionPoint(info->text);

    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = strlen(args->args[0].string);
    block.ptr = args->args[0].string;
    point = XawTextSourceSearch(info->source, position, direction, &block);

    if (point != XawTextSearchError) {
	XmuSnprintf(buffer, sizeof(buffer), "%ld\n", point);
	*result = buffer;
    }

    return (True);
}

static Bool
SearchForward(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    return (Search(info, args, result, XawsdRight));
}

static Bool
SearchBackward(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    return (Search(info, args, result, XawsdLeft));
}

static Bool
ReplaceText(XeditReqInfo *info, XeditReqArgs *args, char **result)
{
    XawTextPosition left, right;
    XawTextBlock block;
    char *string;

    left = args->args[0].integer;
    right = args->args[1].integer;
    string = args->args[2].string;

    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = strlen(string);
    block.ptr = string;
    XawTextReplace(info->text, left, right, &block);

    return (True);
}
