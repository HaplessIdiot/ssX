/* $XFree86$ */
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

Except as contained in this notice, the name of Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY

*/
/* $XConsortium: xf86vmstr.h /main/6 1996/01/01 09:22:35 kaleb $ */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86MISCSTR_H_
#define _XF86MISCSTR_H_

#include "xf86misc.h"

#define XF86MISCNAME		"XFree86-Misc"

#define XF86MISC_MAJOR_VERSION	0	/* current version numbers */
#define XF86MISC_MINOR_VERSION	0

typedef struct _XF86MiscQueryVersion {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscQueryVersion */
    CARD16	length B16;
} xXF86MiscQueryVersionReq;
#define sz_xXF86MiscQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of XFree86-Misc */
    CARD16	minorVersion B16;	/* minor version of XFree86-Misc */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXF86MiscQueryVersionReply;
#define sz_xXF86MiscQueryVersionReply	32

typedef struct _XF86MiscGetSaver {
    CARD8       reqType;                /* always XF86MiscReqCode */
    CARD8       xf86miscReqType;     /* always X_XF86MiscGetSaver */
    CARD16      length B16; 
    CARD16      screen B16;
    CARD16      pad B16;
} xXF86MiscGetSaverReq;
#define sz_xXF86MiscGetSaverReq	8

typedef struct _XF86MiscSetSaver {
    CARD8	reqType;		/* always XF86MiscReqCode */
    CARD8	xf86miscReqType;	/* always X_XF86MiscSetSaver */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
    CARD32	suspendTime B32;
    CARD32	offTime B32;
} xXF86MiscSetSaverReq;
#define sz_xXF86MiscSetSaverReq	16

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
} xXF86MiscGetSaverReply;
#define sz_xXF86MiscGetSaverReply	32

#endif /* _XF86VIDMODESTR_H_ */
