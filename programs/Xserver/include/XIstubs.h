/* $XConsortium$ */
/* $XFree86$ */

/************************************************************

Copyright (c) 1996  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

********************************************************/

#ifndef XI_STUBS_H
#define XI_STUBS_H 1

int
ChangeKeyboardDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* old_dev */,
	DeviceIntPtr           /* new_dev */
#endif
	);

int
ChangePointerDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* old_dev */,
	DeviceIntPtr           /* new_dev */,
	unsigned char          /* x */,
	unsigned char          /* y */
#endif
	);

void
CloseInputDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* d */,
	ClientPtr              /* client */
#endif
	);

void
AddOtherInputDevices (
#if NeedFunctionPrototypes
	void
#endif
	);

void
OpenInputDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */,
	int *                  /* status */
#endif
	);

int
SetDeviceMode (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* mode */
#endif
	);

int
SetDeviceValuators (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int *                  /* valuators */,
	int                    /* first_valuator */,
	int                    /* num_valuators */
#endif
	);

int
ChangeDeviceControl (
#if NeedFunctionPrototypes
	ClientPtr             /* client */,
	DeviceIntPtr          /* dev */,
	xDeviceCtl *          /* control */
#endif
	);

#endif /* XI_STUBS_H */
