/* (c) Itai Nahshon
 *
 * This code is derived from and inspired by the I2C driver
 * from the Linux kernel.
 *      (c) 1998 Gerd Knorr <kraxel@cs.tu-berlin.de>
 */

/* $XFree86$ */

#include "xf86Module.h"

MODULEINITPROTO(i2cModuleInit);
static MODULESETUPPROTO(i2cSetup);

static XF86ModuleVersionInfo i2cVersRec =
{
        "i2c",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,
        ABI_CLASS_VIDEODRV,                     /* This is a ????  driver */
        ABI_VIDEODRV_VERSION,
        {0,0,0,0}
};

void
i2cModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
              ModuleTearDownProc *teardown)
{
    *vers = &i2cVersRec;
    *setup = i2cSetup;
    *teardown = NULL;
}

static pointer
i2cSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
   ErrorF("i2cSetup\n");
   return (pointer)1;
}
