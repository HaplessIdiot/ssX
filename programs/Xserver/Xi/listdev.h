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

#ifndef LISTDEV_H
#define LISTDEV_H 1

int
SProcXListInputDevices(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
ProcXListInputDevices (
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

void
SizeDeviceInfo (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* d */,
	int *                  /* namesize */,
	int *                  /* size */
#endif
	);

void
ListDeviceInfo (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	xDeviceInfoPtr         /* dev */,
	char **                /* devbuf */,
	char **                /* classbuf */,
	char **                /* namebuf */
#endif
	);

void
CopyDeviceName (
#if NeedFunctionPrototypes
	char **                /* namebuf */,
	char *                 /* name */
#endif
	);

void
CopySwapDevice (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	int                    /* num_classes */,
	char **                /* buf */
#endif
	);

void
CopySwapKeyClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	KeyClassPtr            /* k */,
	char **                /* buf */
#endif
	);

void
CopySwapButtonClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	ButtonClassPtr         /* b */,
	char **                /* buf */
#endif
	);

int
CopySwapValuatorClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	ValuatorClassPtr       /* v */,
	char **                /* buf */
#endif
	);

void
SRepXListInputDevices (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	int                    /* size */,
	xListInputDevicesReply * /* rep */
#endif
	);

#endif /* LISTDEV_H */
