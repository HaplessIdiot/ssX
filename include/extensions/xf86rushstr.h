/* $XFree86$ */
/*

Copyright (c) 1998  Daryll Strauss

*/

#ifndef _XF86RUSHSTR_H_
#define _XF86RUSHSTR_H_

#include "xf86rush.h"

#define XF86RUSHNAME "XFree86-Rush"

#define XF86RUSH_MAJOR_VERSION	1	/* current version numbers */
#define XF86RUSH_MINOR_VERSION	0

typedef struct _XF86RushQueryVersion {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushQueryVersion */
    CARD16	length B16;
} xXF86RushQueryVersionReq;
#define sz_xXF86RushQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of Rush protocol */
    CARD16	minorVersion B16;	/* minor version of Rush protocol */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXF86RushQueryVersionReply;
#define sz_xXF86RushQueryVersionReply	32

typedef struct _XF86RushLockPixmap {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushLockPixmap */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
    CARD32	pixmap B32;
} xXF86RushLockPixmapReq;
#define sz_xXF86RushLockPixmapReq	12

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	addr B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXF86RushLockPixmapReply;
#define sz_xXF86RushLockPixmapReply	32

typedef struct _XF86RushUnlockPixmap {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushUnlockPixmap */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
    CARD32	pixmap B32;
} xXF86RushUnlockPixmapReq;
#define sz_xXF86RushUnlockPixmapReq	12

typedef struct _XF86RushUnlockAllPixmaps {
    CARD8	reqType;		/* always RushReqCode */
    CARD8	rushReqType;		/* always X_RushUnlockAllPixmaps */
    CARD16      length B16;
} xXF86RushUnlockAllPixmapsReq;
#define sz_xXF86RushUnlockAllPixmapsReq   4

#endif /* _XF86RUSHSTR_H_ */
