/* $XFree86: xc/include/extensions/xf86dgastr.h,v 3.0 1995/12/09 11:03:16 dawes Exp $ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995  XFree86 Inc.

*/

#ifndef _XF86DGASTR_H_
#define _XF86DGASTR_H_

#include "xf86dga.h"

#define XF86DGANAME "XFree86-DGA"

#define XF86DGA_MAJOR_VERSION	0	/* current version numbers */
#define XF86DGA_MINOR_VERSION	1

typedef struct _XF86DGAQueryVersion {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_DGAQueryVersion */
    CARD16	length B16;
} xXF86DGAQueryVersionReq;
#define sz_xXF86DGAQueryVersionReq	4

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
} xXF86DGAQueryVersionReply;
#define sz_xXF86DGAQueryVersionReply	32

typedef struct _XF86DGAGetVideoLL {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGAGetVideoLL */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16      pad B16;
} xXF86DGAGetVideoLLReq;
#define sz_xXF86DGAGetVideoLLReq	8

typedef struct _XF86DGAInstallColormap{
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad2; 
    CARD32	id B32;  /* colormap. */
} xXF86DGAInstallColormapReq;
#define sz_xXF86DGAInstallColormapReq        12


typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	offset B32;
    CARD32	width B32;
    CARD32	bank_size B32;
    CARD32	ram_size B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXF86DGAGetVideoLLReply;
#define sz_xXF86DGAGetVideoLLReply	32

typedef struct _XF86DGADirectVideo {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGADirectVideo */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	enable B16;
} xXF86DGADirectVideoReq;
#define sz_xXF86DGADirectVideoReq	8


typedef struct _XF86DGAGetViewPort {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGAGetViewPort */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16      pad B16;
} xXF86DGAGetViewPortReq;
#define sz_xXF86DGAGetViewPortReq	8

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	x B32;
    CARD32	y B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXF86DGAGetViewPortReply;
#define sz_xXF86DGAGetViewPortReply	32

typedef struct _XF86DGASetViewPort {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGASetViewPort */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16	pad B16;
    CARD32      x B32;
    CARD32	y B32;
} xXF86DGASetViewPortReq;
#define sz_xXF86DGASetViewPortReq	16

typedef struct _XF86DGAGetVidPage {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGAGetVidPage */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16      pad B16;
} xXF86DGAGetVidPageReq;
#define sz_xXF86DGAGetVidPageReq	8

typedef struct {
    BYTE	type;
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	vpage B32;
    CARD32	pad B32;
} xXF86DGAGetVidPageReply;
#define sz_xXF86DGAGetVidPageReply	16


typedef struct _XF86DGASetVidPage {
    CARD8	reqType;		/* always DGAReqCode */
    CARD8	dgaReqType;		/* always X_XF86DGASetVidPage */
    CARD16	length B16;
    CARD16	screen B16;
    CARD16      vpage B16;
} xXF86DGASetVidPageReq;
#define sz_xXF86DGASetVidPageReq	8

#endif /* _XF86DGASTR_H_ */

