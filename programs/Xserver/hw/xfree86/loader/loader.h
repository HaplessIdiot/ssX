/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.h,v 1.11 1998/03/21 11:08:49 dawes Exp $ */




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
#ifndef _LOADER_H
#define _LOADER_H

#include "sym.h"

#if defined(Lynx) && defined(sun)
#define const /**/
#endif

#if (defined(__i386__) || defined(__ix86)) && !defined(i386)
#define i386
#endif

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>

/* For LOOKUP definition */
#include "sym.h"

#define LD_UNKNOWN	-1
#define LD_ARCHIVE	0
#define LD_ELFOBJECT	1
#define LD_COFFOBJECT	2
#define LD_XCOFFOBJECT	3
#define LD_AOUTOBJECT   4
#define LD_AOUTDLOBJECT	5

#define LD_PROCESSED_ARCHIVE -1

/* #define UNINIT_SECTION */
#define HANDLE_IN_HASH_ENTRY

/*
 * COFF Section nmumbers
 */
#define N_TEXT       1
#define N_DATA       2
#define N_BSS        3
#define N_COMMENT    4

#define TestFree(a) if (a) { xfree (a); a = NULL; }

#define HASHDIV 8
#define HASHSIZE (1<<HASHDIV)

typedef struct _elf_reloc   *ELFRelocPtr;
typedef struct _elf_COMMON  *ELFCommonPtr;
typedef struct _coff_reloc  *COFFRelocPtr;
typedef struct _coff_COMMON *COFFCommonPtr;
typedef struct AOUT_RELOC   *AOUTRelocPtr;
typedef struct AOUT_COMMON  *AOUTCommonPtr;

typedef struct _LoaderReloc {
    int modtype;
    struct _LoaderReloc *next;
    COFFRelocPtr coff_reloc;
    ELFRelocPtr elf_reloc;
    AOUTRelocPtr aout_reloc;
} LoaderRelocRec, *LoaderRelocPtr;

typedef struct _loader_item {
	char	*name ;
	void	*address ;
	struct _loader_item *next ;
	int	handle ;
	int	module ;
#if defined(__powerpc__)
	/*
	 * PowerPC file formats require special routines in some circumstances
	 * to assist in the linking process. See the specific loader for
	 * more details.
	 */
	union {
		unsigned short	plt[8];		/* ELF */
		unsigned short	glink[14];	/* XCOFF */
	} code ;
#endif
	} itemRec, *itemPtr ;

/*
 * The loader uses loader specific alloc/calloc/free functions that
 * are mapped to either to the regular Xserver functions, or in a couple
 * of special cases, mapped to the C library functions.
 */
#if !defined(PowerMAX_OS) && !(defined(linux) && defined(__alpha__))
#define xf86loadermalloc(size) xalloc(size)
#define xf86loaderrealloc(ptr,size) xrealloc(ptr,size)
#define xf86loadercalloc(num,size) xcalloc(num,size)
#define xf86loaderfree(ptr) xfree(ptr)
#define xf86loaderstrdup(ptr) xf86strdup(ptr)
#else
/*
 * On Some OSes, xalloc() et al uses mmap to allocate space for large
 * allocation. This has the effect of placing the text section of some
 * modules very far away from the rest which are placed on the heap.
 * Certain relocations are limited in the size of the offsets that can be
 * handled, and this seperation causes these relocation to overflow. This
 * is fixes by just using the C library allocation functions for the loader
 * to ensure that all text sections are located on the head. OSes that have
 * this problem are:
 *	PowerMAXOS/PPC
 * 	Linux/Alpha
 */
#define xf86loadermalloc(size) malloc(size)
#define xf86loaderrealloc(ptr,size) realloc(ptr,size)
#define xf86loadercalloc(num,size) calloc(num,size)
#define xf86loaderfree(ptr) free(ptr)
#define xf86loaderstrdup(ptr) strdup(ptr)
#endif

typedef struct _loader *loaderPtr;

/*
 * _loader_funcs hold the entry points for a module format.
 */

typedef void * (*LoadModuleProcPtr)(
#if NeedNestedPrototypes
	loaderPtr,	/* modrec */
	int,		/* fd */
	LOOKUP **
#endif
	);
typedef void (*ResolveSymbolsProcPtr)(
#if NeedNestedPrototypes
	void *
#endif
	);
typedef int (*CheckForUnresolvedProcPtr)(
#if NeedNestedPrototypes
	int,
	void *
#endif
	);
typedef void (*LoaderUnloadProcPtr)(
#if NeedNestedPrototypes
	void *
#endif
	);

typedef struct _loader_funcs {
	LoadModuleProcPtr	LoadModule;
	ResolveSymbolsProcPtr	ResolveSymbols;
	CheckForUnresolvedProcPtr CheckForUnresolved;
	LoaderUnloadProcPtr	LoaderUnload;
	LoaderRelocRec		pRelocs; /* type specific relocations */
} loader_funcs;

/* Each module loaded has a loaderRec */
typedef struct _loader {
	int	handle;		/* Unique id used to remove symbols from
				   this module when it is unloaded */
	int	module;		/* Unique id to identify compilation units */
	char	*name;
	void	*private;	/* format specific data */
	loader_funcs	*funcs;	/* funcs for operating on this module */
	loaderPtr next;
} loaderRec;

void LoaderInit(void);

/* Internal Functions */

void
LoaderAddSymbols(
#if NeedFunctionPrototypes
int,
int,
LOOKUP *
#endif
);
void
LoaderDefaultFunc(
#if NeedFunctionPrototypes
void
#endif
);
void
LoaderDuplicateSymbol(
#if NeedFunctionPrototypes
const char *,
const int
#endif
);
void
LoaderFixups(void);
void
LoaderResolve(
#if NeedFunctionPrototypes
void
#endif
);
int
LoaderResolveSymbols(
#if NeedFunctionPrototypes
void
#endif
);
char *
LoaderHandleToName(
#if NeedFunctionPrototypes
int
#endif
);
int
_LoaderHandleUnresolved(
#if NeedFunctionPrototypes
char *, char *, int
#endif
);
void
LoaderHashAdd(
#if NeedFunctionPrototypes
itemPtr
#endif
);
itemPtr
LoaderHashDelete(
#if NeedFunctionPrototypes
const char *
#endif
);
itemPtr
LoaderHashFind(
#if NeedFunctionPrototypes
const char *
#endif
);
void
LoaderHashTraverse( 
#if NeedFunctionPrototypes
void *,
int (*)(void *, itemPtr)
#endif
);
#if NeedFunctionPrototypes
void LoaderPrintAddress(const char *);
void LoaderPrintItem(itemPtr);
void LoaderPrintSymbol(unsigned long);
#else
void LoaderPrintAddress();
void LoaderPrintItem();
void LoaderPrintSymbol();
#endif
void LoaderResolve(
#if NeedFunctionPrototypes
void
#endif
);

/*
 * File interface functions
 */
void *
_LoaderFileToMem(
#if NeedFunctionPrototypes
int,	/* fd */
unsigned long,	/* offset */
int,	/* size */
char *	/* label */
#endif
);

void
_LoaderFreeFileMem(
#if NeedFunctionPrototypes
void *,	/* addr */
int	/* size */
#endif
);

int
_LoaderFileRead(
#if NeedFunctionPrototypes
int,		/* fd */
unsigned int,	/* offset */
void *,		/* addr */
int		/* size */
#endif
);

/*
 * Relocation list manipulation routines
 */
LoaderRelocPtr
_LoaderGetRelocations(
#if NeedFunctionPrototypes
void *
#endif
);

/*
 * object to name lookup routines
 */
char *
_LoaderHandleToName(
#if NeedFunctionPrototypes
int            /* handle */
#endif
);

/*
 * Entry points for the different loader types
 */
#include "aoutloader.h"
#include "coffloader.h"
#include "elfloader.h"
/* LD_ARCHIVE */
void *ARCHIVELoadModule(
#if NeedFunctionPrototypes
loaderPtr,
int,
LOOKUP **
#endif
);


/* LD_DLOBJECT */
void *DLLoadModule(
#if NeedFunctionPrototypes
loaderPtr,
int,
LOOKUP **
#endif
);
void DLResolveSymbols(
#if NeedFunctionPrototypes
void *
#endif
);
int DLCheckForUnresolved(
#if NeedFunctionPrototypes
int,
void *
#endif
);
void DLUnloadModule(
#if NeedFunctionPrototypes
void *
#endif
);
void *DLFindSymbol(
#if NeedFuncionPrototypes
char *
#endif
);


#endif /* _LOADER_H */
