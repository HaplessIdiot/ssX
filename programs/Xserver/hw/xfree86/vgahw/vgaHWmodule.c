/* $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHWmodule.c,v 1.1.2.4 1998/07/19 13:22:09 dawes Exp $ */

/*
 * Copyright 1998 by The XFree86 Project, Inc
 */

#ifdef XFree86LOADER

#include "xf86Module.h"

MODULEINITPROTO(vgahwModuleInit);
static MODULESETUPPROTO(vgaHWSetup);

static XF86ModuleVersionInfo VersRec = {
	"vgahw",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00000001,			/* version 0.1 */
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	{0, 0, 0, 0}
};

void
vgahwModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = NULL;
    *teardown = NULL;
}

static pointer
vgaHWSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    /* vgaHW's colourmap support needs cfb's */
    return LoadSubModule(module, "cfb", NULL, NULL, errmaj, errmin);
}

#endif
