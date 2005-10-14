/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86opt.c,v 1.2tsi Exp $ */

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86ProcessOptions(int i, pointer p, OptionInfoPtr o)
{
}

Bool
xf86GetOptValBool(const OptionInfoRec *o, int i, Bool *b)
{
    return FALSE;
}

