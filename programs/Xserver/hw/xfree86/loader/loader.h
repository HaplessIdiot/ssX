/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.h,v 1.3 1997/02/18 12:04:30 dawes Exp $ */




/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
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
#if defined(Lynx) && defined(sun)
#define const /**/
#endif

#if (defined(__i386__) || defined(__ix86)) && !defined(i386)
#define i386
#endif

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>

#ifdef DBMALLOC
#include <debug/malloc.h>
#endif

#define LD_UNKNOWN	-1
#define LD_ARCHIVE	0
#define LD_ELFOBJECT	1
#define LD_COFFOBJECT	2
#define LD_XCOFFOBJECT	3
#define LD_AOUTOBJECT   4
#define LD_OS2AOUTOBJECT 5
#define LD_DLOPEN	6

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

typedef struct _loader_item {
	char	*name ;
	char	*address ;
	struct _loader_item *next ;
#ifdef HANDLE_IN_HASH_ENTRY
	int	handle ;
#endif
#if defined(__powerpc__)
	unsigned short	plt[8];
#endif
	} itemRec, *itemPtr ;

/*
 * _loader_funcs hold the entry points for a module format.
 */

typedef struct _loader_funcs {
	void *(*LoadModule)(
#if NeedFunctionPrototypes
	int,	/* modtype */
	char *,	/* modname */
	int,	/* handle */
	int	/* fd */
#endif
	);
	void (*ResolveSymbols)(
#if NeedFunctionPrototypes
	void
#endif
	);
	int (*CheckForUnresolved)(
#if NeedFunctionPrototypes
	void
#endif
	);
	void (*LoaderUnload)(
#if NeedFunctionPrototypes
	void *
#endif
	);
	} loader_funcs;


/* Each module loaded has a loaderRec */
typedef struct _loader {
	int	handle;		/* Unique id used to remove symbols from
				   this module whne it is unloaded */
	char	*name;
	void	*private;	/* format specific data */
	loader_funcs	funcs;	/* funcs for operating on this module */
	struct _loader *next ;
} loaderRec, *loaderPtr;


/* Internal Functions */

void
LoaderAddSymbols(
#if NeedFunctionPrototypes
int,
LOOKUP *
#endif
);
void
LoaderResolve(
#if NeedFunctionPrototypes
void
#endif
);
itemPtr
LoaderHashFind(
#if NeedFunctionPrototypes
char *
#endif
);
int
LoaderHashAdd(
#if NeedFunctionPrototypes
itemPtr
#endif
);
void
LoaderHashTraverse( 
#if NeedFunctionPrototypes
void *,
int (*)()
#endif
);
itemPtr
LoaderHashFindNearest(
#if NeedFunctionPrototypes
int
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

char *
_LoaderHandleToName(
#if NeedFunctionPrototypes
int            /* handle */
#endif
);

/*
 * Entry points for the different loader types
 */

/* LD_COFFOBJECT */
void *COFF2LoadModule(
#if NeedFunctionPrototypes
int,
char *,
int,
int
#endif
);
void COFF2ResolveSymbols(
#if NeedFunctionPrototypes
void
#endif
);
int COFF2CheckForUnresolved(
#if NeedFunctionPrototypes
void
#endif
);
void COFF2UnloadModule(
#if NeedFunctionPrototypes
void *
#endif
);

/* LD_ARCHIVE */
void *ARCHIVELoadModule(
#if NeedFunctionPrototypes
int,
char *,
int,
int
#endif
);

/* LD_ELFOBJECT */
void *ELFLoadModule(
#if NeedFunctionPrototypes
int,
char *,
int,
int
#endif
);
void ELFResolveSymbols(
#if NeedFunctionPrototypes
void
#endif
);
int ELFCheckForUnresolved(
#if NeedFunctionPrototypes
void
#endif
);
void ELFUnloadModule(
#if NeedFunctionPrototypes
void *
#endif
);


/* LD_AOUTOBJECT */
void *AOUTLoadModule(
#if NeedFunctionPrototypes
int,
char *,
int,
int
#endif
);
void AOUTResolveSymbols(
#if NeedFunctionPrototypes
void
#endif
);
int AOUTCheckForUnresolved(
#if NeedFunctionPrototypes
void
#endif
);
void AOUTUnloadModule(
#if NeedFunctionPrototypes
void *
#endif
);


