/* $XFree86: xc/include/extensions/xf86mscstr.h,v 3.0 1996/01/16 15:00:33 dawes Exp $ */

/*
 * Copyright (c) 1995, 1996  The XFree86 Project, Inc
 */

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
