/* $XFree86: xc/programs/Xserver/hw/xfree86/rac/xf86RACmodule.c,v 1.2 1998/12/13 10:33:50 dawes Exp $ */

#include "xf86Module.h"
MODULEINITPROTO(racModuleInit);

static XF86ModuleVersionInfo racVersRec =
{
    "rac",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_VIDEODRV,		/* a video driver module */
    ABI_VIDEODRV_VERSION,
    NULL,
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
