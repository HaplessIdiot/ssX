/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dlloader.c,v 1.3 1997/11/16 11:51:14 dawes Exp $ */





/*
 *
 * Copyright (c) 1997 The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * XFree86 Project, Inc. not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The Xfree86 Project, Inc. makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE XFREE86 PROJECT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE XFREE86 PROJECT, INC. BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.  */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "loader.h"

#ifdef DL_LAZY
#define DLOPEN_FLAGS DL_LAZY
#else
#ifdef RTLD_LAZY
#define DLOPEN_FLAGS RTLD_LAZY
#else
#ifdef __FreeBSD__
#define DLOPEN_FLAGS 1
#endif
#endif
#endif

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */
typedef struct {
    int handle;
    void *dlhandle;
} DLModuleRec, *DLModulePtr;

/* 
 * a list of loaded modules XXX can be improved
 */
typedef struct DLModuleList {
    DLModulePtr module;
    struct DLModuleList *next;
} DLModuleList;

DLModuleList *dlModuleList = NULL;

/*
 * Search a symbol in the module list
 */
void *
DLFindSymbol(char *name)
{
    DLModuleList *l;
    void *p;
    char *n;

    n = malloc(strlen(name) + 2);
    sprintf(n, "_%s", name);

    for (l = dlModuleList; l != NULL; l = l->next) {
	p = dlsym(l->module->dlhandle, n);
	if (p != NULL) {
	    free(n);
	    return p;
	}
    }
    free(n);
    return NULL;
}

/*
 * public interface
 */
void *
DLLoadModule(int modtype,
	     char *modname,
	     int handle,
	     int fd)
{
    DLModulePtr dlfile;
    DLModuleList *l;

    if (modtype != LD_AOUTDLOBJECT) {
	ErrorF("DLLoadModule(): modtype != shared object\n");
	return NULL;
    }
    if ((dlfile=(DLModulePtr)calloc(1,sizeof(DLModuleRec)))==NULL) {
	ErrorF("Unable  to allocate DLModuleRec\n");
	return NULL;
    }
    dlfile->handle = handle;
    dlfile->dlhandle = dlopen(modname, DLOPEN_FLAGS);
    if (handle == 0) {
	ErrorF("dlopen: %s\n", dlerror());
	free(dlfile);
	return NULL;
    }
    /* Add it to the module list */
    l = (DLModuleList *)malloc(sizeof(DLModuleList));
    l->module = dlfile;
    l->next = dlModuleList;
    dlModuleList = l;

    return dlfile;
}

void
DLResolveSymbols(void)
{
    return;
}

int
DLCheckForUnresolved(int color_depth)
{
    return 0;
}

void
DLUnloadModule(void *modptr)
{
    DLModulePtr dlfile = (DLModulePtr)modptr;
    DLModuleList *l, *p;

    /*  remove it from dlModuleList */
    if (dlModuleList->module == modptr) {
	l = dlModuleList;
	dlModuleList = l->next;
	free(l);
    } else {
	p = dlModuleList;
	for (l = dlModuleList->next; l != NULL; l = l->next) {
	    if (l->module == modptr) {
		p->next = l->next;
		free(l);
		break;
	    }
	    p = l;
	}
    }
    dlclose(dlfile->dlhandle);
    free(modptr);
}
