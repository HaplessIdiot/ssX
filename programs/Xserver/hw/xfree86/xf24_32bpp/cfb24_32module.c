/* $XFree86$ */

#ifdef XFree86LOADER

#include "xf86Module.h"

MODULEINITPROTO(xf24_32bppModuleInit);
static MODULESETUPPROTO(xf24_32bppSetup);

static XF86ModuleVersionInfo VersRec =
{
        "xf24_32bpp",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_ANSIC,                /* Only need the ansic layer */
        ABI_ANSIC_VERSION,
	NULL,
        {0,0,0,0}       /* signature, to be patched into the file by a tool */
};

void
xf24_32bppModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
              ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = xf24_32bppSetup;
    *teardown = NULL;
}

static pointer
xf24_32bppSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    if (!LoadSubModule(module, "cfb24", NULL, NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb32", NULL, NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    return (pointer)1;  /* non-NULL required to indicate success */
}

#endif
