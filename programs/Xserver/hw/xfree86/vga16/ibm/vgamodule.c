/* $XFree86: xc/programs/Xserver/hw/xfree86/vga16/ibm/vgamodule.c,v 1.2 1998/03/20 21:38:02 hohndel Exp $ */
/*
 * Copyright (C) 1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

#ifdef XFree86LOADER
#include "xf86.h"
#include "xf86Version.h"

static XF86ModuleVersionInfo VersRec =
{
        "libvga16.a",
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
        * data = (pointer) &VersRec;
        * magic= MAGIC_VERSION;
        break;
    default:
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif
