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
/* $XFree86: xc/lib/font/Type1/module/type1mod.c,v 1.3 1998/12/13 05:32:30 dawes Exp $ */

#include "X.h"
#include "misc.h"
#include "xf86_ansic.h"

#include "fontmod.h"
#include "xf86Module.h"

MODULEINITPROTO(type1ModuleInit);
static MODULESETUPPROTO(type1Setup);

    /*
     * this is the module init function that is executed when loading
     * libtype1 as a module. Its name has to be ModuleInit.
     * With this we initialize the function and variable pointers used
     * in generic parts of XFree86
     */

static XF86ModuleVersionInfo VersRec =
{
	"type1",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_FONT,			/* Font module */
	ABI_FONT_VERSION,
	{0,0,0,0}       /* signature, to be patched into the file by a tool */
};

void
type1ModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = type1Setup;
    *teardown = NULL;
}

extern void Type1RegisterFontFileFunctions(void);

FontModule type1Module = {
    Type1RegisterFontFileFunctions,
    "Type1",
    NULL
};

static pointer
type1Setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    type1Module.module = module;
    LoadFont(&type1Module);

    /* Need a non-NULL return */
    return (pointer)1;
}
