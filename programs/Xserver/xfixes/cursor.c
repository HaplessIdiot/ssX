/*
 * $XFree86: $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "xfixesint.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixevents.h"
#include "servermd.h"

static RESTYPE		CursorClientType;
static RESTYPE		CursorWindowType;
static int		CursorScreenPrivateIndex = -1;
static int		CursorGeneration;
static CursorPtr	CursorCurrent;

/*
 * There is a global list of windows selecting for cursor events
 */

typedef struct _CursorEvent *CursorEventPtr;

typedef struct _CursorEvent {
    CursorEventPtr	next;
    CARD32		eventMask;
    ClientPtr		pClient;
    WindowPtr		pWindow;
    XID			clientResource;
} CursorEventRec;

static CursorEventPtr	    cursorEvents;

/*
 * Wrap DisplayCursor to catch cursor change events
 */

typedef struct _CursorScreen {
    DisplayCursorProcPtr	DisplayCursor;
    CloseScreenProcPtr		CloseScreen;
} CursorScreenRec, *CursorScreenPtr;

#define GetCursorScreen(s)	((CursorScreenPtr) ((s)->devPrivates[CursorScreenPrivateIndex].ptr))
#define GetCursorScreenIfSet(s) ((CursorScreenPrivateIndex != -1) ? GetCursorScreen(s) : NULL)
#define SetCursorScreen(s,p)	((s)->devPrivates[CursorScreenPrivateIndex].ptr = (pointer) (p))
#define Wrap(as,s,elt,func)	(((as)->elt = (s)->elt), (s)->elt = func)
#define Unwrap(as,s,elt)	((s)->elt = (as)->elt)

static Bool
CursorDisplayCursor (ScreenPtr pScreen,
		     CursorPtr pCursor)
{
    CursorScreenPtr	cs = GetCursorScreen(pScreen);
    Bool		ret;

    ErrorF ("CursorDisplayCursor %d\n", pCursor->serialNumber);
    Unwrap (cs, pScreen, DisplayCursor);
    ret = (*pScreen->DisplayCursor) (pScreen, pCursor);
    if (pCursor != CursorCurrent)
    {
	CursorEventPtr	e;

	CursorCurrent = pCursor;
	for (e = cursorEvents; e; e = e->next)
	{
	    if (e->eventMask & XFixesDisplayCursorNotifyMask)
	    {
		xXFixesCursorNotifyEvent	ev;
		ErrorF ("Sending cursor event to win 0x%x id 0x%x\n",
			e->pWindow->drawable.id, e->clientResource);
		ev.type = XFixesEventBase + XFixesCursorNotify;
		ev.subtype = XFixesDisplayCursorNotify;
		ev.sequenceNumber = e->pClient->sequence;
		ev.window = e->pWindow->drawable.id;
		ev.cursorSerial = pCursor->serialNumber;
		ev.timestamp = currentTime.milliseconds;
		WriteEventsToClient (e->pClient, 1, (xEvent *) &ev);
	    }
	}
    }
    Wrap (cs, pScreen, DisplayCursor, CursorDisplayCursor);
    return ret;
}

static Bool
CursorCloseScreen (int index, ScreenPtr pScreen)
{
    CursorScreenPtr	cs = GetCursorScreen (pScreen);
    Bool		ret;

    Unwrap (cs, pScreen, CloseScreen);
    Unwrap (cs, pScreen, DisplayCursor);
    ret = (*pScreen->CloseScreen) (index, pScreen);
    xfree (cs);
    if (index == 0)
	CursorScreenPrivateIndex = -1;
    return ret;
}

#define CursorAllEvents (XFixesDisplayCursorNotifyMask)

static int
XFixesSelectCursorInput (ClientPtr	pClient,
			 WindowPtr	pWindow,
			 CARD32		eventMask)
{
    CursorEventPtr	*prev, e;

    ErrorF("SelectCursor input mask 0x%x\n", eventMask);
    for (prev = &cursorEvents; (e = *prev); prev = &e->next)
    {
	if (e->pClient == pClient &&
	    e->pWindow == pWindow)
	{
	    break;
	}
    }
    if (!eventMask)
    {
	if (e)
	{
	    ErrorF("Freeing existing event record 0x%x\n", e->clientResource);
	    FreeResource (e->clientResource, 0);
	}
	return Success;
    }
    if (!e)
    {
	e = (CursorEventPtr) xalloc (sizeof (CursorEventRec));
	if (!e)
	    return BadAlloc;

	e->next = 0;
	e->pClient = pClient;
	e->pWindow = pWindow;
	e->clientResource = FakeClientID(pClient->index);
	ErrorF ("Adding new event record 0x%x\n", e->clientResource);

	/*
	 * Add a resource hanging from the window to
	 * catch window destroy
	 */
	if (!LookupIDByType(pWindow->drawable.id, CursorWindowType))
	    if (!AddResource (pWindow->drawable.id, CursorWindowType,
			      (pointer) pWindow))
	    {
		xfree (e);
		return BadAlloc;
	    }

	if (!AddResource (e->clientResource, CursorClientType, (pointer) e))
	    return BadAlloc;

	*prev = e;
    }
    e->eventMask = eventMask;
    return Success;
}

int
ProcXFixesSelectCursorInput (ClientPtr client)
{
    REQUEST (xXFixesSelectCursorInputReq);
    WindowPtr	pWin;

    REQUEST_SIZE_MATCH (xXFixesSelectCursorInputReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityReadAccess);
    if (!pWin)
        return(BadWindow);
    if (stuff->eventMask & ~CursorAllEvents)
    {
	client->errorValue = stuff->eventMask;
	return( BadValue );
    }
    return XFixesSelectCursorInput (client, pWin, stuff->eventMask);
}

static int
GetBit (unsigned char *line, int x)
{
    unsigned char   mask;
    
    if (screenInfo.bitmapBitOrder == LSBFirst)
	mask = (1 << (x & 7));
    else
	mask = (0x80 >> (x & 7));
    /* XXX assumes byte order is host byte order */
    line += (x >> 3);
    if (*line & mask)
	return 1;
    return 0;
}
int
SProcXFixesSelectCursorInput (ClientPtr client)
{
    register int n;
    REQUEST(xXFixesSelectCursorInputReq);

    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    swapl(&stuff->eventMask, n);
    return ProcXFixesSelectCursorInput(client);
}
    
void
SXFixesCursorNotifyEvent (xXFixesCursorNotifyEvent *from,
			  xXFixesCursorNotifyEvent *to)
{
    to->type = from->type;
    cpswaps (from->sequenceNumber, to->sequenceNumber);
    cpswapl (from->window, to->window);
    cpswapl (from->cursorSerial, to->cursorSerial);
    cpswapl (from->timestamp, to->timestamp);
}


int
ProcXFixesGetCursorImage (ClientPtr client)
{
/*    REQUEST(xXFixesGetCursorImageReq); */
    xXFixesGetCursorImageReply	*rep;
    CursorPtr			pCursor;
    CARD32			*image;
    int				npixels;
    int				width, height;
    int				x, y;

    REQUEST_SIZE_MATCH(xXFixesGetCursorImageReq);
    pCursor = CursorCurrent;
    if (!pCursor)
	return BadCursor;
    GetSpritePosition (&x, &y);
    width = pCursor->bits->width;
    height = pCursor->bits->height;
    npixels = width * height;
    rep = xalloc (sizeof (xXFixesGetCursorImageReply) +
		  npixels * sizeof (CARD32));
    if (!rep)
	return BadAlloc;

    rep->type = X_Reply;
    rep->sequenceNumber = client->sequence;
    rep->length = npixels;
    rep->width = width;
    rep->height = height;
    rep->x = x;
    rep->y = y;
    rep->xhot = pCursor->bits->xhot;
    rep->yhot = pCursor->bits->yhot; 
    rep->cursorSerial = pCursor->serialNumber;

    image = (CARD32 *) (rep + 1);
    if (pCursor->bits->argb)
	memcpy (image, pCursor->bits->argb, npixels * sizeof (CARD32));
    else
    {
	unsigned char	*srcLine = pCursor->bits->source;
	unsigned char	*mskLine = pCursor->bits->mask;
	int		stride = BitmapBytePad (width);
	int		x, y;
	CARD32		fg, bg;
	
	fg = (0xff000000 | 
	      ((pCursor->foreRed & 0xff00) << 8) |
	      (pCursor->foreGreen & 0xff00) |
	      (pCursor->foreBlue >> 8));
	bg = (0xff000000 | 
	      ((pCursor->backRed & 0xff00) << 8) |
	      (pCursor->backGreen & 0xff00) |
	      (pCursor->backBlue >> 8));
	for (y = 0; y < height; y++)
	{
	    for (x = 0; x < width; x++)
	    {
		if (GetBit (mskLine, x))
		{
		    if (GetBit (srcLine, x))
			*image++ = fg;
		    else
			*image++ = bg;
		}
		else
		    *image++ = 0;
	    }
	    srcLine += stride;
	    mskLine += stride;
	}
	image = (CARD32 *) (rep + 1);
    }
    if (client->swapped)
    {
	int n;
	swaps (&rep->sequenceNumber, n);
	swapl (&rep->length, n);
	swaps (&rep->x, n);
	swaps (&rep->y, n);
	swaps (&rep->width, n);
	swaps (&rep->height, n);
	swaps (&rep->xhot, n);
	swaps (&rep->yhot, n);
	swapl (&rep->cursorSerial, n);
	SwapLongs (image, npixels);
    }
    (void) WriteToClient(client, sizeof (xXFixesGetCursorImageReply) +
			 (npixels << 2), (char *) rep);
    xfree (rep);
    return client->noClientException;
}

int
SProcXFixesGetCursorImage (ClientPtr client)
{
    int n;
    REQUEST(xXFixesGetCursorImageReq);
    swaps (&stuff->length, n);
    return ProcXFixesGetCursorImage (client);
}

static int
CursorFreeClient (pointer data, XID id)
{
    CursorEventPtr	old = (CursorEventPtr) data;
    CursorEventPtr	*prev, e;
    
    ErrorF ("CursorFreeClient xid 0x%x\n", id);
    for (prev = &cursorEvents; (e = *prev); prev = &e->next)
    {
	if (e == old)
	{
	    ErrorF ("Found existing cursor event record\n");
	    *prev = e->next;
	    xfree (e);
	    break;
	}
    }
    return 1;
}

static int
CursorFreeWindow (pointer data, XID id)
{
    WindowPtr		pWindow = (WindowPtr) data;
    CursorEventPtr	e, next;

    ErrorF ("CursorFreeWindow xid 0x%x\n", id);
    for (e = cursorEvents; e; e = next)
    {
	next = e->next;
	if (e->pWindow == pWindow)
	{
	    ErrorF ("Found matching cursor event record id 0x%x\n",
		    e->clientResource);
	    FreeResource (e->clientResource, 0);
	}
    }
    return 1;
}

Bool
XFixesCursorInit (void)
{
    int	i;
    
    if (CursorGeneration != serverGeneration)
    {
	CursorScreenPrivateIndex = AllocateScreenPrivateIndex ();
	if (CursorScreenPrivateIndex < 0)
	    return FALSE;
	CursorGeneration = serverGeneration;
    }
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	ScreenPtr	pScreen = screenInfo.screens[i];
	CursorScreenPtr	cs;

	cs = (CursorScreenPtr) xalloc (sizeof (CursorScreenRec));
	if (!cs)
	    return FALSE;
	Wrap (cs, pScreen, CloseScreen, CursorCloseScreen);
	Wrap (cs, pScreen, DisplayCursor, CursorDisplayCursor);
	SetCursorScreen (pScreen, cs);
    }
    CursorClientType = CreateNewResourceType(CursorFreeClient);
    CursorWindowType = CreateNewResourceType(CursorFreeWindow);
    return CursorClientType && CursorWindowType;
}
