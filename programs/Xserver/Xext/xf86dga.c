/* $XFree86: xc/programs/Xserver/Xext/xf86dga.c,v 3.9.2.4 1998/06/14 15:40:46 dawes Exp $ */

/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995, 1996  XFree86 Inc

*/

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "xf86dgastr.h"
#include "swaprep.h"
#include "dgaproc.h"

static int DGAErrorBase;

static DISPATCH_PROC(ProcXF86DGAQueryVersion);
static DISPATCH_PROC(ProcXF86DGADirectVideo);
static DISPATCH_PROC(ProcXF86DGADispatch);
static DISPATCH_PROC(ProcXF86DGAGetVidPage);
static DISPATCH_PROC(ProcXF86DGAGetVideoLL);
static DISPATCH_PROC(ProcXF86DGAGetViewPortSize);
static DISPATCH_PROC(ProcXF86DGASetVidPage);
static DISPATCH_PROC(ProcXF86DGASetViewPort);
static DISPATCH_PROC(ProcXF86DGAInstallColormap);
static DISPATCH_PROC(ProcXF86DGAQueryDirectVideo);
static DISPATCH_PROC(ProcXF86DGAViewPortChanged);

/*
 * SProcs should probably be deleted, a local connection can never
 * be byte flipped!? - Jon.
 */
static DISPATCH_PROC(SProcXF86DGADirectVideo);
static DISPATCH_PROC(SProcXF86DGADispatch);
static DISPATCH_PROC(SProcXF86DGAQueryVersion);

static void XF86DGAResetProc(ExtensionEntry* extEntry);

static unsigned char DGAReqCode = 0;

void
XFree86DGAExtensionInit()
{
    ExtensionEntry* extEntry;
#ifdef XF86DGA_EVENTS
    int		    i;
    ScreenPtr	    pScreen;

    EventType = CreateNewResourceType(XF86DGAFreeEvents);
    ScreenPrivateIndex = AllocateScreenPrivateIndex ();
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	SetScreenPrivate (pScreen, NULL);
    }
#endif

    if (
#ifdef XF86DGA_EVENTS
        EventType && ScreenPrivateIndex != -1 &&
#endif
	(extEntry = AddExtension(XF86DGANAME,
				XF86DGANumberEvents,
				XF86DGANumberErrors,
				ProcXF86DGADispatch,
				SProcXF86DGADispatch,
				XF86DGAResetProc,
				StandardMinorOpcode))) {
	DGAReqCode = (unsigned char)extEntry->base;
	DGAErrorBase = extEntry->errorBase;
    }
}

/*ARGSUSED*/
static void
XF86DGAResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

static int
ProcXF86DGAQueryVersion(client)
    register ClientPtr client;
{
    xXF86DGAQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86DGAQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86DGA_MAJOR_VERSION;
    rep.minorVersion = XF86DGA_MINOR_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xXF86DGAQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGAGetVideoLL(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGAGetVideoLLReq);
    xXF86DGAGetVideoLLReply rep;
    register int n;
    Bool supported;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetVideoLLReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    supported = DGAAvailable(stuff->screen) &&
		DGAGetParameters(stuff->screen, &rep.offset, &rep.bank_size,
				 &rep.ram_size, &rep.width);
    if (!supported)
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.offset, n);
    	swapl(&rep.width, n);
    	swapl(&rep.bank_size, n);
    	swapl(&rep.ram_size, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAGetVideoLLReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGADirectVideo(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGADirectVideoReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGADirectVideoReq);

    if (!DGAAvailable(stuff->screen)) {
       /* chipset doesn't know about directVideoMode */
	return DGAErrorBase + XF86DGANoDirectVideoMode;
    }

    /* Check that the current screen is active. */
    if (!DGAScreenActive(stuff->screen)) {
	return DGAErrorBase + XF86DGAScreenNotActive;
    }

    DGASetFlags(stuff->screen, stuff->enable);

    if (stuff->enable & XF86DGADirectGraphics) {
	DGAEnableDirectMode(stuff->screen);
    } else {
	DGADisableDirectMode(stuff->screen);
    }

    return (client->noClientException);
}

static int
ProcXF86DGAGetViewPortSize(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGAGetViewPortSizeReq);
    xXF86DGAGetViewPortSizeReply rep;
    register int n;
    int width, height;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetViewPortSizeReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    DGAGetViewPortSize(stuff->screen, &width, &height);
    rep.width = width;
    rep.height = height;

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.width, n);
    	swapl(&rep.height, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAGetViewPortSizeReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGASetViewPort(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGASetViewPortReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGASetViewPortReq);

    if (!DGAAvailable(stuff->screen))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if (!DGAGetDirectMode(stuff->screen))
	return DGAErrorBase + XF86DGADirectNotActivated;

    if (!DGASetViewPort(stuff->screen, stuff->x, stuff->y))
	return DGAErrorBase + XF86DGAOperationNotSupported;

    return (client->noClientException);
}

static int
ProcXF86DGAGetVidPage(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGAGetVidPageReq);
    xXF86DGAGetVidPageReply rep;
    register int n;
    int page;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetVidPageReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    page = DGAGetVidPage(stuff->screen);

    if (page < 0)
	return DGAErrorBase + XF86DGAOperationNotSupported;

    rep.vpage = page;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.vpage, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAGetVidPageReply), (char *)&rep);
    return (client->noClientException);
}


static int
ProcXF86DGASetVidPage(client)
    register ClientPtr client;
{
    REQUEST(xXF86DGASetVidPageReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGASetVidPageReq);

    if (!DGAAvailable(stuff->screen))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if (!DGAGetDirectMode(stuff->screen))
	return DGAErrorBase + XF86DGADirectNotActivated;

    if (!DGASetVidPage(stuff->screen, stuff->vpage))
	return DGAErrorBase + XF86DGAOperationNotSupported;

    return (client->noClientException);
}


static int
ProcXF86DGAInstallColormap(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xXF86DGAInstallColormapReq);

    REQUEST_SIZE_MATCH(xXF86DGAInstallColormapReq);

    if (!DGAAvailable(stuff->screen))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if (!DGAGetDirectMode(stuff->screen))
	return DGAErrorBase + XF86DGADirectNotActivated;

    pcmp = (ColormapPtr  )LookupIDByType(stuff->id, RT_COLORMAP);
    if (pcmp)
    {
	DGASetFlags(stuff->screen,
		    DGAGetFlags(stuff->screen) | XF86DGADirectColormap
					       | XF86DGAHasColormap);
        (*(pcmp->pScreen->InstallColormap)) (pcmp);
	DGASetFlags(stuff->screen,
		    DGAGetFlags(stuff->screen) & ~XF86DGAHasColormap);
        return (client->noClientException);
    }
    else
    {
        client->errorValue = stuff->id;
        return (BadColor);
    }
}

static int
ProcXF86DGAQueryDirectVideo(register ClientPtr client)
{
    REQUEST(xXF86DGAQueryDirectVideoReq);
    xXF86DGAQueryDirectVideoReply rep;
    register int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAQueryDirectVideoReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (DGAAvailable(stuff->screen)) {
	rep.flags = DGAGetFlags(stuff->screen) | XF86DGADirectPresent;
    } else {
	rep.flags = DGAGetFlags(stuff->screen) & ~XF86DGADirectPresent;
    }

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.flags, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAQueryDirectVideoReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGAViewPortChanged(register ClientPtr client)
{
    REQUEST(xXF86DGAViewPortChangedReq);
    xXF86DGAViewPortChangedReply rep;
    register int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAViewPortChangedReq);

    if (!DGAAvailable(stuff->screen))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if (!DGAGetDirectMode(stuff->screen))
	return DGAErrorBase + XF86DGADirectNotActivated;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.result = DGAViewPortChanged(stuff->screen, 0);

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.result, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAViewPortChangedReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGADispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    if (!LocalClient(client))
	return DGAErrorBase + XF86DGAClientNotLocal;

    switch (stuff->data)
    {
    case X_XF86DGAQueryVersion:
	return ProcXF86DGAQueryVersion(client);
    case X_XF86DGAGetVideoLL:
	return ProcXF86DGAGetVideoLL(client);
    case X_XF86DGADirectVideo:
	return ProcXF86DGADirectVideo(client);
    case X_XF86DGAGetViewPortSize:
	return ProcXF86DGAGetViewPortSize(client);
    case X_XF86DGASetViewPort:
	return ProcXF86DGASetViewPort(client);
    case X_XF86DGAGetVidPage:
	return ProcXF86DGAGetVidPage(client);
    case X_XF86DGASetVidPage:
	return ProcXF86DGASetVidPage(client);
    case X_XF86DGAInstallColormap:
	return ProcXF86DGAInstallColormap(client);
    case X_XF86DGAQueryDirectVideo:
	return ProcXF86DGAQueryDirectVideo(client);
    case X_XF86DGAViewPortChanged:
	return ProcXF86DGAViewPortChanged(client);
    default:
	return BadRequest;
    }
}

static int
SProcXF86DGAQueryVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xXF86DGAQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcXF86DGAQueryVersion(client);
}

static int
SProcXF86DGADirectVideo(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86DGADirectVideoReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86DGADirectVideoReq);
    swaps(&stuff->screen, n);
    swaps(&stuff->enable, n);
    return ProcXF86DGADirectVideo(client);
}

static int
SProcXF86DGADispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    /* It is bound to be non-local when there is byte swapping */
    if (!LocalClient(client))
	return DGAErrorBase + XF86DGAClientNotLocal;

    switch (stuff->data)
    {
    case X_XF86DGAQueryVersion:
	return SProcXF86DGAQueryVersion(client);
    case X_XF86DGADirectVideo:
	return SProcXF86DGADirectVideo(client);
    default:
	return BadRequest;
    }
}

