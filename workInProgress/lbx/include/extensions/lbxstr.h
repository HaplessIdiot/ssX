/* $XConsortium: lbxstr.h,v 1.5 94/03/27 11:43:24 dpw Exp $ */
/*
 * $NCDId: @(#)lbxstr.h,v 1.19 1994/03/22 18:22:47 lemke Exp $
 *
 * Copyright 1992 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of NCD. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCD. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NCD.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, Network Computing Devices
 */
 
#ifndef _LBXSTR_H_
#define _LBXSTR_H_

#include "XLbx.h"

#define LBXNAME "LBX"

#define LBX_MAJOR_VERSION	0	/* current version numbers */
#define LBX_MINOR_VERSION	0

typedef struct {
    CARD8	byteOrder;
    BOOL	useTag;
    CARD16	majorVersion B16, minorVersion B16;
    CARD16	nbytesAuthProto B16;    /* Authorization protocol */
    CARD16	nbytesAuthString B16;   /* Authorization string */
    CARD16	pad2 B16;
} xLbxConnClientPrefix;

typedef struct {
    BOOL	success;		/* TRUE */
    BOOL	tagOnly;
    CARD16	majorVersion B16,
		minorVersion B16;
    CARD16	length B16;		/* 1/4 additional bytes in setup info */
    CARD32	tag B32;
} xLbxConnSetupPrefix;

typedef struct _LbxQueryVersion {
    CARD8	reqType;		/* always LbxReqCode */
    CARD8	lbxReqType;		/* always X_LbxQueryVersion */
    CARD16	length B16;
} xLbxQueryVersionReq;
#define sz_xLbxQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of LBX protocol */
    CARD16	minorVersion B16;	/* minor version of LBX protocol */
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xLbxQueryVersionReply;
#define sz_xLbxQueryVersionReply	32

typedef struct _LbxStartProxy {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxStartProxy */
    CARD16	length B16;
    CARD16	deltaN B16;	/* size of delta cache */
    CARD16	deltaMaxLen B16;/* max length of messages in delta cache */
    CARD32	comptype B32;	/* LbxCompressNone etc */
} xLbxStartProxyReq;
#define sz_xLbxStartProxyReq	    12

typedef struct _LbxStopProxy {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxStopProxy */
    CARD16	length B16;
} xLbxStopProxyReq;
#define sz_xLbxStopProxyReq	    4

typedef struct _LbxSwitch {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxSwitch */
    CARD16	length B16;
    CARD32	client B32;	/* new client */
} xLbxSwitchReq;
#define sz_xLbxSwitchReq	8

typedef struct _LbxNewClient {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxNewClient */
    CARD16	length B16;
    CARD32	client B32;	/* new client */
} xLbxNewClientReq;
#define sz_xLbxNewClientReq	8

typedef struct _LbxCloseClient {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxCloseClient */
    CARD16	length B16;
    CARD32	client B32;	/* new client */
} xLbxCloseClientReq;
#define sz_xLbxCloseClientReq	8

typedef struct _LbxModifySequence {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxModifySequence */
    CARD16	length B16;
    CARD32	adjust B32;
} xLbxModifySequenceReq;
#define sz_xLbxModifySequenceReq    8
    
typedef struct _LbxAllowMotion {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxAllowMotion */
    CARD16	length B16;
    CARD32	num B32;
} xLbxAllowMotionReq;
#define sz_xLbxAllowMotionReq    8
    
typedef struct _LbxIncrementPixel {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxIncrementPixel */
    CARD16	length B16;
    CARD32	cmap B32;
    CARD32	pixel B32;
    CARD32	amount B32;
} xLbxIncrementPixelReq;
#define sz_xLbxIncrementPixelReq    16

typedef struct _LbxDelta {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxDelta */
    CARD16	length B16;
    CARD8	diffs;		/* number of diffs */
    CARD8	cindex;		/* cache index */
				/* list of diffs follows */
} xLbxDeltaReq;
#define sz_xLbxDeltaReq    6

typedef struct _LbxGetModifierMapping {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxGetModifierMapping */
    CARD16	length B16;
} xLbxGetModifierMappingReq;
#define	sz_xLbxGetModifierMappingReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	keyspermod;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	tag B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xLbxGetModifierMappingReply;
#define sz_xLbxGetModifierMappingReply	32

typedef struct _LbxGetKeyboardMapping {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxGetKeyboardMapping */
    CARD16	length B16;
    KeyCode	firstKeyCode;
    CARD8	count;
    CARD16	pad1 B16;
} xLbxGetKeyboardMappingReq;
#define	sz_xLbxGetKeyboardMappingReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	keysperkeycode;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	tag B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xLbxGetKeyboardMappingReply;
#define sz_xLbxGetKeyboardMappingReply	32

typedef struct _LbxQueryFont {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxQueryFont */
    CARD16	length B16;
    CARD32	fid B32;
} xLbxQueryFontReq;
#define	sz_xLbxQueryFontReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	pad;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	tag B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    /* X_QueryFont sticks much of the data in the base reply packet,
     * but we hope that it won't be needed, (and it won't fit in 32 bytes
     * with the tag anyways)
     *
     * if additional data is needed, its sent just like X_QueryFont, with
     * an xQueryFontReply struct plus the xCharInfos
     */
} xLbxQueryFontReply;
#define sz_xLbxQueryFontReply	32

typedef struct _LbxChangeProperty {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxChangeProperty */
    CARD16	length B16;
    Window	window B32;
    Atom	property B32;
    Atom	type B32;
    CARD8	format;
    CARD8	mode;
    BYTE	pad[2];
    CARD32	nUnits B32;
} xLbxChangePropertyReq;
#define	sz_xLbxChangePropertyReq	24

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	pad;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	tag B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xLbxChangePropertyReply;
#define sz_xLbxChangePropertyReply	32

typedef struct _LbxGetProperty {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxGetProperty */
    CARD16	length B16;
    Window	window B32;
    Atom	property B32;
    Atom	type B32;
    CARD8	delete;
    BYTE	pad[3];
    CARD32	longOffset B32;
    CARD32	longLength B32;
} xLbxGetPropertyReq;
#define	sz_xLbxGetPropertyReq	24

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	format;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Atom	propertyType B32;
    CARD32	bytesAfter B32;
    CARD32	nItems B32;
    CARD32	tag B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
} xLbxGetPropertyReply;
#define sz_xLbxGetPropertyReply	32

typedef struct _LbxTagData {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxTagData */
    CARD16	length B16;
    XID		tag B32;
    CARD32	real_length B32;
    /* XXX need other info?  type?  format? */
    /* data */
} xLbxTagDataReq;
#define	sz_xLbxTagDataReq	12

typedef struct _LbxQueryTag {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxQueryTag */
    CARD16	length B16;
    CARD32	tag B32;
} xLbxQueryTagReq;
#define	sz_xLbxQueryTagReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	valid;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	real_length B32;
    CARD32	pad0 B32;		/* byte swapping info ? */
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    /* data */
} xLbxQueryTagReply;
#define sz_xLbxQueryTagReply	32

/* XXX combine in some way with InvalidateTag event? */
typedef struct _LbxInvalidateTag {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;	/* always X_LbxInvalidateTag */
    CARD16	length B16;
    CARD32	tag B32;
} xLbxInvalidateTagReq;
#define	sz_xLbxInvalidateTagReq	8

/* Following used for LbxPolyPoint, LbxPolyLine, LbxPolySegment,
   LbxPolyRectangle, LbxPolyArc, LbxPolyFillRectangle and LbxPolyFillArc */

typedef struct _LbxPolyPoint {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;
    CARD16	length B16;
    Drawable	drawable B32;
    GContext	gc B32;
    CARD8	padBytes;
} xLbxPolyPointReq;
#define sz_xLbxPolyPointReq	13

typedef xLbxPolyPointReq xLbxPolyLineReq;
typedef xLbxPolyPointReq xLbxPolySegmentReq;
typedef xLbxPolyPointReq xLbxPolyRectangleReq;
typedef xLbxPolyPointReq xLbxPolyArcReq;
typedef xLbxPolyPointReq xLbxPolyFillRectangleReq;
typedef xLbxPolyPointReq xLbxPolyFillArcReq;

#define sz_xLbxPolyLineReq		13
#define sz_xLbxPolySegmentReq		13
#define sz_xLbxPolyRectangleReq		13
#define sz_xLbxPolyArcReq		13
#define sz_xLbxPolyFillRectangleReq	13
#define sz_xLbxPolyFillArc		13

typedef struct _LbxFillPoly {
    CARD8	reqType;	/* always LbxReqCode */
    CARD8	lbxReqType;
    CARD16	length B16;
    Drawable	drawable B32;
    GContext	gc B32;
    BYTE	shape;
    CARD8	padBytes;
} xLbxFillPolyReq;
#define sz_xLbxFillPolyReq	14

typedef struct {
    CARD8       offset;
    CARD8       diff;
} xLbxDiffItem;
#define sz_xLbxDiffItem    2


typedef struct {
    BYTE	type;		/* X_Reply */
    CARD8	unused;
    CARD16	sequenceNumber B16;
    CARD32	length B32;	/* addition scheme-specific data follows */
    CARD16	deltaN B16;	/* delta cache size */
    CARD16	deltaMaxLen B16;/* max. length of messages in delta cache */
    CARD32	comptype B32;	/* LbxCompressNone etc */
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
} xLbxStartReply;
#define sz_xLbxStartReply	32

typedef struct _LbxEvent {
    BYTE	type;		/* always eventBase + LbxEvent */
    BYTE	lbxType;
    CARD16	sequenceNumber B16;
    CARD32	client B32;	/* client */
    CARD32	detail1 B32;
    CARD32	detail2 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xLbxEvent;
#define sz_xLbxEventEvent	32

#ifndef NDEBUG

#define DBG_SWITCH	0x00000001
#define DBG_CLOSE	0x00000002
#define DBG_IO		0x00000004
#define DBG_READ_REQ	0x00000008
#define DBG_LEN		0x00000010
#define DBG_BLOCK	0x00000020
#define DBG_CLIENT	0x00000040
#define DBG_DELTA	0x00000080

extern int lbxDebug;

#define DBG(n,m)    if (lbxDebug & (n)) { fprintf m; } else
#else
#define DBG(n,m)
#endif

#endif	/* _LBXSTR_H_ */
