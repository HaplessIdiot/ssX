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

#ifndef CHGFCTL_H
#define CHGFCTL_H 1

int
SProcXChangeFeedbackControl(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
ProcXChangeFeedbackControl(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
ChangeKbdFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	KbdFeedbackPtr         /* k */,
	xKbdFeedbackCtl *      /* f */
#endif
	);

int
ChangePtrFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	PtrFeedbackPtr         /* p */,
	xPtrFeedbackCtl *      /* f */
#endif
	);

int
ChangeIntegerFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	IntegerFeedbackPtr     /* i */,
	xIntegerFeedbackCtl *  /* f */
#endif
	);

int
ChangeStringFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	StringFeedbackPtr      /* s */,
	xStringFeedbackCtl *   /* f */
#endif
	);

int
ChangeBellFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	BellFeedbackPtr        /* b */,
	xBellFeedbackCtl *     /* f */
#endif
	);

int
ChangeLedFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	LedFeedbackPtr         /* l */,
	xLedFeedbackCtl *      /* f */
#endif
	);

#endif /* CHGFCTL_H */
