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

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */
#ifndef EXGLOBALS_H
#define EXGLOBALS_H 1

extern	int 	IReqCode;
extern	int	BadDevice;
extern	int	BadEvent;
extern	int	BadMode;
extern	int	DeviceBusy;
extern	int	BadClass;

extern	Mask	DevicePointerMotionMask;
extern	Mask	DevicePointerMotionHintMask;
extern	Mask	DeviceFocusChangeMask;
extern	Mask	DeviceStateNotifyMask;
extern	Mask	ChangeDeviceNotifyMask;
extern	Mask	DeviceMappingNotifyMask;
extern	Mask	DeviceOwnerGrabButtonMask;
extern	Mask	DeviceButtonGrabMask;
extern	Mask	DeviceButtonMotionMask;
extern	Mask	PropagateMask[];

extern	int	DeviceValuator;
extern	int	DeviceKeyPress;
extern	int	DeviceKeyRelease;
extern	int	DeviceButtonPress;
extern	int	DeviceButtonRelease;
extern	int	DeviceMotionNotify;
extern	int	DeviceFocusIn;
extern	int	DeviceFocusOut;
extern	int	ProximityIn;
extern	int	ProximityOut;
extern	int	DeviceStateNotify;
extern	int	DeviceKeyStateNotify;
extern	int	DeviceButtonStateNotify;
extern	int	DeviceMappingNotify;
extern	int	ChangeDeviceNotify;

extern	int	RT_INPUTCLIENT;

/* FIXME: in dix */
extern	InputInfo inputInfo;
extern	void		(* ReplySwapVector[256]) (
#if NeedFunctionPrototypes
		ClientPtr ,
		int ,
		void *
#endif
	);

#endif /* EXGLOBALS_H */
