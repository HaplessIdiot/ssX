/* $XConsortium: xedit.c,v 1.28 94/03/26 17:06:28 rws Exp $ */
 
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be 
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */
/* $XFree86: xc/programs/xedit/xedit.c,v 1.1 1998/10/25 07:12:17 dawes Exp $ */

#include "xedit.h"
#include <time.h>
#define randomize()	srand((unsigned)time((time_t*)NULL))

static XtActionsRec actions[] = {
{"quit", QuitAction},
{"save-file", SaveFile},
{"load-file", LoadFile},
{"find-file", FindFile},
{"cancel-find-file", CancelFindFile},
{"file-completion", FileCompletion},
{"popup-menu", PopupMenu},
{"kill-file", KillFile},
{"split-window", SplitWindow},
{"delete-window", DeleteWindow},
{"xedit-focus", XeditFocus},
{"other-window", OtherWindow},
{"switch-source", SwitchSource},
};

#define DEF_HINT_INTERVAL	300	/* in seconds, 5 minutes */

static Atom wm_delete_window;
static Widget hintswindow;

Widget topwindow, textwindow, messwidget, labelwindow, filenamewindow;
Widget scratch, hpane, vpanes[2], labels[3], texts[3];
Boolean international;

extern void ResetSourceChanged(xedit_flist_item*);

static void makeButtonsAndBoxes(Widget);
static void HintsTimer(XtPointer, XtIntervalId*);
static void StartHints(void);

Display *CurDpy;

struct _app_resources app_resources;
struct _xedit_flist flist;

#define Offset(field) XtOffsetOf(struct _app_resources, field)

static XtResource resources[] = {
   {"enableBackups", "EnableBackups", XtRBoolean, sizeof(Boolean),
         Offset(enableBackups), XtRImmediate, FALSE},
   {"backupNamePrefix", "BackupNamePrefix", XtRString, sizeof(char *),
         Offset(backupNamePrefix),XtRString, ""},
   {"backupNameSuffix", "BackupNameSuffix", XtRString, sizeof(char *),
         Offset(backupNameSuffix),XtRString, ".BAK"},
   {"hints", "Hint", XtRString, sizeof(char *),
	 Offset(hints.resource), XtRImmediate, NULL},
   {"hintsInterval", XtCInterval, XtRInt, sizeof(long),
	 Offset(hints.interval), XtRImmediate, (XtPointer)DEF_HINT_INTERVAL},
   {"changedBitmap", "Changed", XtRString, sizeof(char*),
	 Offset(changed_pixmap_name), XtRString, "dot"},
};

#undef Offset

void
main(argc, argv)
int argc;
char **argv;
{
  XtAppContext appcon;
  unsigned num_loaded = 0;

  XtSetLanguageProc(NULL, NULL, NULL);
  topwindow = XtAppInitialize(&appcon, "Xedit", NULL, 0, &argc, argv, NULL, NULL, 0);

  XtAppAddActions(appcon, actions, XtNumber(actions));
  XtOverrideTranslations
      (topwindow, XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));
  
  XtGetApplicationResources(topwindow, (XtPointer) &app_resources, resources,
			    XtNumber(resources), NULL, 0);

  CurDpy = XtDisplay(topwindow);
  XawSimpleMenuAddGlobalActions(appcon);
  XtRegisterGrabAction(PopupMenu, True, 
		       ButtonPressMask | ButtonReleaseMask,
		       GrabModeAsync, GrabModeAsync);

  makeButtonsAndBoxes(topwindow);

  StartHints();
  XtRealizeWidget(topwindow);
  
  wm_delete_window = XInternAtom(XtDisplay(topwindow), "WM_DELETE_WINDOW",
				 False);
  (void) XSetWMProtocols (XtDisplay(topwindow), XtWindow(topwindow),
			  &wm_delete_window, 1);

  if (argc > 1) {
      Boolean exists;
      xedit_flist_item *item;
      FileAccess file_access;
      char *filename;
      Widget source;
      Arg args[2];
      unsigned i, num_args;
      char buf[BUFSIZ];

      for (i = 1; i < argc; i++) {
	  num_args = 0;
	  filename = ResolveName(argv[i]);
	  if (filename == NULL || FindTextSource(NULL, filename) != NULL)
	      continue;
	  switch (file_access = CheckFilePermissions(filename, &exists)) {
	  case NO_READ:
	      if (exists)
		  XmuSnprintf(buf, sizeof(buf), "File %s, %s %s", argv[i],
			      "exists, and could not be opened for",
			      "reading.\n");
	      else
		  XmuSnprintf(buf, sizeof(buf), "File %s %s %s %s", argv[i],
			      "does not exist, and",
			      "the directory could not be opened for",
			      "writing.\n");
	      break;
	  case READ_OK:
	      XtSetArg(args[num_args], XtNeditType, XawtextRead); num_args++;
	      XmuSnprintf(buf, sizeof(buf), "File %s opened READ ONLY.\n",
			  argv[i]);
	      break;
	  case WRITE_OK:
	      XtSetArg(args[num_args], XtNeditType, XawtextEdit); num_args++;
	      XmuSnprintf(buf, sizeof(buf), "File %s opened read - write.\n",
			  argv[i]);
	      break;
	  }
	  if (file_access != NO_READ) {
	      int flags;

	      if (exists) {
		  flags = EXISTS_BIT;
		  XtSetArg(args[num_args], XtNstring, filename);num_args++;
	      }
	      else {
		  flags = 0;
		  XtSetArg(args[num_args], XtNstring, NULL);	num_args++;
	      }
	      source = XtVaCreateWidget("textSource", international ?
					multiSrcObjectClass
					: asciiSrcObjectClass, topwindow,
					XtNtype, XawAsciiFile,
					XtNeditType, XawtextEdit,
					NULL, NULL);
	      XtSetValues(source, args, num_args);
	      item = AddTextSource(source, argv[i], filename,
				   flags, file_access);
	      if (!num_loaded)
		  SwitchTextSource(item);
	      ++num_loaded;
	      ResetSourceChanged(item);
	  }
	  XeditPrintf(buf);
      }
  }

  if (num_loaded == 0)
      XtSetKeyboardFocus(topwindow, filenamewindow);
  else
      XtSetKeyboardFocus(topwindow, textwindow);

  XtAppMainLoop(appcon);
}

static void
makeButtonsAndBoxes(Widget parent)
{
  Widget outer, b_row;
  Arg arglist[10];
  Cardinal num_args;
  xedit_flist_item *item;
  static char *labelWindow = "labelWindow", *editWindow = "editWindow";

  outer = XtCreateManagedWidget( "paned", panedWidgetClass, parent,
				NULL, ZERO);
 
  b_row= XtCreateManagedWidget("buttons", panedWidgetClass, outer, NULL, ZERO);
  {
    MakeCommandButton(b_row, "quit", DoQuit);
    MakeCommandButton(b_row, "save", DoSave);
    MakeCommandButton(b_row, "load", DoLoad);
    filenamewindow = MakeStringBox(b_row, "filename", NULL);
  }
  hintswindow = XtCreateManagedWidget("bc_label", labelWidgetClass,
				      outer, NULL, ZERO);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNeditType, XawtextEdit); num_args++;
  messwidget = XtCreateManagedWidget("messageWindow", asciiTextWidgetClass,
				      outer, arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNorientation, XtorientHorizontal); num_args++;
  hpane = XtCreateManagedWidget("hpane", panedWidgetClass, outer,
				arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNorientation, XtorientVertical); num_args++;
  vpanes[0] = XtCreateManagedWidget("vpane", panedWidgetClass, hpane,
				    arglist, num_args);
  XtSetArg(arglist[num_args], XtNheight, 1);	++num_args;
  XtSetArg(arglist[num_args], XtNwidth, 1);	++num_args;
  vpanes[1] = XtCreateWidget("vpane", panedWidgetClass, hpane,
			     arglist, num_args);
  
  labelwindow = XtCreateManagedWidget(labelWindow,labelWidgetClass, 
				      vpanes[0], NULL, 0);
  labels[0] = labelwindow;
  labels[2] = XtCreateWidget(labelWindow,labelWidgetClass, 
			     vpanes[1], NULL, 0);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNtype, XawAsciiFile); num_args++;
  XtSetArg(arglist[num_args], XtNeditType, XawtextEdit); num_args++;
  textwindow =  XtCreateManagedWidget(editWindow, asciiTextWidgetClass, 
				      vpanes[0], arglist, num_args);
  num_args = 0;
  XtSetArg(arglist[num_args], XtNinternational, &international); ++num_args;
  XtGetValues(textwindow, arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNtype, XawAsciiFile); num_args++;
  XtSetArg(arglist[num_args], XtNeditType, XawtextEdit); num_args++;
  scratch = XtVaCreateWidget("textSource", international ?
			     multiSrcObjectClass
			     : asciiSrcObjectClass, topwindow,
			     XtNtype, XawAsciiFile,
			     XtNeditType, XawtextEdit,
			     NULL, NULL);
  XtSetValues(scratch, arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNtextSource, scratch); ++num_args;
  XtSetValues(textwindow, arglist, num_args);

  texts[0] = textwindow;
  num_args = 0;
  XtSetArg(arglist[num_args], XtNtextSource, scratch); ++num_args;
  XtSetArg(arglist[num_args], XtNdisplayCaret, False); ++num_args;
  texts[2] = XtCreateWidget(editWindow, asciiTextWidgetClass,
			    vpanes[1], arglist, num_args);

  labels[1] = XtCreateWidget(labelWindow,labelWidgetClass, 
			     vpanes[0], NULL, 0);

  texts[1] = XtCreateWidget(editWindow, asciiTextWidgetClass,
			    vpanes[0], arglist, num_args);

  item = AddTextSource(scratch, "*scratch*", "*scratch*",
		       0, WRITE_OK);
  ResetSourceChanged(item);
}

/*	Function Name: Feep
 *	Description: feeps the bell.
 *	Arguments: none.
 *	Returns: none.
 */

void
Feep(void)
{
  XBell(CurDpy, 0);
}

/*ARGSUSED*/
static
void HintsTimer(XtPointer closure, XtIntervalId *id)
{
    Arg args[1];
    xedit_hints *hints = (xedit_hints*)closure;

    hints->cur_hint = rand() % hints->num_hints;

    XtSetArg(args[0], XtNlabel, hints->hints[hints->cur_hint]);
    XtSetValues(hintswindow, args, 1);

    hints->timer = XtAppAddTimeOut(XtWidgetToApplicationContext(topwindow),
				   hints->interval, HintsTimer, closure);
}

#define MAX_HINT_LEN		255
#define MIN_HINT_INTERVAL	5
static
void StartHints(void)
{
    char *str, *p;
    unsigned i, len;
    xedit_hints *hints = &(app_resources.hints);

    /* if resource was not set, or was overriden */
    if (hints->resource == NULL || !*hints->resource)
	return;

    randomize();

    if (hints->interval < MIN_HINT_INTERVAL)
	hints->interval = DEF_HINT_INTERVAL;
    hints->interval *= 1000;
    hints->hints = (char**)XtMalloc(sizeof(char*));
    hints->hints[hints->cur_hint = 0] = p = hints->resource;
    hints->num_hints = 1;

    while ((p = strchr(p, '\n')) != NULL) {
	if (*++p == '\0')
	    break;
	hints->hints = (char**)
	    XtRealloc((char*)hints->hints,
		      sizeof(char*) * (hints->num_hints + 1));
	hints->hints[hints->num_hints++] = p;
    }

    /* make a private copy of the resource values, so that one can change
     * the Xrm database safely.
     */
    for (i = 0; i < hints->num_hints; i++) {
	if ((p = strchr(hints->hints[i], '\n')) != NULL)
	    len = p - hints->hints[i];
	else
	    len = strlen(hints->hints[i]);
	if (len > MAX_HINT_LEN)
	    len = MAX_HINT_LEN;
	str = XtMalloc(len + 1);
	strncpy(str, hints->hints[i], len);
	str[len] = '\0';
	hints->hints[i] = str;
    }

    hints->timer = XtAppAddTimeOut(XtWidgetToApplicationContext(topwindow),
				   hints->interval, HintsTimer,
				   (XtPointer)hints);
}
