/*
 *	rcs_id[] = "$XConsortium: xedit.h,v 1.19 89/10/07 14:59:46 kit Exp $";
 */
 
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
/* $XFree86: xc/programs/xedit/xedit.h,v 1.2 1998/11/15 04:30:44 dawes Exp $ */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>

typedef struct _xedit_hints {
    char *resource;
    unsigned long interval;
    XtIntervalId timer;
    char **hints;
    unsigned num_hints;
    unsigned cur_hint;
} xedit_hints;

typedef enum {NO_READ, READ_OK, WRITE_OK} FileAccess;

#define CHANGED_BIT	0x01
#define EXISTS_BIT	0x02
typedef struct _xedit_flist_item {
    Widget source, sme;
    String name;
    String filename;
    int flags;
    FileAccess file_access;
    XawTextPosition display_position, insert_position;
} xedit_flist_item;

extern struct _xedit_flist {
    Widget popup;
    Pixmap pixmap;
    xedit_flist_item **itens;
    Cardinal num_itens;
} flist;

extern struct _app_resources {
    Boolean enableBackups;
    char *backupNamePrefix;
    char *backupNameSuffix;
    xedit_hints hints;
    char *changed_pixmap_name;
    char *position_format;
} app_resources;

extern Widget topwindow, textwindow, labelwindow, filenamewindow, messwidget;
extern Boolean international;

/*	externals in xedit.c 	*/

void Feep(void);

/*	externals in util.c 	*/

void XeditPrintf(char*);
Widget MakeCommandButton(Widget, char*, XtCallbackProc);
Widget MakeStringBox(Widget, String, String);
String GetString(Widget);
FileAccess MaybeCreateFile(char*), CheckFilePermissions(char*, Boolean*);
xedit_flist_item *AddTextSource(Widget, String, String, int, FileAccess);
xedit_flist_item *FindTextSource(Widget, char*);
Bool KillTextSource(xedit_flist_item*);
char *ResolveName(char*);
void DeleteWindow(Widget, XEvent*, String*, Cardinal*);
void SplitWindow(Widget, XEvent*, String*, Cardinal*);
void SwitchTextSource(xedit_flist_item*);
void PopupMenu(Widget, XEvent*, String*, Cardinal*);
void OtherWindow(Widget, XEvent*, String*, Cardinal*);
void SwitchSource(Widget, XEvent*, String*, Cardinal*);
void XeditFocus(Widget, XEvent*, String*, Cardinal*);

/*	externs in commands.c 	*/

void DoQuit(Widget, XtPointer, XtPointer);
void QuitAction(Widget, XEvent*, String*, Cardinal*);
void DoSave(Widget, XtPointer, XtPointer);
void SaveFile(Widget, XEvent*, String*, Cardinal*);
void DoLoad(Widget, XtPointer, XtPointer);
void CancelFindFile(Widget, XEvent*, String*, Cardinal*);
void FindFile(Widget, XEvent*, String*, Cardinal*);
void LoadFile(Widget, XEvent*, String*, Cardinal*);
void FileCompletion(Widget, XEvent*, String*, Cardinal*);
void KillFile(Widget, XEvent*, String*, Cardinal*);
