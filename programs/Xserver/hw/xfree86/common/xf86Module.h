/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Module.h,v 1.3 1998/08/13 14:45:48 dawes Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains modules related things that non-XFree86 specific
 * code needs for loadable module support.  It should include a bare
 * minimum of other headers, and this should be the only XFree86-specific
 * header required by that other code.
 *
 * Longer term, the module/loader code should probably live directly under
 * Xserver/.
 *
 * XXX This file arguably belongs in xfree86/loader/.
 */

#ifndef _XF86MODULE_H
#define _XF86MODULE_H

#include "misc.h"
#include "xf86Version.h"

typedef enum {
    LD_RESOLV_IFDONE		= 0,	/* only check if no more
					   delays pending */
    LD_RESOLV_NOW		= 1,	/* finish one delay step */
    LD_RESOLV_FORCE		= 2	/* force checking... */
} LoaderResolveOptions;

typedef enum {
    ABI_CLASS_NONE		= 0,	/* No ABI used */
    ABI_CLASS_ANSIC,			/* only requires the ansic wrapper */
    ABI_CLASS_VIDEODRV,			/* Requires the video driver ABI */
    ABI_CLASS_XINPUT,			/* XInput driver */
    ABI_CLASS_EXTENSION,		/* extension module */
    ABI_CLASS_FONT			/* font renderer */
} LoaderAbiTypes;

/*
 * ABI versions.  Each version has a major and minor revision.  Modules
 * using lower minor revisions must work with servers of a higher minor
 * revision.  There is no compatibility between different major revisions.
 * Whenever the ABI_ANSIC_VERSION is changed, the others must also be
 * changed.  The minor revision mask is 0x0000FFFF and the major revision
 * mask is 0xFFFF0000.
 */
#define ABI_ANSIC_VERSION	0x00000001	/* 0.1 */
#define ABI_VIDEODRV_VERSION	0x00000001	/* 0.1 */
#define ABI_XINPUT_VERSION	0x00000000	/* 0.0 */
#define ABI_EXTENSION_VERSION	0x00000000	/* 0.0 */
#define ABI_FONT_VERSION	0x00000000	/* 0.0 */

#define MODINFOSTRING1	0xef23fdc5
#define MODINFOSTRING2	0x10dc023a

#ifndef MODULEVENDORSTRING
#define MODULEVENDORSTRING	"The XFree86 Project"
#endif

/* Error return codes for errmaj */
typedef enum {
    LDR_NOERROR = 0,
    LDR_NOMEM,		/* memory allocation failed */
    LDR_NOENT,		/* Module file does not exist */
    LDR_NOSUBENT,	/* pre-requsite file to be sub-loaded does not exist */
    LDR_NOSPACE,	/* internal module array full */
    LDR_NOMODOPEN,	/* module file could not be opened (check errmin) */
    LDR_UNKTYPE,	/* file is not a recognized module type */
    LDR_NOLOAD,		/* type specific loader failed */
    LDR_ONCEONLY,	/* Module should only be loaded once (not an error) */
    LDR_NOPORTOPEN,	/* could not open port (check errmin) */
    LDR_NOHARDWARE	/* could not query/initialize the hardware device */
} LoaderErrorCode;

/* This structure is expected to be returned by the initfunc */
typedef struct {
    char *	modname;	/* name of module, e.g. "foo" */
    char *	vendor;		/* vendor specific string */
    CARD32	_modinfo1_;	/* constant MODINFOSTRING1/2 to find */
    CARD32	_modinfo2_;	/* infoarea with a binary editor or sign tool */
    CARD32	xf86version;	/* contains XF86_VERSION_CURRENT */
    CARD32	modversion;	/* contains a module specific version id */
    CARD32	abiclass;	/* ABI class that the module uses */
    CARD32	abiversion;	/* ABI version */
    CARD32	checksum[4];	/* contains a digital signature of the */
				/* version info structure */
} XF86ModuleVersionInfo;

#define INITARGS void

typedef void (*InitExtension)(INITARGS);

typedef struct {
    InitExtension	initFunc;
    char *		name;
    Bool		*disablePtr;
    InitExtension	setupFunc;	
} ExtensionModule;

extern ExtensionModule extension[];

#ifndef IN_LOADER
/* Prototypes with opaque pointers for use by modules */
pointer LoadModule(const char *, const char *, pointer, int *, int *);
pointer LoadSubModule(pointer, const char *, const char *, pointer,
		      int *, int *);
void UnloadModule(pointer);
pointer DuplicateModule(pointer);
void LoadFont(pointer);
#endif
void LoaderErrorMsg(const char *, const char *, int, int);
void LoadExtension(ExtensionModule *);
void LoaderRefSymLists(const char **, ...);
void LoaderReqSymLists(const char **, ...);
void LoaderReqSymbols(const char *, ...);

typedef pointer (*ModuleSetupProc)(pointer, pointer, int *, int *);
typedef void (*ModuleTearDownProc)(pointer);
#define MODULEINITARGS	XF86ModuleVersionInfo **, ModuleSetupProc *,	\
			ModuleTearDownProc *
typedef void (*ModuleInitProc)(MODULEINITARGS);
#define MODULEINITPROTO(func) void func(MODULEINITARGS)
#define MODULESETUPPROTO(func) pointer func(pointer, pointer, int*, int*)
#define MODULETEARDOWNPROTO(func) void func(pointer)

#endif /* _XF86STR_H */
