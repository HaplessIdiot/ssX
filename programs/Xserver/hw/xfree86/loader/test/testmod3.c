/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/test/testmod3.c,v 1.1.2.1 1998/07/04 13:44:05 dawes Exp $ */

#include "xf86Module.h"

MODULEINITPROTO(test3ModuleInit);
static MODULESETUPPROTO(test3Setup);
static MODULETEARDOWNPROTO(test3Teardown);

static XF86ModuleVersionInfo versRec =
{
	"test3",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00000003,
	ABI_CLASS_ANSIC,
	ABI_ANSIC_VERSION,
	{0,0,0,0}
};

void
test3ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    static int count = 0;

    *vers = &versRec;
    *setup = test3Setup;
    *teardown = test3Teardown;

    count++;
    ErrorF("test3ModuleInit call # %d\n", count);
}

static pointer
test3Setup(pointer mod, pointer opts, int *errmaj, int *errmin)
{
    static int count = 0;

    count++;
    ErrorF("test3Setup call # %d\n", count);
    return (pointer)count;
}

void
test3Teardown(pointer data)
{
    static int count = 0;

    count++;
    ErrorF("test3Teardown call # %d with data %d\n", count, (int)data);
}
