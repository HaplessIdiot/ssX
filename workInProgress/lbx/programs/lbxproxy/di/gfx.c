/* $XConsortium: gfx.c,v 1.3 94/03/27 14:06:51 dpw Exp $ */
/*
 * Copyright 1994 Network Computing Devices, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name Network Computing Devices, Inc. not be
 * used in advertising or publicity pertaining to distribution of this 
 * software without specific, written prior permission.
 * 
 * THIS SOFTWARE IS PROVIDED `AS-IS'.  NETWORK COMPUTING DEVICES, INC.,
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT
 * LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NONINFRINGEMENT.  IN NO EVENT SHALL NETWORK
 * COMPUTING DEVICES, INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA,
 * OR PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS OF
 * WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * $NCDId: @(#)gfx.c,v 1.3 1994/03/24 17:54:44 lemke Exp $
 */
/*
 * graphics acceleration
 */

#include	<stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include	<X11/X.h>	/* for KeymapNotify */
#include	<X11/Xproto.h>
#include	"assert.h"
#include	"lbxdata.h"
#include	"util.h"
#include	"lbx.h"		/* gets dixstruct.h */
#include	"resource.h"
#include	"wire.h"
#define _XLBX_SERVER_
#include	"lbxstr.h"	/* gets dixstruct.h */

static int  pad[4] = {0, 3, 2, 1};

/*
 * Routines for re-encoding line, rectangle and arc requests
 */

#define REENCODE_SHORT(val, out) \
    if ((val) >= (short)0x0800 || (val) < (short)0xf800) \
	return 0; \
    if ((val) <= -113 || (val) >= 128) \
	*(out)++ = 0x80 | (((val) >> 8) & 0x0f); \
    *(out)++ = (val) & 0xff;

#define REENCODE_USHORT(val, out) \
    if ((unsigned short)(val) >= 0x1000) \
	return 0; \
    if ((val) >= 240) \
	*(out)++ = 0xf0 | ((val) >> 8); \
    *(out)++ = (val) & 0xff;

#define REENCODE_ANGLE(val, out) \
    if ((val) < ((-360) << 6) || (val) > (360 << 6)) \
	return 0; \
    if (((val) & 0x007f) == 0) { \
	short angle = (val) >> 7; \
	if (angle % 45) { \
	    *(out)++ = (val) >> 8; \
	    *(out)++ = (val) & 0xff; \
	} \
	else \
	    *(out)++ = 0x80 | ((angle / 45) + 4); \
    }

static int
reencode_poly(client, lbxreq, reencode_rtn)
    ClientPtr   client;
    CARD8       lbxreq;
    int         (*reencode_rtn) ();
{
    REQUEST(xPolyPointReq);
    XServerPtr  server = client->server;
    int         len = stuff->length << 2;
    xLbxPolyPointReq *newreq = NULL;
    int         bytes;

    /* If it's delta compressible, don't try to reencode */
    if (DELTA_CACHEABLE(&server->outdeltas, len) ||
	    (newreq = (xLbxPolyPointReq *) xalloc(len)) == NULL ||
	    (bytes = (*reencode_rtn) (client, (short *) &stuff[1],
				      (char *) newreq + sz_xLbxPolyPointReq,
				      (len - sz_xPolyPointReq) >> 2)) == 0) {
	if (newreq)
	    xfree(newreq);
	return ProcStandardRequest(client);
    }
    FinishLBXRequest(client, REQ_PASSTHROUGH);

    newreq->reqType = server->lbxReq;
    newreq->lbxReqType = lbxreq;
    newreq->drawable = stuff->drawable;
    newreq->gc = stuff->gc;
    len = sz_xLbxPolyPointReq + bytes;
    newreq->padBytes = pad[len % 4];
    len += newreq->padBytes;
    newreq->length = len >> 2;
    WriteToServer(client, len, (char *) newreq);
    xfree(newreq);
    return Success;
}

/* ARGSUSED */
static int
reencode_points_origin(client, in, out, count)
    ClientPtr   client;
    register short *in;
    register char *out;
    int         count;
{
    register short diff;
    int         i;
    short       last_x = 0;
    short       last_y = 0;
    char       *start_out = out;

    for (i = 0; i < count; i++, in += 2) {
	diff = in[0] - last_x;
	last_x = in[0];
	REENCODE_SHORT(diff, out);
	diff = in[1] - last_y;
	last_y = in[1];
	REENCODE_SHORT(diff, out);
    }
    return out - start_out;
}

/* ARGSUSED */
static int
reencode_points_previous(client, in, out, count)
    ClientPtr   client;
    register short *in;
    register char *out;
    int         count;
{
    int         i;
    char       *start_out = out;

    for (i = 0; i < count; i++, in += 2) {
	REENCODE_SHORT(in[0], out);
	REENCODE_SHORT(in[1], out);
    }
    return out - start_out;
}

/* ARGSUSED */
static int
reencode_segment(client, in, out, count)
    ClientPtr   client;
    register short *in;
    register char *out;
    int         count;
{
    register short diff;
    int         i;
    short       last_x = 0;
    short       last_y = 0;
    char       *start_out = out;

    count >>= 1;
    for (i = 0; i < count; i++, in += 4) {
	/* reencode (x1, y1) */
	diff = in[0] - last_x;
	last_x = in[0];
	REENCODE_SHORT(diff, out);
	diff = in[1] - last_y;
	last_y = in[1];
	REENCODE_SHORT(diff, out);

	/* reencode (x2, y2) */
	diff = in[2] - last_x;
	REENCODE_SHORT(diff, out);
	diff = in[3] - last_y;
	REENCODE_SHORT(diff, out);
    }
    return out - start_out;
}

/* ARGSUSED */
static int
reencode_rectangle(client, in, out, count)
    ClientPtr   client;
    register short *in;
    register char *out;
    int         count;
{
    register short diff;
    int         i;
    short       last_x = 0;
    short       last_y = 0;
    char       *start_out = out;

    count >>= 1;
    for (i = 0; i < count; i++, in += 4) {
	/* reencode (x, y) origin */
	diff = in[0] - last_x;
	last_x = in[0];
	REENCODE_SHORT(diff, out);
	diff = in[1] - last_y;
	last_y = in[1];
	REENCODE_SHORT(diff, out);

	/* reencode (width, height) */
	REENCODE_USHORT(in[2], out);
	REENCODE_USHORT(in[3], out);
    }
    return out - start_out;
}

/* ARGSUSED */
static int
reencode_arc(client, in, out, count)
    ClientPtr   client;
    register short *in;
    register char *out;
    int         count;
{
    register short diff;
    int         i;
    short       last_x = 0;
    short       last_y = 0;
    char       *start_out = out;

    count = count / 3;
    for (i = 0; i < count; i++, in += 6) {
	/* reencode (x, y) origin */
	diff = in[0] - last_x;
	last_x = in[0];
	REENCODE_SHORT(diff, out);
	diff = in[1] - last_y;
	last_y = in[1];
	REENCODE_SHORT(diff, out);

	/* reencode (width, height) */
	REENCODE_USHORT(in[2], out);
	REENCODE_USHORT(in[3], out);

	/* reencode (angle1, angle2) */
	REENCODE_ANGLE(in[4], out);
	REENCODE_ANGLE(in[5], out);
    }
    return out - start_out;
}

int
ProcLBXPolyPoint(client)
    ClientPtr   client;
{
    REQUEST(xPolyPointReq);
    int         (*reencode_rtn) ();

    reencode_rtn = (stuff->coordMode) ? reencode_points_previous :
	reencode_points_origin;
    return reencode_poly(client, X_LbxPolyPoint, reencode_rtn);
}

int
ProcLBXPolyLine(client)
    ClientPtr   client;
{
    REQUEST(xPolyLineReq);
    int         (*reencode_rtn) ();

    reencode_rtn = (stuff->coordMode) ? reencode_points_previous :
	reencode_points_origin;
    return reencode_poly(client, X_LbxPolyLine, reencode_rtn);
}

int
ProcLBXPolySegment(client)
    ClientPtr   client;
{
    return reencode_poly(client, X_LbxPolySegment, reencode_segment);
}

int
ProcLBXPolyRectangle(client)
    ClientPtr   client;
{
    return reencode_poly(client, X_LbxPolyRectangle, reencode_rectangle);
}

int
ProcLBXPolyArc(client)
    ClientPtr   client;
{
    return reencode_poly(client, X_LbxPolyArc, reencode_arc);
}

int
ProcLBXFillPoly(client)
    ClientPtr   client;
{
    REQUEST(xFillPolyReq);
    XServerPtr  server = client->server;
    int         len = stuff->length << 2;
    xLbxFillPolyReq *newreq = NULL;
    int         bytes;
    int         (*reencode_rtn) ();

    reencode_rtn = (stuff->coordMode) ? reencode_points_previous :
	reencode_points_origin;

    /* If it's delta compressible, don't try to reencode */
    if (DELTA_CACHEABLE(&server->outdeltas, len) ||
	    (newreq = (xLbxFillPolyReq *) xalloc(len)) == NULL ||
	    (bytes = (*reencode_rtn) (client, (short *) &stuff[1],
				      (char *) newreq + sz_xLbxFillPolyReq,
				      (len - sz_xFillPolyReq) >> 2)) == 0) {
	if (newreq)
	    xfree(newreq);
	return ProcStandardRequest(client);
    }
    FinishLBXRequest(client, REQ_PASSTHROUGH);

    newreq->reqType = server->lbxReq;
    newreq->lbxReqType = X_LbxFillPoly;
    newreq->drawable = stuff->drawable;
    newreq->gc = stuff->gc;
    newreq->shape = stuff->shape;
    len = sz_xLbxPolyPointReq + bytes;
    newreq->padBytes = pad[len % 4];
    len += newreq->padBytes;
    newreq->length = len >> 2;
    WriteToServer(client, len, (char *) newreq);
    xfree(newreq);
    return Success;
}

int
ProcLBXPolyFillRectangle(client)
    ClientPtr   client;
{
    return reencode_poly(client, X_LbxPolyFillRectangle, reencode_rectangle);
}

int
ProcLBXPolyFillArc(client)
    ClientPtr   client;
{
    return reencode_poly(client, X_LbxPolyFillArc, reencode_arc);
}
