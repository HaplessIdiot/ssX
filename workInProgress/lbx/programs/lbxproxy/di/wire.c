/* $XConsortium: wire.c,v 1.7 94/03/27 14:01:53 dpw Exp $ */
/*
 * $NCDOr: wire.c,v 1.2 1993/11/19 21:29:10 keithp Exp keithp $
 * $NCDId: @(#)wire.c,v 1.36 1994/03/24 17:54:36 lemke Exp $
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

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "opaque.h"
#include "input.h"
#include <stdio.h>
#include "wire.h"
#include <errno.h>
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxdata.h"
#include "swap.h"

extern int  NewOutputPending;

#ifdef LBX_STATS
extern int  delta_out_total;
extern int  delta_out_attempts;
extern int  delta_out_hits;
extern int  delta_in_total;
extern int  delta_in_attempts;
extern int  delta_in_hits;
#endif

#ifndef NDEBUG
int         lbxDebug = 0;

#endif

int         lbxDeltaN = 16;
int         lbxDeltaMaxLen = 64;
int         lbxLZW = 14;

#define MAXBYTESDIFF		8

static unsigned char tempdeltabuf[256];

/*
 * Any request that could be delta compressed comes through here
 */
WriteReqToServer(client, len, buf)
    ClientPtr   client;
    int         len;
    char       *buf;
{
    XServerPtr  server = client->server;
    xLbxDeltaReq *p = (xLbxDeltaReq *) tempdeltabuf;
    int         diffs;
    int         cindex;
    int         newlen;
    Bool        written = FALSE;

#ifdef LBX_STATS
    delta_out_total++;
#endif
    if (DELTA_CACHEABLE(&server->outdeltas, len)) {
#ifdef LBX_STATS
	delta_out_attempts++;
#endif
	if ((diffs = LBXDeltaMinDiffs(&server->outdeltas, buf, len,
			     min(MAXBYTESDIFF, (len - sz_xLbxDeltaReq) >> 1),
				      &cindex)) >= 0) {
#ifdef LBX_STATS
	    delta_out_hits++;
#endif
	    LBXEncodeDelta(&server->outdeltas, buf, diffs, cindex,
			   &tempdeltabuf[sz_xLbxDeltaReq]);
	    p->reqType = server->lbxReq;
	    p->lbxReqType = X_LbxDelta;
	    p->diffs = diffs;
	    p->cindex = cindex;
	    newlen = sz_xLbxDeltaReq + sz_xLbxDiffItem * diffs;
	    p->length = (newlen + 3) >> 2;	/* BYTESWAP!! */
	    WriteToServer(client, newlen, (char *) p);
	    written = TRUE;
	}
	LBXAddDeltaOut(&server->outdeltas, buf, len);
    }
    if (!written)
	WriteToServer(client, len, buf);
}

WriteToServer(client, len, buf)
    ClientPtr   client;
    int         len;
    char       *buf;
{
    XServerPtr  server = client->server;

    if (server->serverClient->clientGone)
	return;
    if (server->send != client) {
	xLbxSwitchReq s;

	DBG(DBG_SWITCH, (stderr, "switch downstream to %d\n", client->index));
	s.reqType = server->lbxReq;
	s.lbxReqType = X_LbxSwitch;
	s.length = 2;
	s.client = client->index;
	WriteToClient(server->serverClient, sizeof(s), &s);
	server->send = client;
    }
    DBG(DBG_IO, (stderr, "downstream %d len %d\n", client->index, len));
    WriteToClient(server->serverClient, len, buf);
}

/* all these requests may need to be swapped back to the order of
 * ther client they're being executed for
 */

NewClient(client, setuplen)
    ClientPtr   client;
    int         setuplen;
{
    xLbxNewClientReq n;
    XServerPtr  server = client->server;

    DBG(DBG_CLIENT, (stderr, "new client %d\n", client->index));
    n.reqType = server->lbxReq;
    n.lbxReqType = X_LbxNewClient;
    n.length = 2 + (setuplen >> 2);
    n.client = client->index;
    if (clients[0]->swapped) {
    	SwapNewClient(&n);
    }
    WriteToServer(clients[0], sizeof(n), (char *) &n);
    ++server->serverClient->sequence;
}

CloseClient(client)
    ClientPtr   client;
{
    xLbxCloseClientReq n;
    XServerPtr  server = client->server;

    if (!client->server)
	return;

    if (client->server->serverClient == client)
	return;
    if (client->server->serverClient->clientGone)
	return;
    DBG(DBG_CLIENT, (stderr, "closing down client %d\n", client->index));
    if (client->closeDownMode != DestroyAll) {
	n.reqType = server->lbxReq;
	n.lbxReqType = X_LbxCloseClient;
	n.length = 2;
	n.client = client->index;
	if (client->swapped) {
	    SwapCloseClient(&n);
	}
	WriteToServer(client, sizeof(n), (char *) &n);
    }
}

ModifySequence(client, num)
    ClientPtr   client;
    int         num;
{
    xLbxModifySequenceReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;
    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxModifySequence;
    req.length = 2;
    req.adjust = num;
    if (client->swapped) {
    	SwapModifySequence(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

AllowMotion(client, num)
    ClientPtr   client;
    int         num;
{
    client->server->motion_allowed += num;
}

SendIncrementPixel(client, cmap, pixel)
    ClientPtr   client;
    XID         cmap;
    unsigned long pixel;
{
    xLbxIncrementPixelReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;
    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxIncrementPixel;
    req.length = 4;
    req.cmap = cmap;
    req.pixel = pixel;
    req.amount = 1;
    if (client->swapped) {
    	SwapIncrementPixel(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendGetModifierMapping(client)
    ClientPtr   client;
{
    xLbxGetModifierMappingReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxGetModifierMapping;
    req.length = 1;
    if (client->swapped) {
    	SwapGetModifierMapping(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendGetKeyboardMapping(client)
    ClientPtr   client;
{
    xLbxGetKeyboardMappingReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;

    /*
     * always ask for entire thing so tag always works, and pass on requested
     * subset
     */
    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxGetKeyboardMapping;
    req.length = 2;
    req.firstKeyCode = LBXMinKeyCode(client);
    req.count = LBXMaxKeyCode(client) - LBXMinKeyCode(client) + 1;
    if (client->swapped) {
    	SwapGetKeyboardMapping(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendQueryFont(client, fid)
    ClientPtr   client;
    XID         fid;
{
    xLbxQueryFontReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxQueryFont;
    req.length = 2;
    req.fid = fid;
    if (client->swapped) {
    	SwapQueryFont(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendChangeProperty(client, win, prop, type, format, mode, num)
    ClientPtr   client;
    Window      win;
    Atom        prop,
                type;
    int         format,
                mode;
    unsigned long num;
{
    xLbxChangePropertyReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxChangeProperty;
    req.length = 6;
    req.window = win;
    req.property = prop;
    req.type = type;
    req.format = format;
    req.mode = mode;
    req.nUnits = num;
    if (client->swapped) {
    	SwapChangeProperty(&req);
    }

    WriteToServer(client, sizeof(req), (char *) &req);
}

SendGetProperty(client, win, prop, type, delete, off, len)
    ClientPtr   client;
    Window      win;
    Atom        prop,
                type;
    Bool        delete;
    unsigned long off,
                len;
{
    xLbxGetPropertyReq req;
    XServerPtr  server = client->server;

    if (client->server->serverClient == client)
	return;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxGetProperty;
    req.length = 7;
    req.window = win;
    req.property = prop;
    req.type = type;
    req.delete = delete;
    req.longOffset = off;
    req.longLength = len;
    if (client->swapped) {
    	SwapGetProperty(&req);
    }

    WriteToServer(client, sizeof(req), (char *) &req);
}

SendQueryTag(client, tag)
    ClientPtr   client;
    XID         tag;
{
    xLbxQueryTagReq req;
    XServerPtr  server = client->server;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxQueryTag;
    req.length = 2;
    req.tag = tag;
    if (client->swapped) {
        SwapQueryTag(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendInvalidateTag(client, tag)
    ClientPtr   client;
    XID         tag;
{
    xLbxInvalidateTagReq req;
    XServerPtr  server;

    if (!servers[0])		/* proxy resetting */
	return;
    if (!client)
	client = clients[0];		/* XXX watch multi-proxy */
    server = client->server;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxInvalidateTag;
    req.length = 2;
    req.tag = tag;
    /* need tag type ? */
    if (client->swapped) {
        SwapInvalidateTag(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
}

SendTagData(client, tag, len, data)
    ClientPtr   client;
    XID         tag;
    unsigned long len;
    pointer     data;
{
    xLbxTagDataReq req;
    XServerPtr  server;

    client = clients[0];	    /* XXX watch multi proxy */
    server = client->server;

    req.reqType = server->lbxReq;
    req.lbxReqType = X_LbxTagData;
    req.length = 3 + ((len + 3) >> 2);
    req.real_length = len;
    req.tag = tag;
    /* need tag type ? */
    if (client->swapped) {
    	SwapTagData(&req);
    }
    WriteToServer(client, sizeof(req), (char *) &req);
    if (len)
	WriteToServer(client, len, (char *) data);
}

static unsigned long pendingServerReplySequence;
static void (*serverReplyFunc) ();

ServerReply(server, rep)
    XServerPtr  server;
    xReply     *rep;
{
    if (serverReplyFunc &&
	    rep->generic.sequenceNumber == pendingServerReplySequence) {
	(*serverReplyFunc) (server, rep);
	serverReplyFunc = 0;
    }
}

ExpectServerReply(server, func)
    XServerPtr  server;
    void        (*func) ();
{
    pendingServerReplySequence = server->serverClient->sequence;
    serverReplyFunc = func;
}

extern int  (*ServerVector[]) ();

unsigned long
ServerRequestLength(req, sc, gotnow, partp)
    xReq       *req;
    ClientPtr   sc;
    int         gotnow;
    Bool       *partp;
{
    XServerPtr  server = servers[sc->lbxIndex];
    ClientPtr   client = server->recv;
    xReply     *rep;
    xConnSetupPrefix *pre;

    if (!req)
	req = (xReq *) sc->requestBuffer;
    if (gotnow < sizeof(xReq)) {
	*partp = TRUE;
	return sizeof(xReq);
    }
    if (req->reqType == server->lbxEvent &&
	    req->data == LbxDeltaEvent) {
	*partp = FALSE;
	return req->length << 2;
    }
    if (client->awaitingSetup) {
	if (gotnow < 8) {
	    *partp = TRUE;
	    return 8;
	}
	pre = (xConnSetupPrefix *) req;
	*partp = FALSE;
	return (2 + pre->length) << 2;
    }
    if (gotnow < 8) {
	*partp = TRUE;
	return 8;
    }
    *partp = FALSE;
    rep = (xReply *) req;
    if (rep->generic.type != X_Reply)
	return 32;
    return (8 + rep->generic.length) << 2;
}

ServerProcStandardEvent(sc)
    ClientPtr   sc;
{
    xReply     *rep;
    xLbxEvent  *lbx;
    XServerPtr  server = servers[sc->lbxIndex];
    ClientPtr   client = server->recv;
    int         len;
    Bool        part;
    Bool        cacheable = (server->initialized) ? TRUE : FALSE;

    rep = (xReply *) sc->requestBuffer;

    /* need to calculate length up from for Delta cache */
    if (rep->generic.type != server->lbxEvent) {
	len = RequestLength(rep, sc, 8, &part);
    } else {
	len = sizeof(xLbxEvent);
    }

#ifdef LBX_STATS
    delta_in_total++;
#endif
    if (rep->generic.type == server->lbxEvent &&
	    rep->generic.data1 == LbxDeltaEvent) {
	xLbxDeltaReq *delta = (xLbxDeltaReq *) rep;

#ifdef LBX_STATS
	delta_in_attempts++;
	delta_in_hits++;
#endif
	/* Note that LBXDecodeDelta decodes and adds current msg to the cache */
	len = LBXDecodeDelta(&server->indeltas, ((char *) rep) + sz_xLbxDeltaReq,
			     delta->diffs, delta->cindex, &rep);

	/* Make local copy in case someone writes to the request buffer */
	bcopy((char *) rep, tempdeltabuf, len);
	rep = (xReply *) tempdeltabuf;

	cacheable = FALSE;
    } else if (rep->generic.type == server->lbxEvent &&
	       rep->generic.data1 == LbxSwitchEvent) {
	cacheable = FALSE;
    }

    /* stick in delta buffer before LBX code modified things */
    if (cacheable && DELTA_CACHEABLE(&server->indeltas, len)) {
#ifdef LBX_STATS
	delta_in_attempts++;
#endif
	LBXAddDeltaIn(&server->indeltas, (char *) rep, len);
    }

    if (rep->generic.type != server->lbxEvent) {
	if (!client->awaitingSetup && rep->generic.type == X_Reply) {
	    /*
	     * Xlib never sends more than one request after a synchronous one
	     * (XGetWindowAttributes)
	     */
	    if (rep->generic.sequenceNumber != client->sequence &&
		    rep->generic.sequenceNumber != client->sequence - 1) {
		DBG(DBG_CLIENT, (stderr, "sequence number mismatch %d != %d\n",
				 rep->generic.sequenceNumber,
				 client->sequence));
	    }
	}
	len = RequestLength(rep, sc, 8, &part);
	DBG(DBG_IO, (stderr, "upstream %d len %d\n", client->index, len));
	if (client->index == 0) {
	    ServerReply(server, rep);
	} else {
	    if (!client->clientGone) {
		if (DoLBXReply(client, (char *) rep, len))
		    WriteToClient(client, len, rep);
	    }
	    client->awaitingSetup = FALSE;
	}
    } else {
	len = sizeof(xLbxEvent);
	lbx = (xLbxEvent *) rep;
	switch (lbx->lbxType) {

	case LbxSwitchEvent:
	    DBG(DBG_SWITCH, (stderr, "switch upstream to %d\n", lbx->client));
	    client = clients[lbx->client];
	    server->recv = client;
	    (void) CheckPendingClientInput(sc);
	    break;
	case LbxCloseEvent:
	    DBG(DBG_CLIENT, (stderr, "close client %d\n", lbx->client));
	    client = clients[lbx->client];
	    client->closeDownMode = DestroyAll;
	    CloseDownClient(client);
	    break;
	case LbxInvalidateTagEvent:
	    DBG(DBG_CLIENT, (stderr, "invalidate tag %d\n", lbx->detail1));
	    LbxFreeTag((XID) lbx->detail1, lbx->detail2);
	    break;
	case LbxSendTagDataEvent:
	    DBG(DBG_CLIENT, (stderr, "send tag data %d\n", lbx->detail1));
	    LbxSendTagData((XID) lbx->detail1, lbx->detail2);
	    break;
	case LbxListenToOne:
	    DBG(DBG_CLIENT, (stderr, "listen to one client %d\n", lbx->client));
	    if (lbx->client == 0xffffffff)	/* ignore all clients */
		LbxOnlyListenToOneClient(server->serverClient);
	    else
		LbxOnlyListenToOneClient(clients[lbx->client]);
	    break;
	case LbxListenToAll:
	    DBG(DBG_CLIENT, (stderr, "listen to all clients\n"));
	    LbxListenToAllClients();
	    break;
	}
    }

    return Success;
}

extern int GrabInProgress;
static int lbxIgnoringAll;
static int lbxGrabInProgress;

int
LbxIgnoreAllClients(server)
    XServerPtr  server;
{
    if (!lbxIgnoringAll) {
	if (GrabInProgress) {
	    lbxGrabInProgress = GrabInProgress;
	    ListenToAllClients();
	}
	OnlyListenToOneClient(server->serverClient);
	lbxIgnoringAll = TRUE;
    }
}

/* ARGSUSED */
int
LbxAttendAllClients(server)
    XServerPtr  server;
{
    if (lbxIgnoringAll) {
	ListenToAllClients();
	lbxIgnoringAll = FALSE;
	if (lbxGrabInProgress) {
	    OnlyListenToOneClient(clients[lbxGrabInProgress]);
	    lbxGrabInProgress = 0;
	}
    }
}

int
LbxOnlyListenToOneClient(client)
    ClientPtr   client;
{
    if (lbxIgnoringAll)
	lbxGrabInProgress = client->index;
    else {
	if (GrabInProgress)
	    ListenToAllClients();
	OnlyListenToOneClient (client);
    }
}

int
LbxListenToAllClients()
{
    if (lbxGrabInProgress)
	lbxGrabInProgress = 0;
    else if (!lbxIgnoringAll)
	ListenToAllClients();
}

/* ARGSUSED */
Bool
ProxyWorkProc(dummy, index)
    pointer     dummy;
    int         index;
{
    XServerPtr  server;
    xLbxAllowMotionReq req;

    if ((server = servers[index]) == 0)
	return TRUE;
    if (!server->initialized)
	return TRUE;

    if (server->motion_allowed) {
	DBG(DBG_CLIENT, (stderr, "allow %d motion events\n",
			 server->motion_allowed));
	req.reqType = server->lbxReq;
	req.lbxReqType = X_LbxAllowMotion;
	req.length = 2;
	req.num = server->motion_allowed;
	server->motion_allowed = 0;
	WriteToClient(server->serverClient, sizeof(req), &req);
    }
    /* Need to flush the output buffers before we flush compression buffer */
    if (NewOutputPending)
	FlushAllOutput();

    if (server->lzwHandle) {
	if (LzwInputAvail(server->fd))
	    AvailableClientInput(server->serverClient);
	if (LzwFlush(server->fd) != 0)
	    MarkConnectionWriteBlocked(server->serverClient);
    }
    /*
     * If we've got stuff remaining in the output buffers to the server, then
     * don't allow reads from any other clients, otherwise we could overflow.
     */
    if (PendingClientOutput(server->serverClient))
	LbxIgnoreAllClients(server);
    else
	LbxAttendAllClients(server);

    return FALSE;
}

void
CloseServer(client)
    ClientPtr   client;
{
    XServerPtr  server;
    int         i;

    server = client->server;
    servers[server->index] = 0;
    LBXFreeDeltaCache(&server->indeltas);
    LBXFreeDeltaCache(&server->outdeltas);
    if (server->lzwHandle) {
	LzwFree(server->lzwHandle);
    }
    /* remove all back pointers */
    for (i = 1; i < MAXCLIENTS; i++) {
	if (clients[i] && clients[i]->server == server)
	    clients[i]->server = 0;
    }
    xfree(server);
    CloseDownFileDescriptor(client);
    DBG(DBG_CLOSE, (stderr, "closing down server\n"));
    dispatchException |= DE_RESET;
    isItTimeToYield = 1;
}

extern int  LzwWriteV();
extern int  LzwRead();
extern void LzwCompressOn(), LzwCompressOff();

void
StartProxyReply(server, rep)
    XServerPtr  server;
    xLbxStartReply *rep;
{
    LBXInitDeltaCache(&server->indeltas, rep->deltaN, rep->deltaMaxLen);
    LBXInitDeltaCache(&server->outdeltas, rep->deltaN, rep->deltaMaxLen);
    QueueWorkProc(ProxyWorkProc, NULL, (pointer) server->index);
    if (rep->comptype == LbxCompressLZW) {
	char       *extra = (char *) rep;
	int         maxbits = *(CARD32 *) (extra + sizeof(xReply));
	int         len = sizeof(xReply) + (rep->length << 2);
	int         left = BytesInClientBuffer(server->serverClient);

	server->lzwHandle = (void *) LzwInit(server->fd, maxbits);
	SwitchConnectionFuncs(server->serverClient,
			      LzwRead, LzwWriteV, CloseServer);
	extra += len;
	LzwStuffInput(server->fd, extra, left);
	SkipInClientBuffer(server->serverClient, left + len, 0);
	StartOutputCompression(server->serverClient,
			       LzwCompressOn, LzwCompressOff);
    }
    server->initialized = TRUE;
    MakeClientGrabImpervious (server->serverClient);
}

StartProxy(server)
    XServerPtr  server;
{
    char        buf[sz_xLbxStartProxyReq + 4];
    xLbxStartProxyReq *n = (xLbxStartProxyReq *) buf;
    extern Bool NewOutputPending;

    n->reqType = server->lbxReq;
    n->lbxReqType = X_LbxStartProxy;
    n->deltaN = lbxDeltaN;
    n->deltaMaxLen = lbxDeltaMaxLen;
    if (lbxLZW) {
	n->comptype = LbxCompressLZW;
	*(CARD32 *) (n + 1) = lbxLZW;
	n->length = (sz_xLbxStartProxyReq + 4) >> 2;
    } else {
	n->comptype = LbxCompressNone;
	n->length = sz_xLbxStartProxyReq >> 2;
    }
    WriteToServer(clients[0], n->length << 2, (char *) n);
    server->serverClient->sequence++;
    ExpectServerReply(server, StartProxyReply);
    while (NewOutputPending)
	FlushAllOutput();
}

extern int  read(), writev();

Bool
ConnectToServer(dpy_name)
    char       *dpy_name;
{
    int         i;
    XServerPtr  server;
    int         request,
                error,
                event;
    int         sequence;
    ClientPtr   sc,
                AllocNewConnection();

    for (i = 0; i < MAX_SERVERS; i++)
	if (!servers[i])
	    break;
    if (i == MAX_SERVERS)
	return FALSE;
    server = (XServerPtr) xalloc(sizeof(XServerRec));
    if (!server)
	return FALSE;
    bzero(server, sizeof(XServerRec));
    server->index = i;
    server->fd = InitMux(dpy_name, &request, &event, &error, &sequence);
    if (server->fd < 0) {
	xfree(server);
	return FALSE;
    }
    DBG(DBG_IO, (stderr, "making server connection\n"));
    server->lbxReq = request;
    server->lbxEvent = event;
    server->lbxError = error;
    server->lzwHandle = NULL;
    server->initialized = FALSE;
    server->send = clients[0];
    server->recv = clients[0];
    clients[0]->server = server;
    server->motion_allowed = 0;

    sc = AllocNewConnection(server->fd, read, writev, NULL);
    SwitchConnectionFuncs(sc, read, writev, CloseServer);
    sc->server = server;
    sc->public.requestLength = ServerRequestLength;
    sc->lbxIndex = i;
    sc->requestVector = ServerVector;
    sc->awaitingSetup = FALSE;
    sc->sequence = sequence;

    server->serverClient = sc;
    servers[i] = server;
    StartProxy(server);

    /* Really should wait for reply to StartProxy request here!! */

    return TRUE;
}
