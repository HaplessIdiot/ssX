/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/cache/xf86icache.c,v 3.2 1997/06/03 14:11:18 hohndel Exp $ */

/*
 * To get rid of complains from the Loader about no existing ModuleInit routine
 *
 * (mr ;-) Sat Apr 19 17:10:11 MET 1997)
 *
 */

#ifdef XFree86LOADER
#include "xf86.h"

#include "xf86Priv.h"
#include "xf86Procs.h"
#include "xf86_HWlib.h"
#include "xf86_Config.h"

#include "xf86Version.h"

XF86ModuleVersionInfo xf86cacheVersRec =
{
        "libxf86cache.a",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,
        {0,0,0,0}       /* signature, to be patched into the file by a tool */
};

void
ModuleInit(data,magic)
    pointer   * data;
    INT32     * magic;
{
    static int cnt = 0;

    switch(cnt++)
    {
        /* MAGIC_VERSION must be first in ModuleInit */
    case 0:
        * data = (pointer) &xf86cacheVersRec;
        * magic= MAGIC_VERSION;
        break;
    default:
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif
