/* $XFree86: xc/programs/Xserver/Xext/xf86vmode.c,v 3.9 1995/07/15 16:00:41 dawes Exp $ */

/*

Copyright (c) 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from the Kaleb S. KEITHLEY

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
#define _XF86VIDMODE_SERVER_
#include "xf86vmstr.h"
#include "Xfuncproto.h"
#include "../hw/xfree86/common/xf86.h"

extern int xf86ScreenIndex;

static int VGAHelpErrorBase;

static int ProcVGAHelpDispatch(), SProcVGAHelpDispatch();
static void VGAHelpResetProc();

static unsigned char VGAHelpReqCode = 0;

extern void Swap32Write();

/* The XF86VIDMODE_EVENTS code is far from complete */

#ifdef XF86VIDMODE_EVENTS
static int XF86VidModeEventBase = 0;

static void SXF86VidModeNotifyEvent();


extern WindowPtr *WindowTable;

static RESTYPE EventType;	/* resource type for event masks */

typedef struct _XF86VidModeEvent *XF86VidModeEventPtr;

typedef struct _XF86VidModeEvent {
    XF86VidModeEventPtr	next;
    ClientPtr		client;
    ScreenPtr		screen;
    XID			resource;
    CARD32		mask;
} XF86VidModeEventRec;

static int XF86VidModeFreeEvents();

typedef struct _XF86VidModeScreenPrivate {
    XF86VidModeEventPtr	events;
    Bool		hasWindow;
} XF86VidModeScreenPrivateRec, *XF86VidModeScreenPrivatePtr;
   
static int ScreenPrivateIndex;

#define GetScreenPrivate(s) ((ScreenSaverScreenPrivatePtr)(s)->devPrivates[ScreenPrivateIndex].ptr)
#define SetScreenPrivate(s,v) ((s)->devPrivates[ScreenPrivateIndex].ptr = (pointer) v);
#define SetupScreen(s)  ScreenSaverScreenPrivatePtr pPriv = GetScreenPrivate(s)

#define New(t)  ((t *) xalloc (sizeof (t)))
#endif

void
XFree86VidModeExtensionInit()
{
    ExtensionEntry* extEntry;
    int		    i;
    ScreenPtr	    pScreen;

#ifdef XF86VIDMODE_EVENTS
    EventType = CreateNewResourceType(XF86VidModeFreeEvents);
    ScreenPrivateIndex = AllocateScreenPrivateIndex ();
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	SetScreenPrivate (pScreen, NULL);
    }
#endif

    if (
#ifdef XF86VIDMODE_EVENTS
        EventType && ScreenPrivateIndex != -1 &&
#endif
	(extEntry = AddExtension(XF86VIDMODENAME,
				XF86VidModeNumberEvents,
				XF86VidModeNumberErrors,
				ProcVGAHelpDispatch,
				SProcVGAHelpDispatch,
				VGAHelpResetProc,
				StandardMinorOpcode))) {
	VGAHelpReqCode = (unsigned char)extEntry->base;
	VGAHelpErrorBase = extEntry->errorBase;
#ifdef XF86VIDMODE_EVENTS
	XF86VidModeEventBase = extEntry->eventBase;
	EventSwapVector[XF86VidModeEventBase] = SXF86VidModeNotifyEvent;
#endif
    }
}

/*ARGSUSED*/
static void
VGAHelpResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

#ifdef XF86VIDMODE_EVENTS
static void
CheckScreenPrivate (pScreen)
    ScreenPtr	pScreen;
{
    SetupScreen (pScreen);

    if (!pPriv)
	return;
    if (!pPriv->events && !pPriv->hasWindow) {
	xfree (pPriv);
	SetScreenPrivate (pScreen, NULL);
    }
}
    
static XF86VidModeScreenPrivatePtr
MakeScreenPrivate (pScreen)
    ScreenPtr	pScreen;
{
    SetupScreen (pScreen);

    if (pPriv)
	return pPriv;
    pPriv = New (XF86VidModeScreenPrivateRec);
    if (!pPriv)
	return 0;
    pPriv->events = 0;
    pPriv->hasWindow = FALSE;
    SetScreenPrivate (pScreen, pPriv);
    return pPriv;
}

static unsigned long
getEventMask (pScreen, client)
    ScreenPtr	pScreen;
    ClientPtr	client;
{
    SetupScreen(pScreen);
    XF86VidModeEventPtr pEv;

    if (!pPriv)
	return 0;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
	if (pEv->client == client)
	    return pEv->mask;
    return 0;
}

static Bool
setEventMask (pScreen, client, mask)
    ScreenPtr	pScreen;
    ClientPtr	client;
    unsigned long mask;
{
    SetupScreen(pScreen);
    XF86VidModeEventPtr pEv, *pPrev;

    if (getEventMask (pScreen, client) == mask)
	return TRUE;
    if (!pPriv) {
	pPriv = MakeScreenPrivate (pScreen);
	if (!pPriv)
	    return FALSE;
    }
    for (pPrev = &pPriv->events; pEv = *pPrev; pPrev = &pEv->next)
	if (pEv->client == client)
	    break;
    if (mask == 0) {
	*pPrev = pEv->next;
	xfree (pEv);
	CheckScreenPrivate (pScreen);
    } else {
	if (!pEv) {
	    pEv = New (ScreenSaverEventRec);
	    if (!pEv) {
		CheckScreenPrivate (pScreen);
		return FALSE;
	    }
	    *pPrev = pEv;
	    pEv->next = NULL;
	    pEv->client = client;
	    pEv->screen = pScreen;
	    pEv->resource = FakeClientID (client->index);
	}
	pEv->mask = mask;
    }
    return TRUE;
}

static int
XF86VidModeFreeEvents (value, id)
    pointer value;
    XID id;
{
    XF86VidModeEventPtr	pOld = (XF86VidModeEventPtr)value;
    ScreenPtr pScreen = pOld->screen;
    SetupScreen (pScreen);
    XF86VidModeEventPtr	pEv, *pPrev;

    if (!pPriv)
	return TRUE;
    for (pPrev = &pPriv->events; pEv = *pPrev; pPrev = &pEv->next)
	if (pEv == pOld)
	    break;
    if (!pEv)
	return TRUE;
    *pPrev = pEv->next;
    xfree (pEv);
    CheckScreenPrivate (pScreen);
    return TRUE;
}

static void
SendXF86VidModeNotify (pScreen, state, forced)
    ScreenPtr	pScreen;
    int	    state;
    Bool    forced;
{
    XF86VidModeScreenPrivatePtr	pPriv;
    XF86VidModeEventPtr		pEv;
    unsigned long		mask;
    xXF86VidModeNotifyEvent	ev;
    ClientPtr			client;
    int				kind;

    UpdateCurrentTimeIf ();
    mask = XF86VidModeNotifyMask;
    pScreen = screenInfo.screens[pScreen->myNum];
    pPriv = GetScreenPrivate(pScreen);
    if (!pPriv)
	return;
    kind = XF86VidModeModeChange;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
    {
	client = pEv->client;
	if (client->clientGone)
	    continue;
	if (!(pEv->mask & mask))
	    continue;
	ev.type = XF86VidModeNotify + XF86VidModeEventBase;
	ev.state = state;
	ev.sequenceNumber = client->sequence;
	ev.timestamp = currentTime.milliseconds;
	ev.root = WindowTable[pScreen->myNum]->drawable.id;
	ev.kind = kind;
	ev.forced = forced;
	WriteEventsToClient (client, 1, (xEvent *) &ev);
    }
}

static void
SXF86VidModeNotifyEvent (from, to)
    xXF86VidModeNotifyEvent *from, *to;
{
    to->type = from->type;
    to->state = from->state;
    cpswaps (from->sequenceNumber, to->sequenceNumber);
    cpswapl (from->timestamp, to->timestamp);    
    cpswapl (from->root, to->root);    
    to->kind = from->kind;
    to->forced = from->forced;
}
#endif
	
static int
ProcVGAHelpQueryVersion(client)
    register ClientPtr client;
{
    REQUEST(xVGAHelpQueryVersionReq);
    xVGAHelpQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xVGAHelpQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86VIDMODE_MAJOR_VERSION;
    rep.minorVersion = XF86VIDMODE_MINOR_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xVGAHelpQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcVGAHelpGetModeLine(client)
    register ClientPtr client;
{
    REQUEST(xVGAHelpGetModeLineReq);
    xVGAHelpGetModeLineReply rep;
    register int n;
    ScrnInfoPtr vptr;
    DisplayModePtr mptr;
    int privsize;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    if (!mptr->Private)
	privsize = 0;
    else
	privsize = mptr->PrivSize;

    REQUEST_SIZE_MATCH(xVGAHelpGetModeLineReq);
    rep.type = X_Reply;
    rep.length = (SIZEOF(xVGAHelpGetModeLineReply) - SIZEOF(xGenericReply) +
		  privsize * sizeof(INT32)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.dotclock = vptr->clock[mptr->Clock];
    rep.hdisplay = mptr->HDisplay;
    rep.hsyncstart = mptr->HSyncStart;
    rep.hsyncend = mptr->HSyncEnd;
    rep.htotal = mptr->HTotal;
    rep.vdisplay = mptr->VDisplay;
    rep.vsyncstart = mptr->VSyncStart;
    rep.vsyncend = mptr->VSyncEnd;
    rep.vtotal = mptr->VTotal;
    rep.flags = mptr->Flags;
    rep.privsize = privsize;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.dotclock, n);
    	swaps(&rep.hdisplay, n);
    	swaps(&rep.hsyncstart, n);
    	swaps(&rep.hsyncend, n);
    	swaps(&rep.htotal, n);
    	swaps(&rep.vdisplay, n);
    	swaps(&rep.vsyncstart, n);
    	swaps(&rep.vsyncend, n);
    	swaps(&rep.vtotal, n);
	swapl(&rep.flags, n);
	swapl(&rep.privsize, n);
    }
    WriteToClient(client, sizeof(xVGAHelpGetModeLineReply), (char *)&rep);
    if (privsize) {
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient(client, privsize * sizeof(INT32),
				 mptr->Private);
    }
    return (client->noClientException);
}

static int
ProcVGAHelpModModeLine(client)
    register ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpModModeLineReq);
    ScrnInfoPtr vptr;
    DisplayModePtr mptr;
    DisplayModeRec modetmp;
    int len;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    REQUEST_AT_LEAST_SIZE(xVGAHelpModModeLineReq);
    len = client->req_len - (sizeof(xVGAHelpModModeLineReq) >> 2);
    if (len != stuff->privsize)
	return BadLength;

    if (stuff->hsyncstart < stuff->hdisplay   ||
	stuff->hsyncend   < stuff->hsyncstart ||
	stuff->htotal     < stuff->hsyncend   ||
	stuff->vsyncstart < stuff->vdisplay   ||
	stuff->vsyncend   < stuff->vsyncstart ||
	stuff->vtotal     < stuff->vsyncend)
	return BadValue;

    memcpy(&modetmp, mptr, sizeof(DisplayModeRec));

    modetmp.HDisplay   = stuff->hdisplay;
    modetmp.HSyncStart = stuff->hsyncstart;
    modetmp.HSyncEnd   = stuff->hsyncend;
    modetmp.HTotal     = stuff->htotal;
    modetmp.VDisplay   = stuff->vdisplay;
    modetmp.VSyncStart = stuff->vsyncstart;
    modetmp.VSyncEnd   = stuff->vsyncend;
    modetmp.VTotal     = stuff->vtotal;
    modetmp.Flags      = stuff->flags;
    if (mptr->PrivSize && stuff->privsize) {
	if (mptr->PrivSize != stuff->privsize)
	    return BadValue;
    }
    if (mptr->PrivSize && mptr->Private) {
	modetmp.Private = ALLOCATE_LOCAL(mptr->PrivSize * sizeof(INT32));
	if (stuff->privsize)
	    memcpy(modetmp.Private, &stuff[1], mptr->PrivSize * sizeof(INT32));
	else
	    memcpy(modetmp.Private, mptr->Private,
		   mptr->PrivSize * sizeof(INT32));
    }

    /* Check that the mode is consistent with the monitor specs */
    switch (xf86CheckMode(vptr, &modetmp, vptr->monitor, FALSE)) {
	case MODE_HSYNC:
	    DEALLOCATE_LOCAL(modetmp.Private);
	    return VGAHelpErrorBase + XF86VidModeBadHTimings;
	case MODE_VSYNC:
	    DEALLOCATE_LOCAL(modetmp.Private);
	    return VGAHelpErrorBase + XF86VidModeBadVTimings;
    }

    /* Check that the driver is happy with the mode */
    if (!vptr->ValidMode(&modetmp)) {
	DEALLOCATE_LOCAL(modetmp.Private);
	return VGAHelpErrorBase + XF86VidModeModeUnsuitable;
    }

    DEALLOCATE_LOCAL(modetmp.Private);

    mptr->HDisplay   = stuff->hdisplay;
    mptr->HSyncStart = stuff->hsyncstart;
    mptr->HSyncEnd   = stuff->hsyncend;
    mptr->HTotal     = stuff->htotal;
    mptr->VDisplay   = stuff->vdisplay;
    mptr->VSyncStart = stuff->vsyncstart;
    mptr->VSyncEnd   = stuff->vsyncend;
    mptr->VTotal     = stuff->vtotal;
    mptr->Flags      = stuff->flags;
    mptr->CrtcHDisplay   = stuff->hdisplay;
    mptr->CrtcHSyncStart = stuff->hsyncstart;
    mptr->CrtcHSyncEnd   = stuff->hsyncend;
    mptr->CrtcHTotal     = stuff->htotal;
    mptr->CrtcVDisplay   = stuff->vdisplay;
    mptr->CrtcVSyncStart = stuff->vsyncstart;
    mptr->CrtcVSyncEnd   = stuff->vsyncend;
    mptr->CrtcVTotal     = stuff->vtotal;
    mptr->CrtcVAdjusted = FALSE;
    mptr->CrtcHAdjusted = FALSE;
    if (mptr->Flags & V_DBLSCAN)
    {
	mptr->CrtcVDisplay *= 2;
	mptr->CrtcVSyncStart *= 2;
	mptr->CrtcVSyncEnd *= 2;
	mptr->CrtcVTotal *= 2;
	mptr->CrtcVAdjusted = TRUE;
    }
    if (mptr->PrivSize && stuff->privsize) {
	memcpy(mptr->Private, &stuff[1], mptr->PrivSize * sizeof(INT32));
    }

    (vptr->SwitchMode)(mptr);

    return(client->noClientException);
}

static int
ProcVGAHelpSwitchMode(client)
    register ClientPtr client;
{
    REQUEST(xVGAHelpSwitchModeReq);
    ScreenPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = screenInfo.screens[stuff->screen];

    REQUEST_SIZE_MATCH(xVGAHelpSwitchModeReq);

    xf86ZoomViewport(vptr, (short)stuff->zoom);
    return (client->noClientException);
}

static int
ProcXF86VidModeLockModeSwitch(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeLockModeSwitchReq);
    ScreenPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = screenInfo.screens[stuff->screen];

    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);

    xf86LockZoom(vptr, (short)stuff->lock);
    return (client->noClientException);
}

static int
ProcXF86VidModeSetSaver(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeSetSaverReq);
    ScrnInfoPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;

    REQUEST_SIZE_MATCH(xXF86VidModeSetSaverReq);

    if (stuff->suspendTime < 0)
	return BadValue;
    if (stuff->offTime < 0)
	return BadValue;

    vptr->suspendTime = stuff->suspendTime;
    vptr->offTime = stuff->offTime;

    return (client->noClientException);
}

static int
ProcVGAHelpGetMonitor(client)
    register ClientPtr client;
{
    REQUEST(xVGAHelpGetMonitorReq);
    xVGAHelpGetMonitorReply rep;
    register int n;
    ScrnInfoPtr vptr;
    MonPtr mptr;
    CARD32 *hsyncdata, *vsyncdata;
    int i;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->monitor;

    REQUEST_SIZE_MATCH(xVGAHelpGetMonitorReq);
    rep.type = X_Reply;
    rep.vendorLength = strlen(mptr->vendor);
    rep.modelLength = strlen(mptr->model);
    rep.length = (SIZEOF(xVGAHelpGetMonitorReply) - SIZEOF(xGenericReply) +
		  (mptr->n_hsync + mptr->n_vrefresh) * sizeof(CARD32) +
	          (rep.vendorLength + 3 & ~3) +
		  (rep.modelLength + 3 & ~3)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.nhsync = mptr->n_hsync;
    rep.nvsync = mptr->n_vrefresh;
    rep.bandwidth = (unsigned long)(mptr->bandwidth * 1e6);
    hsyncdata = ALLOCATE_LOCAL(mptr->n_hsync * sizeof(CARD32));
    if (!hsyncdata) {
	return BadAlloc;
    }
    vsyncdata = ALLOCATE_LOCAL(mptr->n_vrefresh * sizeof(CARD32));
    if (!vsyncdata) {
	DEALLOCATE_LOCAL(hsyncdata);
	return BadAlloc;
    }
    for (i = 0; i < mptr->n_hsync; i++) {
	hsyncdata[i] = (unsigned short)(mptr->hsync[i].lo * 100.0) |
		       (unsigned short)(mptr->hsync[i].hi * 100.0) << 16;
    }
    for (i = 0; i < mptr->n_vrefresh; i++) {
	vsyncdata[i] = (unsigned short)(mptr->vrefresh[i].lo * 100.0) |
		       (unsigned short)(mptr->vrefresh[i].hi * 100.0) << 16;
    }
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.bandwidth, n);
    }
    WriteToClient(client, SIZEOF(xVGAHelpGetMonitorReply), (char *)&rep);
    client->pSwapReplyFunc = Swap32Write;
    WriteSwappedDataToClient(client, mptr->n_hsync * sizeof(CARD32),
			     hsyncdata);
    WriteSwappedDataToClient(client, mptr->n_vrefresh * sizeof(CARD32),
			     vsyncdata);
    WriteToClient(client, rep.vendorLength, mptr->vendor);
    WriteToClient(client, rep.modelLength, mptr->model);
    DEALLOCATE_LOCAL(hsyncdata);
    DEALLOCATE_LOCAL(vsyncdata);
    return (client->noClientException);
}

static int
ProcXF86VidModeGetSaver(client)
    register ClientPtr client;
{
    REQUEST(xXF86VidModeGetSaverReq);
    xXF86VidModeGetSaverReply rep;
    register int n;
    ScrnInfoPtr vptr;

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;

    REQUEST_SIZE_MATCH(xXF86VidModeGetSaverReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.suspendTime = vptr->suspendTime;
    rep.offTime = vptr->offTime;
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.suspendTime, n);
    	swapl(&rep.offTime, n);
    }
    WriteToClient(client, SIZEOF(xXF86VidModeGetSaverReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcVGAHelpDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_VGAHelpQueryVersion:
	return ProcVGAHelpQueryVersion(client);
    case X_VGAHelpGetModeLine:
	return ProcVGAHelpGetModeLine(client);
    case X_VGAHelpModModeLine:
	return ProcVGAHelpModModeLine(client);
    case X_VGAHelpSwitchMode:
	return ProcVGAHelpSwitchMode(client);
    case X_VGAHelpGetMonitor:
	return ProcVGAHelpGetMonitor(client);
    case X_XF86VidModeLockModeSwitch:
	return ProcXF86VidModeLockModeSwitch(client);
    case X_XF86VidModeGetSaver:
	return ProcXF86VidModeGetSaver(client);
    case X_XF86VidModeSetSaver:
	return ProcXF86VidModeSetSaver(client);
    default:
	return BadRequest;
    }
}

static int
SProcVGAHelpQueryVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xVGAHelpQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcVGAHelpQueryVersion(client);
}

static int
SProcVGAHelpGetModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpGetModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xVGAHelpGetModeLineReq);
    swapl(&stuff->screen, n);
    return ProcVGAHelpGetModeLine(client);
}

static int
SProcXF86VidModeGetSaver(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeGetSaverReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeGetSaverReq);
    swapl(&stuff->screen, n);
    return ProcXF86VidModeGetSaver(client);
}

static int
SProcXF86VidModeSetSaver(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeSetSaverReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeSetSaverReq);
    swapl(&stuff->screen, n);
    swapl(&stuff->suspendTime, n);
    swapl(&stuff->offTime, n);
    return ProcXF86VidModeSetSaver(client);
}

static int
SProcVGAHelpModModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpModModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xVGAHelpModModeLineReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->hdisplay, n);
    swaps(&stuff->hsyncstart, n);
    swaps(&stuff->hsyncend, n);
    swaps(&stuff->htotal, n);
    swaps(&stuff->vdisplay, n);
    swaps(&stuff->vsyncstart, n);
    swaps(&stuff->vsyncend, n);
    swaps(&stuff->vtotal, n);
    swapl(&stuff->flags, n);
    swapl(&stuff->privsize, n);
    SwapRestL(stuff);
    return ProcVGAHelpModModeLine(client);
}

static int
SProcVGAHelpSwitchMode(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpSwitchModeReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xVGAHelpSwitchModeReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->zoom, n);
    return ProcVGAHelpSwitchMode(client);
}

static int
SProcXF86VidModeLockModeSwitch(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86VidModeLockModeSwitchReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);
    swapl(&stuff->screen, n);
    swaps(&stuff->lock, n);
    return ProcXF86VidModeLockModeSwitch(client);
}

static int
SProcVGAHelpGetMonitor(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpGetMonitorReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xVGAHelpGetMonitorReq);
    swapl(&stuff->screen, n);
    return ProcVGAHelpGetMonitor(client);
}

static int
SProcVGAHelpDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_VGAHelpQueryVersion:
	return SProcVGAHelpQueryVersion(client);
    case X_VGAHelpGetModeLine:
	return SProcVGAHelpGetModeLine(client);
    case X_VGAHelpModModeLine:
	return SProcVGAHelpModModeLine(client);
    case X_VGAHelpSwitchMode:
	return SProcVGAHelpSwitchMode(client);
    case X_VGAHelpGetMonitor:
	return SProcVGAHelpGetMonitor(client);
    case X_XF86VidModeLockModeSwitch:
	return SProcXF86VidModeLockModeSwitch(client);
    case X_XF86VidModeGetSaver:
	return SProcXF86VidModeGetSaver(client);
    case X_XF86VidModeSetSaver:
	return SProcXF86VidModeSetSaver(client);
    default:
	return BadRequest;
    }
}
