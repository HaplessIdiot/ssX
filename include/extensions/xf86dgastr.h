/* $XFree86: xc/include/extensions/xf86dgastr.h,v 3.3 1996/10/18 14:57:25 dawes Exp $ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995  XFree86 Inc.

*/

#ifndef _XF86DGASTR_H_
#define _XF86DGASTR_H_

#include "xf86dga.h"
#include "xf86dga1str.h"

#define XF86DGANAME "XFree86-DGA"

#define XF86DGA_MAJOR_VERSION	2	/* current version numbers */
#define XF86DGA_MINOR_VERSION	0


typedef struct _XDGAQueryVersion {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_DGAQueryVersion */
    CARD16	length B16;
} xXDGAQueryVersionReq;
#define sz_xXDGAQueryVersionReq		4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of DGA protocol */
    CARD16	minorVersion B16;	/* minor version of DGA protocol */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXDGAQueryVersionReply;
#define sz_xXDGAQueryVersionReply	32



#endif /* _XF86DGASTR_H_ */

