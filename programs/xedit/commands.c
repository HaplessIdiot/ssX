/* $XConsortium: commands.c,v 1.33 91/10/21 14:32:18 eswu Exp $ */

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
/* $XFree86: xc/programs/xedit/commands.c,v 1.7 1999/02/05 04:49:56 dawes Exp $ */

#include <X11/Xfuncs.h>
#include <X11/Xos.h>
#include "xedit.h"
#ifdef CRAY
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <X11/Xmu/SysUtil.h>

void ResetSourceChanged(xedit_flist_item*);
static void ResetDC(Widget, XtPointer, XtPointer);
static void SourceChanged(Widget, XtPointer, XtPointer);

static void AddDoubleClickCallback(Widget, Bool);
static Bool ReallyDoLoad(char*, char*);
static char *makeBackupName(String, String, unsigned);

extern Widget scratch, texts[3], labels[3];
static Boolean double_click = FALSE;

#define DC_UNSAVED	1
#define DC_LOADED	2
#define DC_CLOBBER	3
#define DC_KILL		4
#define DC_SAVE		5
static int dc_state;

/*	Function Name: AddDoubleClickCallback(w)
 *	Description: Adds a callback that will reset the double_click flag
 *                   to false when the text is changed.
 *	Arguments: w - widget to set callback upon.
 *                 state - If true add the callback, else remove it.
 *	Returns: none.
 */
static void
AddDoubleClickCallback(Widget w, Bool state)
{
  Arg args[1];
  static XtCallbackRec cb[] = { {NULL, NULL}, {NULL, NULL} };

  if (XtIsSubclass(w, asciiSrcObjectClass)) {
      if (state)
	  XtAddCallback(w, XtNcallback, ResetDC, NULL);
      else
	  XtRemoveCallback(w, XtNcallback, ResetDC, NULL);
  }
  else {
      if (state)
	  cb[0].callback = ResetDC;
      else
	  cb[0].callback = NULL;

      XtSetArg(args[0], XtNcallback, cb);
      XtSetValues(w, args, ONE);
  }
}
  
/*	Function Name: ResetDC
 *	Description: Resets the double click flag.
 *	Arguments: w - the text widget.
 *                 junk, garbage - *** NOT USED ***
 *	Returns: none.
 */

/* ARGSUSED */
static void
ResetDC(Widget w, XtPointer junk, XtPointer garbage)
{
  double_click = FALSE;

  AddDoubleClickCallback(w, FALSE);
}

/*ARGSUSED*/
void
QuitAction(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    DoQuit(w, NULL, NULL);
}

/*ARGSUSED*/
void
DoQuit(Widget w, XtPointer client_data, XtPointer call_data)
{
    unsigned i;
    Bool source_changed = False;

    if (!double_click || (dc_state && dc_state != DC_UNSAVED)) {
	for (i = 0; i < flist.num_itens; i++)
	    if (flist.itens[i]->flags & CHANGED_BIT) {
		source_changed = True;
		break;
	    }
    }
    if(!source_changed)
	exit(0);

    XeditPrintf("Unsaved changes. Save them, or Quit again.\n");
    Feep();
    double_click = TRUE;
    dc_state = DC_UNSAVED;
    AddDoubleClickCallback(XawTextGetSource(textwindow), True);
}

static char *
makeBackupName(String buf, String filename, unsigned len)
{
    if (app_resources.backupNamePrefix
	&& strlen(app_resources.backupNamePrefix)) {
	if (strchr(app_resources.backupNamePrefix, '/'))
	    XmuSnprintf(buf, len, "%s%s%s", app_resources.backupNamePrefix,
			filename, app_resources.backupNameSuffix);
	else {
	    char fname[BUFSIZ];
	    char *name, ch;

	    strncpy(fname, filename, sizeof(fname) - 1);
	    fname[sizeof(fname) - 1] = '\0';
	    if ((name = strrchr(fname, '/')) != NULL)
		++name;
	    else
		name = filename;
	    ch = *name;
	    *name = '\0';
	    ++name;
	    XmuSnprintf(buf, len, "%s%s%c%s%s",
			fname, app_resources.backupNamePrefix, ch, name,
			app_resources.backupNameSuffix);
	}
    }
    else
	XmuSnprintf(buf, len, "%s%s",
		    filename, app_resources.backupNameSuffix);

    return (strcmp(filename, buf) ? buf : NULL);
}
  
#if defined(USG) && !defined(CRAY)
int rename (from, to)
    char *from, *to;
{
    (void) unlink (to);
    if (link (from, to) == 0) {
        unlink (from);
        return 0;
    } else {
        return -1;
    }
}
#endif

/*ARGSUSED*/
void
SaveFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    DoSave(w, NULL, NULL);
}

/*ARGSUSED*/
void
DoSave(Widget w, XtPointer client_data, XtPointer call_data)
{
    String name = GetString(filenamewindow);
    String filename = ResolveName(name);
    char buf[BUFSIZ];
    FileAccess file_access;
    xedit_flist_item *item;
    Boolean exists;
    Widget source = XawTextGetSource(textwindow);

    if (!filename) {
	XeditPrintf("Save: Can't resolve pathname -- nothing saved.\n");
	Feep();
	return;
    }
    else if (*name == '\0') {
	XeditPrintf("Save: No filename specified -- nothing saved.\n");
	Feep();
	return;
    }
    else {
	struct stat st;

	if (stat(filename, &st) == 0 && ((st.st_mode & S_IFMT) != S_IFREG)) {
	    XmuSnprintf(buf, sizeof(buf),
			"Save: file %s is not a regular file -- nothing saved.\n",
			name);
	    XeditPrintf(buf);
	    Feep();
	    return;
	}
    }

    item = FindTextSource(NULL, filename);
    if (item != NULL && item->source != source) {
	if (!double_click || (dc_state && dc_state != DC_LOADED)) {
	    XmuSnprintf(buf, sizeof(buf),
			"Save: file %s is already loaded, "
			"Save again to unload it -- nothing saved.\n",
			name);
	    XeditPrintf(buf);
	    Feep();
	    double_click = TRUE;
	    dc_state = DC_LOADED;
	    AddDoubleClickCallback(XawTextGetSource(textwindow), True);
	    return;
	}
	KillTextSource(item);
	item = FindTextSource(source = XawTextGetSource(textwindow), NULL);
	double_click = FALSE;
	dc_state = 0;
    }
    else if (item && !(item->flags & CHANGED_BIT)) {
	if (!double_click || (dc_state && dc_state != DC_SAVE)) {
	    XeditPrintf("Save: No changes need to be saved, "
			"Save again to override.\n");
	    Feep();
	    double_click = TRUE;
	    dc_state = DC_SAVE;
	    AddDoubleClickCallback(XawTextGetSource(textwindow), True);
	    return; 
	}
	double_click = FALSE;
	dc_state = 0;
    }

    file_access = CheckFilePermissions(filename, &exists);
    if (!item || strcmp(item->filename, filename)) {
	if (file_access == WRITE_OK && exists) {
	    if (!double_click || (dc_state && dc_state != DC_CLOBBER)) {
		XmuSnprintf(buf, sizeof(buf),
			    "Save: file %s already exists, "
			    "Save again to overwrite it -- nothing saved.\n",
			    name);
		XeditPrintf(buf);
		Feep();
		double_click = TRUE;
		dc_state = DC_CLOBBER;
		AddDoubleClickCallback(XawTextGetSource(textwindow), True);
		return;
	    }
	    double_click = FALSE;
	    dc_state = 0;
	}
	if (!item)
	    item = FindTextSource(source, NULL);
    }
  
  if (app_resources.enableBackups && exists) {
    char backup_file[BUFSIZ];

    if (makeBackupName(backup_file, filename, sizeof(backup_file)) == NULL
	|| rename(filename, backup_file) != 0) {
	XmuSnprintf(buf, sizeof(buf),"error backing up file:  %s\n",
		    filename); 
      XeditPrintf(buf);
    }
  }
  
  switch( file_access = MaybeCreateFile(filename)) {
  case NO_READ:
  case READ_OK:
      XmuSnprintf(buf, sizeof(buf),
		  "File %s could not be opened for writing.\n", name);
      Feep();
      break;
  case WRITE_OK:
      if ( XawAsciiSaveAsFile(source, filename) ) {
	  int i;
	  Arg args[1];
	  char label_buf[BUFSIZ];

	  XmuSnprintf(label_buf, sizeof(label_buf),
		      "%s       Read - Write", name);
	  XtSetArg(args[0], XtNlabel, label_buf);
	  for (i = 0; i < 3; i++)
	      if (XawTextGetSource(texts[i]) == source)
		  XtSetValues(labels[i], args, 1);

	  XmuSnprintf(buf, sizeof(buf), "Saved file:  %s\n", name);

	  if (item && item->source != scratch) {
	      XtSetArg(args[0], XtNlabel, filename);
	      XtSetValues(item->sme, args, 1);

	      XtFree(item->name);
	      XtFree(item->filename);
	      item->name = XtNewString(name);
	      item->filename = XtNewString(filename);
	      item->flags = EXISTS_BIT;
	  }
	  else {
	      if (!item)
		  item = flist.itens[0];
	      XtRemoveCallback(item->source, XtNcallback, SourceChanged,
			       (XtPointer)item);
	      item->source = scratch =
		  XtVaCreateWidget("textSource", international ?
				   multiSrcObjectClass : asciiSrcObjectClass,
				   topwindow,
				   XtNtype, XawAsciiFile,
				   XtNeditType, XawtextEdit,
				   NULL, NULL);

	      ResetSourceChanged(item);

	      item = AddTextSource(source, name, filename, EXISTS_BIT,
				   file_access);
	  }
	  item->flags |= EXISTS_BIT;
	  ResetSourceChanged(item);
      }
      else {
	  XmuSnprintf(buf, sizeof(buf), "Error saving file:  %s\n",  name);
	  Feep();
      }
      break;
  default:
      XmuSnprintf(buf, sizeof(buf), "%s %s",
		  "Internal function MaybeCreateFile()",
	      "returned unexpected value.\n");
      Feep();
      break;
  }

  XeditPrintf(buf);
}

/*ARGSUSED*/
void
DoLoad(Widget w, XtPointer client_data, XtPointer call_data)
{
    (void)ReallyDoLoad(GetString(filenamewindow), ResolveName(NULL));
}

static Bool
ReallyDoLoad(char *name, char *filename)
{
    Arg args[5];
    Cardinal num_args = 0;
    char buf[BUFSIZ];
    xedit_flist_item *item;
    Widget source = XawTextGetSource(textwindow);

    if (!filename) {
	XeditPrintf("Load: Can't resolve pathname.\n");
	Feep();
	return (False);
    }
    else if (*name == '\0') {
	XeditPrintf("Load: No file specified.\n");
	Feep();
	return (False);
    }
    if ((item = FindTextSource(NULL, filename)) != NULL) {
	SwitchTextSource(item);
	return (True);
    }

    {
	Boolean exists;
	int flags;
	FileAccess file_access;

	switch( file_access = CheckFilePermissions(filename, &exists) ) {
	case NO_READ:
	    if (exists)
		XmuSnprintf(buf, sizeof(buf), "File %s, %s", name,
			"exists, and could not be opened for reading.\n");
	    else
		XmuSnprintf(buf, sizeof(buf), "File %s %s %s",  name,
			    "does not exist, and",
			"the directory could not be opened for writing.\n");

	    XeditPrintf(buf);
	    Feep();
	    return (False);
	case READ_OK:
	    XtSetArg(args[num_args], XtNeditType, XawtextRead); num_args++;
	    XmuSnprintf(buf, sizeof(buf), "File %s opened READ ONLY.\n",
			name);
	    break;
	case WRITE_OK:
	    XtSetArg(args[num_args], XtNeditType, XawtextEdit); num_args++;
	    XmuSnprintf(buf, sizeof(buf), "File %s opened read - write.\n",
			name);
	    break;
	default:
	    XmuSnprintf(buf, sizeof(buf), "%s %s",
			"Internal function MaybeCreateFile()",
		    "returned unexpected value.\n");
	    XeditPrintf(buf);
	    Feep();
	    return (False);
	}

	XeditPrintf(buf);

	if (exists) {
	    flags = EXISTS_BIT;
	    XtSetArg(args[num_args], XtNstring, filename); num_args++;
	}
	else {
	    flags = 0;
	    XtSetArg(args[num_args], XtNstring, NULL); num_args++;
	}

	source = XtVaCreateWidget("textSource", international ?
				  multiSrcObjectClass : asciiSrcObjectClass,
				  topwindow,
				  XtNtype, XawAsciiFile,
				  XtNeditType, XawtextEdit,
				  NULL, NULL);
	XtSetValues(source, args, num_args);

	item = AddTextSource(source, name, filename, flags, file_access);
	SwitchTextSource(item);
	ResetSourceChanged(item);
    }

    return (True);
}

/*	Function Name: SourceChanged
 *	Description: A callback routine called when the source has changed.
 *	Arguments: w - the text source that has changed.
 *		   client_data - xedit_flist_item associated with text buffer.
 *                 call_data - NULL is unchanged
 *	Returns: none.
 */
/*ARGSUSED*/
static void
SourceChanged(Widget w, XtPointer client_data, XtPointer call_data)
{
    xedit_flist_item *item = (xedit_flist_item*)client_data;
    static Bool first_time = True;
    Bool changed = (Bool)call_data;

    if (changed) {
	if (item->flags & CHANGED_BIT)
	    return;
	item->flags |= CHANGED_BIT;
    }
    else {
	if (item->flags & CHANGED_BIT)
	    ResetSourceChanged(item);
	return;
    }

    if (first_time) {
	if (!flist.pixmap && strlen(app_resources.changed_pixmap_name)) {
	    XrmValue from, to;

	    from.size = strlen(app_resources.changed_pixmap_name);
	    from.addr = app_resources.changed_pixmap_name;
	    to.size = sizeof(Pixmap);
	    to.addr = (XtPointer)&(flist.pixmap);

	    XtConvertAndStore(flist.popup, XtRString, &from, XtRBitmap, &to);
	}
	first_time = False;
    }

    if (flist.pixmap) {
	Arg args[1];
	Cardinal num_args;
	int i;

	num_args = 0;
	XtSetArg(args[num_args], XtNleftBitmap, flist.pixmap);	++num_args;
	XtSetValues(item->sme, args, num_args);

	for (i = 0; i < 3; i++)
	    if (XawTextGetSource(texts[i]) == item->source)
		XtSetValues(labels[i], args, num_args);
    }
}

/*	Function Name: ResetSourceChanged.
 *	Description: Sets the source changed to FALSE, and
 *                   registers a callback to set it to TRUE when
 *                   the source has changed.
 *	Arguments: item - item with widget to register the callback on.
 *	Returns: none.
 */

void
ResetSourceChanged(xedit_flist_item *item)
{
    Arg args[1];
    Cardinal num_args;
    int i;

    num_args = 0;
    XtSetArg(args[num_args], XtNleftBitmap, None);	++num_args;
    XtSetValues(item->sme, args, num_args);

    dc_state = 0;
    double_click = FALSE;
    for (i = 0; i < 3; i++) {
	if (XawTextGetSource(texts[i]) == item->source)
	    XtSetValues(labels[i], args, num_args);
	AddDoubleClickCallback(XawTextGetSource(texts[i]), False);
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNsourceChanged, False);	++num_args;
    XtSetValues(item->source, args, num_args);

    if (XtHasCallbacks(item->source, XtNcallback) != XtCallbackHasSome)
	XtAddCallback(item->source, XtNcallback, SourceChanged, (XtPointer)item);
    item->flags &= ~CHANGED_BIT;
}

/*ARGSUSED*/
void
KillFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int i;
    xedit_flist_item *item = FindTextSource(XawTextGetSource(textwindow), NULL);

    if (item->source == scratch) {
	Feep();
	return;
    }

    if (item->flags & CHANGED_BIT) {
	if (!double_click || (dc_state && dc_state != DC_KILL)) {
	    XeditPrintf("Kill: Unsaved changes. Kill again to override.\n");
	    Feep();
	    double_click = TRUE;
	    dc_state = DC_KILL;
	    AddDoubleClickCallback(XawTextGetSource(textwindow), True);
	    return;
	}
	double_click = FALSE;
	dc_state = 0;
    }
    KillTextSource(item);
}

/*ARGSUSED*/
void
FindFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    XtSetKeyboardFocus(topwindow, filenamewindow);
}

/*ARGSUSED*/
void
LoadFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (ReallyDoLoad(GetString(filenamewindow), ResolveName(NULL)))
	XtSetKeyboardFocus(topwindow, textwindow);
}

/*ARGSUSED*/
void
CancelFindFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Arg args[1];
    xedit_flist_item *item;

    Feep();
    XtSetKeyboardFocus(topwindow, textwindow);

    item = FindTextSource(XawTextGetSource(textwindow), NULL);

    if (item->source != scratch)
	XtSetArg(args[0], XtNstring, item->name);
    else
	XtSetArg(args[0], XtNstring, NULL);

    XtSetValues(filenamewindow, args, 1);
}

static Bool
IsDir(char *path, Bool feep)
{
    char lname[BUFSIZ];
    struct stat st;
    int llen;

    if ((llen = readlink(path, lname, sizeof(lname) - 1)) > 0) {
	lname[llen] = '\0';
	if (stat(lname, &st) == 0)
	    return ((st.st_mode & S_IFDIR) == S_IFDIR);
	else if (feep)
	    Feep();
    }
    else if (feep)
	Feep();

    return (False);
}

/*
 * XXX To use the code under SHOW_MATCHES, it is required to have one
 * completions buffer/window, or change the code to print the matches
 * to stderr.
 */
/*ARGSUSED*/
void
FileCompletion(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    XawTextBlock block;
    String text;
    int length;
    char **matches, *save, *dir_name, *file_name, match[257];
    unsigned n_matches, len, mlen, buflen;
    DIR *dir;
    Bool slash = False, dot = False;
#ifdef SHOW_MATCHES
#define	SM_NEVER	0
#define SM_HINT		1
#define SM_ALWAYS	2
    int show_matches;
#endif

    text = GetString(filenamewindow);

    if (!text) {
	Feep();
	return;
    }

    {
	XawTextPosition pos = XawTextGetInsertionPoint(w);
	char *cslash = strchr(&text[pos], '/'), *cdot = strchr(&text[pos], '.');

	if (cslash != NULL || cdot != NULL) {
	    if (cslash != NULL && (cdot == NULL || cdot > cslash)) {
		length = cslash - text;
		slash = True;
	    }
	    else
		length = cdot - text;
	}
	else
	    length = strlen(text);
    }

#ifdef SHOW_MATCHES
    if (*num_params == 1 && length == strlen(text)) {
	switch (params[0][0]) {
	case 'n':		/* Never */
	case 'N':
	    show_matches = SM_NEVER;
	    break;
	case 'h':		/* Hint */
	case 'H':
	    show_matches = SM_HINT;
	    break;
	case 'a':		/* Always */
	case 'A':
	    show_matches = SM_ALWAYS;
	    break;
	default:
	    show_matches = SM_NEVER;
	    XtAppWarning(XtWidgetToApplicationContext(w),
			 "Bad argument to file-completion, "
			 "must be Never, Hint or Always");
	    break;
	}
    }
    else
	show_matches = SM_NEVER;
#endif

    matches = NULL;
    n_matches = buflen = 0;
    save = XtMalloc(length + 1);
    memmove(save, text, length);
    save[length] = '\0';

    if (save[0] == '~' && save[1]) {
	char *slash2 = strchr(save, '/');
	int nlen;

	if (slash2) {
	    struct passwd *pw;
	    char home[BUFSIZ];
	    char *name;
	    int slen = strlen(save), diff = slash2 - save;

	    *slash2 = '\0';
	    name = save + 1;
	    if ((nlen = strlen(name)) != 0)
		pw = getpwnam(name);
	    else
		pw = getpwuid(getuid());

	    if (pw) {
		char fname[BUFSIZ];
		int hlen;

		strncpy(home, pw->pw_dir, sizeof(home) - 1);
		home[sizeof(home) - 1] = '\0';
		hlen = strlen(home);
		strncpy(fname, slash2 + 1, sizeof(fname) - 1);
		fname[sizeof(fname) - 1] = '\0';
		save = XtRealloc(save, slen - diff + hlen + 2);
		(void)memmove(&save[hlen], slash2, slen - diff + 1);
		(void)memmove(save, home, hlen);
		save[hlen] = '/';
		strcpy(&save[hlen + 1], fname);

		/* expand directory */
		block.length = strlen(save);
		block.ptr = save;
		block.firstPos = 0;
		block.format = FMT8BIT;
		XawTextReplace(filenamewindow, 0, length, &block);
		XawTextSetInsertionPoint(filenamewindow, block.length);
	    }
	    else
		*slash2 = '/';
	}
    }

    if ((file_name = strrchr(save, '/')) != NULL) {
	*file_name = '\0';
	++file_name;
	dir_name = save;
	if (!file_name[0])
	    slash = True;
	if (!dir_name[0])
	    dir_name = "/";
    }
    else {
	dir_name = ".";
	dot = True;
	file_name = save;
    }
    len = strlen(file_name);

    if ((dir = opendir(dir_name)) != NULL) {
	char path[BUFSIZ], *pptr;
	struct dirent *ent;
	int isdir, first = 1, bytes;

	XmuSnprintf(path, sizeof(path), "%s/", dir_name);
	pptr = path + strlen(path);
	bytes = sizeof(path) - (pptr - path) - 1;

	mlen = 0;
	match[0] = '\0';
	(void)readdir(dir);	/* "." */
	(void)readdir(dir);	/* ".." */
	while ((ent = readdir(dir)) != NULL) {
	    unsigned d_namlen = strlen(ent->d_name);

	    if (d_namlen >= len && strncmp(ent->d_name, file_name, len) == 0) {
		char *tmp = &(ent->d_name[len]), *mat = match;
		struct stat st;

		strncpy(pptr, ent->d_name, bytes);
		pptr[bytes] = '\0';
		if (stat(path, &st) != 0) {
		    Feep();
		    continue;
		}

		if (first) {
		    strncpy(match, tmp, sizeof(match) - 1);
		    match[sizeof(match) - 2] = '\0';
		    mlen = strlen(match);
		    first = 0;
		    if ((st.st_mode & S_IFLNK) == S_IFLNK)
			isdir = IsDir(path, True);
		    else
			isdir = (st.st_mode & S_IFDIR) == S_IFDIR;
		}
		else {
		    while (*tmp && *mat && *tmp++ == *mat)
			++mat;
		    if (mlen > mat - match) {
			mlen = mat - match;
			match[mlen] = '\0';
		    }
		}
#ifdef SHOW_MATCHES
		if (show_matches != SM_NEVER) {
		    Bool is_dir;

		    if ((st.st_mode & S_IFLNK) == S_IFLNK)
			is_dir = IsDir(path, False);
		    else
			is_dir = (st.st_mode & S_IFDIR) == S_IFDIR;
		    matches = (char **)XtRealloc((char*)matches, sizeof(char**)
						 * (n_matches + 1));
		    buflen += d_namlen + 1;
		    if (is_dir) {
			matches[n_matches] = XtMalloc(d_namlen + 2);
			strcpy(matches[n_matches], ent->d_name);
			strcat(matches[n_matches], "/");
			++buflen;
		    }
		    else
			matches[n_matches] = XtNewString(ent->d_name);
		}
		else
#endif
		    if (mlen == 0 && n_matches >= 1) {
			++n_matches;
			break;
		    }
		++n_matches;
	    }
	}

	closedir(dir);

	if (n_matches) {
	    Bool add_slash = n_matches == 1 && isdir && !slash;

	    if (mlen >= 1 && match[mlen - 1] == '.' && text[length] == '.')
		--mlen;
	    if (mlen || add_slash) {
		XawTextPosition pos;

		block.firstPos = 0;
		block.format = FMT8BIT;
		if (mlen) {
		    pos = XawTextGetInsertionPoint(filenamewindow);
		    block.length = mlen;
		    block.ptr = match;
		    XawTextReplace(filenamewindow, pos, pos, &block);
		    XawTextSetInsertionPoint(filenamewindow, pos + block.length);
		}
		if (add_slash) {
		    XawTextPosition actual = XawTextGetInsertionPoint(w);

		    pos = XawTextSourceScan(XawTextGetSource(w), 0, XawstAll,
					    XawsdRight, 1, True);
		    block.length = 1;
		    block.ptr = "/";
		    XawTextReplace(filenamewindow, pos, pos, &block);
		    if (actual == pos)
			XawTextSetInsertionPoint(filenamewindow, pos + 1);
		}
	    }
	    else if (n_matches != 1 || isdir) {
#ifdef SHOW_MATCHES
		if (show_matches == SM_NEVER)
#endif
		    Feep();
	    }

#ifdef SHOW_MATCHES
	    if (show_matches != SM_NEVER) {
		if (show_matches == SM_ALWAYS || n_matches != 1) {
		    unsigned i;
		    char *list = XtMalloc(buflen + 1), *ptr = list;

		    for (i = 0; i < n_matches; i++) {
			strcpy(ptr, matches[i]);
			while (*ptr)
			    ++ptr;
			*ptr = '\n';
			++ptr;
		    }
		    *ptr = '\0';
		    XeditPrintf(list);
		    XtFree(list);
		}
		while (--n_matches)
		    XtFree(matches[n_matches]);
		XtFree((char*)matches);
	    }
#endif
	}
	else
	    Feep();
    }
    else
	Feep();

    XtFree(save);
}
