/* $XFree86$ */

#include "xf86Module.h"
#include "PEX.h"

MODULEINITPROTO(pex5ModuleInit);
static MODULESETUPPROTO(pex5Setup);

extern void PexExtensionInit(INITARGS);

static ExtensionModule pex5Ext = {
    PexExtensionInit,
    PEX_NAME_STRING,
    NULL};

static XF86ModuleVersionInfo VersRec =
{
	"pex5",
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
