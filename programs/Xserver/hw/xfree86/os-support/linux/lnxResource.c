/* $XFree86: $ */

/* Resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"

#ifdef __alpha__
resPtr
xf86AccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range,0,0xffffffff,ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range,0,0xffffffff,ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

resPtr
xf86AccResFromOS(resPtr ret)
{
    resRange range;

    /*
     * Fallback is to claim the following areas:
     *
     * 0x000C0000 - 0x000EFFFF  location of VGA and other extensions ROMS
     */

    RANGE(range,0xc0000,0xeffff,ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to claim well known ports in the 0x0 - 0x3ff range */
    /* Possibly should claim some of them as sparse ranges */

    RANGE(range,0,0x1ff,ResExcIoBlock | ResEstimated);
    ret = xf86AddResToList(ret, &range, -1);
    /* XXX add others */
    return ret;
}

#endif
