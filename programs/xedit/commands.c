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
/* $XFree86: xc/programs/xedit/commands.c,v 1.2 1998/10/25 11:56:54 dawes Exp $ */

#include <X11/Xos.h>
#include <X11/Xfuncs.h>
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

extern Widget topwindow, textwindow, labelwindow, filenamewindow;

void ResetSourceChanged();

static void ResetDC();

static char *makeBackupName(String, String, unsigned);
static Bool ReallyDoLoad(void);

static Boolean double_click = FALSE, source_changed = FALSE;

/*	Function Name: AddDoubleClickCallback(w)
 *	Description: Adds a callback that will reset the double_click flag
 *                   to false when the text is changed.
 *	Arguments: w - widget to set callback upon.
 *                 state - If true add the callback, else remove it.
 *	Returns: none.
 */

AddDoubleClickCallback(w, state)
Widget w;
Boolean state;
{
  Arg args[1];
  static XtCallbackRec cb[] = { {NULL, NULL}, {NULL, NULL} };
 
  if (state) 
    cb[0].callback = ResetDC;
  else
    cb[0].callback = NULL;

  XtSetArg(args[0], XtNcallback, cb);
  XtSetValues(w, args, ONE);
}
  
/*	Function Name: ResetDC
 *	Description: Resets the double click flag.
 *	Arguments: w - the text widget.
 *                 junk, garbage - *** NOT USED ***
 *	Returns: none.
 */

/* ARGSUSED */
static void
ResetDC(w, junk, garbage)
Widget w;
XtPointer junk, garbage;
{
  double_click = FALSE;

  AddDoubleClickCallback(w, FALSE);
}

void
DoQuit()
{
  if( double_click || !source_changed ) 
    exit(0); 

  XeditPrintf("Unsaved changes. Save them, or Quit again.\n");
  Feep();
  double_click = TRUE;
  AddDoubleClickCallback(textwindow, TRUE);
}

static char *
makeBackupName(String buf, String filename, unsigned len)
{
  XmuSnprintf(buf, len, "%s%s%s", app_resources.backupNamePrefix,
	  filename, app_resources.backupNameSuffix);
  return (buf);
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

void
DoSave()
{
  String filename = GetString(filenamewindow);
  char buf[BUFSIZ];

  if( (filename == NULL) || (strlen(filename) == 0) ){
    XeditPrintf("Save:  no filename specified -- nothing saved\n");
    Feep();
    return;
  }
  
  if (app_resources.enableBackups) {
    char backup_file[BUFSIZ];
    makeBackupName(backup_file, filename, sizeof(backup_file));

    if (rename(filename, backup_file) != 0) {
	XmuSnprintf(buf, sizeof(buf),"error backing up file:  %s\n",
		    backup_file); 
      XeditPrintf(buf);
    }
  }
  
  switch( MaybeCreateFile(filename)) {
  case NO_READ:
  case READ_OK:
      XmuSnprintf(buf, sizeof(buf),
		  "File %s could not be opened for writing.\n", filename);
      Feep();
      break;
  case WRITE_OK:
      if ( XawAsciiSaveAsFile(XawTextGetSource(textwindow), filename) ) {
	  Arg args[1];
	  char label_buf[BUFSIZ];

	  XmuSnprintf(buf, sizeof(buf), "Saved file:  %s\n", filename);
	  XmuSnprintf(label_buf, sizeof(label_buf), "%s       Read - Write",
		      filename);
	  ResetSourceChanged(textwindow);
	  XtSetArg(args[0], XtNlabel, label_buf);
	  XtSetValues(labelwindow, args, 1);
      }
      else {
	  XmuSnprintf(buf, sizeof(buf), "Error saving file:  %s\n",  filename);
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

void
DoLoad()
{
    (void)ReallyDoLoad();
}

static Bool
ReallyDoLoad(void)
{
    Arg args[5];
    Cardinal num_args = 0;
    String filename = GetString(filenamewindow);
    char buf[BUFSIZ], label_buf[BUFSIZ];

    if ( source_changed && !double_click) {
	XeditPrintf("Unsaved changes. Save them, or press Load again.\n");
	Feep();
	double_click = TRUE;
	AddDoubleClickCallback(textwindow, TRUE);
	return (False);
    }
    double_click = FALSE;
    
    if ( (filename != NULL)  &&  ((int) strlen(filename) > 0) ) {
	Boolean exists;

	switch( CheckFilePermissions(filename, &exists) ) {
	case NO_READ:
	    if (exists)
		XmuSnprintf(buf, sizeof(buf), "File %s, %s", filename,
			"exists, and could not be opened for reading.\n");
	    else
		XmuSnprintf(buf, sizeof(buf), "File %s %s %s",  filename,
			    "does not exist, and",
			"the directory could not be opened for writing.\n");

	    XeditPrintf(buf);
	    Feep();
	    return (False);
	case READ_OK:
	    XtSetArg(args[num_args], XtNeditType, XawtextRead); num_args++;
	    XmuSnprintf(label_buf, sizeof(label_buf), "%s       READ ONLY",
			filename);
	    XmuSnprintf(buf, sizeof(buf), "File %s opened READ ONLY.\n",
			filename);
	    break;
	case WRITE_OK:
	    XtSetArg(args[num_args], XtNeditType, XawtextEdit); num_args++;
	    XmuSnprintf(label_buf, sizeof(label_buf), "%s       Read - Write",
			filename);
	    XmuSnprintf(buf, sizeof(buf), "File %s opened read - write.\n",
			filename);
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
	    XtSetArg(args[num_args], XtNstring, filename); num_args++;
	}
	else {
	    XtSetArg(args[num_args], XtNstring, NULL); num_args++;
	}

	XtSetValues( textwindow, args, num_args);
	
	num_args = 0;
	XtSetArg(args[num_args], XtNlabel, label_buf); num_args++;
	XtSetValues( labelwindow, args, num_args);
	ResetSourceChanged(textwindow);
	return (True);
    }

    XeditPrintf("Load: No file specified.\n");
    Feep();
    return (False);
}

/*	Function Name: SourceChanged
 *	Description: A callback routine called when the source has changed.
 *	Arguments: w - the text source that has changed.
 *                 junk, garbage - *** UNUSED ***.
 *	Returns: none.
 */

static void
SourceChanged(w, junk, garbage)
Widget w;
XtPointer junk, garbage;
{
    XtRemoveCallback(w, XtNcallback, SourceChanged, NULL);
    source_changed = TRUE;
}

/*	Function Name: ResetSourceChanged.
 *	Description: Sets the source changed to FALSE, and
 *                   registers a callback to set it to TRUE when
 *                   the source has changed.
 *	Arguments: widget - widget to register the callback on.
 *	Returns: none.
 */

void
ResetSourceChanged(widget)
Widget widget;
{
    XtAddCallback(XawTextGetSource(widget), XtNcallback, SourceChanged, NULL);
    source_changed = FALSE;
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
    if (ReallyDoLoad())
	XtSetKeyboardFocus(topwindow, textwindow);
}

/*ARGSUSED*/
void
CancelFindFile(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Feep();
    XtSetKeyboardFocus(topwindow, textwindow);
}

static Bool
IsDir(char *dir_name, char *file_name, Bool feep)
{
    char lname[BUFSIZ], path[BUFSIZ];
    struct stat st;
    int llen;

    XmuSnprintf(path, sizeof(path), "%s/%s", dir_name, file_name);

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
    length = strlen(text);

    if (!text) {
	Feep();
	return;
    }

#ifdef SHOW_MATCHES
    if (*num_params == 1) {
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
	struct dirent *ent;
	int isdir, first = 1;

	mlen = 0;
	match[0] = '\0';
	(void)readdir(dir);	/* "." */
	(void)readdir(dir);	/* ".." */
	while ((ent = readdir(dir)) != NULL) {
	    if (strlen(ent->d_name) >= len
		&& strncmp(ent->d_name, file_name, len) == 0) {
		char *tmp = &(ent->d_name[len]), *mat = match;

		if (first) {
		    strncpy(match, tmp, sizeof(match) - 1);
		    match[sizeof(match) - 2] = '\0';
		    mlen = strlen(match);
		    first = 0;
#if 0
		    if (ent->d_type == DT_LNK)
			isdir = IsDir(dir_name, ent->d_name, True);
		    else
			isdir = ent->d_type == DT_DIR;
#endif
		}
		else {
		    while (*tmp++ == *mat++)
			;
		    mlen = mat - match - 1;
		    match[mlen] = '\0';
		}
#ifdef SHOW_MATCHES
		if (show_matches != SM_NEVER) {
		    Bool is_dir;

#if 0
		    if (ent->d_type == DT_LNK)
			is_dir = IsDir(dir_name, ent->d_name, False);
		    else
			is_dir = ent->d_type == DT_DIR;
#endif
		    matches = (char **)XtRealloc((char*)matches, sizeof(char**)
						 * (n_matches + 1));
		    buflen += strlen(ent->d_name) + 1;
		    if (is_dir) {
			matches[n_matches] = XtMalloc(strlen(ent->d_name) + 2);
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

	    if (mlen || add_slash) {
		XawTextPosition pos;

		/* XXX pos must be filenamewindow->text.lastPos */
		XawTextSetInsertionPoint(filenamewindow, 1024);

		pos = XawTextGetInsertionPoint(filenamewindow);
		block.length = mlen + add_slash;
		block.ptr = match;
		if (add_slash)
		    strcat(match, "/");
		block.firstPos = 0;
		block.format = FMT8BIT;
		XawTextReplace(filenamewindow, pos, pos, &block);
		XawTextSetInsertionPoint(filenamewindow, pos + block.length);
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
