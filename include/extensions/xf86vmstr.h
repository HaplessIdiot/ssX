/* $XConsortium: xf86vmstr.h /main/2 1995/09/01 10:41:25 kaleb $ */
/* $XFree86: xc/include/extensions/xf86vmstr.h,v 3.7 1995/12/02 05:03:04 dawes Exp $ */
/*

Copyright (c) 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from the Kaleb S. KEITHLEY

*/
/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86VIDMODESTR_H_
#define _XF86VIDMODESTR_H_

#include "xf86vmode.h"

#define XF86VIDMODENAME "XFree86-VidModeExtension"

#define XF86VIDMODE_MAJOR_VERSION	0	/* current version numbers */
#define XF86VIDMODE_MINOR_VERSION	4

typedef struct _VGAHelpQueryVersion {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_VGAHelpQueryVersion */
    CARD16	length B16;
} xVGAHelpQueryVersionReq;
#define sz_xVGAHelpQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of VGAHelp protocol */
    CARD16	minorVersion B16;	/* minor version of VGAHelp protocol */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xVGAHelpQueryVersionReply;
#define sz_xVGAHelpQueryVersionReply	32

typedef struct _VGAHelpGetModeLine {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_VGAHelpGetModeLine */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
} xVGAHelpGetModeLineReq, xVGAHelpGetMonitorReq, xXF86VidModeGetSaverReq;
#define sz_xVGAHelpGetModeLineReq	8
#define sz_xVGAHelpGetMonitorReq	8
#define sz_xXF86VidModeGetSaverReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	dotclock B32;
    CARD16	hdisplay B16;
    CARD16	hsyncstart B16;
    CARD16	hsyncend B16;
    CARD16	htotal B16;
    CARD16	vdisplay B16;
    CARD16	vsyncstart B16;
    CARD16	vsyncend B16;
    CARD16	vtotal B16;
    CARD32	flags B32;
    CARD32	privsize B32;
} xVGAHelpGetModeLineReply;
#define sz_xVGAHelpGetModeLineReply	36

typedef struct _VGAHelpModModeLine {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_VGAHelpModModeLine */
    CARD16	length B16;
    CARD32	screen B32;
    CARD16	hdisplay B16;
    CARD16	hsyncstart B16;
    CARD16	hsyncend B16;
    CARD16	htotal B16;
    CARD16	vdisplay B16;
    CARD16	vsyncstart B16;
    CARD16	vsyncend B16;
    CARD16	vtotal B16;
    CARD32	flags B32;
    CARD32	privsize B32;
} xVGAHelpModModeLineReq;
#define sz_xVGAHelpModModeLineReq	32

typedef struct _VGAHelpSwitchMode {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_VGAHelpSwitchMode */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	zoom B16;
} xVGAHelpSwitchModeReq;
#define sz_xVGAHelpSwitchModeReq	8

typedef struct _XF86VidModeLockModeSwitch {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_XF86VidModeLockModeSwitch */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	lock B16;
} xXF86VidModeLockModeSwitchReq;
#define sz_xXF86VidModeLockModeSwitchReq	8

typedef struct _XF86VidModeSetSaver {
    CARD8	reqType;		/* always VgaHelpReqCode */
    CARD8	vgahelpReqType;		/* always X_XF86VidModeSetSaver */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
    CARD32	suspendTime B32;
    CARD32	offTime B32;
} xXF86VidModeSetSaverReq;
#define sz_xXF86VidModeSetSaverReq	16

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD8	vendorLength;
    CARD8	modelLength;
    CARD8	nhsync;
    CARD8	nvsync;
    CARD32	bandwidth B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xVGAHelpGetMonitorReply;
#define sz_xVGAHelpGetMonitorReply	32

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	suspendTime B32;
    CARD32	offTime B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXF86VidModeGetSaverReply;
#define sz_xXF86VidModeGetSaverReply	32

#endif /* _XF86VIDMODESTR_H_ */
