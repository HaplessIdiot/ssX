/* $XFree86$ */


#ifdef XFree86LOADER

#include "xf86Module.h"

MODULEINITPROTO(cfb8_32ModuleInit);
static MODULESETUPPROTO(cfb8_32Setup);

static XF86ModuleVersionInfo VersRec =
{
        "xf8_32bpp",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_ANSIC,                /* Only need the ansic layer */
        ABI_ANSIC_VERSION,
        {0,0,0,0}       /* signature, to be patched into the file by a tool */
};

void
cfb8_32ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
              ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = cfb8_32Setup;
    *teardown = NULL;
}

static pointer
cfb8_32Setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    if (!LoadSubModule(module, "cfb", NULL, NULL, NULL, NULL, errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb32", NULL, NULL, NULL, NULL, errmaj, errmin))
        return NULL;
    return (pointer)1;  /* non-NULL required to indicate success */
}

#endif
