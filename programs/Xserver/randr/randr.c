/*
 * $XFree86: xc/programs/Xserver/randr/randr.c,v 1.2 2001/05/23 04:15:06 keithp Exp $
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
    if (pScrPriv->pSizes)
	xfree (pScrPriv->pSizes);
    if (pScrPriv->pSetsOfVisualSets)
	xfree (pScrPriv->pSetsOfVisualSets);
    if (pScrPriv->pVisualSets)
	xfree (pScrPriv->pVisualSets);
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

    /*
     * Calling function best set these function vectors
     */
    pScrPriv->rrSetConfig = 0;
    pScrPriv->rrGetInfo = 0;
    /*
     * This value doesn't really matter -- any client must call
     * GetScreenInfo before reading it which will automatically update
     * the time
     */
    pScrPriv->lastSetTime.milliseconds = 0;
    pScrPriv->lastSetTime.months = 0;
    
    wrap (pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScrPriv->rotations = RR_ROTATE_0;
    pScrPriv->swaps = 0;
    pScrPriv->nVisualSets = 0;
    pScrPriv->nVisualSetsInUse = 0;
    pScrPriv->pVisualSets = 0;
    
    pScrPriv->nSetsOfVisualSets = 0;
    pScrPriv->nSetsOfVisualSetsInUse = 0;
    pScrPriv->pSetsOfVisualSets = 0;
    
    pScrPriv->nSizes = 0;
    pScrPriv->nSizesInUse = 0;
    pScrPriv->pSizes = 0;
    
    pScrPriv->rotation = RR_ROTATE_0;
    pScrPriv->swap = 0;
    pScrPriv->pSize = 0;
    pScrPriv->pVisualSet = 0;
    
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
	se.timestamp = pScrPriv->lastSetTime.milliseconds;
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
	pScrPriv->lastSetTime = currentTime;
	WalkTree (pScreen, TellChanged, (pointer) pScreen);
    }
    return TRUE;
}

static void
RRSendConfigNotify (ScreenPtr pScreen)
{
    WindowPtr	pWin = WindowTable[pScreen->myNum];
    xEvent	event;

    event.u.u.type = ConfigureNotify;
    event.u.configureNotify.window = pWin->drawable.id;
    event.u.configureNotify.aboveSibling = None;
    event.u.configureNotify.x = 0;
    event.u.configureNotify.y = 0;

    /* XXX xinerama stuff ? */
    
    event.u.configureNotify.width = pWin->drawable.width;
    event.u.configureNotify.height = pWin->drawable.height;
    event.u.configureNotify.borderWidth = wBorderWidth (pWin);
    event.u.configureNotify.override = pWin->overrideRedirect;
    DeliverEvents(pWin, &event, 1, NullWindow);
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

static Bool
RRVisualSetContains (RRVisualSetPtr pVisualSet,
		     VisualID	    visual)
{
    int	    i;

    for (i = 0; i < pVisualSet->nvisuals; i++)
	if (pVisualSet->visuals[i]->vid == visual)
	    return TRUE;
    return FALSE;
}

static CARD16
RRNumMatchingVisualSets (ScreenPtr  pScreen,
			 VisualID   visual)
{
    rrScrPriv(pScreen);
    int		    i;
    CARD16	    n = 0;
    RRVisualSetPtr  pVisualSet;

    for (i = 0; i < pScrPriv->nVisualSets; i++)
    {
	pVisualSet = &pScrPriv->pVisualSets[i];
	if (pVisualSet->referenced && RRVisualSetContains (pVisualSet, visual))
	    n++;
    }
    return n;
}

static void
RRGetMatchingVisualSets (ScreenPtr	pScreen,
			 VisualID	visual,
			 VISUALSETID	*pVisualSetIDs)
{
    rrScrPriv(pScreen);
    int		    i;
    CARD16	    n = 0;
    RRVisualSetPtr  pVisualSet;

    for (i = 0; i < pScrPriv->nVisualSets; i++)
    {
	pVisualSet = &pScrPriv->pVisualSets[i];
	if (pVisualSet->referenced && RRVisualSetContains (pVisualSet, visual))
	    *pVisualSetIDs++ = pVisualSet->id;
    }
}

extern char	*ConnectionInfo;

static int padlength[4] = {0, 3, 2, 1};

static void
RREditConnectionInfo (ScreenPtr pScreen)
{
    xConnSetup	    *connSetup;
    char	    *vendor;
    xPixmapFormat   *formats;
    xWindowRoot	    *root;
    xDepth	    *depth;
    xVisualType	    *visual;
    int		    screen = 0;

    connSetup = (xConnSetup *) ConnectionInfo;
    vendor = (char *) connSetup + sizeof (xConnSetup);
    formats = (xPixmapFormat *) ((char *) vendor +
				 connSetup->nbytesVendor +
				 padlength[connSetup->nbytesVendor & 3]);
    root = (xWindowRoot *) ((char *) formats +
			    sizeof (xPixmapFormat) * screenInfo.numPixmapFormats);
    while (screen != pScreen->myNum)
    {
	depth = (xDepth *) ((char *) root + 
			    sizeof (xWindowRoot));
	visual = (xVisualType *) ((char *) depth +
				  sizeof (xDepth));
	root = (xWindowRoot *) ((char *) visual + 
				depth->nVisuals * sizeof (xVisualType));
	screen++;
    }
    root->pixWidth = pScreen->width;
    root->pixHeight = pScreen->height;
    root->mmWidth = pScreen->mmWidth;
    root->mmHeight = pScreen->mmHeight;
}

static int
ProcRRGetScreenInfo (ClientPtr client)
{
    REQUEST(xRRGetScreenInfoReq);
    xRRGetScreenInfoReply   rep;
    WindowPtr	    	    pWin;
    int			    n;
    ScreenPtr		    pScreen;
    rrScrPrivPtr	    pScrPriv;
    CARD8		    *extra;
    int			    extraLen;

    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);

    if (!pWin)
	return BadWindow;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    if (!pScrPriv)
    {
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.timestamp = currentTime.milliseconds;
	rep.rotation = 0;
	rep.nSizes = 0;
	rep.nVisualSets = 0;
	rep.nAccelerated = 0;
	rep.nRotations = 0;
	rep.sizeSetID = 0;
	rep.visualSetID = 0;
	extra = 0;
	extraLen = 0;
    }
    else
    {
	int		    i, j;
	int		    nSetOfVisualSetsElements;
	RRSetOfVisualSetPtr pSetOfVisualSets;
	int		    nVisualSetElements;
	RRVisualSetPtr	    pVisualSet;
	xScreenSizes	    *size;
	CARD16		    *data16;
	CARD32		    *data32;
    
	RRGetInfo (pScreen);

	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.timestamp = pScrPriv->lastSetTime.milliseconds;
	rep.rotation = pScrPriv->rotation;
	rep.nSizes = pScrPriv->nSizesInUse;
	rep.nVisualSets = pScrPriv->nVisualSetsInUse;
	rep.nSetsOfVisualSets = pScrPriv->nSetsOfVisualSetsInUse;
	rep.nAccelerated = RRNumMatchingVisualSets (pScreen, wVisual(pWin));
	rep.nRotations = Ones (pScrPriv->rotations);
	if (pScrPriv->pSize)
	    rep.sizeSetID = pScrPriv->pSize->id;
	else
	    return BadImplementation;
	if (pScrPriv->pVisualSet)
	    rep.visualSetID = pScrPriv->pVisualSet->id;
	else
	    return BadImplementation;
	/*
	 * Count up the total number of spaces needed to transmit
	 * the sets of visual sets
	 */
	nSetOfVisualSetsElements = 0;
	for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
	{
	    pSetOfVisualSets = &pScrPriv->pSetsOfVisualSets[i];
	    if (pSetOfVisualSets->referenced)
		nSetOfVisualSetsElements += pSetOfVisualSets->nsets + 1;
	}
	/*
	 * Count up the total number of spaces needed to transmit
	 * the visual sets
	 */
	nVisualSetElements = 0;
	for (i = 0; i < pScrPriv->nVisualSets; i++)
	{
	    pVisualSet = &pScrPriv->pVisualSets[i];
	    if (pVisualSet->referenced)
		nVisualSetElements += pVisualSet->nvisuals + 1;
	}
	/*
	 * Allocate space for the extra information
	 */
	extraLen = (rep.nSizes * sizeof (xScreenSizes) +
		    nVisualSetElements * sizeof (CARD32) +
		    nSetOfVisualSetsElements * sizeof (CARD16) +
		    rep.nAccelerated * sizeof (CARD16) +
		    rep.nRotations * sizeof (CARD16));
	extra = (CARD8 *) xalloc (extraLen);
	if (!extra)
	    return BadAlloc;
	/*
	 * First comes the size information
	 */
	size = (xScreenSizes *) extra;
	for (i = 0; i < pScrPriv->nSizes; i++)
	{
	    if (pScrPriv->pSizes[i].referenced)
	    {
		size->widthInPixels = pScrPriv->pSizes[i].width;
		size->heightInPixels = pScrPriv->pSizes[i].height;
		size->widthInMillimeters = pScrPriv->pSizes[i].mmWidth;
		size->heightInMillimeters = pScrPriv->pSizes[i].mmHeight;
		size->visualGroup = pScrPriv->pSetsOfVisualSets[pScrPriv->pSizes[i].setOfVisualSets].id;
		if (client->swapped)
		{
		    swaps (&size->widthInPixels, n);
		    swaps (&size->heightInPixels, n);
		    swaps (&size->widthInMillimeters, n);
		    swaps (&size->heightInMillimeters, n);
		    swaps (&size->visualGroup, n);
		}
		size++;
	    }
	}
	data32 = (CARD32 *) size;
	/*
	 * Next comes the visual sets
	 */
	for (i = 0; i < pScrPriv->nVisualSets; i++)
	{
	    pVisualSet = &pScrPriv->pVisualSets[i];
	    if (pVisualSet->referenced)
	    {
		*data32++ = pVisualSet->nvisuals;
		for (j = 0; j < pVisualSet->nvisuals; j++)
		    *data32++ = pVisualSet->visuals[j]->vid;
	    }
	}
	if (client->swapped)
	    SwapLongs (data32 - nVisualSetElements, nVisualSetElements);
	/*
	 * Next comes the sets of visual sets
	 */
	data16 = (CARD16 *) data32;
	for (i = 0; i < pScrPriv->nSetsOfVisualSets; i++)
	{
	    pSetOfVisualSets = &pScrPriv->pSetsOfVisualSets[i];
	    if (pSetOfVisualSets->referenced)
	    {
		*data16++ = (CARD16) pSetOfVisualSets->nsets;
		for (j = 0; j < pSetOfVisualSets->nsets; j++)
		{
		    pVisualSet = &pScrPriv->pVisualSets[pSetOfVisualSets->sets[j]];
		    *data16++ = (CARD16) pVisualSet->id;
		}
	    }
	}
	/*
	 * Now for the list of possible rotations
	 */
	if (pScrPriv->rotations & RR_ROTATE_0)
	    *data16++ = 0;
	if (pScrPriv->rotations & RR_ROTATE_90)
	    *data16++ = 90;
	if (pScrPriv->rotations & RR_ROTATE_180)
	    *data16++ = 180;
	if (pScrPriv->rotations & RR_ROTATE_270)
	    *data16++ = 270;
	/*
	 * Finally, the list of accelerated visual sets
	 */
	RRGetMatchingVisualSets (pScreen, wVisual (pWin), 
				 (VISUALSETID *) data16);
	data16 += rep.nAccelerated;
	if (client->swapped)
	    SwapShorts ((CARD16 *) data32, data16 - (CARD16 *) data32);
	if ((CARD8 *) data16 - (CARD8 *) extra != extraLen)
	    FatalError ("RRGetScreenInfo bad extra len %d != %d\n",
			(CARD8 *) data16 - (CARD8 *) extra, extraLen);
	rep.length = extraLen >> 2;
    }
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.timestamp, n);
	swaps(&rep.rotation, n);
	swaps(&rep.nSizes, n);
	swaps(&rep.nVisualSets, n);
	swaps(&rep.nAccelerated, n);
	swaps(&rep.nRotations, n);
	swaps(&rep.sizeSetID, n);
	swaps(&rep.visualSetID, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenInfoReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, extra);
	xfree (extra);
    }
    return (client->noClientException);
}

static int
ProcRRSetScreenConfig (ClientPtr client)
{
    REQUEST(xRRSetScreenConfigReq);
    xRRSetScreenConfigReply rep;
    DrawablePtr		    pDraw;
    int			    n;
    ScreenPtr		    pScreen;
    rrScrPrivPtr	    pScrPriv;
    TimeStamp		    time;

    REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
    SECURITY_VERIFY_DRAWABLE(pDraw, stuff->drawable, client,
			     SecurityWriteAccess);

    fprintf(stderr, "got to procRRSetScreenConfig, draw = %d, time = %d, sizeseti = %d, visualseti = %d, rotation = %d\n",
	    pDraw,
	    stuff->timestamp,
	    stuff->sizeSetIndex,
	    stuff->visualSetIndex,
	    stuff->rotation);

    pScreen = pDraw->pScreen;

    pScrPriv= rrGetScrPriv(pScreen);
    
    if (!pScrPriv)
    {
	time = currentTime;
    }
    else
    {
	if (!RRGetInfo (pScreen))
	    return BadAlloc;

	time = ClientTimeToServerTime(stuff->timestamp);
    
	/* 
	 * if the client's time stamp is not the same as the last time
	 * the screen information changed, then ignore the request
	 */
    
	if (CompareTimeStamps (time, pScrPriv->lastSetTime) == 0)
	{
	    RRSizeInfoPtr	pSize;
	    RRVisualSetPtr	pVisualSet;
	    RRSetOfVisualSetPtr	pSetOfVisualSets;
	    int			i;
	    int			rotation;
	    short		oldWidth, oldHeight;

	    for (i = 0; i < pScrPriv->nSizes; i++)
	    {
		pSize = &pScrPriv->pSizes[i];
		if (pSize->referenced && pSize->id == stuff->sizeSetIndex)
		    break;
	    }
	    if (i == pScrPriv->nSizes)
	    {
		client->errorValue = stuff->sizeSetIndex;
		return BadValue;
	    }
	    for (i = 0; i < pScrPriv->nVisualSets; i++)
	    {
		pVisualSet = &pScrPriv->pVisualSets[i];
		if (pVisualSet->referenced && pVisualSet->id == stuff->visualSetIndex)
		    break;
	    }
	    if (i == pScrPriv->nVisualSets)
	    {
		client->errorValue = stuff->visualSetIndex;
		return BadValue;
	    }
	    pSetOfVisualSets = &pScrPriv->pSetsOfVisualSets[pSize->setOfVisualSets];
	    for (i = 0; i < pSetOfVisualSets->nsets; i++)
	    {
		if (pSetOfVisualSets->sets[i] == pVisualSet - pScrPriv->pVisualSets)
		    break;
	    }
	    if (i == pSetOfVisualSets->nsets)
		return BadMatch;
	    switch (stuff->rotation) {
	    case 0:	rotation = RR_ROTATE_0; break;
	    case 90:	rotation = RR_ROTATE_90; break;
	    case 180:	rotation = RR_ROTATE_180; break;
	    case 270:	rotation = RR_ROTATE_270; break;
	    default:
		client->errorValue = stuff->rotation;
		return BadValue;
	    }
	    if (!pScrPriv->rotations & rotation)
		return BadMatch;
	    
	    oldWidth = pScreen->width;
	    oldHeight = pScreen->height;
	    
	    /* call out to ddx routine */
	    if (!(*pScrPriv->rrSetConfig) (pScreen, rotation, 0, pSize, pVisualSet))
		return BadAlloc;
	    /* set current configuration */
	    RRSetCurrentConfig (pScreen, rotation, 0, pSize, pVisualSet);
	    /* tell interested clients */
	    WalkTree (pScreen, TellChanged, (pointer) pScreen);
	    if (oldWidth != pScreen->width || oldHeight != pScreen->height)
		RRSendConfigNotify (pScreen);
	    RREditConnectionInfo (pScreen);
	    ScreenRestructured (pScreen);
	}
	time = pScrPriv->lastSetTime;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.newtimestamp = time.milliseconds;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.newtimestamp, n);
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
    swapl(&stuff->window, n);
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

void
RRSetCurrentConfig (ScreenPtr	    pScreen,
		    int		    rotation,
		    int		    swap,
		    RRSizeInfoPtr   pSize,
		    RRVisualSetPtr  pVisualSet)
{
    rrScrPriv (pScreen);

    if (!pScrPriv)
	return;

    pScrPriv->rotation = rotation;
    pScrPriv->swap = swap;
    pScrPriv->pSize = pSize;
    pScrPriv->pVisualSet = pVisualSet;
}
