/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_ldext.h,v 3.2 1997/04/08 14:54:26 hohndel Exp $ */
/*
 * Copyright (C) 1997 Matthieu Herrb
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Matthieu Herrb not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Matthieu Herrb   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MATTHIEU HERRB DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL MATTHIEU HERRB BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
#ifndef _XF86_LDEXT_H
#define _XF86_LDEXT_H

#if NeedFunctionPrototypes
#define INITARGS void
#else
#define INITARGS /*nothing*/
#endif
typedef void (*InitExtension)(INITARGS);

typedef struct ExtensionModule {
    InitExtension initFunc;
    char *name;
    Bool *disablePtr;
} ExtensionModule;

extern ExtensionModule extension[];

#endif
