/*
   Copyright (c) 1999 - The XFree86 Project Inc.

   Written by Mark Vojkovich
*/
/* $XFree86: xc/programs/Xserver/Xext/xf86dga2.c,v 1.1 1999/03/21 07:34:44 dawes Exp $ */


#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "xf86dgastr.h"
#include "swaprep.h"
#include "dgaproc.h"
#include "string.h"


static DISPATCH_PROC(ProcXDGADispatch);
static DISPATCH_PROC(SProcXDGADispatch);
static DISPATCH_PROC(ProcXDGAQueryVersion);
static DISPATCH_PROC(ProcXDGAQueryModes);
static DISPATCH_PROC(ProcXDGASetMode);
static DISPATCH_PROC(ProcXDGAOpenFramebuffer);
static DISPATCH_PROC(ProcXDGACloseFramebuffer);
static DISPATCH_PROC(ProcXDGASetViewport);
static DISPATCH_PROC(ProcXDGAInstallColormap);

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
ProcXDGAQueryVersion(ClientPtr client)
{
    xXDGAQueryVersionReply rep;

    REQUEST_SIZE_MATCH(xXDGAQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XDGA_MAJOR_VERSION;
    rep.minorVersion = XDGA_MINOR_VERSION;

    WriteToClient(client, sizeof(xXDGAQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}


static int
ProcXDGAOpenFramebuffer(ClientPtr client)
{
    REQUEST(xXDGAOpenFramebufferReq);
    xXDGAOpenFramebufferReply rep;
    char *deviceName;
    int nameSize;

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    if (!DGAAvailable(stuff->screen)) 
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    REQUEST_SIZE_MATCH(xXDGAOpenFramebufferReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if(!DGAOpenFramebuffer(stuff->screen, &deviceName, 
			(unsigned char**)(&rep.mem1),
			(int*)&rep.size, (int*)&rep.offset, (int*)&rep.extra)) 
    {
	return BadAlloc;
    }

    nameSize = deviceName ? (strlen(deviceName) + 1) : 0;
    rep.length = (nameSize + 3) >> 2;

    WriteToClient(client, sizeof(xXDGAOpenFramebufferReply), (char *)&rep);
    if(rep.length)
	WriteToClient(client, nameSize, deviceName);

    return (client->noClientException);
}


static int
ProcXDGACloseFramebuffer(ClientPtr client)
{
    REQUEST(xXDGACloseFramebufferReq);

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    if (!DGAAvailable(stuff->screen)) 
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    REQUEST_SIZE_MATCH(xXDGACloseFramebufferReq);

    DGACloseFramebuffer(stuff->screen);

    return (client->noClientException);
}

static int
ProcXDGAQueryModes(ClientPtr client)
{
    int i, num, size;
    REQUEST(xXDGAQueryModesReq);
    xXDGAQueryModesReply rep;
    xXDGAModeInfo info;
    XDGAModePtr mode;

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    REQUEST_SIZE_MATCH(xXDGAQueryModesReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.number = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen)) 
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if(!(num = DGAGetModes(stuff->screen))) {
	WriteToClient(client, sz_xXDGAQueryModesReply, (char*)&rep);
	return (client->noClientException);
    }

    if(!(mode = (XDGAModePtr)xalloc(num * sizeof(XDGAModeRec))))
	return BadAlloc;

    for(i = 0; i < num; i++)
	DGAGetModeInfo(stuff->screen, mode + i, i + 1);

    size = num * sz_xXDGAModeInfo;
    for(i = 0; i < num; i++)
	size += (strlen(mode[i].name) + 4) & ~3L;  /* plus NULL */

    rep.number = num;
    rep.length = size >> 2;

    WriteToClient(client, sz_xXDGAQueryModesReply, (char*)&rep);

    for(i = 0; i < num; i++) {
	size = strlen(mode[i].name) + 1;

	info.byte_order = mode[i].byteOrder;
	info.depth = mode[i].depth;
	info.num = mode[i].num;
	info.bpp = mode[i].bitsPerPixel;
	info.name_size = (size + 3) & ~3L;
	info.vsync_num = mode[i].VSync_num;
	info.vsync_den = mode[i].VSync_den;
	info.flags = mode[i].flags;
	info.image_width = mode[i].imageWidth;
	info.image_height = mode[i].imageHeight;
	info.pixmap_width = mode[i].pixmapWidth;
	info.pixmap_height = mode[i].pixmapHeight;
	info.bytes_per_scanline = mode[i].bytesPerScanline;
	info.red_mask = mode[i].red_mask;
	info.green_mask = mode[i].green_mask;
	info.blue_mask = mode[i].blue_mask;
	info.viewport_width = mode[i].viewportWidth;
	info.viewport_height = mode[i].viewportHeight;
	info.viewport_xstep = mode[i].xViewportStep;
	info.viewport_ystep = mode[i].yViewportStep;
 	info.viewport_xmax = mode[i].maxViewportX;
	info.viewport_ymax = mode[i].maxViewportY;
	info.viewport_flags = mode[i].viewportFlags;
	info.reserved1 = mode[i].reserved1;
	info.reserved2 = mode[i].reserved2;
	
	WriteToClient(client, sz_xXDGAModeInfo, (char*)(&info));
	WriteToClient(client, size, mode[i].name);
    }

    xfree(mode);

    return (client->noClientException);
}


static int
ProcXDGASetMode(ClientPtr client)
{
    REQUEST(xXDGASetModeReq);
    xXDGASetModeReply rep;
    XDGAModeRec mode;
    xXDGAModeInfo info;
    PixmapPtr pPix;
    int size;

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    REQUEST_SIZE_MATCH(xXDGASetModeReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.offset = 0;
    rep.flags = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen)) 
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if(!stuff->mode) {
	DGASetMode(stuff->screen, 0, &mode, &pPix);
	WriteToClient(client, sz_xXDGASetModeReply, (char*)&rep);
	return (client->noClientException);
    } 

    if(Success != DGASetMode(stuff->screen, stuff->mode, &mode, &pPix))
	return BadValue;

    if(pPix) {
	if(AddResource(stuff->pid, RT_PIXMAP, (pointer)(pPix))) {
	    pPix->drawable.id = (int)stuff->pid;
	    rep.flags = DGA_PIXMAP_AVAILABLE;
	}
    }

    size = strlen(mode.name) + 1;
   
    info.byte_order = mode.byteOrder;
    info.depth = mode.depth;
    info.num = mode.num;
    info.bpp = mode.bitsPerPixel;
    info.name_size = (size + 3) & ~3L;
    info.vsync_num = mode.VSync_num;
    info.vsync_den = mode.VSync_den;
    info.flags = mode.flags;
    info.image_width = mode.imageWidth;
    info.image_height = mode.imageHeight;
    info.pixmap_width = mode.pixmapWidth;
    info.pixmap_height = mode.pixmapHeight;
    info.bytes_per_scanline = mode.bytesPerScanline;
    info.red_mask = mode.red_mask;
    info.green_mask = mode.green_mask;
    info.blue_mask = mode.blue_mask;
    info.viewport_width = mode.viewportWidth;
    info.viewport_height = mode.viewportHeight;
    info.viewport_xstep = mode.xViewportStep;
    info.viewport_ystep = mode.yViewportStep;
    info.viewport_xmax = mode.maxViewportX;
    info.viewport_ymax = mode.maxViewportY;
    info.viewport_flags = mode.viewportFlags;
    info.reserved1 = mode.reserved1;
    info.reserved2 = mode.reserved2;

    rep.length = (sz_xXDGAModeInfo + info.name_size) >> 2;

    WriteToClient(client, sz_xXDGASetModeReply, (char*)&rep);
    WriteToClient(client, sz_xXDGAModeInfo, (char*)(&info));
    WriteToClient(client, size, mode.name);

    return (client->noClientException);
}

static int
ProcXDGASetViewport(ClientPtr client)
{
    REQUEST(xXDGASetViewportReq);

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    if (!DGAActive(stuff->screen)) 
        return DGAErrorBase + XF86DGADirectNotActivated;

    REQUEST_SIZE_MATCH(xXDGASetViewportReq);

    DGASetViewport(stuff->screen, stuff->x, stuff->y, stuff->flags);

    return (client->noClientException);
}

static int
ProcXDGAInstallColormap(ClientPtr client)
{
    ColormapPtr cmap;
    REQUEST(xXDGAInstallColormapReq);

    if (stuff->screen > screenInfo.numScreens)
        return BadValue;

    if (!DGAActive(stuff->screen)) 
        return DGAErrorBase + XF86DGADirectNotActivated;

    REQUEST_SIZE_MATCH(xXDGAInstallColormapReq);
   
    cmap = (ColormapPtr)LookupIDByType(stuff->cmap, RT_COLORMAP);
    if (cmap) {
        DGAInstallColormap(cmap);
        return (client->noClientException);
    } else {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }

    return (client->noClientException);
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
#if 1
    if( (stuff->data <= X_XF86DGAViewPortChanged) && 
	(stuff->data >= X_XF86DGAGetVideoLL)) 
	return ProcXF86DGADispatch(client);
#endif

    switch (stuff->data){
    case X_XDGAQueryVersion:
	return ProcXDGAQueryVersion(client);
    case X_XDGAQueryModes:
	return ProcXDGAQueryModes(client);
    case X_XDGASetMode:
	return ProcXDGASetMode(client);
    case X_XDGAOpenFramebuffer:
	return ProcXDGAOpenFramebuffer(client);
    case X_XDGACloseFramebuffer:
	return ProcXDGACloseFramebuffer(client);
    case X_XDGASetViewport:
	return ProcXDGASetViewport(client);
    case X_XDGAInstallColormap:
	return ProcXDGAInstallColormap(client);
    default:
	return BadRequest;
    }
}
