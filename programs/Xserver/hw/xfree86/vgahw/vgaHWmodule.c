/* $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHWmodule.c,v 1.2 1998/07/25 16:58:36 dawes Exp $ */

/*
 * Copyright 1998 by The XFree86 Project, Inc
 */

#ifdef XFree86LOADER

#include "xf86Module.h"

MODULEINITPROTO(vgahwModuleInit);

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

#endif
