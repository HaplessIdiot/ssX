/* $XConsortium: TextSrc.c,v 1.15 94/04/17 20:13:14 kaleb Exp $ */
/*

Copyright (c) 1989, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

/* $XFree86: xc/lib/Xaw/TextSrc.c,v 1.3 1998/06/28 12:32:23 dawes Exp $ */

/*
 * Author:  Chris Peterson, MIT X Consortium.
 * Much code taken from X11R3 String and Disk Sources.
 */

#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xaw/TextSrcP.h>
#include <X11/Xaw/XawInit.h>
#include "XawI18n.h"
#include "Private.h"

/*
 * Class Methods
 */
static Boolean ConvertSelection(Widget, Atom*, Atom*, Atom*, XtPointer*,
				unsigned long*, int*);
static XawTextPosition Read(Widget, XawTextPosition, XawTextBlock*, int);
static int  Replace(Widget, XawTextPosition, XawTextPosition, XawTextBlock*);
static  XawTextPosition Scan(Widget, XawTextPosition, XawTextScanType,
			      XawTextScanDirection, int, Bool);
static XawTextPosition Search(Widget, XawTextPosition, XawTextScanDirection,
			      XawTextBlock*);
static void SetSelection(Widget, XawTextPosition, XawTextPosition, Atom);
static void XawTextSrcClassInitialize(void);
static void XawTextSrcClassPartInitialize(WidgetClass);

/*
 * Prototypes
 */
static void CvtStringToEditMode(XrmValuePtr, Cardinal*,
				 XrmValuePtr, XrmValuePtr);
static Boolean CvtEditModeToString(Display*, XrmValuePtr, Cardinal*,
				   XrmValuePtr, XrmValuePtr, XtPointer*);

/*
 * External
 */
XrmQuark _XawTextFormat(TextWidget);
wchar_t *_XawTextMBToWC(Display*, char*, int*);
char *_XawTextWCToMB(Display*, wchar_t*, int*);

/*
 * Initialization
 */
#define offset(field) XtOffsetOf(TextSrcRec, textSrc.field)
static XtResource resources[] = {
  {
    XtNeditType,
    XtCEditType,
    XtREditMode,
    sizeof(XawTextEditType),
    offset(edit_mode),
    XtRString,
    "read"
  },
};

#define Superclass	(&objectClassRec)
TextSrcClassRec textSrcClassRec = {
  /* object */
  {
    (WidgetClass)Superclass,		/* superclass */
    "TextSrc",				/* class_name */
    sizeof(TextSrcRec),			/* widget_size */
    XawTextSrcClassInitialize,		/* class_initialize */
    XawTextSrcClassPartInitialize,	/* class_part_initialize */
    False,				/* class_inited */
    NULL,				/* initialize */
    NULL,				/* initialize_hook */
    NULL,				/* realize */
    NULL,				/* actions */
    0,					/* num_actions */
    resources,				/* resources */
    XtNumber(resources),		/* num_resources */
    NULLQUARK,				/* xrm_class */
    False,				/* compress_motion */
    False,				/* compress_exposure */
    False,				/* compress_enterleave */
    False,				/* visible_interest */
    NULL,				/* destroy */
    NULL,				/* resize */
    NULL,				/* expose */
    NULL,				/* set_values */
    NULL,				/* set_values_hook */
    NULL,				/* set_values_almost */
    NULL,				/* get_values_hook */
    NULL,				/* accept_focus */
    XtVersion,				/* version */
    NULL,				/* callback_private */
    NULL,				/* tm_table */
    NULL,				/* query_geometry */
    NULL,				/* display_accelerator */
    NULL,				/* extension */
  },
  /* text_src */
  {
    Read,				/* Read */
    Replace,				/* Replace */
    Scan,				/* Scan */
    Search,				/* Search */
    SetSelection,			/* SetSelection */
    ConvertSelection,			/* ConvertSelection */
  },
};

WidgetClass textSrcObjectClass = (WidgetClass)&textSrcClassRec;

static XrmQuark QRead, QAppend, QEdit;

/*
 * Implementation
 */
static void 
XawTextSrcClassInitialize(void)
{
  XawInitializeWidgetSet();

    QRead   = XrmPermStringToQuark(XtEtextRead);
    QAppend = XrmPermStringToQuark(XtEtextAppend);
    QEdit   = XrmPermStringToQuark(XtEtextEdit);
    XtAddConverter(XtRString, XtREditMode,   CvtStringToEditMode,   NULL, 0);
  XtSetTypeConverter(XtREditMode, XtRString, CvtEditModeToString, NULL, 0,
		     XtCacheNone, NULL);
}

static void
XawTextSrcClassPartInitialize(WidgetClass wc)
{
  TextSrcObjectClass t_src, superC;

  t_src = (TextSrcObjectClass)wc;
  superC = (TextSrcObjectClass)t_src->object_class.superclass;

  /*
 * We don't need to check for null super since we'll get to TextSrc
   * eventually
 */
    if (t_src->textSrc_class.Read == XtInheritRead) 
      t_src->textSrc_class.Read = superC->textSrc_class.Read;

    if (t_src->textSrc_class.Replace == XtInheritReplace) 
      t_src->textSrc_class.Replace = superC->textSrc_class.Replace;

    if (t_src->textSrc_class.Scan == XtInheritScan) 
      t_src->textSrc_class.Scan = superC->textSrc_class.Scan;

    if (t_src->textSrc_class.Search == XtInheritSearch) 
      t_src->textSrc_class.Search = superC->textSrc_class.Search;

    if (t_src->textSrc_class.SetSelection == XtInheritSetSelection) 
      t_src->textSrc_class.SetSelection = superC->textSrc_class.SetSelection;

    if (t_src->textSrc_class.ConvertSelection == XtInheritConvertSelection) 
      t_src->textSrc_class.ConvertSelection =
	                               superC->textSrc_class.ConvertSelection;
}

/*
 * Function:
 *	Read
 *
 * Parameters:
 *	w      - TextSrc Object
 *	pos    - position of the text to retreive
 *	text   - text block that will contain returned text
 *	length - maximum number of characters to read
 *
 * Description:
 *	This function reads the source.
 */
/*ARGSUSED*/
static XawTextPosition
Read(Widget w, XawTextPosition pos, XawTextBlock *text, int length)
{
  return ((XawTextPosition)0);
}

/*
 * Function:
 *	Replace
 *
 * Parameters:
 *	src	 - Text Source Object
 *	startPos - ends of text that will be removed
 *	endPos	 - ""
 *	text	 - new text to be inserted into buffer at startPos
 *
 * Description:
 *	Replaces a block of text with new text.
 */
/*ARGSUSED*/
static int 
Replace(Widget w, XawTextPosition startPos, XawTextPosition endPos,
	XawTextBlock *text)
{
  return (XawEditError);
}

/*
 * Function:
 *	Scan
 *
 * Parameters:
 *	w	 - TextSrc Object
 *	position - position to start scanning
 *	type	 - type of thing to scan for
 *	dir	 - direction to scan
 *	count	 - which occurance if this thing to search for
 *                 include - whether or not to include the character found in
 *		   the position that is returned
 *
 * Description:
 *	Scans the text source for the number and type of item specified.
 */
/*ARGSUSED*/
static XawTextPosition 
Scan(Widget w, XawTextPosition position, XawTextScanType type,
     XawTextScanDirection dir, int count, Bool include)
{
  return ((XawTextPosition)0);
}

/*
 * Function:
 *	Search
 *
 * Parameters:
 *	w	 - TextSource Object
 *	position - position to start searching
 *	dir	 - direction to search
 *	text	 - the text block to search for
 *
 * Description:
 *	Searchs the text source for the text block passed
 */
/*ARGSUSED*/
static XawTextPosition
Search(Widget w, XawTextPosition position, XawTextScanDirection dir,
       XawTextBlock *text)
{
  return (XawTextSearchError);
}

/*ARGSUSED*/
static Boolean
ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type,
		 XtPointer *value, unsigned long *length, int *format)
{
  return (False);
}

/*ARGSUSED*/
static void
SetSelection(Widget w, XawTextPosition left, XawTextPosition right,
	     Atom selection)
{
}

/*ARGSUSED*/
static void 
CvtStringToEditMode(XrmValuePtr args, Cardinal *num_args,
		    XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextEditType editType;
   XrmQuark    q;
  char name[7];
 
  XmuNCopyISOLatin1Lowered(name, (char *)fromVal->addr, sizeof(name));
  q = XrmStringToQuark(name);

  if (q == QRead)
    editType = XawtextRead;
  else if (q == QAppend)
    editType = XawtextAppend;
  else if (q == QEdit)
    editType = XawtextEdit;
  else
    {
      toVal->size = 0;
      toVal->addr = NULL;
    XtStringConversionWarning((char *)fromVal->addr, XtREditMode);
    }
  toVal->size = sizeof(XawTextEditType);
  toVal->addr = (XPointer)&editType;
}

/*ARGSUSED*/
static Boolean
CvtEditModeToString(Display *dpy, XrmValuePtr args, Cardinal *num_args,
		    XrmValuePtr fromVal, XrmValuePtr toVal,
		    XtPointer *data)
{
  static String buffer;
  Cardinal size;

  switch (*(XawTextEditType *)fromVal->addr)
    {
    case XawtextAppend:
    case XawtextRead:
      buffer = XtEtextRead;
      break;
      buffer = XtEtextAppend;
      break;
    case XawtextEdit:
      buffer = XtEtextEdit;
      break;
    default:
      XawTypeToStringWarning(dpy, XtREditMode);
      toVal->addr = NULL;
      toVal->size = 0;
      return (False);
    }

  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
    {
      if (toVal->size < size)
	{
	  toVal->size = size;
	  return (False);
	}
      strcpy((char *)toVal->addr, buffer);
    }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}

/*
 * Public Functions
 */
static String bad_subclass =
"XawTextSourceReplace's 1st parameter must be subclass of asciiSrc.";

/*
 * Function:
 *	XawTextSourceRead
 *
 * Parameters:
 *	w      - TextSrc Object
 *	pos    - position of the text to retrieve
 *	text   - text block that will contain returned text (return)
 *	length - maximum number of characters to read
 *
 * Description:
 *	This function reads the source.
 *
 * Returns:
 *	The number of characters read into the buffer
 */
XawTextPosition
XawTextSourceRead(Widget w, XawTextPosition pos, XawTextBlock *text,
		  int length)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass)w->core.widget_class;

  if (!XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  return ((*cclass->textSrc_class.Read)(w, pos, text, length));
}

/*
 * Function:
 *	XawTextSourceReplace
 *
 * Parameters:
 *	src	 - Text Source Object
 *	startPos - ends of text that will be removed
 *	endPos	 - ""
 *	text	 - new text to be inserted into buffer at startPos
 *
 * Description:
 *	Replaces a block of text with new text.
 *
 * Returns:
 *	XawEditError or XawEditDone.
 */
/*ARGSUSED*/
int
XawTextSourceReplace(Widget w, XawTextPosition startPos,
		      XawTextPosition endPos, XawTextBlock *text)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass) w->core.widget_class;

  if (!XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  return ((*cclass->textSrc_class.Replace)(w, startPos, endPos, text));
}

/*
 * Function:
 *	XawTextSourceScan
 *
 * Parameters:
 *	w	 - TextSrc Object
 *	position - position to start scanning
 *	type	 - type of thing to scan for
 *	dir	 - direction to scan
 *	count	 - which occurance if this thing to search for
 *                 include - whether or not to include the character found in
 *                           the position that is returned. 
 *
 * Description:
 *	Scans the text source for the number and type of item specified.
 *
 * Returns:
 *	The position of the text
 */
XawTextPosition
XawTextSourceScan(Widget w, XawTextPosition position,
		  XawTextScanType type, XawTextScanDirection dir,
		  int count, Bool include)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass)w->core.widget_class;

  if ( !XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  return ((*cclass->textSrc_class.Scan)
	  (w, position, type, dir, count, include));
}

/*
 * Function:
 *	XawTextSourceSearch
 *
 * Parameters:
 *	w	 - TextSource Object
 *	position - position to start scanning
 *	dir	 - direction to scan
 *                 text - the text block to search for.
 *
 * Returns:
 *	The position of the text we are searching for or XawTextSearchError.
 *
 * Description:
 *	Searchs the text source for the text block passed
 */
XawTextPosition 
XawTextSourceSearch(Widget w, XawTextPosition position,
		    XawTextScanDirection dir, XawTextBlock *text)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass)w->core.widget_class;

  if (!XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  return ((*cclass->textSrc_class.Search)(w, position, dir, text));
}

/*
 * Function:
 *	XawTextSourceConvertSelection
 *
 * Parameters:
 *	w	  - TextSrc object
 *	selection - current selection atom
 *	target	  - current target atom
 *	type	  - type to conver the selection to
 *	value	  - return value that has been converted
 *	length	  - ""
 *	format	  - format of the returned value
 *
 * Returns:
 *	True if the selection has been converted
 */
Boolean
XawTextSourceConvertSelection(Widget w, Atom *selection, Atom *target, 
			      Atom *type, XtPointer *value,
			      unsigned long *length, int *format)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass)w->core.widget_class;

  if ( !XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  return((*cclass->textSrc_class.ConvertSelection)
	 (w, selection, target, type, value, length, format));
}

/*
 * Function:
 *	XawTextSourceSetSelection
 *
 * Parameters:
 *	w	  - TextSrc object
 *	left	  - bounds of the selection
 *	rigth	  - ""
 *	selection - selection atom
 *
 * Description:
 *	Allows special setting of the selection.
 */
void
XawTextSourceSetSelection(Widget w, XawTextPosition left, 
			  XawTextPosition right, Atom selection)
{
  TextSrcObjectClass cclass = (TextSrcObjectClass)w->core.widget_class;

  if (!XtIsSubclass(w, textSrcObjectClass))
    XtErrorMsg("bad argument", "textSource", "XawError", bad_subclass,
		   NULL, NULL);

  (*cclass->textSrc_class.SetSelection)(w, left, right, selection);
}

/*
 * External Functions for Multi Text
 */
/*
 * TextFormat(): 
 *	returns the format of text: FMT8BIT or FMTWIDE
 */
XrmQuark
_XawTextFormat(TextWidget tw)
{
  return (((TextSrcObject)(tw->text.source))->textSrc.text_format);
}

/* _XawTextWCToMB():
 *	Convert the wchar string to external encoding
 *	The caller is responsible for freeing both the source and ret string
 *
 *	wstr	   - source wchar string
 * len_in_out - lengh of string.
 *		     As In, length of source wchar string, measured in wchar
 *		     As Out, length of returned string
 */
char *
_XawTextWCToMB(Display *d, wchar_t *wstr, int *len_in_out)
{
    XTextProperty textprop;

    if (XwcTextListToTextProperty(d, (wchar_t**)&wstr, 1,
				XTextStyle, &textprop) < Success)
    {
      XtWarningMsg("convertError", "textSource", "XawError",
                 "Non-character code(s) in buffer.", NULL, NULL);
      *len_in_out = 0;
      return (NULL);
    }
    *len_in_out = textprop.nitems;

  return ((char *)textprop.value);
}

/* _XawTextMBToWC():
 *	Convert the string to internal processing codeset WC.
 *   The caller is responsible for freeing both the source and ret string.
 * 
 *	str	   - source string
 *	len_in_out - lengh of string
 *		     As In, it is length of source string
 *		     As Out, it is length of returned string, measured in wchar
 */
wchar_t *
_XawTextMBToWC(Display *d, char *str, int *len_in_out)
{
    XTextProperty textprop;
    char *buf;
    wchar_t **wlist, *wstr;
    int count;

  if (*len_in_out == 0)
    return (NULL);

    buf = XtMalloc(*len_in_out + 1);

    strncpy(buf, str, *len_in_out);
    *(buf + *len_in_out) = '\0';
  if (XmbTextListToTextProperty(d, &buf, 1, XTextStyle, &textprop) != Success)
    {
	XtWarningMsg("convertError", "textSource", "XawError",
		 "No Memory, or Locale not supported.", NULL, NULL);
	XtFree(buf);
	*len_in_out = 0;
	return (NULL);
    }

    XtFree(buf);
    if (XwcTextPropertyToTextList(d, &textprop,
				(wchar_t***)&wlist, &count) != Success)
    {
	XtWarningMsg("convertError", "multiSourceCreate", "XawError",
		 "Non-character code(s) in source.", NULL, NULL);
	*len_in_out = 0;
	return (NULL);
    }
    wstr = wlist[0];
    *len_in_out = wcslen(wstr);
    XtFree((XtPointer)wlist);

  return (wstr);
}
