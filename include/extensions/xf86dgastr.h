/* $XFree86: xc/include/extensions/xf86dgastr.h,v 3.4 1999/03/28 15:31:33 dawes Exp $ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995  XFree86 Inc.

*/

#ifndef _XF86DGASTR_H_
#define _XF86DGASTR_H_

#include "xf86dga.h"
#include "xf86dga1str.h"

#define XF86DGANAME "XFree86-DGA"

#define XDGA_MAJOR_VERSION	2	/* current version numbers */
#define XDGA_MINOR_VERSION	0


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

typedef struct _XDGAQueryModes {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
} xXDGAQueryModesReq;
#define sz_xXDGAQueryModesReq		8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	number B32;		/* number of modes available */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXDGAQueryModesReply;
#define sz_xXDGAQueryModesReply	32


typedef struct _XDGASetMode {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	mode B32;		/* mode number to init */
    CARD32	pid B32;		/* Pixmap descriptor */
} xXDGASetModeReq;
#define sz_xXDGASetModeReq		16

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;			/* pixmap is available */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	offset B32;		/* offset into framebuffer map */
    CARD32	flags B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXDGASetModeReply;
#define sz_xXDGASetModeReply	32

typedef struct {
   CARD8	byte_order;
   CARD8	depth;
   CARD16 	num B16;
   CARD16	bpp B16;
   CARD16	name_size B16;
   CARD32	vsync_num B32;
   CARD32	vsync_den B32;
   CARD32	flags B32;
   CARD16	image_width B16;
   CARD16	image_height B16;
   CARD16	pixmap_width B16;
   CARD16	pixmap_height B16;
   CARD32	bytes_per_scanline B32;
   CARD32	red_mask B32;
   CARD32	green_mask B32;
   CARD32	blue_mask B32;
   CARD16	viewport_width B16;
   CARD16	viewport_height B16;
   CARD16	viewport_xstep B16;
   CARD16	viewport_ystep B16;
   CARD16	viewport_xmax B16;
   CARD16	viewport_ymax B16;
   CARD32	viewport_flags B32;
   CARD32	reserved1 B32;
   CARD32	reserved2 B32;
} xXDGAModeInfo;
#define sz_xXDGAModeInfo 68

typedef struct _XDGAOpenFramebuffer {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
} xXDGAOpenFramebufferReq;
#define sz_xXDGAOpenFramebufferReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;		/* device name size if there is one */
    CARD32	mem1 B32;		/* physical memory */	
    CARD32	mem2 B32;		/* spillover for _alpha_ */
    CARD32	size B32;		/* size of map in bytes */
    CARD32	offset B32;		/* optional offset into device */
    CARD32	extra B32;		/* extra info associated with the map */
    CARD32	pad2 B32;
} xXDGAOpenFramebufferReply;
#define sz_xXDGAOpenFramebufferReply	32


typedef struct _XDGACloseFramebuffer {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
} xXDGACloseFramebufferReq;
#define sz_xXDGACloseFramebufferReq	8


typedef struct _XDGASetViewport {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
    CARD16	x B16;
    CARD16	y B16;
    CARD32	flags B32;
} xXDGASetViewportReq;
#define sz_xXDGASetViewportReq	16


typedef struct _XDGAInstallColormap {
    CARD8	reqType;
    CARD8	dgaReqType;
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	cmap B32;
} xXDGAInstallColormapReq;
#define sz_xXDGAInstallColormapReq	12



#endif /* _XF86DGASTR_H_ */

