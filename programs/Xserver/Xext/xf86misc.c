/* $XFree86*/

/*
 * Copyright (c) 1995, 1996  The XFree86 Project, Inc
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86MISC_SERVER_
#include "xf86mscstr.h"
#include "Xfuncproto.h"
#include "../hw/xfree86/common/xf86.h"

extern int xf86ScreenIndex;

static int miscErrorBase;

static int ProcXF86MiscDispatch(), SProcXF86MiscDispatch();
static void XF86MiscResetProc();

static unsigned char XF86MiscReqCode = 0;

extern void Swap32Write();


void
XFree86MiscExtensionInit()
{
    ExtensionEntry* extEntry;
    int		    i;
    ScreenPtr	    pScreen;


    if (
	(extEntry = AddExtension(XF86MISCNAME,
				XF86MiscNumberEvents,
				XF86MiscNumberErrors,
				ProcXF86MiscDispatch,
				SProcXF86MiscDispatch,
				XF86MiscResetProc,
				StandardMinorOpcode))) {
	XF86MiscReqCode = (unsigned char)extEntry->base;
	miscErrorBase = extEntry->errorBase;
    }
}

/*ARGSUSED*/
static void
XF86MiscResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

static int
ProcXF86MiscQueryVersion(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscQueryVersionReq);
    xXF86MiscQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86MiscQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86MISC_MAJOR_VERSION;
    rep.minorVersion = XF86MISC_MINOR_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swaps(&rep.majorVersion, n);
    	swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xXF86MiscQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86MiscSetSaver(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscSetSaverReq);
    ScrnInfoPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;

    REQUEST_SIZE_MATCH(xXF86MiscSetSaverReq);

    if (stuff->suspendTime < 0)
	return BadValue;
    if (stuff->offTime < 0)
	return BadValue;

    vptr->suspendTime = stuff->suspendTime * 1000;
    vptr->offTime = stuff->offTime * 1000;

    return (client->noClientException);
}

static int
ProcXF86MiscGetSaver(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscGetSaverReq);
    xXF86MiscGetSaverReply rep;
    register int n;
    ScrnInfoPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;

    REQUEST_SIZE_MATCH(xXF86MiscGetSaverReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.suspendTime = vptr->suspendTime / 1000;
    rep.offTime = vptr->offTime / 1000;
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.suspendTime, n);
    	swapl(&rep.offTime, n);
    }
    WriteToClient(client, SIZEOF(xXF86MiscGetSaverReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86MiscDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XF86MiscQueryVersion:
	return ProcXF86MiscQueryVersion(client);
    case X_XF86MiscGetSaver:
	return ProcXF86MiscGetSaver(client);
    case X_XF86MiscSetSaver:
	return ProcXF86MiscSetSaver(client);
    default:
	return BadRequest;
    }
}

static int
SProcXF86MiscQueryVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xXF86MiscQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcXF86MiscQueryVersion(client);
}

static int
SProcXF86MiscGetSaver(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscGetSaverReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscGetSaverReq);
    swaps(&stuff->screen, n);
    return ProcXF86MiscGetSaver(client);
}

static int
SProcXF86MiscSetSaver(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscSetSaverReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscSetSaverReq);
    swaps(&stuff->screen, n);
    swapl(&stuff->suspendTime, n);
    swapl(&stuff->offTime, n);
    return ProcXF86MiscSetSaver(client);
}

static int
SProcXF86MiscDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XF86MiscQueryVersion:
	return SProcXF86MiscQueryVersion(client);
    case X_XF86MiscGetSaver:
	return SProcXF86MiscGetSaver(client);
    case X_XF86MiscSetSaver:
	return SProcXF86MiscSetSaver(client);
    default:
	return BadRequest;
    }
}
