/* $XFree86: xc/programs/Xserver/dbe/dbemodule.c,v 1.2 1998/08/13 14:45:37 dawes Exp $ */

#include "xf86Module.h"

MODULEINITPROTO(dbeModuleInit);
static MODULESETUPPROTO(dbeSetup);

extern void DbeExtensionInit(INITARGS);

ExtensionModule dbeExt = {
    DbeExtensionInit,
    "DOUBLE-BUFFER",
    NULL,
    NULL
};

static XF86ModuleVersionInfo VersRec =
{
        "dbe",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_EXTENSION,
        ABI_EXTENSION_VERSION,
        {0,0,0,0}
};

/* 
 * Entry point for the loader code
 */
void
dbeModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = dbeSetup;
    *teardown = NULL;
}

static pointer
dbeSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    LoadExtension(&dbeExt);

    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}
