/* $XConsortium: lbxgfx.c,v 1.3 94/03/27 13:15:58 dpw Exp $ */
/*
 * Copyright 1993 Network Computing Devices, Inc.
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
 * $NCDId: @(#)lbxgfx.c,v 1.4 1994/03/24 17:54:34 lemke Exp $
 *
 * Author:  Dave Lemke, Network Computing Devices
 */

/* various bits of DIX-level mangling */

#include <sys/types.h>
#include <stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "servermd.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "lbxtags.h"
#include "Xfuncproto.h"

extern int (*ProcVector[256])();

int
LbxDecodePoly(client, xreqtype, decode_rtn)
    register ClientPtr  client;
    CARD8		xreqtype;
    int			(*decode_rtn)();
{
    REQUEST(xLbxPolyPointReq);
    char		*in;
    xPolyPointReq	*xreq;
    int			len;
    int			retval;

    len = (stuff->length << 2) - sz_xLbxPolyPointReq;
    if ((xreq = (xPolyPointReq *) 
	    xalloc(sizeof(xPolyPointReq) + (len << 1))) == NULL)
	return BadAlloc;
    in = (char *)stuff + sz_xLbxPolyPointReq;
    len = (*decode_rtn)(in, in + len - stuff->padBytes, &xreq[1]);
    xreq->reqType = xreqtype;
    xreq->coordMode = 1;
    xreq->drawable = stuff->drawable;
    xreq->gc = stuff->gc;
    client->req_len = xreq->length = (sizeof(xPolyPointReq) + len) >> 2;
    client->requestBuffer = (pointer)xreq;

    retval = (*ProcVector[xreqtype])(client);
    xfree(xreq);
    return retval;
}

int
LbxDecodeFillPoly(client)
    register ClientPtr  client;
{
    REQUEST(xLbxFillPolyReq);
    char		*in;
    xFillPolyReq	*xreq;
    int			len;
    int			retval;

    len = (stuff->length << 2) - sz_xLbxFillPolyReq;
    if ((xreq = (xFillPolyReq *) 
	    xalloc(sizeof(xFillPolyReq) + (len << 1))) == NULL)
	return BadAlloc;
    in = (char *)stuff + sz_xLbxFillPolyReq;
    len = LbxDecodePoints(in, in + len - stuff->padBytes, (short *) &xreq[1]);
    xreq->reqType = X_FillPoly;
    xreq->drawable = stuff->drawable;
    xreq->gc = stuff->gc;
    xreq->shape = stuff->shape;
    xreq->coordMode = 1;
    client->req_len = xreq->length = (sizeof(xFillPolyReq) + len) >> 2;
    client->requestBuffer = (pointer)xreq;

    retval = (*ProcVector[X_FillPoly])(client);
    xfree(xreq);
    return retval;
}

/*
 * Routines for decoding line drawing requests
 */

#define DECODE_SHORT(in, val) \
    if ((*(in) & 0xf0) != 0x80) \
        (val) = (short)*(in)++; \
    else { \
	(val) = ((short)(*(in) & 0x0f) << 8) | (unsigned char)*((in) + 1); \
	if (*(in) & 0x08) (val) |= 0xf000; \
	(in) += 2; \
    }

#define DECODE_USHORT(in, val) \
    if ((*(in) & 0xf0) != 0xf0) \
	(val) = (unsigned short)*(in)++; \
    else { \
	(val) = ((short)(*(in) & 0x0f) << 8) | (unsigned char)*((in) + 1); \
	(in) += 2; \
    }

#define DECODE_ANGLE(in, val) \
    if ((*(in) & 0xf0) == 0x80) \
	(val) = (((short)(*(in)++ & 0x0f) - 4) * 45) << 7; \
    else { \
	(val) = ((short)*(in) << 8) | (unsigned char)*((in) + 1); \
	(in) += 2; \
    }

int
LbxDecodePoints(in, inend, out)
    register char  *in;
    char	   *inend;
    register short *out;
{
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, *out);
	out++;
	DECODE_SHORT(in, *out);
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeSegment(in, inend, out)
    register char  *in;
    char	   *inend;
    register short *out;
{
    register short diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeRectangle(in, inend, out)
    register char  *in;
    char	   *inend;
    register short *out;
{
    register short diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeArc(in, inend, out)
    register char  *in;
    char	   *inend;
    register short *out;
{
    register short diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
	DECODE_USHORT(in, *(unsigned short *)out);
	out++;

	DECODE_ANGLE(in, *out);
	out++;
	DECODE_ANGLE(in, *out);
	out++;
    }
    return ((char *)out - start_out);
}
