/* $XFree86: xc/programs/Xserver/Xext/xf86dga.c,v 3.13 1999/03/07 11:40:26 dawes Exp $ */

/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995, 1996, 1999  XFree86 Inc

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

static void XF86DGAResetProc(ExtensionEntry *extEntry);

static unsigned char DGAReqCode = 0;

void
XFree86DGAExtensionInit(void)
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
XF86DGAResetProc (ExtensionEntry *extEntry)
{
}

static int
ProcXF86DGAQueryVersion(ClientPtr client)
{
    xXF86DGAQueryVersionReply rep;
    int n;

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
ProcXF86DGAGetVideoLL(ClientPtr client)
{
    REQUEST(xXF86DGAGetVideoLLReq);
    xXF86DGAGetVideoLLReply rep;
    XDGADeviceRec device;
    int n, mode;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetVideoLLReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if(!DGAAvailable(stuff->screen))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if(!(mode = DGAGetOldDGAMode(stuff->screen)))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    DGAGetDeviceInfo(stuff->screen, &device, mode);

    rep.offset = (CARD32)device.data;
    rep.width = device.mode.bytesPerScanline / 
		(device.mode.bitsPerPixel >> 3);
    rep.bank_size = device.mode.bytesPerScanline * device.mode.imageHeight;
    rep.bank_size = (rep.bank_size + 1023) & ~1023;
    rep.ram_size = rep.bank_size >> 10;

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
ProcXF86DGADirectVideo(ClientPtr client)
{
    int mode;
    XDGADeviceRec dev;
    REQUEST(xXF86DGADirectVideoReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGADirectVideoReq);

    if (!DGAAvailable(stuff->screen)) 
	return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (stuff->enable & XF86DGADirectGraphics) {
	if(!(mode = DGAGetOldDGAMode(stuff->screen)))
	    return (DGAErrorBase + XF86DGANoDirectVideoMode);
    } else
	mode = 0;

    if(Success != DGASetMode(stuff->screen, mode, &dev))
	return (DGAErrorBase + XF86DGAScreenNotActive);

    return (client->noClientException);
}

static int
ProcXF86DGAGetViewPortSize(ClientPtr client)
{
    int mode;
    XDGADeviceRec device;
    REQUEST(xXF86DGAGetViewPortSizeReq);
    xXF86DGAGetViewPortSizeReply rep;
    int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetViewPortSizeReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen)) 
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    if(!(mode = DGAGetOldDGAMode(stuff->screen)))
	return (DGAErrorBase + XF86DGANoDirectVideoMode);

    DGAGetDeviceInfo(stuff->screen, &device, mode);

    rep.width = device.mode.viewportWidth;
    rep.height = device.mode.viewportHeight;

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
ProcXF86DGASetViewPort(ClientPtr client)
{
    REQUEST(xXF86DGASetViewPortReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGASetViewPortReq);

    if (!DGAActive(stuff->screen))
	return (DGAErrorBase + XF86DGADirectNotActivated);

    if (!DGASetViewport(stuff->screen, stuff->x, stuff->y, DGA_FLIP_RETRACE))
	return DGAErrorBase + XF86DGADirectNotActivated;

    return (client->noClientException);
}

static int
ProcXF86DGAGetVidPage(ClientPtr client)
{
    REQUEST(xXF86DGAGetVidPageReq);
    xXF86DGAGetVidPageReply rep;
    int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAGetVidPageReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.vpage = 0;  /* silently fail */
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.vpage, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAGetVidPageReply), (char *)&rep);
    return (client->noClientException);
}


static int
ProcXF86DGASetVidPage(ClientPtr client)
{
    REQUEST(xXF86DGASetVidPageReq);

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGASetVidPageReq);

    /* silently fail */

    return (client->noClientException);
}


static int
ProcXF86DGAInstallColormap(ClientPtr client)
{
    ColormapPtr pcmp;
    REQUEST(xXF86DGAInstallColormapReq);

    REQUEST_SIZE_MATCH(xXF86DGAInstallColormapReq);

    if (!DGAActive(stuff->screen))
	return (DGAErrorBase + XF86DGADirectNotActivated);

    pcmp = (ColormapPtr  )LookupIDByType(stuff->id, RT_COLORMAP);
    if (pcmp) {
	DGAInstallColormap(pcmp);
        return (client->noClientException);
    } else {
        client->errorValue = stuff->id;
        return (BadColor);
    }
}

static int
ProcXF86DGAQueryDirectVideo(ClientPtr client)
{
    REQUEST(xXF86DGAQueryDirectVideoReq);
    xXF86DGAQueryDirectVideoReply rep;
    int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAQueryDirectVideoReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.flags = 0;

    if (DGAAvailable(stuff->screen))
	rep.flags = XF86DGADirectPresent;

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.flags, n);
    }
    WriteToClient(client, SIZEOF(xXF86DGAQueryDirectVideoReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DGAViewPortChanged(ClientPtr client)
{
    REQUEST(xXF86DGAViewPortChangedReq);
    xXF86DGAViewPortChangedReply rep;
    int n;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    REQUEST_SIZE_MATCH(xXF86DGAViewPortChangedReq);

    if (!DGAActive(stuff->screen))
	return (DGAErrorBase + XF86DGADirectNotActivated);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.result = 1;

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

