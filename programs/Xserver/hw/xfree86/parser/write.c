/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/write.c,v 1.1.2.2 1998/01/30 07:31:46 dawes Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

int
xf86WriteConfigFile (const char *filename, XF86ConfigPtr cptr)
{
	FILE *cf;

	if ((cf = fopen (filename, "w")) == NULL)
	{
		return 0;
	}

	printLayoutSection (cf, cptr->conf_layout_lst);

	fprintf (cf, "Section \"Files\"\n");
	printFileSection (cf, cptr->conf_files);
	fprintf (cf, "EndSection\n\n");

	fprintf (cf, "Section \"Module\"\n");
	printModuleSection (cf, cptr->conf_modules);
	fprintf (cf, "EndSection\n\n");

	printVendorSection (cf, cptr->conf_vendor_lst);

	printServerFlagsSection (cf, cptr->conf_flags);

	fprintf (cf, "Section \"Keyboard\"\n");
	printKeyboardSection (cf, cptr->conf_keyboard);
	fprintf (cf, "EndSection\n\n");

	fprintf (cf, "Section \"Pointer\"\n");
	printPointerSection (cf, cptr->conf_pointer);
	fprintf (cf, "EndSection\n\n");

	printMonitorSection (cf, cptr->conf_monitor_lst);

	printDeviceSection (cf, cptr->conf_device_lst);

	printScreenSection (cf, cptr->conf_screen_lst);

	return 1;
}
