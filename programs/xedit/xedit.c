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
/* $XFree86: contrib/programs/xedit/xedit.c,v 1.4 1998/10/11 11:20:00 dawes Exp $ */

#include "xedit.h"
#include <time.h>
#define randomize()	srand((unsigned)time((time_t*)NULL))

static XtActionsRec actions[] = {
{"quit", DoQuit},
{"save-file", DoSave},
{"load-file", LoadFile},
{"find-file", FindFile},
{"cancel-find-file", CancelFindFile},
{"file-completion", FileCompletion},
};

#define DEF_HINT_INTERVAL	300	/* in seconds, 5 minutes */

static Atom wm_delete_window;
static Widget hintswindow;

Widget topwindow, textwindow, messwidget, labelwindow, filenamewindow;

void ResetSourceChanged();

static void makeButtonsAndBoxes();
static void HintsTimer(XtPointer, XtIntervalId*);
static void StartHints(void);

Display *CurDpy;

struct _app_resources app_resources;

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
};

#undef Offset

void
main(argc, argv)
int argc;
char **argv;
{
  XtAppContext appcon;
  String filename = NULL;

  XtSetLanguageProc(NULL, NULL, NULL);
  topwindow = XtAppInitialize(&appcon, "Xedit", NULL, 0, &argc, argv, NULL, NULL, 0);

  XtAppAddActions(appcon, actions, XtNumber(actions));
  XtOverrideTranslations
      (topwindow, XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));
  
  XtGetApplicationResources(topwindow, (XtPointer) &app_resources, resources,
			    XtNumber(resources), NULL, 0);

  CurDpy = XtDisplay(topwindow);

  if (argc > 1) {
    Boolean exists;
    filename = argv[1];

    switch ( CheckFilePermissions(filename, &exists)) {
    case NO_READ:
	if (exists)
	    fprintf(stderr, 
		    "File %s exists, and could not opened for reading.\n", 
		    filename);
	else
	    fprintf(stderr, "File %s %s %s",  filename, "does not exist,",
		    "and the directory could not be opened for writing.\n");
	exit(1);
    case READ_OK:
    case WRITE_OK:
	makeButtonsAndBoxes(topwindow, filename);
	break;
    default:
	fprintf(stderr, "%s %s", "Internal function MaybeCreateFile()",
		"returned unexpected value.\n");
	exit(1);
    }
  }  
  else
      makeButtonsAndBoxes(topwindow, NULL);

  StartHints();
  XtRealizeWidget(topwindow);
  
  wm_delete_window = XInternAtom(XtDisplay(topwindow), "WM_DELETE_WINDOW",
				 False);
  (void) XSetWMProtocols (XtDisplay(topwindow), XtWindow(topwindow),
			  &wm_delete_window, 1);

  if (!filename)
    XtSetKeyboardFocus(topwindow, filenamewindow);
  else
    XtSetKeyboardFocus(topwindow, textwindow);

  XtAppMainLoop(appcon);
}

static void
makeButtonsAndBoxes(parent, filename)
Widget parent;
char * filename;
{
  Widget outer, b_row;
  Arg arglist[10];
  Cardinal num_args;

  outer = XtCreateManagedWidget( "paned", panedWidgetClass, parent,
				NULL, ZERO);
 
  b_row= XtCreateManagedWidget("buttons", panedWidgetClass, outer, NULL, ZERO);
  {
    MakeCommandButton(b_row, "quit", DoQuit);
    MakeCommandButton(b_row, "save", DoSave);
    MakeCommandButton(b_row, "load", DoLoad);
    filenamewindow = MakeStringBox(b_row, "filename", filename); 
  }
  hintswindow = XtCreateManagedWidget("bc_label", labelWidgetClass,
				      outer, NULL, ZERO);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNeditType, XawtextEdit); num_args++;
  messwidget = XtCreateManagedWidget("messageWindow", asciiTextWidgetClass,
				      outer, arglist, num_args);

  num_args = 0;
  if (filename != NULL) 
    XtSetArg(arglist[num_args], XtNlabel, filename); num_args++;

  labelwindow = XtCreateManagedWidget("labelWindow",labelWidgetClass, 
				      outer, arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNtype, XawAsciiFile); num_args++;
  XtSetArg(arglist[num_args], XtNeditType, XawtextEdit); num_args++;
  textwindow =  XtCreateManagedWidget("editWindow", asciiTextWidgetClass, 
				      outer, arglist, num_args);

  if (filename != NULL)
      DoLoad();
  else
      ResetSourceChanged(textwindow);
}

/*	Function Name: Feep
 *	Description: feeps the bell.
 *	Arguments: none.
 *	Returns: none.
 */

void
Feep()
{
  XBell(CurDpy, 0);
}

/*ARGSUSED*/
static
void HintsTimer(XtPointer closure, XtIntervalId *id)
{
    Arg args[1];
    xedit_hints *hints = (xedit_hints*)closure;

    hints->cur_hint = random() % hints->num_hints;

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
