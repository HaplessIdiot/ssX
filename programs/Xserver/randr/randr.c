/*
 * $XFree86: xc/programs/Xserver/randr/randr.c,v 1.1 2001/05/23 03:29:44 keithp Exp $
 *
 * Copyright © 2000 Compaq Computer Corporation, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Compaq makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL COMPAQ
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Compaq Computer Corporation, Inc.
 */

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "servermd.h"
#include "randr.h"
#include "randrproto.h"
#include "randrstr.h"
#include "Xfuncproto.h"
#ifdef EXTMODULE
#include "xf86_ansic.h"
#endif

#define RR_VALIDATE
int	RRGeneration;
int	RRNScreens;

static int ProcRRQueryVersion (ClientPtr pClient);
static int ProcRRDispatch (ClientPtr pClient);
static int SProcRRDispatch (ClientPtr pClient);
static int SProcRRQueryVersion (ClientPtr pClient);

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

static CARD8	RRReqCode;
static int	RRErrBase;
static int	RREventBase;
static RESTYPE ClientType, EventType; /* resource types for event masks */

/*
 * each window has a list of clients requesting
 * RRNotify events.  Each client has a resource
 * for each window it selects RRNotify input for,
 * this resource is used to delete the RRNotifyRec
 * entry from the per-window queue.
 */

typedef struct _RREvent *RREventPtr;

typedef struct _RREvent {
    RREventPtr  next;
    ClientPtr	client;
    WindowPtr	window;
    XID		clientResource;
} RREventRec;

int	rrPrivIndex = -1;

static void
RRResetProc (ExtensionEntry *extEntry)
{
}

    
static Bool
RRCloseScreen (int i, ScreenPtr pScreen)
{
    rrScrPriv(pScreen);

    unwrap (pScrPriv, pScreen, CloseScreen);
    xfree (pScrPriv);
    RRNScreens -= 1;	/* ok, one fewer screen with RandR running */
    return (*pScreen->CloseScreen) (i, pScreen);    
}

static void
SRRScreenChangeNotifyEvent(from, to)
    xRRScreenChangeNotifyEvent *from, *to;
{
    to->type = from->type;
    to->state = from->state;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->root, to->root);
}

Bool RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr   pScrPriv;

    fprintf(stderr, "RRGeneration = %d, serverGeneration = %d\n",
	    RRGeneration, serverGeneration);
    if (RRGeneration != serverGeneration)
    {
	if ((rrPrivIndex = AllocateScreenPrivateIndex()) < 0) {
	  fprintf(stderr, "rrPrivIndex = %d\n", rrPrivIndex);
	return FALSE;
	}
	fprintf(stderr, "rrPrivIndex = %d\n", rrPrivIndex);
	RRGeneration = serverGeneration;
    }

    pScrPriv = (rrScrPrivPtr) xalloc (sizeof (rrScrPrivRec));
    if (!pScrPriv)
    {
	return FALSE;
    }
    SetRRScreen(pScreen, pScrPriv);

    pScrPriv->rotation = 0;
    /*    pScrPriv->lastSetTime = 0;	/* XXX is this right? */
    
    wrap (pScrPriv, pScreen, CloseScreen, RRCloseScreen);
    RRNScreens += 1;	/* keep count of screens that implement randr */
    return TRUE;
}

/*ARGSUSED*/
static int
RRFreeClient (pointer data, XID id)
{
    RREventPtr   pRREvent;
    WindowPtr	    pWin;
    RREventPtr   *pHead, pCur, pPrev;

    pRREvent = (RREventPtr) data;
    pWin = pRREvent->window;
    pHead = (RREventPtr *) LookupIDByType(pWin->drawable.id, EventType);
    if (pHead) {
	pPrev = 0;
	for (pCur = *pHead; pCur && pCur != pRREvent; pCur=pCur->next)
	    pPrev = pCur;
	if (pCur)
	{
	    if (pPrev)
	    	pPrev->next = pRREvent->next;
	    else
	    	*pHead = pRREvent->next;
	}
    }
    xfree ((pointer) pRREvent);
    return 1;
}

/*ARGSUSED*/
static int
RRFreeEvents (pointer data, XID id)
{
    RREventPtr   *pHead, pCur, pNext;

    pHead = (RREventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
	pNext = pCur->next;
	FreeResource (pCur->clientResource, ClientType);
	xfree ((pointer) pCur);
    }
    xfree ((pointer) pHead);
    return 1;
}

void
RRExtensionInit (void)
{
    ExtensionEntry *extEntry;

    if (RRNScreens == 0) return;

    ClientType = CreateNewResourceType(RRFreeClient);
    if (!ClientType)
	return;
    EventType = CreateNewResourceType(RRFreeEvents);
    if (!EventType)
	return;
    extEntry = AddExtension (RANDR_NAME, RRNumberEvents, RRNumberErrors,
			     ProcRRDispatch, SProcRRDispatch,
			     RRResetProc, StandardMinorOpcode);
    if (!extEntry)
	return;
    RRReqCode = (CARD8) extEntry->base;
    RRErrBase = extEntry->errorBase;
    RREventBase = extEntry->eventBase;
    EventSwapVector[RREventBase + RRScreenChangeNotify] = (EventSwapPtr) 
      SRRScreenChangeNotifyEvent;

    return;
}
		
static int
TellChanged (WindowPtr pWin, pointer value)
{
    RREventPtr			*pHead, pRREvent;
    ClientPtr			client;
    xRRScreenChangeNotifyEvent	se;
    ScreenPtr			pScreen = pWin->drawable.pScreen;
    rrScrPriv(pScreen);
    WindowPtr			pRoot = WindowTable[pScreen->myNum];

    pHead = (RREventPtr *) LookupIDByType (pWin->drawable.id, EventType);
    if (!pHead)
	return;
    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) 
    {
	client = pRREvent->client;
	if (client == serverClient || client->clientGone)
	    continue;
	se.type = RRScreenChangeNotify + RREventBase;
	se.sequenceNumber = client->sequence;
	se.timestamp = currentTime.milliseconds;
	se.root = pRoot->drawable.id;
	WriteEventsToClient (client, 1, (xEvent *) &se);
    }
}

static Bool
RRGetInfo (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    int		    i, j;
    Bool	    changed;
    int		    rotations, swaps;

    for (i = 0; i < pScrPriv->nVisualSets; i++)
    {
	pScrPriv->pVisualSets[i].oldReferenced = pScrPriv->pVisualSets[i].referenced;
	pScrPriv->pVisualSets[i].referenced = FALSE;
    }
    for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
    {
	pScrPriv->pSetsOfVisualSets[i].oldReferenced = pScrPriv->pSetsOfVisualSets[i].referenced;
	pScrPriv->pSetsOfVisualSets[i].referenced = FALSE;
    }
    for (i = 0; i < pScrPriv->nSizes; i++)
    {
	pScrPriv->pSizes[i].oldReferenced = pScrPriv->pSizes[i].referenced;
	pScrPriv->pSizes[i].referenced = FALSE;
    }
    if (!(*pScrPriv->rrGetInfo) (pScreen, &rotations, &swaps))
	return FALSE;

    changed = FALSE;

    /*
     * Check whether anything changed and simultaneously generate
     * the protocol id values for the objects
     */
    if (rotations != pScrPriv->rotations)
    {
	pScrPriv->rotations = rotations;
	changed = TRUE;
    }
    if (swaps != pScrPriv->swaps)
    {
	pScrPriv->swaps = swaps;
	changed = TRUE;
    }

    j = 0;
    for (i = 0; i < pScrPriv->nVisualSets; i++)
    {
	if (pScrPriv->pVisualSets[i].oldReferenced != pScrPriv->pVisualSets[i].referenced)
	    changed = TRUE;
	if (pScrPriv->pVisualSets[i].referenced)
	    pScrPriv->pVisualSets[i].id = j++;
    }
    pScrPriv->nVisualSetsInUse = j;
    j = 0;
    for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
    {
	if (pScrPriv->pSetsOfVisualSets[i].oldReferenced != pScrPriv->pSetsOfVisualSets[i].referenced)
	    changed = TRUE;
	if (pScrPriv->pSetsOfVisualSets[i].referenced)
	    pScrPriv->pSetsOfVisualSets[i].id = j++;
    }
    pScrPriv->nSetsOfVisualSetsInUse = j;
    j = 0;
    for (i = 0; i < pScrPriv->nSizes; i++)
    {
	if (pScrPriv->pSizes[i].oldReferenced != pScrPriv->pSizes[i].referenced)
	    changed = TRUE;
	if (pScrPriv->pSizes[i].referenced)
	    pScrPriv->pSizes[i].id = j++;
    }
    pScrPriv->nSizesInUse = j;
    if (changed)
    {
	UpdateCurrentTime ();
	WalkTree (pScreen, TellChanged, (pointer) pScreen);
    }
}


static int
ProcRRQueryVersion (ClientPtr client)
{
    xRRQueryVersionReply rep;
    register int n;
    REQUEST(xRRQueryVersionReq);

    REQUEST_SIZE_MATCH(xRRQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = RANDR_MAJOR;
    rep.minorVersion = RANDR_MINOR;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.majorVersion, n);
	swapl(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xRRQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcRRGetScreenInfo (ClientPtr client)
{
    xRRGetScreenInfoReply rep;
    register DrawablePtr pDraw;
    register int n;
    REQUEST(xRRGetScreenInfoReq);
    rrScrPrivPtr    pScrPriv;

    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    SECURITY_VERIFY_DRAWABLE (pDraw, stuff->drawable, client,
			      SecurityReadAccess);

    pScrPriv = rrGetScrPriv(pDraw->pScreen);
    if (!pScrPriv)
    
    RRGetInfo (pDraw->pScreen);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.timestamp = 
    rep.rotation =
    rep.nSizes = 
    rep.nVisualSets = 
    rep.nAccelerated = 
    rep.nRotations =
    rep.sizeSetID =
    rep.visualSetID = 0;
    
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenInfoReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcRRSetScreenConfig (ClientPtr client)
{
    xRRSetScreenConfigReply rep;
    rrScrPrivPtr pScrPriv;
    register int n;
    register DrawablePtr pDraw;
    TimeStamp time;
    REQUEST(xRRSetScreenConfigReq);

    REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
    SECURITY_VERIFY_DRAWABLE(pDraw, stuff->drawable, client,
			     SecurityWriteAccess);
    /* 
     * make sure any input is processed, as some screen
     * may have a pending config event outstanding 
     * not yet reflected back to users. Time only goes forwards...
     */
    UpdateCurrentTime();

    fprintf(stderr, "got to procRRSetScreenConfig, draw = %d, time = %d, sizeseti = %d, visualseti = %d, rotation = %d\n",
	    pDraw,
	    stuff->timestamp,
	    stuff->sizeSetIndex,
	    stuff->visualSetIndex,
	    stuff->rotation);

    time = ClientTimeToServerTime(stuff->timestamp);

    /* 
     * if the client's time stamp is older than the last time
     *  the client got screen information, then we should  
     */

    /* call out to ddx routine */

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xRRSetScreenConfigReply), (char *)&rep);

    /* need code here to generate ScreenChangeNotifyEvent */
    return (client->noClientException);
}

static int
ProcRRScreenChangeSelectInput (ClientPtr client)
{
    REQUEST(xRRScreenChangeSelectInputReq);
    WindowPtr	pWin;
    RREventPtr	pRREvent, pNewRREvent, *pHead;
    XID		clientResource;

    REQUEST_SIZE_MATCH(xRRScreenChangeSelectInputReq);
    pWin = SecurityLookupWindow (stuff->window, client, SecurityWriteAccess);
    if (!pWin)
	return BadWindow;
    pHead = (RREventPtr *)SecurityLookupIDByType(client,
			pWin->drawable.id, EventType, SecurityWriteAccess);
    switch (stuff->enable) {
    case xTrue:
	if (pHead) {

	    /* check for existing entry. */
	    for (pRREvent = *pHead;
		 pRREvent;
 		 pRREvent = pRREvent->next)
	    {
		if (pRREvent->client == client)
		    return Success;
	    }
	}

	/* build the entry */
    	pNewRREvent = (RREventPtr)
			    xalloc (sizeof (RREventRec));
    	if (!pNewRREvent)
	    return BadAlloc;
    	pNewRREvent->next = 0;
    	pNewRREvent->client = client;
    	pNewRREvent->window = pWin;
    	/*
 	 * add a resource that will be deleted when
     	 * the client goes away
     	 */
   	clientResource = FakeClientID (client->index);
    	pNewRREvent->clientResource = clientResource;
    	if (!AddResource (clientResource, ClientType, (pointer)pNewRREvent))
	    return BadAlloc;
    	/*
     	 * create a resource to contain a pointer to the list
     	 * of clients selecting input.  This must be indirect as
     	 * the list may be arbitrarily rearranged which cannot be
     	 * done through the resource database.
     	 */
    	if (!pHead)
    	{
	    pHead = (RREventPtr *) xalloc (sizeof (RREventPtr));
	    if (!pHead ||
	    	!AddResource (pWin->drawable.id, EventType, (pointer)pHead))
	    {
	    	FreeResource (clientResource, RT_NONE);
	    	return BadAlloc;
	    }
	    *pHead = 0;
    	}
    	pNewRREvent->next = *pHead;
    	*pHead = pNewRREvent;
	break;
    case xFalse:
	/* delete the interest */
	if (pHead) {
	    pNewRREvent = 0;
	    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
		if (pRREvent->client == client)
		    break;
		pNewRREvent = pRREvent;
	    }
	    if (pRREvent) {
		FreeResource (pRREvent->clientResource, ClientType);
		if (pNewRREvent)
		    pNewRREvent->next = pRREvent->next;
		else
		    *pHead = pRREvent->next;
		xfree (pRREvent);
	    }
	}
	break;
    default:
	client->errorValue = stuff->enable;
	return BadValue;
    }
    return Success;
}

static int
ProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RRQueryVersion:
	return ProcRRQueryVersion(client);
    case X_RRGetScreenInfo:
        return ProcRRGetScreenInfo(client);
    case X_RRSetScreenConfig:
        return ProcRRSetScreenConfig(client);
    case X_RRScreenChangeSelectInput:
        return ProcRRScreenChangeSelectInput(client);
    default:
	return BadRequest;
    }
}

static int
SProcRRQueryVersion (ClientPtr client)
{
    register int n;
    REQUEST(xRRQueryVersionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->majorVersion, n);
    swapl(&stuff->minorVersion, n);
    return ProcRRQueryVersion(client);
}

static int
SProcRRGetScreenInfo (ClientPtr client)
{
    register int n;
    REQUEST(xRRGetScreenInfoReq);

    swaps(&stuff->length, n);
    swapl(&stuff->drawable, n);
    return ProcRRGetScreenInfo(client);
}

static int
SProcRRSetScreenConfig (ClientPtr client)
{
    register int n;
    REQUEST(xRRSetScreenConfigReq);

    swaps(&stuff->length, n);
    swapl(&stuff->drawable, n);
    swapl(&stuff->timestamp, n);
    swaps(&stuff->sizeSetIndex, n);
    swaps(&stuff->visualSetIndex, n);
    swaps(&stuff->rotation, n);
    return ProcRRSetScreenConfig(client);
}

static int
SProcRRScreenChangeSelectInput (ClientPtr client)
{
    register int n;
    REQUEST(xRRScreenChangeSelectInputReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return ProcRRScreenChangeSelectInput(client);
}

static int
SProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RRQueryVersion:
	return SProcRRQueryVersion(client);
    case X_RRGetScreenInfo:
        return SProcRRGetScreenInfo(client);
    case X_RRSetScreenConfig:
        return SProcRRSetScreenConfig(client);
    case X_RRScreenChangeSelectInput:
        return SProcRRScreenChangeSelectInput(client);
    default:
	return BadRequest;
    }
}

/*
 * Utility functions for creating the set of possible
 * configurations
 */

RRVisualSetPtr
RRCreateVisualSet (ScreenPtr pScreen)
{
    RRVisualSetPtr  pVisualSet;
    
    pVisualSet = (RRVisualSetPtr) xalloc (sizeof (RRVisualSet));
    pVisualSet->nvisuals = 0;
    pVisualSet->visuals = 0;
    pVisualSet->referenced = TRUE;
    pVisualSet->oldReferenced = FALSE;
    return pVisualSet;
}

void
RRDestroyVisualSet (ScreenPtr	    pScreen,
		    RRVisualSetPtr  pVisualSet)
{
#ifdef RR_VALIDATE
    int	i;
    rrScrPriv(pScreen);

    for (i = 0; i < pScrPriv->nVisualSets; i++)
	if (pVisualSet == &pScrPriv->pVisualSets[i])
	    FatalError ("Freeing registered visual set");
#endif
    xfree (pVisualSet->visuals);
    xfree (pVisualSet);
}

Bool
RRAddVisualToVisualSet (ScreenPtr	pScreen,
			RRVisualSetPtr	pVisualSet,
			VisualPtr	pVisual)
{
    VisualPtr	*new;

    new = xrealloc (pVisualSet->visuals, 
		    (pVisualSet->nvisuals + 1) * sizeof (VisualPtr));
    if (!new)
	return FALSE;
    (pVisualSet->visuals = new)[pVisualSet->nvisuals++] = pVisual;
    return TRUE;
}

Bool
RRAddDepthToVisualSet (ScreenPtr	pScreen,
		       RRVisualSetPtr	pVisualSet,
		       DepthPtr		pDepth)
{
    int		i;
    int		v;

    for (i = 0; i < pDepth->numVids; i++)
	for (v = 0; v < pScreen->numVisuals; v++)
	    if (pScreen->visuals[v].vid == pDepth->vids[i])
		if (!RRAddVisualToVisualSet (pScreen, pVisualSet,
					     &pScreen->visuals[v]))
		    return FALSE;
    return TRUE;
}

/*
 * Return true if a and b reference the same set of visuals
 */

static Bool
RRVisualSetMatches (RRVisualSetPtr  a,
		    RRVisualSetPtr  b)
{
    int	ai, bi;
    
    if (a->nvisuals != b->nvisuals)
	return FALSE;
    for (ai = 0; ai < a->nvisuals; ai++)
    {
	for (bi = 0; bi < b->nvisuals; bi++)
	    if (a->visuals[ai] == b->visuals[bi])
		break;
	if (bi == b->nvisuals)
	    return FALSE;
    }
    return TRUE;
}

RRVisualSetPtr
RRRegisterVisualSet (ScreenPtr	    pScreen,
		     RRVisualSetPtr pVisualSet)
{
    rrScrPriv (pScreen);
    int	    i;
    RRVisualSetPtr  pNew;

    if (!pScrPriv)
    {
	RRDestroyVisualSet (pScreen, pVisualSet);
	return 0;
    }
    for (i = 0; i < pScrPriv->nVisualSets; i++)
	if (RRVisualSetMatches (pVisualSet,
				&pScrPriv->pVisualSets[i]))
	{
	    RRDestroyVisualSet (pScreen, pVisualSet);
	    pScrPriv->pVisualSets[i].referenced = TRUE;
	    return &pScrPriv->pVisualSets[i];
	}
    pNew = xrealloc (pScrPriv->pVisualSets,
		     (pScrPriv->nVisualSets + 1) * sizeof (RRVisualSet));
    if (!pNew)
    {
	RRDestroyVisualSet (pScreen, pVisualSet);
	return 0;
    }
    pNew[pScrPriv->nVisualSets++] = *pVisualSet;
    xfree (pVisualSet);
    pScrPriv->pVisualSets = pNew;
    return &pNew[pScrPriv->nVisualSets-1];
}

RRSetOfVisualSetPtr
RRCreateSetOfVisualSet (ScreenPtr pScreen)
{
    RRSetOfVisualSetPtr  pSetOfVisualSet;
    
    pSetOfVisualSet = (RRSetOfVisualSetPtr) xalloc (sizeof (RRSetOfVisualSet));
    pSetOfVisualSet->nsets = 0;
    pSetOfVisualSet->sets = 0;
    pSetOfVisualSet->referenced = TRUE;
    pSetOfVisualSet->oldReferenced = FALSE;
    return pSetOfVisualSet;
}

void
RRDestroySetOfVisualSet (ScreenPtr		pScreen,
			 RRSetOfVisualSetPtr	pSetOfVisualSet)
{
#ifdef RR_VALIDATE
    int	i;
    rrScrPriv(pScreen);

    for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
	if (pSetOfVisualSet == &pScrPriv->pSetsOfVisualSets[i])
	    FatalError ("Freeing registered visual set");
#endif
    xfree (pSetOfVisualSet->sets);
    xfree (pSetOfVisualSet);
}

Bool
RRAddVisualSetToSetOfVisualSet (ScreenPtr	    pScreen,
				RRSetOfVisualSetPtr pSetOfVisualSet,
				RRVisualSetPtr	    pVisualSet)
{
    rrScrPriv(pScreen);
    int		*new;

#ifdef RR_VALIDATE
    int	i;
    for (i = 0; i < pScrPriv->nVisualSets; i++)
	if (pVisualSet == &pScrPriv->pVisualSets[i])
	    break;

    if (i == pScrPriv->nVisualSets)
	FatalError ("Adding unregistered visual set");
#endif
    new = (int*) xrealloc (pSetOfVisualSet->sets, 
			   (pSetOfVisualSet->nsets + 1) * sizeof (int *));
    if (!new)
	return FALSE;
    (pSetOfVisualSet->sets = new)[pSetOfVisualSet->nsets++] = pVisualSet - pScrPriv->pVisualSets;
    return TRUE;
}

/*
 * Return true if a and b reference the same set of sets
 */

static Bool
RRSetOfVisualSetMatches (RRSetOfVisualSetPtr  a,
			 RRSetOfVisualSetPtr  b)
{
    int	ai, bi;
    
    if (a->nsets != b->nsets)
	return FALSE;
    for (ai = 0; ai < a->nsets; ai++)
    {
	for (bi = 0; bi < b->nsets; bi++)
	    if (a->sets[ai] == b->sets[bi])
		break;
	if (bi == b->nsets)
	    return FALSE;
    }
    return TRUE;
}

RRSetOfVisualSetPtr
RRRegisterSetOfVisualSet (ScreenPtr		pScreen,
			  RRSetOfVisualSetPtr	pSetOfVisualSet)
{
    rrScrPriv (pScreen);
    int			i;
    RRSetOfVisualSetPtr pNew;

    if (!pScrPriv)
    {
	RRDestroySetOfVisualSet (pScreen, pSetOfVisualSet);
	return 0;
    }
    for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
	if (RRSetOfVisualSetMatches (pSetOfVisualSet,
				     &pScrPriv->pSetsOfVisualSets[i]))
	{
	    RRDestroySetOfVisualSet (pScreen, pSetOfVisualSet);
	    pScrPriv->pSetsOfVisualSets[i].referenced = TRUE;
	    return &pScrPriv->pSetsOfVisualSets[i];
	}
    pNew = xrealloc (pScrPriv->pSetsOfVisualSets,
		     (pScrPriv->nSetsOfVisualSets + 1) * sizeof (RRSetOfVisualSet));
    if (!pNew)
    {
	RRDestroySetOfVisualSet (pScreen, pSetOfVisualSet);
	return 0;
    }
    pNew[pScrPriv->nSetsOfVisualSets++] = *pSetOfVisualSet;
    xfree (pSetOfVisualSet);
    pScrPriv->pSetsOfVisualSets = pNew;
    return &pNew[pScrPriv->nSetsOfVisualSets-1];
}

static Bool
RRSizeInfoMatches (RRSizeInfoPtr  a,
		   RRSizeInfoPtr  b)
{
    if (a->width != b->width)
	return FALSE;
    if (a->height != b->height)
	return FALSE;
    if (a->mmWidth != b->mmWidth)
	return FALSE;
    if (a->mmHeight != b->mmHeight)
	return FALSE;
    if (a->setOfVisualSets != b->setOfVisualSets)
	return FALSE;
    return TRUE;
}

RRSizeInfoPtr
RRRegisterSize (ScreenPtr	    pScreen,
		short		    width, 
		short		    height,
		short		    mmWidth,
		short		    mmHeight,
		RRSetOfVisualSet    *pSetOfVisualSets)
{
    rrScrPriv (pScreen);
    int		    i;
    RRSizeInfo	    tmp;
    RRSizeInfoPtr   pNew;

    if (!pScrPriv)
	return 0;
    
#ifdef RR_VALIDATE
    for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
	if (pSetOfVisualSets == &pScrPriv->pSetsOfVisualSets[i])
	    break;

    if (i == pScrPriv->nSetsOfVisualSets)
	FatalError ("Adding unregistered set of visual sets");
#endif
    
    tmp.width = width;
    tmp.height= height;
    tmp.mmWidth = mmWidth;
    tmp.mmHeight = mmHeight;
    tmp.setOfVisualSets = pSetOfVisualSets - pScrPriv->pSetsOfVisualSets;
    tmp.referenced = TRUE;
    tmp.oldReferenced = FALSE;
    for (i = 0; i < pScrPriv->nSizes; i++)
	if (RRSizeInfoMatches (&tmp, &pScrPriv->pSizes[i]))
	{
	    pScrPriv->pSizes[i].referenced = TRUE;
	    return &pScrPriv->pSizes[i];
	}
    pNew = xrealloc (pScrPriv->pSizes,
		     (pScrPriv->nSizes + 1) * sizeof (RRSizeInfo));
    if (!pNew)
	return 0;
    pNew[pScrPriv->nSizes++] = tmp;
    pScrPriv->pSizes = pNew;
    return &pNew[pScrPriv->nSizes-1];
}
