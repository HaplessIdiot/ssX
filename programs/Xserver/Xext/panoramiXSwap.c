/* $TOG: panoramiXSwap.c /main/1 1997/10/29 13:26:35 kaleb $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/
#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#if 0
#include <sys/workstation.h>
#include <X11/Xserver/ws.h> 
#endif
#include "panoramiX.h"
#include "panoramiXproto.h"


/*
/*
 *	External references for data variables
 */

extern Bool noPanoramiXExtension;
extern Bool PanoramiXVisibilityNotifySent;
extern WindowPtr *WindowTable;
extern int defaultBackingStore;
extern char *ConnectionInfo;
extern int connBlockScreenStart;
extern int (* ProcVector[256]) ();

#if NeedFunctionPrototypes
#define PROC_EXTERN(pfunc)      extern int pfunc(ClientPtr)
#else
#define PROC_EXTERN(pfunc)      extern int pfunc()
#endif     

PROC_EXTERN(ProcPanoramiXQueryVersion);
PROC_EXTERN(ProcPanoramiXGetState);
PROC_EXTERN(ProcPanoramiXGetScreenCount);
PROC_EXTERN(PropPanoramiXGetScreenSize);

static int
#if NeedFunctionPrototypes      
SProcPanoramiXQueryVersion (ClientPtr client)
#else
SProcPanoramiXQueryVersion (client)
    register ClientPtr  client;
#endif
{
    register 	int	n;
    REQUEST(xPanoramiXQueryVersionReq);

    swaps(&stuff->length,n);
    REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
    return ProcPanoramiXQueryVersion(client);
}

static int
#if NeedFunctionPrototypes      
SProcPanoramiXGetState(ClientPtr client)
#else
SProcPanoramiXGetState(client)
        register ClientPtr      client;
#endif
{
	REQUEST(xPanoramiXGetStateReq);
	register int			n;

 	swaps (&stuff->length, n);	
	REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);

}

static int 
#if NeedFunctionPrototypes      
SProcPanoramiXGetScreenCount(ClientPtr client)
#else
SProcPanoramixGetScreenCount(client)
	register ClientPtr	client;
#endif
{
	REQUEST(xPanoramiXGetScreenCountReq);
	register int			n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
	return ProcPanoramiXGetScreenCount(client);
}

static int 
#if NeedFunctionPrototypes      
SProcPanoramiXGetScreenSize(ClientPtr client)
#else 
SProcPanoramiXGetScreenSize(client)
        register ClientPtr      client;
#endif 
{
	REQUEST(xPanoramiXGetScreenSizeReq);
    	WindowPtr			pWin;
	register int			n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
	return ProcPanoramiXGetScreenSize(client);
}

int
#if NeedFunctionPrototypes      
SProcPanoramiXDispatch (ClientPtr client)
#else
SProcPanoramiXDispatch (client) 
    ClientPtr   client;
#endif
{   REQUEST(xReq);
    switch (stuff->data)
    {
	case X_PanoramiXQueryVersion:
	     return SProcPanoramiXQueryVersion(client);
	case X_PanoramiXGetState:
	     return SProcPanoramiXGetState(client);
	case X_PanoramiXGetScreenCount:
	     return SProcPanoramiXGetScreenCount(client);
	case X_PanoramiXGetScreenSize:
	     return SProcPanoramiXGetScreenSize(client);
    return BadRequest;
    }
}
