/* $XFree86: xc/programs/Xserver/XIE/xiemodule.c,v 1.4 1999/01/17 10:53:47 dawes Exp $ */

#include "xf86Module.h"
#include "XIE.h"		

MODULEINITPROTO(xieModuleInit);
static MODULESETUPPROTO(xieSetup);

extern void XieInit(INITARGS);

ExtensionModule XieExt =
{
    XieInit,
    xieExtName,
    NULL,
    NULL
};

static XF86ModuleVersionInfo VersRec =
{
	"xie",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_EXTENSION,
	ABI_EXTENSION_VERSION,
	NULL,
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

