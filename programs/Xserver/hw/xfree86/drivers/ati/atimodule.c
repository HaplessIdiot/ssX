/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atimodule.c,v 1.1tsi Exp $ */
/*
 * Copyright 1997,1998 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef XFree86LOADER

#include "ati.h"
#include "ativersion.h"
#include "xf86Version.h"

static XF86ModuleVersionInfo ATIModuleVersionInfo =
{
    "ati_drv.o",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    ATI_VERSION_CURRENT,
    {0, 0, 0, 0}
};

/*
 * ModuleInit --
 *
 * This function provides the interface with the module loader.  Its name must
 * be ModuleInit.
 */
void
ModuleInit(void ** ModuleDataValue, INT32 * ModuleDataType)
{
    static unsigned int CallCount = 0;

    switch(CallCount++)
    {
        case 0:
            /* Return module version information */
            *ModuleDataValue = &ATIModuleVersionInfo;
            *ModuleDataType = MAGIC_VERSION;
            break;

        case 1:
            /* Return driver descriptor */
            *ModuleDataValue = &ATI;
            *ModuleDataType = MAGIC_ADD_VIDEO_CHIP_REC;
            break;

        default:
            /* Ask loader for VGA support */
            xf86issvgatype = TRUE;
            *ModuleDataType = MAGIC_DONE;
            break;
    }
}
#endif
