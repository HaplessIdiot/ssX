/*
 * $XFree86: xc/programs/Xserver/randr/randr.c,v 1.14tsi Exp $
 *
 * Copyright © 2000, Compaq Computer Corporation, 
 * Copyright © 2002, Hewlett Packard, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq or HP not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  HP makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * HP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL HP
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, HP Labs, Hewlett-Packard, Inc.
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
#include "render.h" 	/* we share subpixel order information */
#include "picturestr.h"
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
    int		mask;
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
    xfree (pScrPriv);
    RRNScreens -= 1;	/* ok, one fewer screen with RandR running */
    return (*pScreen->CloseScreen) (i, pScreen);    
}

static void
SRRScreenChangeNotifyEvent(xRRScreenChangeNotifyEvent *from,
			   xRRScreenChangeNotifyEvent *to)
{
    to->type = from->type;
    to->rotation = from->rotation;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    cpswaps(from->sizeID, to->sizeID);
    cpswaps(from->widthInPixels, to->widthInPixels);
    cpswaps(from->heightInPixels, to->heightInPixels);
    cpswaps(from->widthInMillimeters, to->widthInMillimeters);
    cpswaps(from->heightInMillimeters, to->heightInMillimeters);
    cpswaps(from->subpixelOrder, to->subpixelOrder);
}

Bool RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr   pScrPriv;

    if (RRGeneration != serverGeneration)
    {
	if ((rrPrivIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	RRGeneration = serverGeneration;
    }

    pScrPriv = (rrScrPrivPtr) xalloc (sizeof (rrScrPrivRec));
    if (!pScrPriv)
	return FALSE;

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
    pScrPriv->lastSetTime = currentTime;
    pScrPriv->lastConfigTime = currentTime;
    
    wrap (pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScrPriv->rotations = RR_Rotate_0;
    
    pScrPriv->nSizes = 0;
    pScrPriv->nSizesInUse = 0;
    pScrPriv->pSizes = 0;
    
    pScrPriv->rotation = RR_Rotate_0;
    pScrPriv->pSize = 0;
    
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
    RRScreenSizePtr		pSize = pScrPriv->pSize;
    WindowPtr			pRoot = WindowTable[pScreen->myNum];

    pHead = (RREventPtr *) LookupIDByType (pWin->drawable.id, EventType);
    if (!pHead)
	return WT_WALKCHILDREN;

    se.type = RRScreenChangeNotify + RREventBase;
    se.rotation = (CARD8) pScrPriv->rotation;
    se.timestamp = pScrPriv->lastSetTime.milliseconds;
    se.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
    se.root =  pRoot->drawable.id;
    se.window = pWin->drawable.id;
    se.subpixelOrder = PictureGetSubpixelOrder (pScreen);
    if (pSize)
    {
	se.sizeID = pSize->id;
	se.widthInPixels = pSize->width;
	se.heightInPixels = pSize->height;
	se.widthInMillimeters = pSize->mmWidth;
	se.heightInMillimeters = pSize->mmHeight;
    }
    else
    {
	/*
	 * This "shouldn't happen", but a broken DDX can
	 * forget to set the current configuration on GetInfo
	 */
	se.sizeID = 0xffff;
	se.widthInPixels = 0;
	se.heightInPixels = 0;
	se.widthInMillimeters = 0;
	se.heightInMillimeters = 0;
    }    
    fprintf(stderr, "size = %ld\n, widthmm = %d, heightmm=%d\n",
	    (unsigned long)sizeof(se),
	    se.widthInMillimeters, se.heightInMillimeters);
    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) 
    {
	client = pRREvent->client;
	if (client == serverClient || client->clientGone)
	    continue;
	se.sequenceNumber = client->sequence;
	if(pRREvent->mask & RRScreenChangeNotifyMask)
	  WriteEventsToClient (client, 1, (xEvent *) &se);
    }
    return WT_WALKCHILDREN;
}

static Bool
RRGetInfo (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    int		    i, j;
    Bool	    changed;
    Rotation	    rotations;

    for (i = 0; i < pScrPriv->nSizes; i++)
    {
	pScrPriv->pSizes[i].oldReferenced = pScrPriv->pSizes[i].referenced;
	pScrPriv->pSizes[i].referenced = FALSE;
    }
    if (!(*pScrPriv->rrGetInfo) (pScreen, &rotations))
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
	pScrPriv->lastConfigTime = currentTime;
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
    (void) stuff->majorVersion;
    (void) stuff->minorVersion;
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
    int		    d;

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
	for (d = 0; d < root->nDepths; d++)
	{
	    visual = (xVisualType *) ((char *) depth +
				      sizeof (xDepth));
	    depth = (xDepth *) ((char *) visual +
				depth->nVisuals * sizeof (xVisualType));
	}
	root = (xWindowRoot *) ((char *) depth);
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
    rep.pad1 = rep.pad2 = 0;
    if (!pScrPriv)
    {
	rep.type = X_Reply;
	rep.setOfRotations = RR_Rotate_0;;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.root = WindowTable[pWin->drawable.pScreen->myNum]->drawable.id;
	rep.timestamp = currentTime.milliseconds;
	rep.configTimestamp = currentTime.milliseconds;
	rep.nSizes = 0;
	rep.sizeID = 0;
	rep.rotation = RR_Rotate_0;
	extra = 0;
	extraLen = 0;
    }
    else
    {
	int			i;
	xScreenSizes		*size;
	CARD32			*data32;
    
	RRGetInfo (pScreen);

	rep.type = X_Reply;
	rep.setOfRotations = pScrPriv->rotations;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.root = WindowTable[pWin->drawable.pScreen->myNum]->drawable.id;
	rep.timestamp = pScrPriv->lastSetTime.milliseconds;
	rep.configTimestamp = pScrPriv->lastConfigTime.milliseconds;
	
	rep.rotation = pScrPriv->rotation;
	rep.nSizes = pScrPriv->nSizesInUse;
	if (pScrPriv->pSize)
	    rep.sizeID = pScrPriv->pSize->id;
	else
	    return BadImplementation;

	extraLen = rep.nSizes * sizeof (xScreenSizes) ;

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
		if (client->swapped)
		{
		    swaps (&size->widthInPixels, n);
		    swaps (&size->heightInPixels, n);
		    swaps (&size->widthInMillimeters, n);
		    swaps (&size->heightInMillimeters, n);
		}
		size++;
	    }
	}
	data32 = (CARD32 *) size;

	if ((CARD8 *) data32 - (CARD8 *) extra != extraLen)
	    FatalError ("RRGetScreenInfo bad extra len %d != %d\n",
			(CARD8 *) data32 - (CARD8 *) extra, extraLen);
	rep.length =  (extraLen + 3) >> 2;
    }
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.timestamp, n);
	swaps(&rep.rotation, n);
	swaps(&rep.nSizes, n);
	swaps(&rep.sizeID, n);
    }
    WriteToClient(client, sizeof(xRRGetScreenInfoReply), (char *)&rep);
    if (extraLen)
    {
	WriteToClient (client, extraLen, (char *) extra);
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
    TimeStamp		    configTime;
    TimeStamp		    time;
    RRScreenSizePtr	    pSize;
    int			    i;
    Rotation		    rotation;
    short		    oldWidth, oldHeight;

    UpdateCurrentTime ();

    REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
    SECURITY_VERIFY_DRAWABLE(pDraw, stuff->drawable, client,
			     SecurityWriteAccess);

    pScreen = pDraw->pScreen;

    pScrPriv= rrGetScrPriv(pScreen);
    
    time = ClientTimeToServerTime(stuff->timestamp);
    configTime = ClientTimeToServerTime(stuff->configTimestamp);
    
    oldWidth = pScreen->width;
    oldHeight = pScreen->height;
    
    if (!pScrPriv)
    {
	time = currentTime;
	rep.status = RRSetConfigFailed;
	goto sendReply;
    }
    if (!RRGetInfo (pScreen))
	return BadAlloc;
    
    /*
     * if the client's config timestamp is not the same as the last config
     * timestamp, then the config information isn't up-to-date and
     * can't even be validated
     */
    if (CompareTimeStamps (configTime, pScrPriv->lastConfigTime) != 0)
    {
	rep.status = RRSetConfigInvalidConfigTime;
	goto sendReply;
    }
    
    /*
     * Search for the requested size
     */
    pSize = 0;
    for (i = 0; i < pScrPriv->nSizes; i++)
    {
	pSize = &pScrPriv->pSizes[i];
	if (pSize->referenced && pSize->id == stuff->sizeID)
	    break;
    }
    if (i == pScrPriv->nSizes)
    {
	/*
	 * Invalid size ID
	 */
	client->errorValue = stuff->sizeID;
	return BadValue;
    }
    
    /*
     * Validate requested rotation
     */
    rotation = (Rotation) stuff->rotation;

    /* test the rotation bits only! */
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_90:
    case RR_Rotate_180:
    case RR_Rotate_270:
	break;
    default:
	/*
	 * Invalid rotation
	 */
	client->errorValue = stuff->rotation;
	return BadValue;
    }

    if ((~pScrPriv->rotations) & rotation)
    {
	/*
	 * requested rotation or reflection not supported by screen
	 */
	client->errorValue = stuff->rotation;
	return BadMatch;
    }

    /*
     * Make sure the requested set-time is not older than
     * the last set-time
     */
    if (CompareTimeStamps (time, pScrPriv->lastSetTime) < 0)
    {
	rep.status = RRSetConfigInvalidTime;
	goto sendReply;
    }

    /*
     * call out to ddx routine to effect the change
     */
    if (!(*pScrPriv->rrSetConfig) (pScreen, rotation, 
				   pSize))
    {
	/*
	 * unknown DDX failure, report to client
	 */
	rep.status = RRSetConfigFailed;
	goto sendReply;
    }
    
    /*
     * set current extension configuration pointers
     */
    RRSetCurrentConfig (pScreen, rotation, pSize);
    
    /*
     * Deliver ScreenChangeNotify events whenever
     * the configuration is updated
     */
    WalkTree (pScreen, TellChanged, (pointer) pScreen);
    
    /*
     * Deliver ConfigureNotify events when root changes
     * pixel size
     */
    if (oldWidth != pScreen->width || oldHeight != pScreen->height)
	RRSendConfigNotify (pScreen);
    RREditConnectionInfo (pScreen);
    
    /*
     * Fix pointer bounds and location
     */
    ScreenRestructured (pScreen);
    pScrPriv->lastSetTime = time;
    
    /*
     * Report Success
     */
    rep.status = RRSetConfigSuccess;
    
sendReply:
    
    rep.type = X_Reply;
    /* rep.status has already been filled in */
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.newTimestamp = pScrPriv->lastSetTime.milliseconds;
    rep.newConfigTimestamp = pScrPriv->lastConfigTime.milliseconds;
    rep.root = WindowTable[pDraw->pScreen->myNum]->drawable.id;

    if (client->swapped) 
    {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.newTimestamp, n);
	swapl(&rep.newConfigTimestamp, n);
	swapl(&rep.root, n);
    }
    WriteToClient(client, sizeof(xRRSetScreenConfigReply), (char *)&rep);

    return (client->noClientException);
}

static int
ProcRRSelectInput (ClientPtr client)
{
    REQUEST(xRRSelectInputReq);
    WindowPtr	pWin;
    RREventPtr	pRREvent, pNewRREvent, *pHead;
    XID		clientResource;

    REQUEST_SIZE_MATCH(xRRSelectInputReq);
    pWin = SecurityLookupWindow (stuff->window, client, SecurityWriteAccess);
    if (!pWin)
	return BadWindow;
    pHead = (RREventPtr *)SecurityLookupIDByType(client,
			pWin->drawable.id, EventType, SecurityWriteAccess);
    

      if (stuff->enable & 
	  (RRScreenChangeNotifyMask)) {

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
	pNewRREvent->mask = stuff->enable;
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
      }
      else if (stuff->enable == xFalse) {
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
      }
      else {
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
    case X_RRSetScreenConfig:
        return ProcRRSetScreenConfig(client);
    case X_RRSelectInput:
        return ProcRRSelectInput(client);
    case X_RRGetScreenInfo:
        return ProcRRGetScreenInfo(client);
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
    swaps(&stuff->sizeID, n);
    swaps(&stuff->rotation, n);
    return ProcRRSetScreenConfig(client);
}

static int
SProcRRSelectInput (ClientPtr client)
{
    register int n;
    REQUEST(xRRSelectInputReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return ProcRRSelectInput(client);
}


static int
SProcRRDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RRQueryVersion:
	return SProcRRQueryVersion(client);
    case X_RRSetScreenConfig:
        return SProcRRSetScreenConfig(client);
    case X_RRSelectInput:
        return SProcRRSelectInput(client);
    case X_RRGetScreenInfo:
        return SProcRRGetScreenInfo(client);
    default:
	return BadRequest;
    }
}


static Bool
RRScreenSizeMatches (RRScreenSizePtr  a,
		   RRScreenSizePtr  b)
{
    if (a->width != b->width)
	return FALSE;
    if (a->height != b->height)
	return FALSE;
    if (a->mmWidth != b->mmWidth)
	return FALSE;
    if (a->mmHeight != b->mmHeight)
	return FALSE;
    return TRUE;
}

RRScreenSizePtr
RRRegisterSize (ScreenPtr	    pScreen,
		short		    width, 
		short		    height,
		short		    mmWidth,
		short		    mmHeight)
{
    rrScrPriv (pScreen);
    int		    i;
    RRScreenSize	    tmp;
    RRScreenSizePtr   pNew;

    if (!pScrPriv)
	return 0;
    
    tmp.width = width;
    tmp.height= height;
    tmp.mmWidth = mmWidth;
    tmp.mmHeight = mmHeight;
    tmp.referenced = TRUE;
    tmp.oldReferenced = FALSE;
    for (i = 0; i < pScrPriv->nSizes; i++)
	if (RRScreenSizeMatches (&tmp, &pScrPriv->pSizes[i]))
	{
	    pScrPriv->pSizes[i].referenced = TRUE;
	    return &pScrPriv->pSizes[i];
	}
    pNew = xrealloc (pScrPriv->pSizes,
		     (pScrPriv->nSizes + 1) * sizeof (RRScreenSize));
    if (!pNew)
	return 0;
    pNew[pScrPriv->nSizes++] = tmp;
    pScrPriv->pSizes = pNew;
    return &pNew[pScrPriv->nSizes-1];
}

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    RRScreenSizePtr	pSize)
{
    rrScrPriv (pScreen);

    if (!pScrPriv)
	return;

    pScrPriv->rotation = rotation;
    pScrPriv->pSize = pSize;
}
