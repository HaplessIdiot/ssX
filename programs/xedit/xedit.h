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
/* $XFree86: contrib/programs/xedit/xedit.h,v 1.3 1998/10/11 11:20:00 dawes Exp $ */

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

extern struct _app_resources {
    Boolean enableBackups;
    char *backupNamePrefix;
    char *backupNameSuffix;
    xedit_hints hints;
} app_resources;

typedef enum {NO_READ, READ_OK, WRITE_OK} FileAccess;

/*	externals in xedit.c 	*/

extern void Feep();

/*	externals in util.c 	*/

extern void   XeditPrintf();
extern Widget MakeCommandButton();
extern Widget MakeStringBox();
extern String GetString();
extern FileAccess MaybeCreateFile(), CheckFilePermissions();

/*	externs in commands.c 	*/

extern void DoQuit();
extern void DoSave();
extern void DoLoad();
extern void CancelFindFile(Widget, XEvent*, String*, Cardinal*);
extern void FindFile(Widget, XEvent*, String*, Cardinal*);
extern void LoadFile(Widget, XEvent*, String*, Cardinal*);
extern void FileCompletion(Widget, XEvent*, String*, Cardinal*);
