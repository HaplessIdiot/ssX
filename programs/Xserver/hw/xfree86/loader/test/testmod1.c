/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/test/testmod1.c,v 1.1.2.1 1998/07/04 13:44:04 dawes Exp $ */

#include "xf86Module.h"

MODULEINITPROTO(test1ModuleInit);
static MODULESETUPPROTO(test1Setup);
static MODULETEARDOWNPROTO(test1Teardown);

static XF86ModuleVersionInfo versRec =
{
	"test1",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00000001,
	ABI_CLASS_ANSIC,
	ABI_ANSIC_VERSION,
	{0,0,0,0}
};

void
test1ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    static int count = 0;

    *vers = &versRec;
    *setup = test1Setup;
    *teardown = test1Teardown;

    count++;
    ErrorF("test1ModuleInit call # %d\n", count);
}

static pointer
test1Setup(pointer mod, pointer opts, int *errmaj, int *errmin)
{
    static int count = 0;
    pointer p;

    count++;
    ErrorF("test1Setup call # %d\n", count);
    if (count >= 2) {
	p = LoadSubModule(mod, "test2", NULL, opts, errmaj, errmin);
	p = LoadSubModule(mod, "test3", NULL, opts, errmaj, errmin);
	return p;
    }
    return (pointer)count;
}

void
test1Teardown(pointer data)
{
    static int count = 0;

    count++;
    ErrorF("test1Teardown call # %d with data %d\n", count, (int)data);
}
