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

/********************************************************************
 * Interface of extinit.c
 */

#ifndef EXTINIT_H
#define EXTINIT_H

void
XInputExtensionInit(
#if NeedFunctionPrototypes
	void
#endif
	);


int
ProcIDispatch (
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
SProcIDispatch(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

void
SReplyIDispatch (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	int                    /* len */,
	xGrabDeviceReply *     /* rep */
#endif
	);

void
SEventIDispatch (
#if NeedFunctionPrototypes
	xEvent *               /* from */,
	xEvent *               /* to */
#endif
	);

void
SEventDeviceValuator (
#if NeedFunctionPrototypes
	deviceValuator *       /* from */,
	deviceValuator *       /* to */
#endif
	);

void
SEventFocus (
#if NeedFunctionPrototypes
	deviceFocus *          /* from */,
	deviceFocus *          /* to */
#endif
	);

void
SDeviceStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceStateNotify *    /* from */,
	deviceStateNotify *    /* to */
#endif
	);

void
SDeviceKeyStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceKeyStateNotify * /* from */,
	deviceKeyStateNotify * /* to */
#endif
	);

void
SDeviceButtonStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceButtonStateNotify * /* from */,
	deviceButtonStateNotify * /* to */
#endif
	);

void
SChangeDeviceNotifyEvent (
#if NeedFunctionPrototypes
	changeDeviceNotify *   /* from */,
	changeDeviceNotify *   /* to */
#endif
	);

void
SDeviceMappingNotifyEvent (
#if NeedFunctionPrototypes
	deviceMappingNotify *  /* from */,
	deviceMappingNotify *  /* to */
#endif
	);

void
FixExtensionEvents (
#if NeedFunctionPrototypes
	ExtensionEntry 	*      /* extEntry */
#endif
	);

void
RestoreExtensionEvents (
#if NeedFunctionPrototypes
	void
#endif
	);

void
IResetProc(
#if NeedFunctionPrototypes
	ExtensionEntry *       /* unused */
#endif
	);

void
AssignTypeAndName (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	Atom                   /* type */,
	char *                 /* name */
#endif
	);

void
MakeDeviceTypeAtoms (
#if NeedFunctionPrototypes
	void
#endif
);

DeviceIntPtr
LookupDeviceIntRec (
#if NeedFunctionPrototypes
	CARD8                  /* id */
#endif
	);

void
SetExclusiveAccess (
#if NeedFunctionPrototypes
	Mask                   /* mask */
#endif
	);

void
AllowPropagateSuppress (
#if NeedFunctionPrototypes
	Mask                   /* mask */
#endif
	);

Mask
GetNextExtEventMask (
#if NeedFunctionPrototypes
	void
#endif
);

void
SetMaskForExtEvent(
#if NeedFunctionPrototypes
	Mask                   /* mask */,
	int                    /* event */
#endif
	);

void
SetEventInfo(
#if NeedFunctionPrototypes
	Mask                   /* mask */,
	int                    /* constant */
#endif
	);

#endif /* EXTINIT_H */
