/*
 * $Id$
 *
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
/* $XFree86: $ */

#include <X11/Xdefs.h>
#include <font.h>
#include "fontxlfd.h"
#include "fntfil.h"
#include "fntfilio.h"
#include "fntfilst.h"

typedef struct _BuiltinFile {
    char    *name;
    int	    len;
    char    *bits;
} BuiltinFileRec, *BuiltinFilePtr;

typedef struct _BuiltinDir {
    char    *file_name;
    char    *font_name;
} BuiltinDirRec, *BuiltinDirPtr;

typedef struct _BuiltinAlias {
    char    *alias_name;
    char    *font_name;
} BuiltinAliasRec, *BuiltinAliasPtr;

extern BuiltinFileRec	builtin_files[];
extern int		builtin_files_count;

extern BuiltinDirRec	builtin_dir[];
extern int		builtin_dir_count;

extern BuiltinAliasRec	builtin_alias[];
extern int		builtin_alias_count;

extern FontFilePtr	BuiltinFileOpen ();
extern int		BuiltinFileClose ();
