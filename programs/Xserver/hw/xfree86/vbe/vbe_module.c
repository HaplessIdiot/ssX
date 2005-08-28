/* $XFree86: xc/programs/Xserver/hw/xfree86/vbe/vbe_module.c,v 1.1 2003/02/17 17:06:46 dawes Exp $ */

#ifdef XFree86LOADER

#include "xf86.h"
#include "vbe.h"

extern const char *vbe_ddcSymbols[];

static pointer
vbeSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;
    
    if (!setupDone) {
	setupDone = TRUE;

	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(vbe_ddcSymbols, NULL);
    } 
    /*
     * The return value must be non-NULL on success even though there
     * is no TearDownProc.
     */
    return (pointer)1;
}

static XF86ModuleVersionInfo vbeVersRec =
{
    "vbe",			/* modname */
    MODULEVENDORSTRING,		/* vendor */
    MODINFOSTRING1,		/* _modinfo1_ */
    MODINFOSTRING2,		/* _modinfo2_ */
    XF86_VERSION_CURRENT,	/* xf86version */
    1, 1, 0,			/* majorversion, minorversion, patchlevel */
    ABI_CLASS_VIDEODRV,		/* needs the video driver ABI */
    ABI_VIDEODRV_VERSION,	/* abiversion */
    MOD_CLASS_NONE,		/* moduleclass */
    {0,0,0,0}			/* checksum */
};

XF86ModuleData vbeModuleData =
{
    &vbeVersRec,		/* vers */
    vbeSetup,			/* setup */
    NULL			/* teardown */
};

#endif /* XFree86LOADER */
