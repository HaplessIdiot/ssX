/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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

#include "../xedit.h"
#include <X11/Xaw/TextSrcP.h>	/* Needs some private definitions */
#include <X11/Xaw/TextSinkP.h>	/* Also needs private definitions... */
#include <X11/Xmu/Xmu.h>
#define XEDIT_LISP_PRIVATE
#include "xedit.h"

/*
 * Types
 */
typedef struct {
    XawTextPosition left, right;
    XrmQuark property;
} EntityInfo;

/*
 * Prototypes
 */
static void XeditPrint(LispMac*, Widget, LispObj*);
static void XeditInteractiveCallback(Widget, XtPointer, XtPointer);

/*
 * Initialization
 */
extern LispMac *lisp_handler;		/* Hack... */

static LispObj *Oauto_mode, *Osyntax_highlight;

/* Just to make calling interactive reparse easier */
static LispObj interactive_left, interactive_right;
static LispObj interactive_arguments[3];

static LispObj *justify_modes[4];
static LispObj *wrap_modes[3];
static LispObj *scan_types[5];
static LispObj *scan_directions[2];
static LispObj execute_stream;
static LispString execute_string;
static LispObj result_stream;
static LispString result_string;
static char result_buffer[8192];
static XawTextPropertyList **property_lists;
static Cardinal num_property_lists;

static LispBuiltin xeditbuiltins[] = {
    {LispFunction, Xedit_AddEntity, "add-entity offset length identifier"},
    {LispFunction, Xedit_AutoFill, "auto-fill &optional (value nil specified)"},
    {LispFunction, Xedit_Background, "background &optional (color nil specified)"},
    {LispFunction, Xedit_ClearEntities, "clear-entities left right"},
    {LispFunction, Xedit_ConvertPropertyList, "convert-property-list name definition"},
    {LispFunction, Xedit_Font, "font &optional (font nil specified)"},
    {LispFunction, Xedit_Foreground, "foreground &optional (color nil specified)"},
    {LispFunction, Xedit_HorizontalScrollbar, "horizontal-scrollbar &optional (state nil specified)"},
    {LispFunction, Xedit_Insert, "insert text"},
    {LispFunction, Xedit_Justification, "justification &optional (value nil specified)"},
    {LispFunction, Xedit_LeftColumn, "left-column &optional (left nil specified)"},
    {LispFunction, Xedit_Point, "point &optional (position nil specified)"},
    {LispFunction, Xedit_PointMax, "point-max"},
    {LispFunction, Xedit_PointMin, "point-min"},
    {LispFunction, Xedit_PropertyList, "property-list &optional (value nil specified)"},
    {LispFunction, Xedit_ReadText, "read-text offset length"},
    {LispFunction, Xedit_ReplaceText, "replace-text left right text"},
    {LispFunction, Xedit_RightColumn, "right-column &optional (left nil specified)"},
    {LispFunction, Xedit_Scan, "scan offset type direction &key (count 1) include"},
    {LispFunction, Xedit_SearchBackward, "search-backward string &optional ignore-case"},
    {LispFunction, Xedit_SearchForward, "search-forward string &optional ignore-case"},
    {LispFunction, Xedit_VerticalScrollbar, "vertical-scrollbar &optional (state nil specified)"},
    {LispFunction, Xedit_WrapMode, "wrap-mode &optional (value nil specified)"},

	/* This should be available from elsewhere at some time... */
    {LispFunction, Xedit_XrmStringToQuark, "xrm-string-to-quark string"},
};

/*
 * Implementation
 */
void
LispXeditInitialize(LispMac *mac)
{
    int i;
    char *string;
    LispObj *xedit, *list, *savepackage;

    justify_modes[0]	= KEYWORD("LEFT");
    justify_modes[1]	= KEYWORD("RIGHT");
    justify_modes[2]	= KEYWORD("CENTER");
    justify_modes[3]	= KEYWORD("FULL");

    wrap_modes[0]	= KEYWORD("NEVER");
    wrap_modes[1]	= KEYWORD("LINE");
    wrap_modes[2]	= KEYWORD("WORD");

    scan_types[0]	= KEYWORD("POSITIONS");
    scan_types[1]	= KEYWORD("WHITE-SPACE");
    scan_types[2]	= KEYWORD("EOL");
    scan_types[3]	= KEYWORD("PARAGRAPH");
    scan_types[4]	= KEYWORD("ALL");
    scan_types[5]	= KEYWORD("ALPHA-NUMERIC");

    scan_directions[0]	= justify_modes[0];
    scan_directions[1]	= justify_modes[1];

    /* Remember value of current package */
    savepackage = PACKAGE;

    /* Create the XEDIT package */
    xedit = LispNewPackage(mac, STRING("XEDIT"), NIL);

    /* Update list of packages */
    PACK = CONS(xedit, PACK);

    /* Temporarily switch to the XEDIT package */
    mac->pack = mac->savepack = xedit->data.package.package;
    PACKAGE = xedit;

    /* Add XEDIT builtin functions */
    for (i = 0; i < sizeof(xeditbuiltins) / sizeof(xeditbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &xeditbuiltins[i]);

    /* Create these objects in the xedit package */
    Oauto_mode		= STATIC_ATOM("AUTO-MODE");
    Osyntax_highlight	= STATIC_ATOM("SYNTAX-HIGHLIGHT");

    /*  Import symbols from the LISP and EXT packages */
    for (list = PACK; CONS_P(list); list = CDR(list)) {
	string = THESTR(CAR(list)->data.package.name);
	if (strcmp(string, "LISP") == 0 || strcmp(string, "EXT") == 0)
	    LispUsePackage(mac, CAR(list));
    }

    /* Restore previous package */
    mac->pack = savepackage->data.package.package;
    PACKAGE = savepackage;

    /* Initialize helper static objects used when executing expressions */
    execute_stream.type = LispStream_t;
    execute_stream.data.stream.source.string = &execute_string;
    execute_stream.data.stream.pathname = NIL;
    execute_stream.data.stream.type = LispStreamString;
    execute_stream.data.stream.readable = 1;
    execute_stream.data.stream.writable = 0;
    execute_string.output = 0;
    result_stream.type = LispStream_t;
    result_stream.data.stream.source.string = &result_string;
    result_stream.data.stream.pathname = NIL;
    result_stream.data.stream.type = LispStreamString;
    result_stream.data.stream.readable = 0;
    result_stream.data.stream.writable = 1;
    result_string.string = (unsigned char*)result_buffer;
    result_string.fixed = 1;
    result_string.space = sizeof(result_buffer) - 2;

    /* Initialize interactive edition function arguments */
    interactive_left.type = interactive_right.type = LispInteger_t;
    interactive_arguments[0].type = LispCons_t;
    interactive_arguments[0].data.cons.cdr = &interactive_arguments[1];
    interactive_arguments[1].type = LispCons_t;
    interactive_arguments[1].data.cons.car = &interactive_left;
    interactive_arguments[1].data.cons.cdr = &interactive_arguments[2];
    interactive_arguments[2].type = LispCons_t;
    interactive_arguments[2].data.cons.car = &interactive_right;
    interactive_arguments[2].data.cons.cdr = NIL;

    /* Load extra functions and data type definitions */
    EXECUTE("(require \"xedit\")");
}

int
XeditLispExecute(LispMac *mac, Widget output,
		 XawTextPosition left, XawTextPosition right)
{
    int alloced;
    XawTextBlock block;
    XawTextPosition position;
    unsigned char *string, *ptr;
    LispObj *result, *code, *_cod, **presult = &result;

    mac->running = 1;
    if (sigsetjmp(mac->jmp, 1) != 0) {
	mac->running = 0;
	return (0);
    }

    *presult = NIL;

    position = left;
    XawTextSourceRead(XawTextGetSource(textwindow), left, &block, right - left);
    if (block.length < right - left) {
	alloced = 1;
	string = ptr = LispMalloc(mac, right - left);
	memcpy(ptr, block.ptr, block.length);
	position = left + block.length;
	ptr += block.length;
	for (; position < right;) {
	    XawTextSourceRead(XawTextGetSource(textwindow),
			      position, &block, right - position);
	    memcpy(ptr, block.ptr, block.length);
	    position += block.length;
	    ptr += block.length;
	}
    }
    else {
	alloced = 0;
	string = (unsigned char*)block.ptr;
    }

    execute_string.string = string;
    execute_string.length = right - left;
    execute_string.input = 0;
    LispPushInput(mac, &execute_stream);
    _cod = COD;
    if ((code = LispRead(mac)) != NULL) {
	if (code == EOLIST)
	    LispDestroy(mac, "object cannot start with #\\)");
	else if (code == DOT)
	    LispDestroy(mac, "dot allowed only on lists");
	result = EVAL(code);
    }
    COD = _cod;
    LispPopInput(mac, &execute_stream);
    mac->running = 0;

    XeditPrint(mac, output, result);
    if (RETURN_COUNT) {
	int i;

	for (i = 0; i < RETURN_COUNT; i++)
	    XeditPrint(mac, output, RETURN(i));
    }
    LispUpdateResults(mac, code, result);

    if (alloced)
	LispFree(mac, string);
    LispTopLevel(mac);

    return (1);
}

static void
XeditPrint(LispMac *mac, Widget output, LispObj *object)
{
    XawTextBlock block;
    XawTextPosition position;

    result_string.length = result_string.output = 0;
    LispDoWriteObject(mac, &result_stream, object, 1);
    if (result_string.length >= sizeof(result_buffer)) {
	if (result_buffer[0] == '(')
	    memcpy(result_buffer + sizeof(result_buffer) - 5, "...)", 5);
	else
	    memcpy(result_buffer + sizeof(result_buffer) - 4, "...\n", 4);
    }
    else
	result_buffer[result_string.length++] = '\n';

    position = XawTextGetInsertionPoint(output);
    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = result_string.length;
    block.ptr = result_buffer;
    XawTextReplace(output, position, position, &block);
    XawTextSetInsertionPoint(output, position + block.length);
}

/*
 *  This function is defined here to avoid exporting all the lisp interfaces
 * to the core xedit code.
 */
void
XeditLispSetEditMode(LispMac *mac, xedit_flist_item *item)
{
    GC_ENTER();
    LispObj *syntax, *name;

    /* Don't call abort if a fatal error happens */
    mac->running = 1;
    if (sigsetjmp(mac->jmp, 1) != 0) {
	mac->running = 0;
	return;
    }

    /* Create an object that represents the buffer filename.
     * Note that the entire path is passed to the auto-mode
     * function, so that directory names may be also be used
     * when determining a file type. */
    name = STRING(item->filename);
    GC_PROTECT(name);

    /*  Call the AUTO-MODE function to check if there is a
     * syntax definition for the file being loaded */
    syntax = APPLY1(Oauto_mode, name);

    /* Don't need the name object anymore */
    GC_LEAVE();

    if (syntax != NIL) {
	Arg arg[1];
	LispObj arguments;
	XawTextPropertyList *property_list;

	/* Apply the syntax highlight to the current buffer */
	arguments.type = LispCons_t;
	arguments.data.cons.car = syntax;
	arguments.data.cons.cdr = NIL;
	LispFuncall(mac, Osyntax_highlight, &arguments, 1);

	/*  The previous call added the property list to the widget,
	 * remember it when switching sources. */
	XtSetArg(arg[0], XawNtextProperties, &property_list);
	XtGetValues(XawTextGetSink(textwindow), arg, 1);
	item->properties = property_list;

	/* Add callback for interactive changes */
	XtAddCallback(item->source, XtNpropertyCallback,
		      XeditInteractiveCallback, syntax);
    }
    else
	item->properties = NULL;

    /* Return to the toplevel and or do any cleanup */
    LispTopLevel(mac);
    mac->running = 0;
}

#define MAX_INFOS	32
/*
 *  This callback tries to do it's best in generating correct output while
 * also doing minimal work/redrawing of the screen. It probably will fail
 * for some syntax-definitions, or will just not properly repaint the
 * screen. In the later case, just press Ctrl+L.
 *  There isn't yet any command to force reparsing of some regions, and if
 * the parser becomes confused, you may need to go to a line, press a space
 * and undo, just to force it to reparse the line, and possibly some extra
 * lines until the parser thinks the display is in sync.
 *  Sometimes it will repaint a lot more of text than what is being requested
 * by this callback, this should be fixed at some time, as for certain cases
 * it is also required some redesign in the Xaw interface.
 */
static void
XeditInteractiveCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    LispMac *mac = lisp_handler;
    LispObj *syntax = (LispObj*)client_data;
    XawTextPropertyInfo *info = (XawTextPropertyInfo*)call_data;
    LispObj *result;
    XawTextAnchor *anchor;
    XawTextEntity *entity;
    XawTextPosition first, last, left, right, begin, next, tmp, position;
    int i, j;
    TextSrcObject src = (TextSrcObject)w;
    EntityInfo oinfo[MAX_INFOS], ninfo[MAX_INFOS];
    XrmQuark props[MAX_INFOS];
    int num_oinfo, num_ninfo, num_props;
    XmuScanline *clip, *oclip, *nclip;
    XmuSegment segment, *seg;

    /* Don't call abort if a fatal error happens */
    mac->running = 1;
    if (sigsetjmp(mac->jmp, 1) != 0) {
	mac->running = 0;
	return;
    }

    first = XawTextSourceScan(w, 0, XawstAll, XawsdLeft, 1, True);
    last = XawTextSourceScan(w, 0, XawstAll, XawsdRight, 1, True);

    left = info->left;
    right = left + info->block->length;

    /* Always reparse full lines */
    left = begin = XawTextSourceScan(w, left, XawstEOL, XawsdLeft, 1, False);
    right = next = XawTextSourceScan(w, right, XawstEOL, XawsdRight, 1, False);


    /*  Check properties in the modified text. If a complex nested syntax
     * table was parsed, the newline has it's default property, so, while
     * the newline has a property, backup a line to make sure everything is
     * properly parsed.
     *  Maybe should limit the number of backuped lines, but if the parsing
     * becomes noticeable slow, better to rethink the syntax definition. */
    while (left > first) {
	position = XawTextSourceScan(w, left, XawstEOL, XawsdLeft, 1, True);
	if (XawTextSourceAnchorAndEntity(w, position, &anchor, &entity))
	    left = XawTextSourceScan(w, left, XawstEOL, XawsdLeft, 2, False);
	else
	    break;
    }

    /*	While the newline after the right position has a "hidden" property,
     * keep incrementing a line to be reparsed. */
    while (right < last) {
	position = XawTextSourceScan(w, right, XawstEOL, XawsdRight, 1, True);
	if (XawTextSourceAnchorAndEntity(w, position, &anchor, &entity))
	    right = XawTextSourceScan(w, right, XawstEOL, XawsdRight, 2, False);
	else
	    break;
    }

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#define STORE_STATE(count, info, from, to)				\
    (count) = 0;							\
    if ((anchor = XawTextSourceFindAnchor(w, (from))) != NULL) {	\
	entity = anchor->entities;					\
	/* Find first entity in the region to parse */			\
	while (entity && anchor->position + entity->offset < (from))	\
	    entity = entity->next;					\
	/* Loop storing information */					\
	while (entity &&						\
	    (position = anchor->position + entity->offset) < (to)) {	\
	    (info)[(count)].left = MAX(position, (left));		\
	    position += entity->length;					\
	    (info)[(count)].right = MIN(position, (to));		\
	    (info)[(count)].property = entity->property;		\
	    /* If the changes are so complex, user need press Ctrl+L */	\
	    if (++(count) >= MAX_INFOS)					\
		break;							\
	    if ((entity = entity->next) == NULL &&			\
		(anchor = XawTextSourceNextAnchor(w, anchor)) != NULL)	\
		entity = anchor->entities;				\
	}								\
    }

    /* Remember old state */
    STORE_STATE(num_oinfo, oinfo, begin, right);


    /* Reparse the lines in the modified/edited range of text */
    interactive_arguments[0].data.cons.car = syntax;
    interactive_left.data.integer = left;
    interactive_right.data.integer = right;
    result = APPLY(Osyntax_highlight, &interactive_arguments[0]);

    /* This normally is the same value as right, but the parser may have
     * continued when the syntax table stack did not finish. */
    if (INT_P(result))
	right = result->data.integer;


    /* Check what have changed */
    STORE_STATE(num_ninfo, ninfo, begin, right);

    /* Initialize to redraw everything. */
    clip = XmuNewScanline(0, begin, right);

#define CLIP_MASK(mask, from, to)					\
    if ((from) < (to)) {						\
	segment.x1 = (from);						\
	segment.x2 = (to);						\
	XmuScanlineOrSegment((mask), &segment);				\
    }

    oclip = XmuNewScanline(0, 0, 0);
    nclip = XmuNewScanline(0, 0, 0);

#define CLIP_DEFAULT(mask, from, info, num_info)			\
    for (tmp = (from), i = 0; i < (num_info); i++) {			\
	CLIP_MASK((mask), tmp, (info)[i].left);				\
	tmp = (info)[i].right;						\
    }

    /* First generate masks of regions with the default property */
    CLIP_DEFAULT(oclip, begin, oinfo, num_oinfo);
    CLIP_DEFAULT(nclip, begin, ninfo, num_ninfo);

    /* Store unchanged region in oclip */
    XmuScanlineAnd(oclip, nclip);

    /* Don't need to redraw the region in oclip */
    XmuScanlineXor(clip, oclip);

#define LIST_PROPERTIES(prop, num_prop, info, num_info)			\
    (num_prop) = 0;							\
    for (i = 0; i < (num_info); i++) {					\
	for (j = 0; j < (num_prop); j++)				\
	    if ((prop)[j] == (info)[i].property)			\
		break;							\
	if (j == (num_prop))						\
	    (prop)[(num_prop)++] = (info)[i].property;			\
    }

    /* Prepare to generate masks of regions of text with defined properties */
    LIST_PROPERTIES(props, num_props, oinfo, num_oinfo);

#define CLIP_PROPERTY(mask, prop, info, num_info)			\
    for (j = 0; j < (num_info); j++) {					\
	if ((info)[j].property == (prop)) {				\
	    CLIP_MASK((mask), (info)[j].left, (info)[j].right);		\
	}								\
    }

    /* Only care about the old properties, new ones need to be redrawn */
    for (i = 0; i < num_props; i++) {
	XrmQuark property = props[i];

	/* Reset oclip and nclip */
	XmuScanlineXor(oclip, oclip);
	XmuScanlineXor(nclip, nclip);

	/* Generate masks */
	CLIP_PROPERTY(oclip, property, oinfo, num_oinfo);
	CLIP_PROPERTY(nclip, property, ninfo, num_ninfo);

	/* Store unchanged region in oclip */
	XmuScanlineAnd(oclip, nclip);

	/* Don't need to redraw the region in oclip */
	XmuScanlineXor(clip, oclip);
	XmuOptimizeScanline(clip);
    }

    XmuDestroyScanline(oclip);
    XmuDestroyScanline(nclip);

    /* Tell Xaw that need update some regions */
    for (seg = clip->segment; seg; seg = seg->next) {
	for (i = 0; i < src->textSrc.num_text; i++)
	    /* This really should have an exported interface... */
	    _XawTextNeedsUpdating((TextWidget)(src->textSrc.text[i]),
				  seg->x1, seg->x2 + (seg->x2 > next));
    }
    XmuDestroyScanline(clip);

    /* Return to the toplevel and or do any cleanup */
    LispTopLevel(mac);
    mac->running = 0;
}

/************************************************************************
 * Builtin functions
 ************************************************************************/
LispObj *
Xedit_AddEntity(LispMac *mac, LispBuiltin *builtin)
/*
 add-entity offset length identifier
 */
{
    LispObj *offset, *length, *identifier;

    identifier = ARGUMENT(2);
    length = ARGUMENT(1);
    offset = ARGUMENT(0);

    ERROR_CHECK_INDEX(offset);
    ERROR_CHECK_INDEX(length);
    ERROR_CHECK_INDEX(identifier);

    return (XawTextSourceAddEntity(XawTextGetSource(textwindow), 0, 0, NULL,
				   offset->data.integer,
				   length->data.integer,
				   identifier->data.integer) ? T : NIL);
}

LispObj *
Xedit_AutoFill(LispMac *mac, LispBuiltin *builtin)
/*
 auto-fill &optional (value NIL specified)
 */
{
    Arg arg[1];
    Boolean state;

    LispObj *value, *specified;

    specified = ARGUMENT(1);
    value = ARGUMENT(0);

    if (specified != NIL) {
	XtSetArg(arg[0], XtNautoFill, value == NIL ? False : True);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNautoFill, &state);
	XtGetValues(textwindow, arg, 1);
	value = state ? T : NIL;
    }

    return (value);
}

LispObj *
Xedit_Background(LispMac *mac, LispBuiltin *builtin)
/*
 background &optional (color NIL specified)
 */
{
    Pixel pixel;
    Arg arg[1];
    XrmValue from, to;

    LispObj *color, *specified;

    specified = ARGUMENT(1);
    color = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_STRING(color);

	from.size = STRLEN(color);
	from.addr = (XtPointer)THESTR(color);
	to.size = sizeof(Pixel);
	to.addr = (XtPointer)&pixel;

	if (!XtConvertAndStore(XawTextGetSink(textwindow),
			       XtRString, &from, XtRPixel, &to))
	    LispDestroy(mac, "cannot convert %s to Pixel", STROBJ(color));

	XtSetArg(arg[0], XtNbackground, pixel);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	from.size = sizeof(Pixel);
	from.addr = (XtPointer)&pixel;
	to.size = 0;
	to.addr = NULL;

	XtSetArg(arg[0], XtNbackground, &pixel);
	XtGetValues(XawTextGetSink(textwindow), arg, 1);
	/* This cannot fail */
	XtConvertAndStore(textwindow, XtRPixel, &from, XtRString, &to);

	color = STRING(to.addr);
    }

    return (color);
}

LispObj *
Xedit_ClearEntities(LispMac *mac, LispBuiltin *builtin)
/*
 clear-entities left right
 */
{
    LispObj *left, *right;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    ERROR_CHECK_INDEX(left);
    ERROR_CHECK_INDEX(right);

    XawTextSourceClearEntities(XawTextGetSource(textwindow),
			       left->data.integer,
			       right->data.integer);

    return (T);
}

LispObj *
Xedit_ConvertPropertyList(LispMac *mac, LispBuiltin *builtin)
/*
 convert-property-list name definition
 */
{
    LispObj *result;
    XawTextPropertyList *property_list;

    LispObj *name, *definition;

    definition = ARGUMENT(1);
    name = ARGUMENT(0);

    ERROR_CHECK_STRING(name);
    ERROR_CHECK_STRING(definition);

    result = NIL;
    property_list = XawTextSinkConvertPropertyList(THESTR(name),
						   THESTR(definition),
						   topwindow->core.screen,
						   topwindow->core.colormap,
						   topwindow->core.depth);

    if (property_list) {
	Cardinal i;

	for (i = 0; i < num_property_lists; i++)
	    /* Check if a new property list was created */
	    if (property_lists[i]->identifier == property_list->identifier)
		break;

	/* Remember this pointer when asked back for it */
	if (i == num_property_lists) {
	    property_lists = (XawTextPropertyList**)
		XtRealloc((XtPointer)property_lists,
			  sizeof(XawTextPropertyList) *
			  (num_property_lists + 1));
	    property_lists[num_property_lists++] = property_list;
	}
	result = SMALLINT(property_list->identifier);
    }

    return (result);
}

LispObj *
Xedit_Font(LispMac *mac, LispBuiltin *builtin)
/*
 font &optional (font NIL specified)
 */
{
    XFontStruct *font_struct;
    Arg arg[1];
    XrmValue from, to;

    LispObj *font, *specified;

    specified = ARGUMENT(1);
    font = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_STRING(font);

	from.size = STRLEN(font);
	from.addr = (XtPointer)THESTR(font);
	to.size = sizeof(XFontStruct*);
	to.addr = (XtPointer)&font_struct;

	if (!XtConvertAndStore(textwindow, XtRString, &from, XtRFontStruct, &to))
	    LispDestroy(mac, "cannot convert %s to FontStruct", STROBJ(font));

	XtSetArg(arg[0], XtNfont, font_struct);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	from.size = sizeof(XFontStruct*);
	from.addr = (XtPointer)&font_struct;
	to.size = 0;
	to.addr = NULL;

	XtSetArg(arg[0], XtNfont, &font_struct);
	XtGetValues(XawTextGetSink(textwindow), arg, 1);
	/* This cannot fail */
	XtConvertAndStore(textwindow, XtRFontStruct, &from, XtRString, &to);

	font = STRING(to.addr);
    }

    return (font);
}

LispObj *
Xedit_Foreground(LispMac *mac, LispBuiltin *builtin)
/*
 foreground &optional (color NIL specified)
 */
{
    Pixel pixel;
    Arg arg[1];
    XrmValue from, to;

    LispObj *color, *specified;

    specified = ARGUMENT(1);
    color = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_STRING(color);

	from.size = STRLEN(color);
	from.addr = (XtPointer)THESTR(color);
	to.size = sizeof(Pixel);
	to.addr = (XtPointer)&pixel;

	if (!XtConvertAndStore(XawTextGetSink(textwindow),
			       XtRString, &from, XtRPixel, &to))
	    LispDestroy(mac, "cannot convert %s to Pixel", STROBJ(color));

	XtSetArg(arg[0], XtNforeground, pixel);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	from.size = sizeof(Pixel);
	from.addr = (XtPointer)&pixel;
	to.size = 0;
	to.addr = NULL;

	XtSetArg(arg[0], XtNforeground, &pixel);
	XtGetValues(XawTextGetSink(textwindow), arg, 1);
	/* This cannot fail */
	XtConvertAndStore(textwindow, XtRPixel, &from, XtRString, &to);

	color = STRING(to.addr);
    }

    return (color);
}

LispObj *
Xedit_HorizontalScrollbar(LispMac *mac, LispBuiltin *builtin)
/*
 horizontal-scrollbar &optional (state NIL specified)
 */
{
    Arg arg[1];
    XawTextScrollMode scroll;

    LispObj *state, *specified;

    specified = ARGUMENT(1);
    state = ARGUMENT(0);

    if (specified != NIL) {
	scroll = state == NIL ? XawtextScrollNever : XawtextScrollAlways;
	XtSetArg(arg[0], XtNscrollHorizontal, scroll);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNscrollHorizontal, &scroll);
	XtGetValues(textwindow, arg, 1);
	state = scroll == XawtextScrollAlways ? T : NIL;
    }

    return (state);
}

LispObj *
Xedit_Insert(LispMac *mac, LispBuiltin *builtin)
/*
 insert text
 */
{
    XawTextPosition point = XawTextGetInsertionPoint(textwindow);
    XawTextBlock block;

    LispObj *text;

    text = ARGUMENT(0);

    ERROR_CHECK_STRING(text);
    
    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = STRLEN(text);
    block.ptr = THESTR(text);
    XawTextReplace(textwindow, point, point, &block);
    XawTextSetInsertionPoint(textwindow, point + block.length);

    return (text);
}

LispObj *
Xedit_Justification(LispMac *mac, LispBuiltin *builtin)
/*
 justification &optional (value NIL specified)
 */
{
    int i;
    Arg arg[1];
    XawTextJustifyMode justify;

    LispObj *value, *specified;

    specified = ARGUMENT(1);
    value = ARGUMENT(0);

    if (specified != NIL) {
	for (i = 0; i < 4; i++)
	    if (value == justify_modes[i])
		break;
	if (i >= 4)
	    LispDestroy(mac, "%s: argument must be "
			":LEFT, :RIGHT, :CENTER, or :FULL, not %s",
			STRFUN(builtin), STROBJ(value));
	XtSetArg(arg[0], XtNjustifyMode, (XawTextJustifyMode)i);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNjustifyMode, &justify);
	XtGetValues(textwindow, arg, 1);
	i = (int)justify;
	if (i <= 0 || i >= 4)
	    i = 0;
	value = justify_modes[i];
    }

    return (value);
}

LispObj *
Xedit_LeftColumn(LispMac *mac, LispBuiltin *builtin)
/*
 left-column &optional (left NIL specified)
 */
{
    short left;
    Arg arg[1];

    LispObj *oleft, *specified;

    specified = ARGUMENT(1);
    oleft = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_INDEX(oleft);
	if (oleft->data.integer >= 32767)
	    left = 32767;
	else
	    left = oleft->data.integer;

	XtSetArg(arg[0], XtNleftColumn, left);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNleftColumn, &left);
	XtGetValues(textwindow, arg, 1);

	oleft = SMALLINT(left);
    }

    return (oleft);
}

LispObj *
Xedit_Point(LispMac *mac, LispBuiltin *builtin)
/*
 point &optional (position NIL specified)
 */
{
    LispObj *position, *specified;

    specified = ARGUMENT(1);
    position = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_INDEX(position);
	XawTextSetInsertionPoint(textwindow, position->data.integer);
    }
    else
	position = SMALLINT(XawTextGetInsertionPoint(textwindow));

    return (position);
}

LispObj *
Xedit_PointMax(LispMac *mac, LispBuiltin *builtin)
/*
 point-max
 */
{
    return (SMALLINT(XawTextSourceScan(XawTextGetSource(textwindow), 0,
				       XawstAll, XawsdRight, 1, True)));
}

LispObj *
Xedit_PointMin(LispMac *mac, LispBuiltin *builtin)
/*
 point-min
 */
{
    return (SMALLINT(XawTextSourceScan(XawTextGetSource(textwindow), 0,
				       XawstAll, XawsdLeft, 1, True)));
}

LispObj *
Xedit_PropertyList(LispMac *mac, LispBuiltin *builtin)
/*
 property-list &optional (value NIL specified)
 */
{
    Arg arg[1];
    XawTextPropertyList *property_list;

    LispObj *value, *specified;

    specified = ARGUMENT(1);
    value = ARGUMENT(0);

    if (specified != NIL) {
	Cardinal i;
	XrmQuark quark;

	ERROR_CHECK_INDEX(value);
	property_list = NULL;
	quark = value->data.integer;
	for (i = 0; i < num_property_lists; i++)
	    if (property_lists[i]->identifier == quark) {
		property_list = property_lists[i];
		break;
	    }

	if (property_list) {
	    XtSetArg(arg[0], XawNtextProperties, property_list);
	    XtSetValues(XawTextGetSink(textwindow), arg, 1);
	}
	else
	    /* Maybe should generate an error here */
	    value = NIL;
    }
    else {
	XtSetArg(arg[0], XawNtextProperties, &property_list);
	XtGetValues(XawTextGetSink(textwindow), arg, 1);
	if (property_list)
	    value = SMALLINT(property_list->identifier);
    }

    return (value);
}

LispObj *
Xedit_ReadText(LispMac *mac, LispBuiltin *builtin)
/*
 read-text offset length
 */
{
    XawTextPosition last = XawTextSourceScan(XawTextGetSource(textwindow), 0,
					     XawstAll, XawsdRight, 1, True);
    XawTextPosition from, to, len;
    XawTextBlock block;
    char *string, *ptr;

    LispObj *offset, *length;

    length = ARGUMENT(1);
    offset = ARGUMENT(0);

    ERROR_CHECK_INDEX(offset);
    ERROR_CHECK_INDEX(length);

    from = offset->data.integer;
    to = from + length->data.integer;
    if (from > last)
	from = last;
    if (to > last)
	to = last;

    if (from == to)
	return (STRING(""));

    len = to - from;
    string = LispMalloc(mac, len);

    for (ptr = string; from < to;) {
	XawTextSourceRead(XawTextGetSource(textwindow), from, &block, to - from);
	memcpy(ptr, block.ptr, block.length);
	ptr += block.length;
	from += block.length;
    }

    return (LSTRING2(string, len));
}

LispObj *
Xedit_ReplaceText(LispMac *mac, LispBuiltin *builtin)
/*
 replace-text left right text
 */
{
    XawTextPosition last = XawTextSourceScan(XawTextGetSource(textwindow), 0,
					     XawstAll, XawsdRight, 1, True);
    XawTextPosition left, right;
    XawTextBlock block;

    LispObj *oleft, *oright, *text;

    text = ARGUMENT(2);
    oright = ARGUMENT(1);
    oleft = ARGUMENT(0);

    ERROR_CHECK_INDEX(oleft);
    ERROR_CHECK_INDEX(oright);
    ERROR_CHECK_STRING(text);

    left = oleft->data.integer;
    right = oright->data.integer;
    if (left > last)
	left = last;
    if (right > left)
	right = left;
    else if (right > last)
	right = last;

    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = STRLEN(text);
    block.ptr = THESTR(text);
    XawTextReplace(textwindow, left, right, &block);

    return (text);
}

LispObj *
Xedit_RightColumn(LispMac *mac, LispBuiltin *builtin)
/*
 right-column &optional (right NIL specified)
 */
{
    short right;
    Arg arg[1];

    LispObj *oright, *specified;

    specified = ARGUMENT(1);
    oright = ARGUMENT(0);

    if (specified != NIL) {
	ERROR_CHECK_INDEX(oright);
	if (oright->data.integer >= 32767)
	    right = 32767;
	else
	    right = oright->data.integer;

	XtSetArg(arg[0], XtNrightColumn, right);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNrightColumn, &right);
	XtGetValues(textwindow, arg, 1);

	oright = SMALLINT(right);
    }

    return (oright);
}

LispObj *
Xedit_Scan(LispMac *mac, LispBuiltin *builtin)
/*
 scan offset type direction &key (count 1) include
 */
{
    int i;
    XawTextPosition offset;
    XawTextScanType type;
    XawTextScanDirection direction;
    int count;

    LispObj *ooffset, *otype, *odirection, *ocount, *include;

    include = ARGUMENT(4);
    ocount = ARGUMENT(3);
    odirection = ARGUMENT(2);
    otype = ARGUMENT(1);
    ooffset = ARGUMENT(0);

    ERROR_CHECK_INDEX(ooffset);
    offset = ooffset->data.integer;

    for (i = 0; i < 2; i++)
	if (odirection == scan_directions[i])
	    break;
    if (i >= 2)
	LispDestroy(mac, "%s: direction must be "
		    ":LEFT or :RIGHT, not %s",
		    STRFUN(builtin), STROBJ(odirection));
    direction = (XawTextScanDirection)i;

    for (i = 0; i < 5; i++)
	if (otype == scan_types[i])
	    break;
    if (i >= 5)
	LispDestroy(mac, "%s: direction must be "
		    ":POSITIONS, :WHITE-SPACE, :EOL, "
		    ":PARAGRAPH, :ALL, or :ALPHA-NUMERIC, not %s",
		    STRFUN(builtin), STROBJ(otype));
    type = (XawTextScanType)i;

    ERROR_CHECK_INDEX(ocount);
    count = ocount->data.integer;

    offset = XawTextSourceScan(XawTextGetSource(textwindow),
			       offset, type, direction, count, include != NIL);

    return (SMALLINT(offset));
}

LispObj *
Xedit_SearchBackward(LispMac *mac, LispBuiltin *builtin)
/*
 search-backward string &optional ignore-case
 */
{
    XawTextBlock block;
    XawTextPosition position;

    LispObj *string, *ignore_case;

    ignore_case = ARGUMENT(1);
    string = ARGUMENT(0);

    ERROR_CHECK_STRING(string);

    position = XawTextGetInsertionPoint(textwindow);

    block.firstPos = (ignore_case != NIL) ? 1 : 0;
    block.format = FMT8BIT;
    block.length = STRLEN(string);
    block.ptr = THESTR(string);
    position = XawTextSourceSearch(XawTextGetSource(textwindow),
				   position, XawsdLeft, &block);

    return (position != XawTextSearchError ? SMALLINT(position) : NIL);
}

LispObj *
Xedit_SearchForward(LispMac *mac, LispBuiltin *builtin)
/*
 search-forward string &optional ignore-case
 */
{
    XawTextBlock block;
    XawTextPosition position;

    LispObj *string, *ignore_case;

    ignore_case = ARGUMENT(1);
    string = ARGUMENT(0);

    ERROR_CHECK_STRING(string);

    position = XawTextGetInsertionPoint(textwindow);

    block.firstPos = (ignore_case != NIL) ? 1 : 0;
    block.format = FMT8BIT;
    block.length = STRLEN(string);
    block.ptr = THESTR(string);
    position = XawTextSourceSearch(XawTextGetSource(textwindow),
				   position, XawsdRight, &block);

    return (position != XawTextSearchError ? SMALLINT(position) : NIL);
}

LispObj *
Xedit_VerticalScrollbar(LispMac *mac, LispBuiltin *builtin)
/*
 vertical-scrollbar &optional (state NIL specified)
 */
{
    Arg arg[1];
    XawTextScrollMode scroll;

    LispObj *state, *specified;

    specified = ARGUMENT(1);
    state = ARGUMENT(0);

    if (specified != NIL) {
	scroll = state == NIL ? XawtextScrollNever : XawtextScrollAlways;
	XtSetArg(arg[0], XtNscrollVertical, scroll);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNscrollVertical, &scroll);
	XtGetValues(textwindow, arg, 1);
	state = scroll == XawtextScrollAlways ? T : NIL;
    }

    return (state);
}

LispObj *
Xedit_WrapMode(LispMac *mac, LispBuiltin *builtin)
/*
 wrap-mode &optional (value NIL specified)
 */
{
    int i;
    Arg arg[1];
    XawTextWrapMode wrap;

    LispObj *value, *specified;

    specified = ARGUMENT(1);
    value = ARGUMENT(0);

    if (specified != NIL) {
	for (i = 0; i < 3; i++)
	    if (value == wrap_modes[i])
		break;
	if (i >= 3)
	    LispDestroy(mac, "%s: argument must be "
			":NEVER, :LINE, or :WORD, not %s",
			STRFUN(builtin), STROBJ(value));
	XtSetArg(arg[0], XtNwrap, (XawTextWrapMode)i);
	XtSetValues(textwindow, arg, 1);
    }
    else {
	XtSetArg(arg[0], XtNwrap, &wrap);
	XtGetValues(textwindow, arg, 1);
	i = (int)wrap;
	if (i <= 0 || i >= 3)
	    i = 0;
	value = wrap_modes[i];
    }

    return (value);
}

LispObj *
Xedit_XrmStringToQuark(LispMac *mac, LispBuiltin *builtin)
/*
 xrm-string-to-quark string
 */
{
    LispObj *string;

    string = ARGUMENT(0);

    ERROR_CHECK_STRING(string);

    return (SMALLINT(XrmStringToQuark(THESTR(string))));
}
