/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Module.h,v 1.10 1999/01/24 03:13:53 dawes Exp $ */

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

#define DEFAULT_LIST ((char *)-1)

typedef enum {
    ABI_CLASS_NONE		= 0,	/* No ABI used */
    ABI_CLASS_ANSIC,			/* only requires the ansic wrapper */
    ABI_CLASS_VIDEODRV,			/* Requires the video driver ABI */
    ABI_CLASS_XINPUT,			/* XInput driver */
    ABI_CLASS_EXTENSION,		/* extension module */
    ABI_CLASS_FONT			/* font renderer */
} LoaderAbiTypes;

#define ABI_MINOR_MASK		0x0000FFFF
#define ABI_MAJOR_MASK		0xFFFF0000
#define GET_ABI_MINOR(v)	((v) & ABI_MINOR_MASK)
#define GET_ABI_MAJOR(v)	(((v) & ABI_MAJOR_MASK) >> 16)
#define SET_ABI_VERSION(maj, min) \
		((((maj) << 16) & ABI_MAJOR_MASK) | ((min) & ABI_MINOR_MASK))

/*
 * ABI versions.  Each version has a major and minor revision.  Modules
 * using lower minor revisions must work with servers of a higher minor
 * revision.  There is no compatibility between different major revisions.
 * Whenever the ABI_ANSIC_VERSION is changed, the others must also be
 * changed.  The minor revision mask is 0x0000FFFF and the major revision
 * mask is 0xFFFF0000.
 */
#define ABI_ANSIC_VERSION	SET_ABI_VERSION(0, 1)
#define ABI_VIDEODRV_VERSION	SET_ABI_VERSION(0, 1)
#define ABI_XINPUT_VERSION	SET_ABI_VERSION(0, 0)
#define ABI_EXTENSION_VERSION	SET_ABI_VERSION(0, 0)
#define ABI_FONT_VERSION	SET_ABI_VERSION(0, 0)

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
    LDR_NOHARDWARE,	/* could not query/initialize the hardware device */
    LDR_MISMATCH,	/* the module didn't match the spec'd requirments */
    LDR_BADUSAGE	/* LoadModule is called with bad arguments */
} LoaderErrorCode;

/* This structure is expected to be returned by the initfunc */
typedef struct {
    char *	modname;	/* name of module, e.g. "foo" */
    char *	vendor;		/* vendor specific string */
    CARD32	_modinfo1_;	/* constant MODINFOSTRING1/2 to find */
    CARD32	_modinfo2_;	/* infoarea with a binary editor or sign tool */
    CARD32	xf86version;	/* contains XF86_VERSION_CURRENT */
    CARD8	majorversion;	/* module-specific major version */
    CARD8	minorversion;	/* module-specific minor version */
    CARD16	patchlevel;	/* module-specific patch level */
    CARD32	abiclass;	/* ABI class that the module uses */
    CARD32	abiversion;	/* ABI version */
    char *	abivendor;	/* vendor-defined ABI (overrides abiclass) */
    CARD32	checksum[4];	/* contains a digital signature of the */
				/* version info structure */
} XF86ModuleVersionInfo;

/*
 * This structure can be used to callers of LoadModule and LoadSubModule to
 * specify version and/or ABI requirements.
 */
typedef struct {
    CARD8	majorversion;	/* module-specific major version */
    CARD8	minorversion;	/* moudle-specific minor version */
    CARD16	patchlevel;	/* module-specific patch level */
    CARD32	abiclass;	/* ABI class that the module uses */
    CARD32	abiversion;	/* ABI version */
    char *	abivendor;	/* vendor-defined ABI (overrides abiclass) */
} XF86ModReqInfo;

/* values to indicate unspecified fields in XF86ModReqInfo. */
#define MAJOR_UNSPEC		0xFF
#define MINOR_UNSPEC		0xFF
#define PATCH_UNSPEC		0xFFFF
#define ABI_VERS_UNSPEC		0xFFFFFFFF

#define INITARGS void

typedef void (*InitExtension)(INITARGS);

typedef struct {
    InitExtension	initFunc;
    char *		name;
    Bool		*disablePtr;
    InitExtension	setupFunc;	
} ExtensionModule;

extern ExtensionModule extension[];

/* Prototypes for Loader functions that are exported to modules */
#ifndef IN_LOADER
/* Prototypes with opaque pointers for use by modules */
pointer LoadSubModule(pointer, const char *, const char **,
		      const char **, pointer, const XF86ModReqInfo *,
		      int *, int *);
void UnloadSubModule(pointer);
void LoadFont(pointer);
#endif
pointer LoaderSymbol(const char *);
char **LoaderListDirs(const char **, const char **);
void LoaderFreeDirList(char **);
void LoaderErrorMsg(const char *, const char *, int, int);
void LoadExtension(ExtensionModule *);
void LoaderRefSymLists(const char **, ...);
void LoaderReqSymLists(const char **, ...);
void LoaderReqSymbols(const char *, ...);
int LoaderCheckUnresolved(int);

typedef pointer (*ModuleSetupProc)(pointer, pointer, int *, int *);
typedef void (*ModuleTearDownProc)(pointer);
#define MODULEINITARGS	XF86ModuleVersionInfo **, ModuleSetupProc *,	\
			ModuleTearDownProc *
typedef void (*ModuleInitProc)(MODULEINITARGS);
#define MODULEINITPROTO(func) void func(MODULEINITARGS)
#define MODULESETUPPROTO(func) pointer func(pointer, pointer, int*, int*)
#define MODULETEARDOWNPROTO(func) void func(pointer)

#endif /* _XF86STR_H */
