/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/test/testmod2.c,v 1.1.2.1 1998/07/04 13:44:05 dawes Exp $ */

#include "xf86Module.h"

MODULEINITPROTO(test2ModuleInit);
static MODULESETUPPROTO(test2Setup);
static MODULETEARDOWNPROTO(test2Teardown);

static XF86ModuleVersionInfo versRec =
{
	"test2",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00000002,
	ABI_CLASS_ANSIC,
	ABI_ANSIC_VERSION,
	{0,0,0,0}
};

void
test2ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    static int count = 0;

    *vers = &versRec;
    *setup = test2Setup;
    *teardown = test2Teardown;

    count++;
    ErrorF("test2ModuleInit call # %d\n", count);
}

static pointer
test2Setup(pointer mod, pointer opts, int *errmaj, int *errmin)
{
    static int count = 0;
    pointer p;

    count++;
    ErrorF("test2Setup call # %d\n", count);
    if (count >= 2) {
	p = LoadSubModule(mod, "test3", NULL, opts, errmaj, errmin);
	return p;
    }
    return (pointer)count;
}

void
test2Teardown(pointer data)
{
    static int count = 0;

    count++;
    ErrorF("test2Teardown call # %d with data %d\n", count, (int)data);
}
