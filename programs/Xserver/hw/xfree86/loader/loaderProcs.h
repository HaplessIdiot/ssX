/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loaderProcs.h,v 1.3.2.6 1998/07/05 14:36:22 dawes Exp $ */

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

#define IN_LOADER
#include "xf86Module.h"
#include "loaderErrors.h"
#include "fontmod.h"

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
	const char *path;
} ModuleDesc, *ModuleDescPtr;


/*
 * Extenal API for the loader 
 */

void LoaderInit(void);

ModuleDescPtr LoadDriver(const char *, const char *, int, pointer, int *,
			 int *);
ModuleDescPtr LoadModule(const char *, const char *, pointer, int *, int *);
ModuleDescPtr LoadSubModule(ModuleDescPtr, const char *, const char *,
			    pointer, int *, int *);
ModuleDescPtr DuplicateModule(ModuleDescPtr mod);
void LoadFont (FontModule *);
void UnloadModule (ModuleDescPtr);
void UnloadDriver (ModuleDescPtr);
void FreeModuleDesc (ModuleDescPtr mod);
ModuleDescPtr NewModuleDesc (const char *);
ModuleDescPtr AddSibling (ModuleDescPtr head, ModuleDescPtr new);


int LoaderCheckUnresolved(int, int);
void LoaderShowStack(void);
void *LoaderSymbol(const char *);
void *LoaderSymbolHandle(const char *, int);
int LoaderUnload(int);

