/* $TOG: panoramiXprocs.c /main/9 1998/03/17 06:51:10 kaleb $ */
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
/* $XFree86: xc/programs/Xserver/Xext/panoramiXprocs.c,v 3.14 1999/09/06 11:27:17 dawes Exp $ */

#include <stdio.h>
#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "windowstr.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "inputstr.h"
#include "migc.h"
#include "misc.h"
#include "dixstruct.h"
#include "panoramiX.h"
#include "panoramiXsrv.h"

#define XINERAMA_IMAGE_BUFSIZE (256*1024)

extern ScreenInfo *GlobalScrInfo;
extern int (* SavedProcVector[256])();
extern void (* ReplySwapVector[256])();
extern WindowPtr *WindowTable;
extern char *ConnectionInfo;
extern int connBlockScreenStart;
extern XID PanoramiXVisualTable[256][MAXSCREENS];


extern XID clientErrorValue;

extern void Swap32Write();

extern int connBlockScreenStart;

extern int (* InitialVector[3]) ();
extern int (* ProcVector[256]) ();
extern int (* SwappedProcVector[256]) ();
extern void (* EventSwapVector[128]) ();
extern void (* ReplySwapVector[256]) ();
extern void Swap32Write(), SLHostsExtend(), SQColorsExtend(), 
WriteSConnectionInfo();
extern void WriteSConnSetupPrefix();
extern char *ClientAuthorized();
extern Bool InsertFakeRequest();
static void KillAllClients();
static void DeleteClientFromAnySelections();
extern void ProcessWorkQueue();

extern char isItTimeToYield;

/* Various of the DIX function interfaces were not designed to allow
 * the client->errorValue to be set on BadValue and other errors.
 * Rather than changing interfaces and breaking untold code we introduce
 * a new global that dispatch can use.
 */
extern XID clientErrorValue;   /* XXX this is a kludge */


#define SAME_SCREENS(a, b) (\
    (a.pScreen == b.pScreen))



extern int Ones();

int PanoramiXCreateWindow(register ClientPtr client)
{
    register WindowPtr 	pParent;
    REQUEST(xCreateWindowReq);
    int 		result, j = 0;
    unsigned 		len;
    Window 		winID;
    Window 		parID;
    PanoramiXWindow 	*localWin;
    PanoramiXWindow 	*parentWin = PanoramiXWinRoot;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXCmap 	*pPanoramiXCmap = NULL;
    PanoramiXPmap         *pBackgndPmap = NULL;
    PanoramiXPmap         *pBorderPmap = NULL;
    VisualID            orig_visual;
    XID			orig_wid;
    int			orig_x, orig_y;
    register Mask	orig_mask;
    int 		cmap_offset = 0;
    int                 pback_offset = 0;
    int                 pbord_offset = 0;


    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);
    
    len = client->req_len - (sizeof(xCreateWindowReq) >> 2);
    IF_RETURN((Ones((Mask)stuff->mask) != len), BadLength);
    orig_mask = stuff->mask;
    PANORAMIXFIND_ID(parentWin, stuff->parent);
    if (parentWin) {
	localWin = (PanoramiXWindow *)Xcalloc(sizeof(PanoramiXWindow));
	IF_RETURN(!localWin, BadAlloc);
    } else {
	return BadWindow;
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWBackPixmap)) {
	XID pmapID;

	pback_offset = Ones((Mask)stuff->mask & (CWBackPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pback_offset);
	if (pmapID) {
	    pBackgndPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBackgndPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWBorderPixmap)) {
	XID pmapID;

	pbord_offset = Ones((Mask)stuff->mask & (CWBorderPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pbord_offset);
	if (pmapID) {
	    pBorderPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBorderPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWColormap)) {
	Colormap cmapID;

	cmap_offset = Ones((Mask)stuff->mask & (CWColormap - 1));
	cmapID = *((CARD32 *) &stuff[1] + cmap_offset);
	if (cmapID) {
	    pPanoramiXCmap = PanoramiXCmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXCmap, cmapID);
	}
    }
    orig_x = stuff->x;
    orig_y = stuff->y;
    orig_wid = stuff->wid;
    orig_visual = stuff->visual;
    for (j = 0; j <= PanoramiXNumScreens - 1; j++) {
	winID = j ? FakeClientID(client->index) : orig_wid;
	localWin->info[j].id = winID;
    }
    localWin->FreeMe = FALSE;
    localWin->visibility = VisibilityNotViewable;
    localWin->VisibilitySent = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXWin, PanoramiXWinRoot);
    pPanoramiXWin->next = localWin;
   
    if (stuff->class == InputOnly) {
	/* Kludge.  DEC forgot about InputOnly windows */
	stuff->visual = orig_visual = 0;
    }

    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	if (parentWin == PanoramiXWinRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	stuff->wid = localWin->info[j].id;
	parID = (XID)(parentWin->info[j].id);
	pParent = (WindowPtr)SecurityLookupWindow(parID, client,SecurityReadAccess);
	IF_RETURN((!pParent),BadWindow);
	stuff->parent = parID;
	if ( orig_visual != CopyFromParent ) 
	    stuff->visual = PanoramiXVisualTable[orig_visual][j];
	if (pBackgndPmap)
	    *((CARD32 *) &stuff[1] + pback_offset) = pBackgndPmap->info[j].id;
	if (pBorderPmap)
	    *((CARD32 *) &stuff[1] + pbord_offset) = pBorderPmap->info[j].id;
	if (pPanoramiXCmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = pPanoramiXCmap->info[j].id;
	stuff->mask = orig_mask;
	result = (*SavedProcVector[X_CreateWindow])(client);
	BREAK_IF(result != Success); 
    }
    if (result != Success) {
       pPanoramiXWin->next = NULL;
       if (localWin)
           Xfree(localWin);
    }
    return (result);
}



int PanoramiXChangeWindowAttributes(register ClientPtr client)
{
    REQUEST(xChangeWindowAttributesReq);
    register int  	result;
    int 	  	len;
    int 	  	j;
    Window 	  	winID;
    Mask		orig_valueMask;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;
    PanoramiXCmap 	*pPanoramiXCmap = NULL;
    PanoramiXPmap	*pBackgndPmap = NULL;
    PanoramiXPmap	*pBorderPmap = NULL;
    int			cmap_offset = 0;
    int 		pback_offset = 0;
    int			pbord_offset = 0;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    len = client->req_len - (sizeof(xChangeWindowAttributesReq) >> 2);
    IF_RETURN((len != Ones((Mask) stuff->valueMask)), BadLength);
    orig_valueMask = stuff->valueMask;
    winID = stuff->window;
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->window);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, winID);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWBackPixmap)) {
	XID pmapID;

	pback_offset = Ones((Mask)stuff->valueMask & (CWBackPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pback_offset);
	if (pmapID) {
	    pBackgndPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBackgndPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWBorderPixmap)) {
	XID pmapID;

	pbord_offset = Ones((Mask)stuff->valueMask & (CWBorderPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pbord_offset);
	if (pmapID) {
	    pBorderPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBorderPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWColormap )) {
	Colormap cmapID;

	cmap_offset = Ones((Mask)stuff->valueMask & (CWColormap - 1));
	cmapID = *((CARD32 *) &stuff[1] + cmap_offset);
	if (cmapID) {
	    pPanoramiXCmap = PanoramiXCmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXCmap, cmapID);
	}
    }
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	stuff->valueMask = orig_valueMask;  
	if (pBackgndPmap)
	    *((CARD32 *) &stuff[1] + pback_offset) = pBackgndPmap->info[j].id;
	if (pBorderPmap)
	    *((CARD32 *) &stuff[1] + pbord_offset) = pBorderPmap->info[j].id;
	if (pPanoramiXCmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = pPanoramiXCmap->info[j].id;
	result = (*SavedProcVector[X_ChangeWindowAttributes])(client);
        BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    } 
    return (result);
}


int PanoramiXDestroyWindow(ClientPtr client)
{
    REQUEST(xResourceReq);
    int         	j, result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->id);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    IF_RETURN(!pPanoramiXWin,BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DestroyWindow])(client);
        BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    } 
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXDestroySubwindows(ClientPtr client)
{
    REQUEST(xResourceReq);
    int         	j,result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->id);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DestroySubwindows])(client);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXChangeSaveSet(ClientPtr client)
{
    REQUEST(xChangeSaveSetReq);
    int         	j, result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;

    REQUEST_SIZE_MATCH(xChangeSaveSetReq);
    if (!stuff->window) 
	result = (* SavedProcVector[X_ChangeSaveSet])(client);
    else {
      PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
      IF_RETURN(!pPanoramiXWin, BadWindow);
      FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_ChangeSaveSet])(client);
      }
    }
    return (result);
}


int PanoramiXReparentWindow(register ClientPtr client)
{
    REQUEST(xReparentWindowReq);
    register int 	result;
    int 		j;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow 	*pPanoramiXPar = PanoramiXWinRoot;
    int x = stuff->x;
    int y = stuff->y;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PANORAMIXFIND_ID(pPanoramiXPar, stuff->parent);
    IF_RETURN(!pPanoramiXPar, BadWindow);
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXPar), j) {
	stuff->window = pPanoramiXWin->info[j].id;
	stuff->parent = pPanoramiXPar->info[j].id;
	if(pPanoramiXPar == PanoramiXWinRoot) {
	    stuff->x = x - panoramiXdataPtr[j].x;
	    stuff->y = y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ReparentWindow])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXMapWindow(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j,result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;
    Window		winID;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility */
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client, 
SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	  pPanoramiXWin->VisibilitySent = FALSE;
    }
    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	 winID = pPanoramiXWin->info[j].id;
         pWin = (WindowPtr) SecurityLookupWindow(winID, 
client,SecurityReadAccess);
	 IF_RETURN((!pWin), BadWindow);
	 stuff->id = winID;
	 result = (*SavedProcVector[X_MapWindow])(client);
    }
    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    return (result);
}


int PanoramiXMapSubwindows(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j,result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_MapSubwindows])(client);
    }
    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXUnmapWindow(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j, result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;
   
    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	 stuff->id = pPanoramiXWin->info[j].id;
	 result = (*SavedProcVector[X_UnmapWindow])(client);
    }

    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (client->noClientException);
}


int PanoramiXUnmapSubwindows(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j, result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_UnmapSubwindows])(client);
    }

    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (client->noClientException);
}


int PanoramiXConfigureWindow(register ClientPtr client)
{
    register WindowPtr	pWin;
    REQUEST(xConfigureWindowReq);
    register int 	result;
    unsigned 		len, i, things;
    register Mask	orig_mask;
    int 		j, sib_position;
    Window 		winID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow	*pPanoramiXSib = NULL;
    int 		x_off = 0, y_off = 0;
    XID 		*pStuff;
    XID 		*origStuff, *modStuff;
    Mask 		local_mask;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);
    len = client->req_len - (sizeof(xConfigureWindowReq) >> 2);
    things = Ones((Mask)stuff->mask);
    IF_RETURN((things != len), BadLength);
    orig_mask = stuff->mask;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    if (!pPanoramiXWin) {
	client->errorValue = stuff->window;
	return (BadWindow);
    }
    if (things > 0) {
        pStuff = (XID *) ALLOCATE_LOCAL(things * sizeof(XID));
        memcpy((char *) pStuff, (char *) &stuff[1], things * sizeof(XID));
        local_mask = (CWSibling | CWX | CWY) & ((Mask) stuff->mask);
        if (local_mask & CWSibling) {
       	   sib_position = Ones((Mask) stuff->mask & (CWSibling - 1));
    	   pPanoramiXSib = PanoramiXWinRoot;
	   PANORAMIXFIND_ID(pPanoramiXSib, *(pStuff + sib_position));
        }
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	    winID = pPanoramiXWin->info[j].id;
	    pWin = (WindowPtr)SecurityLookupWindow(winID, client,SecurityReadAccess);
	    if (!pWin) {
		client->errorValue = pPanoramiXWin->info[0].id;
		return (BadWindow);
	    }
	    stuff->window = winID;
	    if (pWin->parent 
		&& (pWin->parent->drawable.id == PanoramiXWinRoot->info[j].id)) {
	        x_off = panoramiXdataPtr[j].x;
	        y_off = panoramiXdataPtr[j].y;
	    }
	    modStuff = (XID *) &stuff[1];
	    origStuff = pStuff;
	    i = things;
	    if (local_mask & CWX) {
	        *modStuff++ = *origStuff++ - x_off;
	        i--;
	    }
	    if (local_mask & CWY) {
	       *modStuff++ = *origStuff++ - y_off;
	       i--;
	    }
	    for ( ; i; i--) 
	       *modStuff++ = *origStuff++;
	    if (pPanoramiXSib)
	       *((XID *) &stuff[1] + sib_position) = pPanoramiXSib->info[j].id;
	    stuff->mask = orig_mask;
	    result = (*SavedProcVector[X_ConfigureWindow])(client);
        }
    DEALLOCATE_LOCAL(pStuff);
    PANORAMIX_FREE(client);
    return (result);
    } else
	return (client->noClientException);
}


int PanoramiXCirculateWindow(register ClientPtr client)
{
    REQUEST(xCirculateWindowReq);
    int 	  j,result;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;


    REQUEST_SIZE_MATCH(xCirculateWindowReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_CirculateWindow])(client);
    }
    return (result);
}


int PanoramiXGetGeometry(register ClientPtr client)
{
    xGetGeometryReply 	 rep;
    register DrawablePtr pDraw;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    VERIFY_GEOMETRABLE (pDraw, stuff->id, client);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.root = WindowTable[pDraw->pScreen->myNum]->drawable.id;
    rep.depth = pDraw->depth;

    if (stuff->id == PanoramiXWinRoot->info[0].id) {
	xWindowRoot *root  = (xWindowRoot *)
				    (ConnectionInfo + connBlockScreenStart);

	rep.width = root->pixWidth;
	rep.height = root->pixHeight;
    } else {
	rep.width = pDraw->width;
	rep.height = pDraw->height;
    }

    /* XXX - Because the pixmap-implementation of the multibuffer extension 
     *       may have the buffer-id's drawable resource value be a pointer
     *       to the buffer's window instead of the buffer itself
     *       (this happens if the buffer is the displayed buffer),
     *       we also have to check that the id matches before we can
     *       truly say that it is a DRAWABLE_WINDOW.
     */

    if ((pDraw->type == UNDRAWABLE_WINDOW) ||
        ((pDraw->type == DRAWABLE_WINDOW) && (stuff->id == pDraw->id))) {
        register WindowPtr pWin = (WindowPtr)pDraw;
	rep.x = pWin->origin.x - wBorderWidth (pWin);
	rep.y = pWin->origin.y - wBorderWidth (pWin);
	rep.borderWidth = pWin->borderWidth;
    } else { 			/* DRAWABLE_PIXMAP or DRAWABLE_BUFFER */
	rep.x = rep.y = rep.borderWidth = 0;
    }
    WriteReplyToClient(client, sizeof(xGetGeometryReply), &rep);
    return (client->noClientException);
}


int PanoramiXChangeProperty(ClientPtr client)
{	      
    int  	  result, j;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xChangePropertyReq);


    REQUEST_AT_LEAST_SIZE(xChangePropertyReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_ChangeProperty])(client);
	if (result != Success) {
	    stuff->window = pPanoramiXWin->info[0].id;
	    break;
        }
    }
    return (result);
}


int PanoramiXDeleteProperty(ClientPtr client)
{	      
    int  	  result, j;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xDeletePropertyReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DeleteProperty])(client);
	BREAK_IF(result != Success);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXSendEvent(ClientPtr client)
{	      
    int  	  result, j; 
    BYTE	  orig_type;
    Mask	  orig_eventMask;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xSendEventReq);

    REQUEST_SIZE_MATCH(xSendEventReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->destination);
    orig_type = stuff->event.u.u.type;
    orig_eventMask = stuff->eventMask;
    if (!pPanoramiXWin) {
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_SendEvent])(client);
	noPanoramiXExtension = FALSE;
    }
    else {
       noPanoramiXExtension = FALSE;
       FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	 stuff->destination = pPanoramiXWin->info[j].id;
	 stuff->eventMask = orig_eventMask;
	 stuff->event.u.u.type = orig_type;
	 if (!j) 
	     noPanoramiXExtension = TRUE;
	 result = (* SavedProcVector[X_SendEvent])(client);
	 noPanoramiXExtension = FALSE;
       }
    }
    return (result);
}


int PanoramiXCreatePixmap(register ClientPtr client)
{
    register DrawablePtr pDraw;
    REQUEST(xCreatePixmapReq);
    int 		 result, j;
    Pixmap 		 pmapID;
    PanoramiXWindow 	 *pPanoramiXWin;
    PanoramiXPmap 	 *pPanoramiXPmap;
    PanoramiXPmap	 *localPmap;
    XID			 orig_pid;


    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    client->errorValue = stuff->pid;

    localPmap =(PanoramiXPmap *) Xcalloc(sizeof(PanoramiXPmap));
    IF_RETURN(!localPmap, BadAlloc);

    pDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE, 
						  SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);

    pPanoramiXWin = (pDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadWindow);

    orig_pid = stuff->pid;
    FOR_NSCREENS_OR_ONCE(pPanoramiXPmap, j) {
	pmapID = j ? FakeClientID(client->index) : orig_pid;
	localPmap->info[j].id = pmapID;
    }
    localPmap->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXPmap, PanoramiXPmapRoot);
    pPanoramiXPmap->next = localPmap;
   
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->pid = localPmap->info[j].id;
	stuff->drawable = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_CreatePixmap])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
	pPanoramiXPmap->next = NULL;
	if (localPmap)
	    Xfree(localPmap);
    }
    return (result);
}


int PanoramiXFreePixmap(ClientPtr client)
{
    int         result, j;
    PanoramiXPmap *pPanoramiXPmap = PanoramiXPmapRoot;
    PanoramiXPmap *pPanoramiXPmapback = NULL;
    REQUEST(xResourceReq);


    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXPmap && (pPanoramiXPmap->info[0].id != stuff->id);
					    pPanoramiXPmap = pPanoramiXPmap->next)
	pPanoramiXPmapback = pPanoramiXPmap;
    if (!pPanoramiXPmap)
	result = (* SavedProcVector[X_FreePixmap])(client);
    else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	    stuff->id = pPanoramiXPmap->info[j].id;
	    result = (* SavedProcVector[X_FreePixmap])(client);
        }
        if ((result == Success) && pPanoramiXPmapback &&
		 pPanoramiXPmap && pPanoramiXPmap->FreeMe ) {
    	    pPanoramiXPmapback->next = pPanoramiXPmap->next;
	    Xfree(pPanoramiXPmap);
        }
    }
    return (result);
}


int PanoramiXCreateGC(register ClientPtr client)
{
    int 	       result, j;
    DrawablePtr        pDraw;
    unsigned 	       len;
    REQUEST(xCreateGCReq);
    GContext 	       GCID;
    PanoramiXWindow   *pPanoramiXWin;
    PanoramiXGC	      *localGC;
    PanoramiXGC       *pPanoramiXGC;
    PanoramiXPmap     *pPanoramiXTile = NULL, *pPanoramiXStip = NULL;
    PanoramiXPmap     *pPanoramiXClip = NULL;
    int		       tile_offset, stip_offset, clip_offset;
    XID		       orig_GC;

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    client->errorValue = stuff->gc;
    pDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						  SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXWin = (pDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);

    len = client->req_len -  (sizeof(xCreateGCReq) >> 2);
    IF_RETURN((len != Ones((Mask)stuff->mask)), BadLength);
    localGC = (PanoramiXGC *) Xcalloc(sizeof(PanoramiXGC));
    IF_RETURN(!localGC, BadAlloc);
    orig_GC = stuff->gc;
    if ((Mask)stuff->mask & GCTile) {
	XID tileID;

	tile_offset = Ones((Mask)stuff->mask & (GCTile - 1));
	tileID = *((CARD32 *) &stuff[1] + tile_offset);
	if (tileID) {
	    pPanoramiXTile = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXTile, tileID);
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	XID stipID;

	stip_offset = Ones((Mask)stuff->mask & (GCStipple - 1));
	stipID = *((CARD32 *) &stuff[1] + stip_offset);
	if (stipID) {
	    pPanoramiXStip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXStip, stipID);
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	XID clipID;

	clip_offset = Ones((Mask)stuff->mask & (GCClipMask - 1));
	clipID = *((CARD32 *) &stuff[1] + clip_offset);
	if (clipID) {
	    pPanoramiXClip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXClip, clipID);
	}
    }
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	GCID = j ? FakeClientID(client->index) : orig_GC;
	localGC->info[j].id = GCID;
    }
    localGC->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXGC, PanoramiXGCRoot);
    pPanoramiXGC->next = localGC;
   
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->gc = localGC->info[j].id;
	stuff->drawable = pPanoramiXWin->info[j].id;
	if (pPanoramiXTile)
	    *((CARD32 *) &stuff[1] + tile_offset) = pPanoramiXTile->info[j].id;
	if (pPanoramiXStip)
	    *((CARD32 *) &stuff[1] + stip_offset) = pPanoramiXStip->info[j].id;
	if (pPanoramiXClip)
	    *((CARD32 *) &stuff[1] + clip_offset) = pPanoramiXClip->info[j].id;
	result = (* SavedProcVector[X_CreateGC])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
       pPanoramiXGC->next = NULL;
       Xfree(localGC);
    }
    return (result);
}

int PanoramiXChangeGC(ClientPtr client)
{
    GC 		*pGC;
    REQUEST(xChangeGCReq);
    int 	result, j;
    unsigned 	len;
    PanoramiXGC   *pPanoramiXGC = PanoramiXGCRoot;
    PanoramiXPmap	*pPanoramiXTile = NULL, *pPanoramiXStip = NULL;
    PanoramiXPmap	*pPanoramiXClip = NULL;
    int		tile_offset, stip_offset, clip_offset;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xChangeGCReq);
    VERIFY_GC(pGC, stuff->gc, client);
    len = client->req_len -  (sizeof(xChangeGCReq) >> 2);
    IF_RETURN((len != Ones((Mask)stuff->mask)), BadLength);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    if ((Mask)stuff->mask & GCTile) {
	XID tileID;

	tile_offset = Ones((Mask)stuff->mask & (GCTile -1) );
	tileID = *((CARD32 *) &stuff[1] + tile_offset);
	if (tileID) {
	    pPanoramiXTile = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXTile, tileID);
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	XID stipID;

	stip_offset = Ones((Mask)stuff->mask & (GCStipple -1 ));
	stipID = *((CARD32 *) &stuff[1] + stip_offset);
	if (stipID) {
	    pPanoramiXStip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXStip, stipID);
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	XID clipID;

	clip_offset = Ones((Mask)stuff->mask & (GCClipMask -1));
	clipID = *((CARD32 *) &stuff[1] + clip_offset);
	if (clipID) {
	    pPanoramiXClip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXClip, clipID);
	}
    }
   FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXTile)
	    *((CARD32 *) &stuff[1] + tile_offset) = pPanoramiXTile->info[j].id;
	if (pPanoramiXStip)
	    *((CARD32 *) &stuff[1] + stip_offset) = pPanoramiXStip->info[j].id;
	if (pPanoramiXClip)
	    *((CARD32 *) &stuff[1] + clip_offset) = pPanoramiXClip->info[j].id;
	result = (* SavedProcVector[X_ChangeGC])(client);
	BREAK_IF(result != Success);
    }
   if (pPanoramiXTile || pPanoramiXStip || pPanoramiXClip)
     {
	PANORAMIX_FREE(client);
     }
    return (result);
}


int PanoramiXCopyGC(ClientPtr client)
{
    int 	j, result;
    PanoramiXGC	*pPanoramiXGCSrc = PanoramiXGCRoot;
    PanoramiXGC	*pPanoramiXGCDst = PanoramiXGCRoot;
    REQUEST(xCopyGCReq);

    REQUEST_SIZE_MATCH(xCopyGCReq);
    PANORAMIXFIND_ID(pPanoramiXGCSrc, stuff->srcGC);
    IF_RETURN(!pPanoramiXGCSrc, BadGC);
    PANORAMIXFIND_ID(pPanoramiXGCDst, stuff->dstGC);
    IF_RETURN(!pPanoramiXGCDst, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGCDst, j) {
	stuff->srcGC = pPanoramiXGCSrc->info[j].id;
	stuff->dstGC = pPanoramiXGCDst->info[j].id;
	result = (* SavedProcVector[X_CopyGC])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXSetDashes(ClientPtr client)
{
    GC 		*pGC;
    REQUEST(xSetDashesReq);
    int 	result, j;
    PanoramiXGC   *pPanoramiXGC = PanoramiXGCRoot;

    REQUEST_FIXED_SIZE(xSetDashesReq, stuff->nDashes);
    VERIFY_GC(pGC, stuff->gc, client);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_SetDashes])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXSetClipRectangles(register ClientPtr client)
{
    int 	result;
    REQUEST(xSetClipRectanglesReq);
    int		j;
    PanoramiXGC	*pPanoramiXGC = PanoramiXGCRoot;

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_SetClipRectangles])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXFreeGC(ClientPtr client)
{
    REQUEST(xResourceReq);
    int		result, j;
    PanoramiXGC	*pPanoramiXGC = PanoramiXGCRoot;
    PanoramiXGC	*pPanoramiXGCback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXGC && (pPanoramiXGC->info[0].id != stuff->id); 
						pPanoramiXGC = pPanoramiXGC->next)
	pPanoramiXGCback = pPanoramiXGC;
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->id = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_FreeGC])(client);
	BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXGCback &&
	         pPanoramiXGC && pPanoramiXGC->FreeMe) {
	pPanoramiXGCback->next = pPanoramiXGC->next;
	if (pPanoramiXGC)
	   Xfree(pPanoramiXGC);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXClearToBackground(register ClientPtr client)
{
    REQUEST(xClearAreaReq);
    register WindowPtr  pWin;
    int 		result, j;
    Window 		winID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    int 		orig_x, orig_y;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xClearAreaReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	winID = pPanoramiXWin->info[j].id;
	pWin = (WindowPtr) SecurityLookupWindow(winID, client, SecurityReadAccess);
	if (!pWin) {
	    client->errorValue = pPanoramiXWin->info[0].id;
	    return (BadWindow);
	}
	stuff->window = winID;
	if (pWin->drawable.id == PanoramiXWinRoot->info[j].id) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ClearArea])(client);
    }
    PANORAMIX_FREE(client);
    return (result);
}


/* 
    For Window to Pixmap copies you're screwed since each screen's
    pixmap will look like what it sees on its screen.  Unless the
    screens overlap and the window lies on each, the two copies
    will be out of sync.  To remedy this we do a GetImage and PutImage
    in place of the copy.  Doing this as a single Image isn't quite
    correct since it will include the obscured areas but we will
    have to fix this later. (MArk).
*/

int PanoramiXCopyArea(ClientPtr client)
{
    int			j, result;
    DrawablePtr		pSrc, pDst;
    GContext		GCID;
    PanoramiXWindow 	*pPanoramiXSrcRoot;
    PanoramiXWindow 	*pPanoramiXDstRoot;
    PanoramiXWindow	*pPanoramiXSrc;
    PanoramiXWindow	*pPanoramiXDst;
    PanoramiXGC		*pPanoramiXGC = PanoramiXGCRoot;
    REQUEST(xCopyAreaReq);
    int           srcx = stuff->srcX, srcy = stuff->srcY;
    int           dstx = stuff->dstX, dsty = stuff->dstY;

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    VERIFY_DRAWABLE(pDst, stuff->dstDrawable, client);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        VERIFY_DRAWABLE(pSrc, stuff->srcDrawable, client);
        if ((pDst->pScreen != pSrc->pScreen) || (pDst->depth != pSrc->depth)) {
            client->errorValue = stuff->dstDrawable;
            return (BadMatch);
        }
    } else 
        pSrc = pDst;

    pPanoramiXSrcRoot = (pSrc->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXDstRoot = (pDst->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXSrc = pPanoramiXSrcRoot;
    pPanoramiXDst = pPanoramiXDstRoot;
    PANORAMIXFIND_ID(pPanoramiXSrc, stuff->srcDrawable);
    IF_RETURN(!pPanoramiXSrc, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXDst, stuff->dstDrawable);
    IF_RETURN(!pPanoramiXDst, BadDrawable);
    GCID = stuff->gc;
    PANORAMIXFIND_ID(pPanoramiXGC, GCID);
    IF_RETURN(!pPanoramiXGC, BadGC);

    if((pDst->type == DRAWABLE_PIXMAP) && (pSrc->type == DRAWABLE_WINDOW)) {
	DrawablePtr drawables[MAXSCREENS];
	GCPtr pGC;
	unsigned char *data;
	int pitch = PixmapBytePad(stuff->width, pDst->depth);
	
	if(!(data = xcalloc(stuff->height, pitch)))
	    return BadAlloc;
	
	drawables[0] = pSrc;
	for(j = 1; j < PanoramiXNumScreens; j++) 
	    VERIFY_DRAWABLE(drawables[j], pPanoramiXSrc->info[j].id, client);

	XineramaGetImageData(drawables, srcx, srcy, 
		stuff->width, stuff->height, ZPixmap, ~0, data, pitch, 
		(pPanoramiXSrc == PanoramiXWinRoot));

	FOR_NSCREENS_OR_ONCE(pPanoramiXSrc, j) {
	    stuff->gc = pPanoramiXGC->info[j].id;
	    VALIDATE_DRAWABLE_AND_GC(pPanoramiXDst->info[j].id, 
					pDst, pGC, client);
	     
	    (*pGC->ops->PutImage) (pDst, pGC, pDst->depth, dstx, dsty, 
				   stuff->width, stuff->height, 
				   0, ZPixmap, data);
	}

	xfree(data);

	result = Success;
    } else {
	FOR_NSCREENS_OR_ONCE(pPanoramiXSrc, j) {
	    stuff->dstDrawable = pPanoramiXDst->info[j].id;
	    stuff->srcDrawable = pPanoramiXSrc->info[j].id;
	    stuff->gc = pPanoramiXGC->info[j].id;
 	    if (pPanoramiXSrc == pPanoramiXSrcRoot) {	
		stuff->srcX = srcx - panoramiXdataPtr[j].x;
		stuff->srcY = srcy - panoramiXdataPtr[j].y;
	    }
 	    if (pPanoramiXDst == pPanoramiXDstRoot) {	
		stuff->dstX = dstx - panoramiXdataPtr[j].x;
		stuff->dstY = dsty - panoramiXdataPtr[j].y;
	    }

	    result = (* SavedProcVector[X_CopyArea])(client);

	    BREAK_IF(result != Success);
	}
    }
    return (result);
}


int PanoramiXCopyPlane(ClientPtr client)
{
    int			j, result;
    DrawablePtr		pSrc, pDst;
    GContext		GCID;
    PanoramiXWindow 	*pPanoramiXSrcRoot;
    PanoramiXWindow 	*pPanoramiXDstRoot;
    PanoramiXWindow	*pPanoramiXSrc;
    PanoramiXWindow	*pPanoramiXDst;
    PanoramiXGC		*pPanoramiXGC = PanoramiXGCRoot;
    REQUEST(xCopyPlaneReq);
    int           srcx = stuff->srcX, srcy = stuff->srcY;
    int           dstx = stuff->dstX, dsty = stuff->dstY;

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

    VERIFY_DRAWABLE(pDst, stuff->dstDrawable, client);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        VERIFY_DRAWABLE(pSrc, stuff->srcDrawable, client);
        if (pDst->pScreen != pSrc->pScreen) {
            client->errorValue = stuff->dstDrawable;
            return (BadMatch);
        }
    } else {
        pSrc = pDst;
    } 

    if(stuff->bitPlane == 0 || (stuff->bitPlane & (stuff->bitPlane - 1)) ||
       (stuff->bitPlane > (1L << (pSrc->depth - 1))))
    {
       client->errorValue = stuff->bitPlane;
       return(BadValue);
    }

    pPanoramiXSrcRoot = (pSrc->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXDstRoot = (pDst->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXSrc = pPanoramiXSrcRoot;
    pPanoramiXDst = pPanoramiXDstRoot;
    PANORAMIXFIND_ID(pPanoramiXSrc, stuff->srcDrawable);
    IF_RETURN(!pPanoramiXSrc, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXDst, stuff->dstDrawable);
    IF_RETURN(!pPanoramiXDst, BadDrawable);
    GCID = stuff->gc;
    PANORAMIXFIND_ID(pPanoramiXGC, GCID);
    IF_RETURN(!pPanoramiXGC, BadGC);

    FOR_NSCREENS_OR_ONCE(pPanoramiXSrc, j) {
        stuff->dstDrawable = pPanoramiXDst->info[j].id;
        stuff->srcDrawable = pPanoramiXSrc->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        if (pPanoramiXSrc == pPanoramiXSrcRoot) {       
            stuff->srcX = srcx - panoramiXdataPtr[j].x;
            stuff->srcY = srcy - panoramiXdataPtr[j].y;
        }
        if (pPanoramiXDst == pPanoramiXDstRoot) {       
            stuff->dstX = dstx - panoramiXdataPtr[j].x;
            stuff->dstY = dsty - panoramiXdataPtr[j].y;
        }
        result = (* SavedProcVector[X_CopyPlane])(client);
        BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXPolyPoint(ClientPtr client)
{
    int 	  result, npoint, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int           x_off = 0, y_off = 0;
    xPoint 	  *origPts;
    xPoint	  *origPtr, *modPtr;
    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
					RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, locDraw->id);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    npoint = ((client->req_len << 2) - sizeof(xPolyPointReq)) >> 2;
    if (npoint > 0) {
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }
	  modPtr = (xPoint *) &stuff[1];
	  origPtr = origPts;
	  for (i = npoint; i; i--) {
	      modPtr->x = origPtr->x - x_off;
	      modPtr++->y = origPtr++->y - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolyPoint])(client);
	  BREAK_IF(result != Success);
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
    }else
	return (client->noClientException);

}


int PanoramiXPolyLine(ClientPtr client)
{
    int 	  result, npoint, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int           x_off = 0, y_off = 0;
    xPoint 	  *origPts;
    xPoint	  *origPtr, *modPtr;
    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, locDraw->id);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != locDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    npoint = ((client->req_len << 2) - sizeof(xPolyLineReq)) >> 2;
    if (npoint > 0){
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }
	  modPtr = (xPoint *) &stuff[1];
	  origPtr = origPts;
	  for (i = npoint; i; i--) {
	      modPtr->x = origPtr->x - x_off;
	      modPtr++->y = origPtr++->y - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolyLine])(client);
	  BREAK_IF(result != Success);
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
   }else
	return (client->noClientException);
}


int PanoramiXPolySegment(ClientPtr client)
{
    int		  result, nsegs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xSegment 	  *origSegs;
    xSegment 	  *origPtr, *modPtr;
    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
				? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != locDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
         PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    nsegs = (client->req_len << 2) - sizeof(xPolySegmentReq);
    IF_RETURN((nsegs & 4), BadLength);
    nsegs >>= 3;
    if (nsegs > 0) {
        origSegs = (xSegment *) ALLOCATE_LOCAL(nsegs * sizeof(xSegment));
        memcpy((char *) origSegs, (char *) &stuff[1], nsegs * sizeof(xSegment));
        FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }
	  modPtr = (xSegment *) &stuff[1];
	  origPtr = origSegs;
	  for (i = nsegs; i; i--) {
	     modPtr->x1 = origPtr->x1 - x_off;
	     modPtr->y1 = origPtr->y1 - y_off;
	     modPtr->x2 = origPtr->x2 - x_off;
	     modPtr++->y2 = origPtr++->y2 - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolySegment])(client);
	  BREAK_IF(result != Success);
    	}
    DEALLOCATE_LOCAL(origSegs);
    return (result);
    }else
	  return (client->noClientException);
}


int PanoramiXPolyRectangle(ClientPtr client)
{
    int 	  result, nrects, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xRectangle 	  *origRecs;
    xRectangle 	  *origPtr, *modPtr;
    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    nrects = (client->req_len << 2) - sizeof(xPolyRectangleReq);
    IF_RETURN((nrects & 4), BadLength);
    nrects >>= 3;
    if (nrects > 0){
       origRecs = (xRectangle *) ALLOCATE_LOCAL(nrects * sizeof(xRectangle));
       memcpy((char *) origRecs, (char *) &stuff[1], nrects * sizeof(xRectangle));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	if (pPanoramiXWin == PanoramiXWinRoot) {
	    x_off = panoramiXdataPtr[j].x;
	    y_off = panoramiXdataPtr[j].y;
	}
	modPtr = (xRectangle *) &stuff[1];
	origPtr = origRecs;
	for (i = nrects; i; i--) {
	    modPtr->x = origPtr->x - x_off;
	    modPtr->y = origPtr->y - y_off;
	    modPtr->width = origPtr->width;
	    modPtr++->height = origPtr++->height;
	}
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_PolyRectangle])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origRecs);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyArc(ClientPtr client)
{
    int 	  result, narcs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int           x_off = 0, y_off = 0;
    xArc	  *origArcs;
    xArc	  *origPtr, *modPtr;
    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
	client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                                    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    narcs = (client->req_len << 2) - sizeof(xPolyArcReq);
    IF_RETURN((narcs % sizeof(xArc)), BadLength);
    narcs /= sizeof(xArc);
    if (narcs > 0){
       origArcs = (xArc *) ALLOCATE_LOCAL(narcs * sizeof(xArc));
       memcpy((char *) origArcs, (char *) &stuff[1], narcs * sizeof(xArc));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }
        modPtr = (xArc *) &stuff[1];
        origPtr = origArcs;
        for (i = narcs; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyArc])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origArcs);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXFillPoly(ClientPtr client)
{
    int 	  result, count, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    DDXPointPtr	  locPts;
    DDXPointPtr	  origPts, modPts;
    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
				? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    count = ((client->req_len << 2) - sizeof(xFillPolyReq)) >> 2;
    if (count > 0){
       locPts = (DDXPointPtr) ALLOCATE_LOCAL(count * sizeof(DDXPointRec));
       memcpy((char *) locPts, (char *) &stuff[1], count * sizeof(DDXPointRec));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	if (pPanoramiXWin == PanoramiXWinRoot) {
	    x_off = panoramiXdataPtr[j].x;
	    y_off = panoramiXdataPtr[j].y;
	}
	modPts = (DDXPointPtr) &stuff[1];
	origPts = locPts;
	for (i = count; i; i--) {
	    modPts->x = origPts->x - x_off;
	    modPts++->y = origPts++->y - y_off;
	}
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_FillPoly])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(locPts);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyFillRectangle(ClientPtr client)
{
    int 	  result, things, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xRectangle	  *origThings;
    xRectangle	  *origPtr, *modPtr;
    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                                    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    things = (client->req_len << 2) - sizeof(xPolyFillRectangleReq);
    IF_RETURN((things & 4), BadLength);
    things >>= 3;
    if (things > 0){
       origThings = (xRectangle *) ALLOCATE_LOCAL(things * sizeof(xRectangle));
       memcpy((char *) origThings, (char *)&stuff[1], things * sizeof(xRectangle));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }
        modPtr = (xRectangle *) &stuff[1];
        origPtr = origThings;
        for (i = things; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyFillRectangle])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origThings);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyFillArc(ClientPtr client)
{
    int 	  result, arcs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xArc	  *origArcs;
    xArc	  *origPtr, *modPtr;
    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                           	 ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    arcs = (client->req_len << 2) - sizeof(xPolyFillArcReq);
    IF_RETURN((arcs % sizeof(xArc)), BadLength);
    arcs /= sizeof(xArc);
    if (arcs > 0) {
       origArcs = (xArc *) ALLOCATE_LOCAL(arcs * sizeof(xArc));
       memcpy((char *) origArcs, (char *)&stuff[1], arcs * sizeof(xArc));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }
        modPtr = (xArc *) &stuff[1];
        origPtr = origArcs;
        for (i = arcs; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyFillArc])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origArcs);
       return (result);
    }else
       return (client->noClientException);
}


/* 64-bit server notes: the protocol restricts padding of images to
 * 8-, 16-, or 32-bits. We would like to have 64-bits for the server
 * to use internally. Removes need for internal alignment checking.
 * All of the PutImage functions could be changed individually, but
 * as currently written, they call other routines which require things
 * to be 64-bit padded on scanlines, so we changed things here.
 * If an image would be padded differently for 64- versus 32-, then
 * copy each scanline to a 64-bit padded scanline.
 * Also, we need to make sure that the image is aligned on a 64-bit
 * boundary, even if the scanlines are padded to our satisfaction.
 */

int PanoramiXPutImage(register ClientPtr client)
{
    register DrawablePtr pDraw;
    int			 j;
    PanoramiXWindow 	 *pPanoramiXWin;
    PanoramiXWindow 	 *pPanoramiXRoot;
    PanoramiXGC 		 *pPanoramiXGC = PanoramiXGCRoot;
    int			 orig_x, orig_y;
    int			 result;


    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(
		client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP) 
				? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin,BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->dstX;
    orig_y = stuff->dstY;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
      if (pPanoramiXWin == pPanoramiXRoot) {
    	  stuff->dstX = orig_x - panoramiXdataPtr[j].x;
	  stuff->dstY = orig_y - panoramiXdataPtr[j].y;
      }       
      stuff->drawable = pPanoramiXWin->info[j].id;
      stuff->gc = pPanoramiXGC->info[j].id;
      result = (* SavedProcVector[X_PutImage])(client);
    }
    return(result);
}


typedef struct _SrcParts{
    int x1, y1, x2, y2, width, ByteWidth;
    char *buf;
} SrcPartsRec;



int PanoramiXGetImage(ClientPtr client)
{
    PanoramiXWindow	*pPanoramiXWin = PanoramiXWinRoot;
    DrawablePtr 	drawables[MAXSCREENS];
    DrawablePtr 	pDraw;
    xGetImageReply	xgi;
    char		*pBuf;
    int         	i, x, y, w, h, format;
    Mask		plane, planemask;
    int			linesDone, nlines, linesPerBuf;
    long		widthBytesLine, length;
    Bool		isRoot;
#ifdef INTERNAL_VS_EXTERNAL_PADDING
    long		widthBytesLineProto, lengthProto;
#endif

    REQUEST(xGetImageReq);

    REQUEST_SIZE_MATCH(xGetImageReq);

    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
	client->errorValue = stuff->format;
        return(BadValue);
    }

    VERIFY_DRAWABLE(pDraw, stuff->drawable, client);

    if(pDraw->type == DRAWABLE_PIXMAP)
	return (*SavedProcVector[X_GetImage])(client);

    if(!((WindowPtr)pDraw)->realized)
	return(BadMatch);

    x = stuff->x;
    y = stuff->y;
    w = stuff->width;
    h = stuff->height;
    format = stuff->format;
    planemask = stuff->planeMask;

    isRoot = (stuff->drawable == pPanoramiXWin->info[0].id);

    if(isRoot) {
      if( /* check for being onscreen */
	x < 0 || x + w > PanoramiXPixWidth ||
	y < 0 || y + h > PanoramiXPixHeight )
	    return(BadMatch);
    } else {
      if( /* check for being onscreen */
	panoramiXdataPtr[0].x + pDraw->x + x < 0 ||
	panoramiXdataPtr[0].x + pDraw->x + x + w > PanoramiXPixWidth ||
        panoramiXdataPtr[0].y + pDraw->y + y < 0 ||
	panoramiXdataPtr[0].y + pDraw->y + y + h > PanoramiXPixHeight ||
	 /* check for being inside of border */
       	x < - wBorderWidth((WindowPtr)pDraw) ||
	x + w > wBorderWidth((WindowPtr)pDraw) + (int)pDraw->width ||
	y < -wBorderWidth((WindowPtr)pDraw) ||
	y + h > wBorderWidth ((WindowPtr)pDraw) + (int)pDraw->height)
	    return(BadMatch);

	PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }

    drawables[0] = pDraw;
    for(i = 1; i < PanoramiXNumScreens; i++)
	VERIFY_DRAWABLE(drawables[i], pPanoramiXWin->info[i].id, client);

    xgi.visual = wVisual (((WindowPtr) pDraw));
    xgi.type = X_Reply;
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if(format == ZPixmap) {
	widthBytesLine = PixmapBytePad(w, pDraw->depth);
	length = widthBytesLine * h;

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	widthBytesLineProto = PixmapBytePadProto(w, pDraw->depth);
	lengthProto 	    = widthBytesLineProto * h;
#endif
    } else {
	widthBytesLine = BitmapBytePad(w);
	plane = ((Mask)1) << (pDraw->depth - 1);
	/* only planes asked for */
	length = widthBytesLine * h *
		 Ones(planemask & (plane | (plane - 1)));

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	widthBytesLineProto = BitmapBytePadProto(w);
	lengthProto = (length / widthBytesLine) * widthBytesLineProto;
#endif
    }

#ifdef INTERNAL_VS_EXTERNAL_PADDING
    xgi.length = (lengthProto + 3) >> 2;
#else
    xgi.length = (length + 3) >> 2;
#endif


    if (widthBytesLine == 0 || h == 0)
	linesPerBuf = 0;
    else if (widthBytesLine >= XINERAMA_IMAGE_BUFSIZE)
	linesPerBuf = 1;
    else {
	linesPerBuf = XINERAMA_IMAGE_BUFSIZE / widthBytesLine;
	if (linesPerBuf > h)
	    linesPerBuf = h;
    }
    length = linesPerBuf * widthBytesLine;
    if(!(pBuf = xalloc(length)))
	return (BadAlloc);

    WriteReplyToClient(client, sizeof (xGetImageReply), &xgi);

    if (linesPerBuf == 0) {
	/* nothing to do */
    }
    else if (format == ZPixmap) {
        linesDone = 0;
        while (h - linesDone > 0) {
	    nlines = min(linesPerBuf, h - linesDone);

	    if(pDraw->depth == 1)
		bzero(pBuf, nlines * widthBytesLine);

	    XineramaGetImageData(drawables, x, y + linesDone, w, nlines,
			format, planemask, pBuf, widthBytesLine, isRoot);

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	    if ( widthBytesLine != widthBytesLineProto ) {
		char *linePtr = pBuf;
		for(i = 0; i < nlines; i++, linePtr += widthBytesLine) {
		    (void)WriteToClient(client, widthBytesLineProto, linePtr);
		}
	    } else
#else
		(void)WriteToClient(client,
				    (int)(nlines * widthBytesLine),
				    pBuf);
#endif
	    linesDone += nlines;
        }
    } else { /* XYPixmap */
        for (; plane; plane >>= 1) {
	    if (planemask & plane) {
	        linesDone = 0;
	        while (h - linesDone > 0) {
		    nlines = min(linesPerBuf, h - linesDone);

		    bzero(pBuf, nlines * widthBytesLine);

		    XineramaGetImageData(drawables, x, y + linesDone, w, 
					nlines, format, plane, pBuf,
					widthBytesLine, isRoot);

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	    if ( widthBytesLine != widthBytesLineProto ) {
		char *linePtr = pBuf;
		for(i = 0; i < nlines; i++, linePtr += widthBytesLine) {
		    (void)WriteToClient(client, widthBytesLineProto, linePtr);
		}
	    } else
#else
		(void)WriteToClient(client,
				    (int)(nlines * widthBytesLine),
				    pBuf);
#endif

		    linesDone += nlines;
		}
            }
	}
    }
    xfree(pBuf);
    return (client->noClientException);
}


int 
PanoramiXPolyText8(register ClientPtr client)
{
    int 	  result, j;

    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC     *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr      pDraw;
    int		     orig_x, orig_y;
    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(
                client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);

    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
                      ? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != pDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	stuff->x = orig_x;
	stuff->y = orig_y; 
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	if (!j)
	    noPanoramiXExtension = TRUE;
	result = (*SavedProcVector[X_PolyText8])(client);
	noPanoramiXExtension = FALSE;
	BREAK_IF(result != Success);
    }
    return (result);
}

int 
PanoramiXPolyText16(register ClientPtr client)
{
    int 	  result, j;

    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    int		  orig_x, orig_y;
    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(
                client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != pDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	stuff->x = orig_x;
	stuff->y = orig_y; 
	if (pPanoramiXWin == pPanoramiXRoot)  {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	} 
	if (!j)
	    noPanoramiXExtension = TRUE;
	result = (*SavedProcVector[X_PolyText16])(client);
	noPanoramiXExtension = FALSE;
	BREAK_IF(result != Success);
    }
    return (result);
}



int PanoramiXImageText8(ClientPtr client)
{
    int 	  result, j;
    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    REQUEST_AT_LEAST_SIZE(xImageTextReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(
                client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ImageText8])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXImageText16(ClientPtr client)
{
    int 	  result, j;
    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    REQUEST_AT_LEAST_SIZE(xImageTextReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(
                client, stuff->drawable, RC_DRAWABLE, SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
				? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ImageText16])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXCreateColormap(register ClientPtr client)
{
    Colormap		mid;
    register WindowPtr	pWin;
    ScreenPtr 		pScreen;

    REQUEST(xCreateColormapReq);

    int 		result;
    int			j = 0;
    VisualID            orig_visual;
    Colormap		cmapID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXCmap	*localCmap;
    PanoramiXCmap	*pPanoramiXCmap = PanoramiXCmapRoot;

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    mid = stuff->mid;
    orig_visual = stuff->visual;
    j = 0;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    if (pPanoramiXWin) {
	localCmap = (PanoramiXCmap *)Xcalloc(sizeof(PanoramiXCmap));
	IF_RETURN(!localCmap, BadAlloc);
    } else {
	return BadWindow;
    }
    for (j = 0; j <= PanoramiXNumScreens - 1; j++) {
	cmapID = j ? FakeClientID(client->index) : mid;
	localCmap->info[j].id = cmapID;
    }
    localCmap->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXCmap, PanoramiXCmapRoot);
    pPanoramiXCmap->next = localCmap;
   
    /* Use Screen 0 to get the matching Visual ID */
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
                                           SecurityReadAccess);

    if (!pWin)
         return(BadWindow);
    pScreen = pWin->drawable.pScreen;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->mid = localCmap->info[j].id;
	stuff->window = pPanoramiXWin->info[j].id;
	if ( orig_visual != CopyFromParent ) 
	    stuff->visual = PanoramiXVisualTable[orig_visual][j];
	result = (* SavedProcVector[X_CreateColormap])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
       pPanoramiXCmap->next = NULL ;
       if (localCmap)
           Xfree(localCmap);
    }
    return (result);
}


int PanoramiXFreeColormap(ClientPtr client)
{
    REQUEST(xResourceReq);
    int         result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;
    PanoramiXCmap *pPanoramiXCmapback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);

    for (; pPanoramiXCmap && (pPanoramiXCmap->info[0].id != stuff->id);
					    pPanoramiXCmap = pPanoramiXCmap->next)
        pPanoramiXCmapback = pPanoramiXCmap;
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->id = pPanoramiXCmap->info[j].id;
        result = (* SavedProcVector[X_FreeColormap])(client);
	BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXCmapback && 
	       pPanoramiXCmap && pPanoramiXCmap->FreeMe) {
        pPanoramiXCmapback->next = pPanoramiXCmap->next;
        Xfree(pPanoramiXCmap);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXInstallColormap(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;

    REQUEST_SIZE_MATCH(xResourceReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->id);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
	stuff->id = pPanoramiXCmap->info[j].id;
	result = (* SavedProcVector[X_InstallColormap])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXUninstallColormap(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->id);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
	stuff->id = pPanoramiXCmap->info[j].id;
	result = (* SavedProcVector[X_UninstallColormap])(client);
	BREAK_IF(result != Success);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXAllocColor(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocColorReq);

    REQUEST_SIZE_MATCH(xAllocColorReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap){
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_AllocColor])(client);
	noPanoramiXExtension = FALSE;
    }else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
	    if (!j)
	        noPanoramiXExtension = TRUE;
            result = (* SavedProcVector[X_AllocColor])(client);
	    noPanoramiXExtension = FALSE;
            BREAK_IF(result != Success);
	}
    }
    return (result);
}


int PanoramiXAllocNamedColor(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocNamedColorReq);

    REQUEST_FIXED_SIZE(xAllocNamedColorReq, stuff->nbytes);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->cmap = pPanoramiXCmap->info[j].id;
	if (!j)
	    noPanoramiXExtension = TRUE;
        result = (* SavedProcVector[X_AllocNamedColor])(client);
	noPanoramiXExtension = FALSE;
        BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXAllocColorCells(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap) {
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_AllocColorCells])(client);
        noPanoramiXExtension = FALSE;
    }else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
	    if (!j)
	        noPanoramiXExtension = TRUE;
            result = (* SavedProcVector[X_AllocColorCells])(client);
	    noPanoramiXExtension = FALSE;
	    /* Because id's are eventually searched for in
	       some client list, we don't check for success 
	       on fake id's last id will be real, we really 
	       only care about results related to real id's 
	     BREAK_IF(result != Success);
	     */
	}
    }
    return (result);
}


int PanoramiXFreeColors(register ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xFreeColorsReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->cmap = pPanoramiXCmap->info[j].id;
        result = (* SavedProcVector[X_FreeColors])(client);
	/* Because id's are eventually searched for in
	   some client list, we don't check for success 
	   on fake id's last id will be real, we really 
	   only care about results related to real id's */
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXStoreColors(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap)
	result = (* SavedProcVector[X_StoreColors])(client);
    else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
            result = (* SavedProcVector[X_StoreColors])(client);
            BREAK_IF(result != Success);
	}
    }
    return (result);
}
