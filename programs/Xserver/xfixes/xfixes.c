/*
 * $XFree86: $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "xfixesint.h"

unsigned char	XFixesReqCode;
int		XFixesEventBase;

static int
ProcXFixesQueryVersion(ClientPtr client)
{
    xXFixesQueryVersionReply rep;
    register int n;
/*    REQUEST(xXFixesQueryVersionReq); */

    REQUEST_SIZE_MATCH(xXFixesQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XFIXES_MAJOR;
    rep.minorVersion = XFIXES_MINOR;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.majorVersion, n);
	swapl(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xXFixesQueryVersionReply), (char *)&rep);
    return(client->noClientException);
}

static int
ProcXFixesDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XFixesQueryVersion:
	return ProcXFixesQueryVersion(client);
    case X_XFixesChangeSaveSet:
	return ProcXFixesChangeSaveSet(client);
    case X_XFixesSelectSelectionInput:
	return ProcXFixesSelectSelectionInput(client);
    case X_XFixesSelectCursorInput:
	return ProcXFixesSelectCursorInput (client);
    case X_XFixesGetCursorImage:
	return ProcXFixesGetCursorImage (client);
    default:
	return BadRequest;
    }
}

static int
SProcXFixesQueryVersion(ClientPtr client)
{
    register int n;
    REQUEST(xXFixesQueryVersionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->majorVersion, n);
    swapl(&stuff->minorVersion, n);
    return ProcXFixesQueryVersion(client);
}

static int
SProcXFixesDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XFixesQueryVersion:
	return SProcXFixesQueryVersion(client);
    case X_XFixesChangeSaveSet:
	return SProcXFixesChangeSaveSet(client);
    case X_XFixesSelectSelectionInput:
	return SProcXFixesSelectSelectionInput(client);
    case X_XFixesSelectCursorInput:
	return SProcXFixesSelectCursorInput (client);
    case X_XFixesGetCursorImage:
	return SProcXFixesGetCursorImage (client);
    default:
	return BadRequest;
    }
}

/*ARGSUSED*/
static void
XFixesResetProc (ExtensionEntry *extEntry)
{
}

void
XFixesExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (XFixesSelectionInit() && XFixesCursorInit () &&
	(extEntry = AddExtension(XFIXES_NAME, XFixesNumberEvents, 0,
				 ProcXFixesDispatch, SProcXFixesDispatch,
				 XFixesResetProc, StandardMinorOpcode)) != 0)
    {
	XFixesReqCode = (unsigned char)extEntry->base;
	XFixesEventBase = extEntry->eventBase;
	EventSwapVector[XFixesEventBase + XFixesSelectionNotify] =
	    (EventSwapPtr) SXFixesSelectionNotifyEvent;
	EventSwapVector[XFixesEventBase + XFixesCursorNotify] =
	    (EventSwapPtr) SXFixesCursorNotifyEvent;
    }
}
