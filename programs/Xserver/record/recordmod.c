/* $XFree86: xc/programs/Xserver/record/recordmod.c,v 1.2 1998/08/13 14:46:16 dawes Exp $ */

#include "xf86Module.h"

extern Bool noTestExtensions;

MODULEINITPROTO(recordModuleInit);
static MODULESETUPPROTO(recordSetup);

extern void RecordExtensionInit(INITARGS);

ExtensionModule recordExt = {
    RecordExtensionInit,
    "RECORD",
    &noTestExtensions,
    NULL
};

static XF86ModuleVersionInfo VersRec = {
	"record",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 13, 0,
	ABI_CLASS_EXTENSION,
	ABI_EXTENSION_VERSION,
	{0,0,0,0}
};

void
recordModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		 ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = recordSetup;
    *teardown = NULL;
}

static pointer
recordSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    LoadExtension(&recordExt);

    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}

