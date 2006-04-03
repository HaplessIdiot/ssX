/*
 * Copyright 1997 Metro Link, Inc.
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dlloader.h,v 1.5 2006/03/16 16:50:34 dawes Exp $ */

#ifndef _DLLOADER_H
#define _DLLOADER_H

#if defined(linux) || \
    (defined(__FreeBSD__) && defined(__ELF__)) || \
    defined(__NetBSD__) || \
    (defined(__OpenBSD__) && defined(__ELF__)) || \
    (defined(sun) && defined(SVR4)) || \
    defined(sgi)
#define HAVE_DLADDR
#endif

extern void *DLLoadModule(loaderPtr, int, LOOKUP **);
extern void DLUnloadModule(void *);
extern void *DLFindSymbol(const char *name);
extern const char *DLAddressToSymbol(void *, unsigned long, unsigned long *,
				     const char **, int);
#endif
