/* $XFree86: xc/programs/Xserver/PEX5/pexmodule.c,v 1.3 1998/12/13 10:33:33 dawes Exp $ */

#include "xf86Module.h"
#include "PEX.h"

MODULEINITPROTO(pex5ModuleInit);
static MODULESETUPPROTO(pex5Setup);

extern void PexExtensionInit(INITARGS);

static ExtensionModule pex5Ext = {
    PexExtensionInit,
    PEX_NAME_STRING,
    NULL,
    NULL
};

static XF86ModuleVersionInfo VersRec =
{
	"pex5",
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
pex5ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	       ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = pex5Setup;
    *teardown = NULL;
}

static pointer
pex5Setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    LoadExtension(&pex5Ext);

    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}
