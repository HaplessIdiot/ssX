/* $XFree86$ */

#include "xf86Module.h"
MODULEINITPROTO(racModuleInit);

static XF86ModuleVersionInfo racVersRec =
{
    "rac",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    0x00010001,			/* 1.1 */
    ABI_CLASS_VIDEODRV,		/* a video driver module */
    ABI_VIDEODRV_VERSION,
    {0,0,0,0}
};


void
racModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &racVersRec;
    *setup = NULL;
    *teardown = NULL;
}
