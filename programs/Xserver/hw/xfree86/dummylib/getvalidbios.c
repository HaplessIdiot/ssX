/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/getvalidbios.c,v 1.3tsi Exp $ */

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

memType
getValidBIOSBase(PCITAG tag, int num)
{
    return 0;
}
