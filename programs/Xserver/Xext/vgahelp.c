/* $XFree86$ */

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
#define _XVGAHELP_SERVER_
#include "VGAHelpstr.h"
#include "Xfuncproto.h"
#include "../hw/xfree86/common/xf86.h"

extern int xf86ScreenIndex;

static int ProcVGAHelpDispatch(), SProcVGAHelpDispatch();
static void VGAHelpResetProc();

static unsigned char VGAHelpReqCode;

void
VGAHelpExtensionInit()
{
    ExtensionEntry* extEntry;

    if (extEntry = AddExtension(VGAHELPNAME,
				VGAHelpNumberEvents,
				VGAHelpNumberErrors,
				ProcVGAHelpDispatch,
				SProcVGAHelpDispatch,
				VGAHelpResetProc,
				StandardMinorOpcode)) {
	VGAHelpReqCode = (unsigned char)extEntry->base;
    }
}

/*ARGSUSED*/
static void
VGAHelpResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

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
    rep.majorVersion = VGAHELP_MAJOR_VERSION;
    rep.minorVersion = VGAHELP_MINOR_VERSION;
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

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    REQUEST_SIZE_MATCH(xVGAHelpGetModeLineReq);
    rep.type = X_Reply;
    rep.length = 0;
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
    }
    WriteToClient(client, sizeof(xVGAHelpGetModeLineReply), (char *)&rep);
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

    if (stuff->screen > screenInfo.numScreens)
	return BadValue;

    vptr = (ScrnInfoPtr) screenInfo.screens[stuff->screen]->devPrivates[xf86ScreenIndex].ptr;
    mptr = vptr->modes;

    REQUEST_SIZE_MATCH(xVGAHelpModModeLineReq);

    if (stuff->hsyncstart < stuff->hdisplay   ||
	stuff->hsyncend   < stuff->hsyncstart ||
	stuff->htotal     < stuff->hsyncend   ||
	stuff->vsyncstart < stuff->vdisplay   ||
	stuff->vsyncend   < stuff->vsyncstart ||
	stuff->vtotal     < stuff->vsyncend)
	return BadValue;

    mptr->HDisplay   = stuff->hdisplay;
    mptr->HSyncStart = stuff->hsyncstart;
    mptr->HSyncEnd   = stuff->hsyncend;
    mptr->HTotal     = stuff->htotal;
    mptr->VDisplay   = stuff->vdisplay;
    mptr->VSyncStart = stuff->vsyncstart;
    mptr->VSyncEnd   = stuff->vsyncend;
    mptr->VTotal     = stuff->vtotal;
    /* Should call ValidMode */
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
    mptr->Flags      = stuff->flags;
    if (mptr->Flags & V_DBLSCAN)
    {
	mptr->CrtcVDisplay *= 2;
	mptr->CrtcVSyncStart *= 2;
	mptr->CrtcVSyncEnd *= 2;
	mptr->CrtcVTotal *= 2;
	mptr->CrtcVAdjusted = TRUE;
    }

    (vptr->SwitchMode)(mptr);

    return(client->noClientException);
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
SProcVGAHelpModModeLine(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xVGAHelpModModeLineReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xVGAHelpModModeLineReq);
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
    return ProcVGAHelpModModeLine(client);
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
    default:
	return BadRequest;
    }
}
