/*
   Copyright (c) 1999 - The XFree86 Project Inc.

   Written by Mark Vojkovich
*/
/* $XFree86$ */


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


static DISPATCH_PROC(ProcXDGADispatch);
static DISPATCH_PROC(SProcXDGADispatch);

extern DISPATCH_PROC(ProcXF86DGADispatch);


static void XDGAResetProc(ExtensionEntry *extEntry);

unsigned char DGAReqCode = 0;
int DGAErrorBase;

void
XFree86DGAExtensionInit(void)
{
    ExtensionEntry* extEntry;

    if ((extEntry = AddExtension(XF86DGANAME,
				XF86DGANumberEvents,
				XF86DGANumberErrors,
				ProcXDGADispatch,
				SProcXDGADispatch,
				XDGAResetProc,
				StandardMinorOpcode))) {
	DGAReqCode = (unsigned char)extEntry->base;
	DGAErrorBase = extEntry->errorBase;
    }
}



static void
XDGAResetProc (ExtensionEntry *extEntry)
{
}

static int
SProcXDGADispatch (ClientPtr client)
{
   return DGAErrorBase + XF86DGAClientNotLocal;
}

static int
ProcXDGADispatch (ClientPtr client)
{
    REQUEST(xReq);

    if (!LocalClient(client))
	return DGAErrorBase + XF86DGAClientNotLocal;

    /* divert old protocol */
    if(stuff->data <= X_XF86DGAViewPortChanged)
	return ProcXF86DGADispatch(client);

    switch (stuff->data){
    default:
	return BadRequest;
    }
}
