/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86allocscripi.c,v 1.1tsi Exp $ */

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

int
xf86AllocateScrnInfoPrivateIndex()
{
    return -1;
}

