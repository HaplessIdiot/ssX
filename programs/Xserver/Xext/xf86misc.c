/* $XFree86: xc/programs/Xserver/Xext/xf86misc.c,v 3.7 1996/01/31 11:46:12 dawes Exp $ */

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
#include "inputstr.h"
#include "servermd.h"
#define _XF86MISC_SERVER_
#include "xf86mscstr.h"
#include "Xfuncproto.h"
#include "xf86.h"
#include "xf86Priv.h"

extern int xf86ScreenIndex;

static int miscErrorBase;

static int ProcXF86MiscDispatch(), SProcXF86MiscDispatch();
static void XF86MiscResetProc();

static unsigned char XF86MiscReqCode = 0;

extern void Swap32Write();

extern InputInfo inputInfo;

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
ProcXF86MiscGetMouseSettings(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscGetMouseSettingsReq);
    xXF86MiscGetMouseSettingsReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86MiscGetMouseSettingsReq);
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.mousetype = xf86Info.mseType;
#ifdef XQUEUE
    if (xf86Info.mseProc == xf86XqueMseProc)
        rep.mousetype = MTYPE_XQUEUE;
#endif
#if defined(USE_OSMOUSE) || defined(OSMOUSE_ONLY)
    if (xf86Info.mseProc == xf86OsMouseProc)
        rep.mousetype = MTYPE_OSMOUSE;
#endif
    rep.baudrate = xf86Info.baudRate;
    rep.samplerate = xf86Info.sampleRate;
    rep.emulate3buttons = xf86Info.emulate3Buttons;
    rep.emulate3timeout = xf86Info.emulate3Timeout;
    rep.chordmiddle = xf86Info.chordMiddle;
    rep.flags = xf86Info.mouseFlags;
    if (xf86Info.mseDevice)
        rep.devnamelen = strlen(xf86Info.mseDevice);
    else
        rep.devnamelen = 0;
    rep.length = (sizeof(xXF86MiscGetMouseSettingsReply) -
		  sizeof(xGenericReply) + (rep.devnamelen+3 & ~3)) >> 2;
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.mousetype, n);
    	swapl(&rep.baudrate, n);
    	swapl(&rep.samplerate, n);
    	swapl(&rep.emulate3buttons, n);
    	swapl(&rep.emulate3timeout, n);
    	swapl(&rep.chordmiddle, n);
    	swapl(&rep.flags, n);
    }
    WriteToClient(client, SIZEOF(xXF86MiscGetMouseSettingsReply), (char *)&rep);
    if (rep.devnamelen)
        WriteToClient(client, rep.devnamelen, xf86Info.mseDevice);
    return (client->noClientException);
}

static int
ProcXF86MiscGetKbdSettings(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscGetKbdSettingsReq);
    xXF86MiscGetKbdSettingsReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86MiscGetKbdSettingsReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.kbdtype = xf86Info.kbdType;
#ifdef XQUEUE
    if (xf86Info.kbdProc == xf86XqueKbdProc)
        rep.kbdtype = KTYPE_XQUEUE;
#endif
    rep.rate = xf86Info.kbdRate;
    rep.delay = xf86Info.kbdDelay;
    rep.servnumlock = xf86Info.serverNumLock;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    	swapl(&rep.kbdtype, n);
    	swapl(&rep.rate, n);
    	swapl(&rep.delay, n);
    }
    WriteToClient(client, SIZEOF(xXF86MiscGetKbdSettingsReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86MiscSetMouseSettings(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscSetMouseSettingsReq);

    REQUEST_SIZE_MATCH(xXF86MiscSetMouseSettingsReq);

    if (stuff->baudrate < 1200)
	return BadValue;
    if (stuff->samplerate < 0)
	return BadValue;
    if (stuff->emulate3timeout < 0)
	return BadValue;
    if (stuff->chordmiddle && stuff->emulate3buttons)
	return BadValue;
    if (stuff->chordmiddle
            && !(stuff->mousetype == MTYPE_MICROSOFT
                 || stuff->mousetype == MTYPE_LOGIMAN) )
	return BadValue;
    if (stuff->flags & (MF_CLEAR_DTR|MF_CLEAR_RTS)
            && stuff->mousetype != MTYPE_MOUSESYS)
	return BadValue;
    if (stuff->baudrate % 1200 != 0
            || stuff->baudrate < 1200 || stuff->baudrate > 9600)
	return BadValue;
    if (stuff->mousetype == MTYPE_LOGIMAN
            && !(stuff->baudrate == 1200 || stuff->baudrate == 9600) )
	return BadValue;
    if (stuff->mousetype == MTYPE_LOGIMAN && stuff->samplerate)
	return BadValue;
#ifdef OSMOUSE_ONLY
    if (stuff->mousetype != MTYPE_OSMOUSE)
	return BadValue;
#else
#ifndef XQUEUE
    if (stuff->mousetype == MTYPE_XQUEUE)
	return BadValue;
#endif
#ifndef USE_OSMOUSE
    if (stuff->mousetype == MTYPE_OSMOUSE)
	return BadValue;
#endif
#endif /* OSMOUSE_ONLY */
    if (stuff->mousetype > MTYPE_OSMOUSE
            || stuff->mousetype < MTYPE_MICROSOFT)
	return BadValue;

    /* I think all three of these can just be changed on the fly */
    xf86Info.emulate3Buttons = stuff->emulate3buttons!=0;
    xf86Info.emulate3Timeout = stuff->emulate3timeout;
    xf86Info.chordMiddle = stuff->chordmiddle!=0;

    /* Change other settings at your own risk - do not expect the
       following to work! */
    if (xf86Info.mseType != stuff->mousetype
            || xf86Info.baudRate != stuff->baudrate 
            || xf86Info.sampleRate != stuff->samplerate 
            || xf86Info.mouseFlags != stuff->flags ) {
	
	ButtonClassPtr butc = inputInfo.pointer->button;

	if (xf86Info.pPointer)
            (xf86Info.mseProc)(xf86Info.pPointer, DEVICE_CLOSE);

	/* Dynamically changing the number of buttons could
	   confuse some clients, but we'll do it anyway */
	if (stuff->mousetype == MTYPE_MMHIT
	        || stuff->mousetype == MTYPE_GLIDEPOINT) {
	    if (xf86Info.mseType != MTYPE_MMHIT
		    && xf86Info.mseType != MTYPE_GLIDEPOINT) {
		butc->numButtons = 4;
		/* should this be set or left alone?? */
		butc->map[4] = 4;
	    }
	} else {
	    if (xf86Info.mseType == MTYPE_MMHIT
		    || xf86Info.mseType == MTYPE_GLIDEPOINT) {
		butc->numButtons = 3;
	    }
	}

        xf86Info.mseType = stuff->mousetype;
#if defined(USE_OSMOUSE) || defined(OSMOUSE_ONLY)
	xf86Info.mseProc = xf86OsMouseProc;
	xf86Info.mseEvents = xf86OsMouseEvents;
#else
	xf86Info.mseProc = xf86MseProc;
	xf86Info.mseEvents = xf86MseEvents;
#endif

#ifdef XQUEUE
	if (xf86Info.mseType == MTYPE_XQUEUE) {
            xf86Info.mseProc = xf86XqueMseProc;
            xf86Info.mseEvents = xf86XqueEvents;
            xf86Info.xqueSema = 0;
	}
#endif
#ifdef USE_OSMOUSE
	if (xf86Info.mseType == MTYPE_OSMOUSE) {
            xf86Info.mseProc = xf86OsMouseProc;
            xf86Info.mseEvents = xf86OsMouseEvents;
	}
#endif
        xf86Info.baudRate = stuff->baudrate;
        xf86Info.sampleRate = stuff->samplerate;
        xf86Info.mouseFlags = stuff->flags;

	xf86Info.pPointer->on = FALSE;
#ifdef XQUEUE
	if (xf86Info.mseType != MTYPE_XQUEUE)
#endif
	xf86MouseInit();
	if (xf86Info.pPointer)
            (xf86Info.mseProc)(xf86Info.pPointer, DEVICE_ON);

    }

    return (client->noClientException);
}

static int
ProcXF86MiscSetKbdSettings(client)
    register ClientPtr client;
{
    REQUEST(xXF86MiscSetKbdSettingsReq);

    REQUEST_SIZE_MATCH(xXF86MiscSetKbdSettingsReq);

    if (stuff->rate < 0)
	return BadValue;
    if (stuff->delay < 0)
	return BadValue;
    if (stuff->kbdtype < KTYPE_84KEY || stuff->kbdtype > KTYPE_XQUEUE)
	return BadValue;

    if (xf86Info.kbdRate!=stuff->rate || xf86Info.kbdDelay!=stuff->delay) {
	char rad;

	xf86Info.kbdRate = stuff->rate;
	xf86Info.kbdDelay = stuff->delay;
        if      (xf86Info.kbdDelay <= 375) rad = 0x00;
        else if (xf86Info.kbdDelay <= 625) rad = 0x20;
        else if (xf86Info.kbdDelay <= 875) rad = 0x40;
        else                               rad = 0x60;
    
        if      (xf86Info.kbdRate <=  2)   rad |= 0x1F;
        else if (xf86Info.kbdRate >= 30)   rad |= 0x00;
        else                               rad |= ((58/xf86Info.kbdRate)-2);
    
        xf86SetKbdRepeat(rad);
    }
    /* Change other settings at your own risk - do not expect the
       following to work! */
    xf86Info.kbdType = stuff->kbdtype;
    xf86Info.serverNumLock = stuff->servnumlock!=0;

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
    case X_XF86MiscGetMouseSettings:
	return ProcXF86MiscGetMouseSettings(client);
    case X_XF86MiscGetKbdSettings:
	return ProcXF86MiscGetKbdSettings(client);
    case X_XF86MiscSetMouseSettings:
	return ProcXF86MiscSetMouseSettings(client);
    case X_XF86MiscSetKbdSettings:
	return ProcXF86MiscSetKbdSettings(client);
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
SProcXF86MiscGetMouseSettings(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscGetMouseSettingsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscGetMouseSettingsReq);
    return ProcXF86MiscGetMouseSettings(client);
}

static int
SProcXF86MiscGetKbdSettings(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscGetKbdSettingsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscGetKbdSettingsReq);
    return ProcXF86MiscGetKbdSettings(client);
}

static int
SProcXF86MiscSetMouseSettings(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscSetMouseSettingsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscSetMouseSettingsReq);
    swapl(&stuff->mousetype, n);
    swapl(&stuff->baudrate, n);
    swapl(&stuff->samplerate, n);
    swapl(&stuff->emulate3timeout, n);
    swapl(&stuff->flags, n);
    return ProcXF86MiscSetMouseSettings(client);
}

static int
SProcXF86MiscSetKbdSettings(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xXF86MiscSetKbdSettingsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXF86MiscSetKbdSettingsReq);
    swapl(&stuff->kbdtype, n);
    swapl(&stuff->rate, n);
    swapl(&stuff->delay, n);
    return ProcXF86MiscSetKbdSettings(client);
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
    case X_XF86MiscGetMouseSettings:
	return SProcXF86MiscGetMouseSettings(client);
    case X_XF86MiscGetKbdSettings:
	return SProcXF86MiscGetKbdSettings(client);
    case X_XF86MiscSetMouseSettings:
	return SProcXF86MiscSetMouseSettings(client);
    case X_XF86MiscSetKbdSettings:
	return SProcXF86MiscSetKbdSettings(client);
    default:
	return BadRequest;
    }
}
