/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loaderProcs.h,v 1.4 1998/01/25 08:28:20 dawes Exp $ */




/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
 *
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

#include "xf86Optrec.h"
#include "loaderErrors.h"

typedef struct module_desc {
	struct module_desc *child;
	struct module_desc *sib;
	struct module_desc *demand_next;
	char *name;
	char *identifier;
	XID client_id;
	int in_use;
	int handle;
	ModuleSetupProc SetupProc;
	ModuleTearDownProc TearDownProc;
	void *TearDownData; /* returned from SetupProc */
} ModuleDesc, *ModuleDescPtr;


/*
 * Extenal API for the loader 
 */
ModuleDescPtr
LoadDriver(const char *, const char *, int, XF86OptionPtr, int*, int *);
ModuleDescPtr
LoadModule(const char *, const char *, XF86OptionPtr, int*, int *);
void UnloadModule (ModuleDescPtr);
void UnloadDriver (ModuleDescPtr);
void FreeModuleDesc (ModuleDescPtr mod);
ModuleDescPtr NewModuleDesc (const char *);
ModuleDescPtr AddSibling (ModuleDescPtr head, ModuleDescPtr new);


int LoaderCheckUnresolved(
#if NeedFunctionPrototypes
int, int
#endif
);
int LoaderOpen(
#if NeedFunctionPrototypes
const char *,
int,
int *,
int *
#endif
);
void LoaderShowStack(
#if NeedFunctionPrototypes
void
#endif
);
void *LoaderSymbol(
#if NeedFunctionPrototypes
const char *
#endif
);
void *
LoaderSymbolHandle(
#if NeedFunctionPrototypes
const char *,
int
#endif
);
int
LoaderUnload(
#if NeedFunctionPrototypes
int
#endif
);
