/* $XFree86$ */

#include "xf86Module.h"
#include "XIE.h"		

MODULEINITPROTO(xieModuleInit);
static MODULESETUPPROTO(xieSetup);

extern void XieInit(INITARGS);

ExtensionModule XieExt =
{
    XieInit,
    xieExtName,
    NULL
};

static XF86ModuleVersionInfo VersRec =
{
        "xie",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,				/* 1.1 */
        ABI_CLASS_EXTENSION,
        ABI_EXTENSION_VERSION,
        {0,0,0,0}
};

void
xieModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = xieSetup;
    *teardown = NULL;
}

static pointer
xieSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    LoadExtension(&XieExt);

    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}

