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
 * Interface of 'exevents.c'
 */

#ifndef EXEVENTS_H
#define EXEVENTS_H

void
RegisterOtherDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* device */
#endif
	);

void
ProcessOtherEvent (
#if NeedFunctionPrototypes
	xEventPtr /* FIXME deviceKeyButtonPointer * xE */,
	DeviceIntPtr           /* other */,
	int                    /* count */
#endif
	);

int
InitProximityClassDeviceStruct(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */
#endif
	);

void
InitValuatorAxisStruct(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	int                    /* axnum */,
	int                    /* minval */,
	int                    /* maxval */,
	int                    /* resolution */,
	int                    /* min_res */,
	int                    /* max_res */
#endif
	);

void
DeviceFocusEvent(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	int                    /* type */,
	int                    /* mode */,
	int                    /* detail */,
	WindowPtr              /* pWin */
#endif
	);

int
GrabButton(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* button */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Cursor                 /* rcursor */,
	Window                 /* rconfineTo */,
	Mask                   /* eventMask */
#endif
	);

int
GrabKey(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* key */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Mask                   /* mask */
#endif
	);

int
SelectForWindow(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Mask                   /* exclusivemasks */,
	Mask                   /* validmasks */
#endif
	);

int 
AddExtensionClient (
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* mskidx */
#endif
	);

void
RecalculateDeviceDeliverableEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */
#endif
	);

int
InputClientGone(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	XID                    /* id */
#endif
	);

int
SendEvent (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	Window                 /* dest */,
	Bool                   /* propagate */,
	xEvent *               /* ev */,
	Mask                   /* mask */,
	int                    /* count */
#endif
	);

int
SetButtonMapping (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* nElts */,
	BYTE *                 /* map */
#endif
	);

int 
SetModifierMapping(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* len */,
	int                    /* rlen */,
	int                    /* numKeyPerModifier */,
	KeyCode *              /* inputMap */,
	KeyClassPtr *          /* k */
#endif
	);

void
SendDeviceMappingNotify(
#if NeedFunctionPrototypes
	CARD8                  /* request, */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* count */,
	DeviceIntPtr           /* dev */
#endif
);

int
ChangeKeyMapping(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned               /* len */,
	int                    /* type */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* keyCodes */,
	CARD8                  /* keySymsPerKeyCode */,
	KeySym *               /* map */
#endif
	);

void
DeleteWindowFromAnyExtEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	Bool                   /* freeResources */
#endif
);

void
DeleteDeviceFromAnyExtEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	DeviceIntPtr           /* dev */
#endif
	);

int
MaybeSendDeviceMotionNotifyHint (
#if NeedFunctionPrototypes
	deviceKeyButtonPointer * /* pEvents */,
	Mask                   /* mask */
#endif
);

void
CheckDeviceGrabAndHintWindow (
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	int                    /* type */,
	deviceKeyButtonPointer * /* xE */,
	GrabPtr                /* grab */,
	ClientPtr              /* client */,
	Mask                   /* deliveryMask */
#endif
	);

Mask
DeviceEventMaskForClient(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */
#endif
);

void
MaybeStopDeviceHint(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */
#endif
	);

int
DeviceEventSuppressForWindow(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* maskndx */
#endif
	);

#endif /* EXEVENTS_H */
