/* 

   XFree86 Xv DDX written by Mark Vojkovich (mvojkovi@ucsd.edu) 

*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86xv.c,v 1.5 1999/01/14 13:04:11 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "validate.h"
#include "resource.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "Xv.h"
#include "Xvproto.h"
#include "xvdix.h"
#ifdef XFree86LOADER
#include "xvmodproc.h"
#endif

#include "xf86xv.h"


/* XvScreenRec fields */

static Bool xf86XVCloseScreen(int, ScreenPtr);
static int xf86XVQueryAdaptors(ScreenPtr, XvAdaptorPtr *, int *);

/* XvAdaptorRec fields */

static int xf86XVAllocatePort(unsigned long, XvPortPtr, XvPortPtr*);
static int xf86XVFreePort(XvPortPtr);
static int xf86XVPutVideo(ClientPtr, DrawablePtr,XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
static int xf86XVPutStill(ClientPtr, DrawablePtr,XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
static int xf86XVGetVideo(ClientPtr, DrawablePtr,XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
static int xf86XVGetStill(ClientPtr, DrawablePtr,XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
static int xf86XVStopVideo(ClientPtr, XvPortPtr, DrawablePtr);
static int xf86XVSetPortAttribute(ClientPtr, XvPortPtr, Atom, INT32);
static int xf86XVGetPortAttribute(ClientPtr, XvPortPtr, Atom, INT32*);
static int xf86XVQueryBestSize(ClientPtr, XvPortPtr, CARD8,
   				CARD16, CARD16,CARD16, CARD16, 
				unsigned int*, unsigned int*);

/* ScreenRec fields */

static void xf86XVWindowExposures(WindowPtr, RegionPtr, RegionPtr);
static Bool xf86XVCreateWindow(WindowPtr pWin);
static Bool xf86XVUnrealizeWindow(WindowPtr pWin);
static void xf86XVClipNotify(WindowPtr pWin, int dx, int dy);
static void xf86XVCopyWindow(WindowPtr, DDXPointRec, RegionPtr);

/* ScrnInfoRec functions */

static Bool xf86XVEnterVT(int, int);
static void xf86XVLeaveVT(int, int);

/* misc */

static Bool xf86XVInitAdaptors(ScreenPtr, XF86VideoInfoPtr);


int XF86XVWindowIndex = -1;
int XF86XvScreenIndex = -1;
static unsigned long XF86XVGeneration = 0;
static unsigned long PortResource = 0;

#ifdef XFree86LOADER
int (*XvGetScreenIndexProc)(void) = NULL;
unsigned long (*XvGetRTPortProc)(void) = NULL;
int (*XvScreenInitProc)(ScreenPtr) = NULL;
#else
int (*XvGetScreenIndexProc)(void) = XvGetScreenIndex;
unsigned long (*XvGetRTPortProc)(void) = XvGetRTPort;
int (*XvScreenInitProc)(ScreenPtr) = XvScreenInit;
#endif


Bool
xf86XVScreenInit(
   ScreenPtr pScreen, 
   XF86VideoInfoPtr infoPtr
){
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  XF86XVScreenPtr ScreenPriv;
  XvScreenPtr pxvs;

  if(XF86XVGeneration != serverGeneration) {
	if((XF86XVWindowIndex = AllocateWindowPrivateIndex()) < 0)
	    return FALSE;
	XF86XVGeneration = serverGeneration;
  }

  if(!AllocateWindowPrivate(pScreen,XF86XVWindowIndex,sizeof(XF86XVWindowRec)))
        return FALSE;

  if(!XvGetScreenIndexProc || !XvGetRTPortProc || !XvScreenInitProc)
	return FALSE;  

  if(Success != (*XvScreenInitProc)(pScreen)) return FALSE;

  XF86XvScreenIndex = (*XvGetScreenIndexProc)();
  PortResource = (*XvGetRTPortProc)();

  pxvs = (XvScreenPtr)pScreen->devPrivates[XF86XvScreenIndex].ptr;


  /* Anyone initializing the Xv layer must provide these two.
     The Xv di layer calls them without even checking if they exist! */

  pxvs->ddCloseScreen = xf86XVCloseScreen;
  pxvs->ddQueryAdaptors = xf86XVQueryAdaptors;

  /* The Xv di layer provides us with a private hook so that we don't
     have to allocate our own screen private.  They also provide
     a CloseScreen hook so that we don't have to wrap it.  I'm not
     sure that I appreciate that.  */

  ScreenPriv = xalloc(sizeof(XF86XVScreenRec));
  pxvs->devPriv.ptr = (pointer)ScreenPriv;

  if(!ScreenPriv) return FALSE;


  ScreenPriv->ClipNotify = pScreen->ClipNotify;
  ScreenPriv->UnrealizeWindow = pScreen->UnrealizeWindow;
  ScreenPriv->WindowExposures = pScreen->WindowExposures;
  ScreenPriv->CreateWindow = pScreen->CreateWindow;
  ScreenPriv->CopyWindow = pScreen->CopyWindow;
  ScreenPriv->EnterVT = pScrn->EnterVT;
  ScreenPriv->LeaveVT = pScrn->LeaveVT;


  pScreen->ClipNotify = xf86XVClipNotify;
  pScreen->UnrealizeWindow = xf86XVUnrealizeWindow;
  pScreen->WindowExposures = xf86XVWindowExposures;
  pScreen->CreateWindow = xf86XVCreateWindow;
  pScreen->CopyWindow = xf86XVCopyWindow;
  pScrn->EnterVT = xf86XVEnterVT;
  pScrn->LeaveVT = xf86XVLeaveVT;

  if(!xf86XVInitAdaptors(pScreen, infoPtr))
	return FALSE;

  return TRUE;
}

static void
xf86XVFreeAdaptor(XvAdaptorPtr pAdaptor)
{
   int i;

   if(pAdaptor->name)
      xfree(pAdaptor->name);

   if(pAdaptor->pEncodings) {
      XvEncodingPtr pEncode = pAdaptor->pEncodings;

      for(i = 0; i < pAdaptor->nEncodings; i++, pEncode++) {
          if(pEncode->name) xfree(pEncode->name);
      }
      xfree(pAdaptor->pEncodings);
   }

   if(pAdaptor->pFormats) 
      xfree(pAdaptor->pFormats);

   if(pAdaptor->pPorts) {
      XvPortPtr pPort = pAdaptor->pPorts;
      XvPortRecPrivatePtr pPriv;

      for(i = 0; i < pAdaptor->nPorts; i++, pPort++) {
          pPriv = (XvPortRecPrivatePtr)pPort->devPriv.ptr;
	  if(pPriv) {
	     if(pPriv->clientClip) 
		REGION_DESTROY(pAdaptor->pScreen, pPriv->clientClip);
             if(pPriv->pCompositeClip && pPriv->FreeCompositeClip) 
		REGION_DESTROY(pAdaptor->pScreen, pPriv->pCompositeClip);
	     xfree(pPriv);
	  }
      }
      xfree(pAdaptor->pPorts);
   }

   if(pAdaptor->devPriv.ptr)
      xfree(pAdaptor->devPriv.ptr);
}

static Bool
xf86XVInitAdaptors(
   ScreenPtr pScreen, 
   XF86VideoInfoPtr infoPtr
) {
  XvScreenPtr pxvs = (XvScreenPtr)(pScreen->devPrivates[XF86XvScreenIndex].ptr);
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  XF86VideoAdaptorPtr adaptorPtr;
  XF86VideoEncodingPtr encodingPtr;
  XF86VideoFormatPtr formatPtr;
  XvAdaptorRecPrivatePtr adaptorPriv;
  XvPortRecPrivatePtr portPriv;
  XvAdaptorPtr pAdaptor, pa;
  int na, numAdaptor;
  XvEncodingPtr pEncode, pe;
  int ne, numEncode;
  XvFormatPtr pFormat, pf;
  int nf, numFormat, totFormat;
  XvPortPtr pPort, pp;
  int np, numPort;

  int numVisuals;
  VisualPtr pVisual;

  pxvs->nAdaptors = 0;
  pxvs->pAdaptors = NULL;

  pAdaptor = xcalloc(infoPtr->NumAdaptors, sizeof(XvAdaptorRec));
  if(!pAdaptor) return FALSE;

  for(pa = pAdaptor, na = 0, numAdaptor = 0; 
      na < infoPtr->NumAdaptors; 
      na++) {

      adaptorPtr = &infoPtr->Adaptors[na];

      if(!adaptorPtr->type) continue;

      pa->pScreen = pScreen; 
      pa->type = adaptorPtr->type; 
      pa->ddAllocatePort = xf86XVAllocatePort;
      pa->ddFreePort = xf86XVFreePort;
      pa->ddPutVideo = xf86XVPutVideo;
      pa->ddPutStill = xf86XVPutStill;
      pa->ddGetVideo = xf86XVGetVideo;
      pa->ddGetStill = xf86XVGetStill;
      pa->ddStopVideo = xf86XVStopVideo;
      pa->ddSetPortAttribute = xf86XVSetPortAttribute;
      pa->ddGetPortAttribute = xf86XVGetPortAttribute;
      pa->ddQueryBestSize = xf86XVQueryBestSize;
      if((pa->name = xalloc(strlen(adaptorPtr->name))))
          strcpy(pa->name, adaptorPtr->name);

      pEncode = xcalloc(adaptorPtr->nEncodings, sizeof(XvEncodingRec));
      if(!pEncode) {
          xf86XVFreeAdaptor(pa);
          continue;
      }
      for(pe = pEncode, ne = 0, numEncode = 0; 
	  ne < adaptorPtr->nEncodings;
	  ne++) {

          encodingPtr = &adaptorPtr->pEncodings[ne];

          pe->id = encodingPtr->id;
          pe->pScreen = pScreen;
          if((pe->name = xalloc(strlen(encodingPtr->name))))
              strcpy(pe->name, encodingPtr->name);
          pe->width = encodingPtr->width;
          pe->height = encodingPtr->height;
          pe->rate.numerator = encodingPtr->rate.numerator;
          pe->rate.denominator = encodingPtr->rate.denominator;

	  pe++;
	  numEncode++;
      }
      pa->nEncodings = numEncode;
      pa->pEncodings = pEncode;  
      if(!numEncode) {
          xf86XVFreeAdaptor(pa);
          continue;
      }

      totFormat = adaptorPtr->nFormats;

      pFormat = xcalloc(totFormat, sizeof(XvFormatRec));
      if(!pFormat) {
          xf86XVFreeAdaptor(pa);
          continue;
      }
      for(pf = pFormat, nf = 0, numFormat = 0; 
	  nf < adaptorPtr->nFormats;
	  nf++) {

          formatPtr = &adaptorPtr->pFormats[nf];

	  numVisuals = pScreen->numVisuals;
          pVisual = pScreen->visuals;

          while(numVisuals--) {
              if((pVisual->class == formatPtr->class) &&
                 (pVisual->nplanes == formatPtr->depth)) {

		   if(numFormat >= totFormat) {
			void *moreSpace; 
			totFormat *= 2;
			moreSpace = xrealloc(pFormat, totFormat);
			if(!moreSpace) break;
			pf = (XvFormatPtr)moreSpace + numFormat;
		   }

                   pf->visual = pVisual->vid; 
		   pf->depth = formatPtr->depth;

		   pf++;
		   numFormat++;
              }
              pVisual++;
          }	
      }
      pa->nFormats = numFormat;
      pa->pFormats = pFormat;  
      if(!numFormat) {
          xf86XVFreeAdaptor(pa);
          continue;
      }


      adaptorPriv = xcalloc(1, sizeof(XvAdaptorRecPrivate));
      if(!adaptorPriv) {
          xf86XVFreeAdaptor(pa);
          continue;
      }

      if(!adaptorPtr->ReclipVideo)
	  adaptorPtr->flags |= VIDEO_NO_CLIPPING;

      adaptorPriv->flags = adaptorPtr->flags;
      adaptorPriv->PutVideo = adaptorPtr->PutVideo;
      adaptorPriv->PutStill = adaptorPtr->PutStill;
      adaptorPriv->GetVideo = adaptorPtr->GetVideo;
      adaptorPriv->GetStill = adaptorPtr->GetStill;
      adaptorPriv->StopVideo = adaptorPtr->StopVideo;
      adaptorPriv->ReclipVideo = adaptorPtr->ReclipVideo;
      adaptorPriv->SetPortAttribute = adaptorPtr->SetPortAttribute;
      adaptorPriv->GetPortAttribute = adaptorPtr->GetPortAttribute;
      adaptorPriv->QueryBestSize = adaptorPtr->QueryBestSize;

      pa->devPriv.ptr = (pointer)adaptorPriv;

      if(pa->type & XvInputMask) {
	 if(!adaptorPtr->PutVideo || !adaptorPtr->PutStill || 
            !adaptorPtr->StopVideo || !adaptorPtr->SetPortAttribute ||
            !adaptorPtr->GetPortAttribute || !adaptorPtr->QueryBestSize) {
		xf86XVFreeAdaptor(pa);
		continue;
	 }
      }

      if(pa->type & XvOutputMask) {
	 if(!adaptorPtr->GetVideo || !adaptorPtr->GetStill ||
            !adaptorPtr->StopVideo || !adaptorPtr->SetPortAttribute ||
            !adaptorPtr->GetPortAttribute || !adaptorPtr->QueryBestSize) {
		xf86XVFreeAdaptor(pa);
		continue;
	 }
      }

      pPort = xcalloc(adaptorPtr->nPorts, sizeof(XvPortRec));
      if(!pPort) {
          xf86XVFreeAdaptor(pa);
          continue;
      }
      for(pp = pPort, np = 0, numPort = 0; 
	  np < adaptorPtr->nPorts;
	  np++) {

          if(!(pp->id = FakeClientID(0))) continue;
	  if(!AddResource(pp->id, PortResource, pp)) continue;

          pp->pAdaptor = pa;
          pp->pNotify = (XvPortNotifyPtr)NULL;
          pp->pDraw = (DrawablePtr)NULL;
          pp->client = (ClientPtr)NULL;
          pp->grab.client = (ClientPtr)NULL;
          pp->time = currentTime;

	  portPriv = xcalloc(1, sizeof(XvPortRecPrivate));
          pp->devPriv.ptr = portPriv;
	  if(!portPriv) continue;

	  portPriv->pScrn = pScrn;
	  portPriv->AdaptorRec = adaptorPriv;
          portPriv->DevPriv.ptr = adaptorPtr->pPortPrivates[np].ptr;
	
          pp++;
          numPort++;
      }
      pa->nPorts = numPort;
      pa->pPorts = pPort;
      if(!numPort) {
          xf86XVFreeAdaptor(pa);
          continue;
      }

      pa->base_id = pPort->id;
      
      pa++;
      numAdaptor++;
  }

  if(numAdaptor) {
      pxvs->nAdaptors = numAdaptor;
      pxvs->pAdaptors = pAdaptor;
  } else {
     xfree(pAdaptor);
     return FALSE;
  }

  return TRUE;
}

/* Video should be clipped to the intersection of the window cliplist
   and the client cliplist specified in the GC for which the video was
   initialized.  When we need to reclip a window, the GC that started
   the video may not even be around anymore.  That's why we save the
   client clip from the GC when the video is initialized.  We then
   use xf86XVUpdateCompositeClip to calculate the new composite clip
   when we need it.  This is different from what DEC did.  They saved
   the GC and used it's clip list when they needed to reclip the window,
   even if the client clip was different from the one the video was
   initialized with.  If the original GC was destroyed, they had to stop
   the video.  I like the new method better (MArk). 
*/

static void 
xf86XVUpdateCompositeClip(
    DrawablePtr pDraw,
    XvPortRecPrivatePtr portPriv
){
   RegionPtr	pregWin, pCompositeClip;
   WindowPtr	pWin = (WindowPtr)pDraw;
   Bool 	freeCompClip = FALSE;

   /* get window clip list */
   if(portPriv->subWindowMode == IncludeInferiors) {
	pregWin = NotClippedByChildren(pWin);
	freeCompClip = TRUE;
   } else
	pregWin = &pWin->clipList;

   if(portPriv->pCompositeClip && portPriv->FreeCompositeClip) {
	REGION_DESTROY(pWin->pScreen, portPriv->pCompositeClip);
   }

   if(!portPriv->clientClip) {
	portPriv->pCompositeClip = pregWin;
	portPriv->FreeCompositeClip = freeCompClip;
	return;
   }

   
   pCompositeClip = REGION_CREATE(pWin->pScreen, NullBox, 1);
   REGION_COPY(pWin->pScreen, pCompositeClip, portPriv->clientClip);
   REGION_TRANSLATE(pWin->pScreen, pCompositeClip,
					 pDraw->x + portPriv->clipOrg.x,
					 pDraw->y + portPriv->clipOrg.y);
   REGION_INTERSECT(pWin->pScreen, pCompositeClip, pregWin, pCompositeClip);

   portPriv->pCompositeClip = pCompositeClip;
   portPriv->FreeCompositeClip = TRUE;

   if(freeCompClip) {
   	REGION_DESTROY(pWin->pScreen, pregWin);
   }
}

/* Save the current clientClip and update the CompositeClip whenever
   we have a fresh GC */

static void
xf86XVCopyClip(
   XvPortRecPrivatePtr portPriv, 
   GCPtr pGC
){
    /* free the old clientClip */
    if(portPriv->clientClip) {
	REGION_DESTROY(pGC->pScreen, portPriv->clientClip);
	portPriv->clientClip = NULL;
    }

    /* copy the new one if it exists */
    if((pGC->clientClipType == CT_REGION) && pGC->clientClip) {
	portPriv->clientClip = REGION_CREATE(pGC->pScreen, NullBox, 1);
	/* Note: this is in window coordinates */
	REGION_COPY(pGC->pScreen, portPriv->clientClip, pGC->clientClip);
    }

    /* get rid of the old clip list */
    if(portPriv->pCompositeClip && portPriv->FreeCompositeClip) {
	REGION_DESTROY(pWin->pScreen, portPriv->pCompositeClip);
    }

    portPriv->clipOrg = pGC->clipOrg;
    portPriv->pCompositeClip = pGC->pCompositeClip;
    portPriv->FreeCompositeClip = FALSE;
}

static int
xf86XVRegetVideo(XvPortRecPrivatePtr portPriv)
{
  RegionRec WinRegion;
  RegionRec ClipRegion;
  BoxRec WinBox;
  ScreenPtr pScreen = portPriv->pDraw->pScreen;
  int ret = Success;

  /* translate the video region to the screen */
  WinBox.x1 = portPriv->pDraw->x + portPriv->drw_x;
  WinBox.y1 = portPriv->pDraw->y + portPriv->drw_y;
  WinBox.x2 = WinBox.x1 + portPriv->drw_w;
  WinBox.y2 = WinBox.y1 + portPriv->drw_h;
  
  /* clip to the window composite clip */
  REGION_INIT(pScreen, &WinRegion, &WinBox, 1);
  REGION_INIT(pScreen, &ClipRegion, NullBox, 1);
  REGION_INTERSECT(Screen, &ClipRegion, &WinRegion, portPriv->pCompositeClip); 

  /* turn off the video if it's on */  
  if(portPriv->isOn) { 
     (*portPriv->AdaptorRec->StopVideo)(
	portPriv->pScrn, portPriv->DevPriv.ptr, FALSE);
     portPriv->isOn = FALSE;
  }
#if 0
  /* that's all if it's totally obscured */
  if(!REGION_NOTEMPTY(pScreen, &ClipRegion))
	goto CLIP_VIDEO_BAILOUT;
#endif
#if 1
  /* if you wanted VIDEO_NO_CLIPPING hardware to grab the window area
     if part of it was visible rather than just failing, you could 
     comment out this section.  There may be security problems
     with that though. */

  /* bailout if we have to clip but the hardware doesn't support it */
  if(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING) {
     BoxPtr clipBox = REGION_RECTS(&ClipRegion);
     if(  (REGION_NUM_RECTS(&ClipRegion) != 1) ||
	  (clipBox->x1 != WinBox.x1) || (clipBox->x2 != WinBox.x2) || 
	  (clipBox->y1 != WinBox.y1) || (clipBox->y2 != WinBox.y2))
		goto CLIP_VIDEO_BAILOUT;
  }

#endif

  if(portPriv->AdaptorRec->flags & VIDEO_INVERT_CLIPLIST) {
     REGION_SUBTRACT(pScreen, &ClipRegion, &WinRegion, &ClipRegion);
  }

  if(!(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING))
	(*portPriv->AdaptorRec->ReclipVideo)( portPriv->pScrn, 
				&ClipRegion, portPriv->DevPriv.ptr);
 
  ret = (*portPriv->AdaptorRec->GetVideo)(portPriv->pScrn, 
			portPriv->vid_x, portPriv->vid_y, 
			portPriv->pDraw->x + portPriv->drw_x, 
			portPriv->pDraw->y + portPriv->drw_y, 
			portPriv->vid_w, portPriv->vid_h, 
			portPriv->drw_w, portPriv->drw_h, 
			portPriv->DevPriv.ptr);
  if(ret == Success)
	portPriv->isOn = TRUE;

CLIP_VIDEO_BAILOUT:

  REGION_UNINIT(pScreen, &WinRegion);
  REGION_UNINIT(pScreen, &ClipRegion);
  
  return ret;
}

static int
xf86XVReputVideo(XvPortRecPrivatePtr portPriv)
{
  RegionRec WinRegion;
  RegionRec ClipRegion;
  BoxRec WinBox;
  ScreenPtr pScreen = portPriv->pDraw->pScreen;
  int ret = Success;

  /* translate the video region to the screen */
  WinBox.x1 = portPriv->pDraw->x + portPriv->drw_x;
  WinBox.y1 = portPriv->pDraw->y + portPriv->drw_y;
  WinBox.x2 = WinBox.x1 + portPriv->drw_w;
  WinBox.y2 = WinBox.y1 + portPriv->drw_h;
  
  /* clip to the window composite clip */
  REGION_INIT(pScreen, &WinRegion, &WinBox, 1);
  REGION_INIT(pScreen, &ClipRegion, NullBox, 1);
  REGION_INTERSECT(Screen, &ClipRegion, &WinRegion, portPriv->pCompositeClip); 

  /* turn off the video if it's on */  
  if(portPriv->isOn) { 
     (*portPriv->AdaptorRec->StopVideo)(
	portPriv->pScrn, portPriv->DevPriv.ptr, FALSE);
     portPriv->isOn = FALSE;
  }

  /* that's all if it's totally obscured */
  if(!REGION_NOTEMPTY(pScreen, &ClipRegion))
	goto CLIP_VIDEO_BAILOUT;
  

  /* bailout if we have to clip but the hardware doesn't support it */
  if(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING) {
     BoxPtr clipBox = REGION_RECTS(&ClipRegion);
     if(  (REGION_NUM_RECTS(&ClipRegion) != 1) ||
	  (clipBox->x1 != WinBox.x1) || (clipBox->x2 != WinBox.x2) || 
	  (clipBox->y1 != WinBox.y1) || (clipBox->y2 != WinBox.y2))
		goto CLIP_VIDEO_BAILOUT;
  }


  if(portPriv->AdaptorRec->flags & VIDEO_INVERT_CLIPLIST) {
     REGION_SUBTRACT(pScreen, &ClipRegion, &WinRegion, &ClipRegion);
  }

  if(!(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING))
	(*portPriv->AdaptorRec->ReclipVideo)( portPriv->pScrn, 
				&ClipRegion, portPriv->DevPriv.ptr);
 
  ret = (*portPriv->AdaptorRec->PutVideo)(portPriv->pScrn, 
			portPriv->vid_x, portPriv->vid_y, 
			portPriv->pDraw->x + portPriv->drw_x, 
			portPriv->pDraw->y + portPriv->drw_y, 
			portPriv->vid_w, portPriv->vid_h, 
			portPriv->drw_w, portPriv->drw_h, 
			portPriv->DevPriv.ptr);
  if(ret == Success)
	portPriv->isOn = TRUE;

CLIP_VIDEO_BAILOUT:

  REGION_UNINIT(pScreen, &WinRegion);
  REGION_UNINIT(pScreen, &ClipRegion);

  return ret;
}

static int
xf86XVReputAllVideo(WindowPtr pWin, pointer data)
{
    XF86XVWindowPtr WinPriv = 
	(XF86XVWindowPtr)pWin->devPrivates[XF86XVWindowIndex].ptr;

    while(WinPriv) {
	xf86XVUpdateCompositeClip((DrawablePtr)pWin, WinPriv->PortRec);
	if(WinPriv->PortRec->type == XvInputMask)
	    xf86XVReputVideo(WinPriv->PortRec);
	else
	    xf86XVRegetVideo(WinPriv->PortRec);
	WinPriv = WinPriv->next;
    }

    return WT_WALKCHILDREN;
}

static int
xf86XVEnlistPortInWindow(WindowPtr pWin, XvPortRecPrivatePtr portPriv)
{
   XF86XVWindowPtr winPriv, PrivRoot;    

   winPriv = PrivRoot = 
		(XF86XVWindowPtr)(pWin->devPrivates[XF86XVWindowIndex].ptr);

  /* Enlist our port in the window private */
   while(winPriv) {
	if(winPriv->PortRec == portPriv) /* we're already listed */
	    break;
	winPriv = winPriv->next;
   }

   if(!winPriv) {
	winPriv = xalloc(sizeof(XF86XVWindowRec));
	if(!winPriv) return BadAlloc;
	winPriv->PortRec = portPriv;
	winPriv->next = PrivRoot;
	pWin->devPrivates[XF86XVWindowIndex].ptr = (pointer)winPriv;
   }   
   return Success;
}


static void
xf86XVRemovePortFromWindow(WindowPtr pWin, XvPortRecPrivatePtr portPriv)
{
     XF86XVWindowPtr winPriv, prevPriv = NULL;

     winPriv = (XF86XVWindowPtr)(pWin->devPrivates[XF86XVWindowIndex].ptr);

     while(winPriv) {
	if(winPriv->PortRec == portPriv) {
	    if(prevPriv) 
		prevPriv->next = winPriv->next;
	    else 
		pWin->devPrivates[XF86XVWindowIndex].ptr = 
					(pointer)winPriv->next;
	    xfree(winPriv);
	    break;
	}
	prevPriv = winPriv; 
	winPriv = winPriv->next;
     }
}

/****  ScreenRec fields ****/


static Bool
xf86XVCreateWindow(WindowPtr pWin)
{
  XvScreenPtr pxvs = 
   (XvScreenPtr) pWin->drawable.pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  int ret;

  ret = (*ScreenPriv->CreateWindow)(pWin);

  if(ret) pWin->devPrivates[XF86XVWindowIndex].ptr = NULL;

  return ret;
}

static void
xf86XVWindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
){
  XvScreenPtr pxvs = 
   (XvScreenPtr) pWin->drawable.pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  XF86XVWindowPtr WinPriv = 
		(XF86XVWindowPtr)pWin->devPrivates[XF86XVWindowIndex].ptr;

  (*ScreenPriv->WindowExposures)(pWin, pReg, pOtherReg);

  while(WinPriv) {
	xf86XVUpdateCompositeClip((DrawablePtr)pWin, WinPriv->PortRec);
	if(WinPriv->PortRec->type == XvInputMask)
	    xf86XVReputVideo(WinPriv->PortRec);
	else
	    xf86XVRegetVideo(WinPriv->PortRec); 
	WinPriv = WinPriv->next;
  }
}

static Bool
xf86XVUnrealizeWindow(WindowPtr pWin)
{
  XvScreenPtr pxvs = 
   (XvScreenPtr) pWin->drawable.pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  XF86XVWindowPtr WinPriv = 
		(XF86XVWindowPtr)pWin->devPrivates[XF86XVWindowIndex].ptr;
  XvPortRecPrivatePtr pPriv;

  while(WinPriv) {
    pPriv = WinPriv->PortRec;

    if(pPriv->isOn) { 
	(*pPriv->AdaptorRec->StopVideo)(
			pPriv->pScrn, pPriv->DevPriv.ptr, FALSE);
	pPriv->isOn = FALSE;
    }	
    WinPriv = WinPriv->next;
  }

  return((* ScreenPriv->UnrealizeWindow)(pWin));
}

static void
xf86XVClipNotify(WindowPtr pWin, int dx, int dy)
{
  XvScreenPtr pxvs = 
   (XvScreenPtr) pWin->drawable.pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  XF86XVWindowPtr WinPriv = 
		(XF86XVWindowPtr)pWin->devPrivates[XF86XVWindowIndex].ptr;
  XvPortRecPrivatePtr pPriv;

  while(WinPriv) {
    pPriv = WinPriv->PortRec;

    if(pPriv->isOn) { 
	(*pPriv->AdaptorRec->StopVideo)(
			pPriv->pScrn, pPriv->DevPriv.ptr, FALSE);
	pPriv->isOn = FALSE;
    }	
    WinPriv = WinPriv->next;
  }
  
  if(ScreenPriv->ClipNotify)
	(* ScreenPriv->ClipNotify)(pWin, dx, dy);
}

static void 
xf86XVCopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc 
){
  XvScreenPtr pxvs = 
   (XvScreenPtr) pWin->drawable.pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  XF86XVWindowPtr WinPriv = 
		(XF86XVWindowPtr)pWin->devPrivates[XF86XVWindowIndex].ptr;
  XvPortRecPrivatePtr pPriv;

  while(WinPriv) {
    pPriv = WinPriv->PortRec;

    if(pPriv->isOn) { 
	(*pPriv->AdaptorRec->StopVideo)(
			pPriv->pScrn, pPriv->DevPriv.ptr, FALSE);
	pPriv->isOn = FALSE;
    }	
    WinPriv = WinPriv->next;
  }

  (*ScreenPriv->CopyWindow)(pWin, ptOldOrg, prgnSrc);
}


/**** Required XvScreenRec fields ****/

static Bool
xf86XVCloseScreen(int i, ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  XvScreenPtr pxvs = (XvScreenPtr) pScreen->devPrivates[XF86XvScreenIndex].ptr;
  XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
  XvAdaptorPtr pa;
  int c;

  if(!ScreenPriv) return TRUE;

  pScreen->CreateWindow = ScreenPriv->CreateWindow;
  pScreen->WindowExposures = ScreenPriv->WindowExposures;
  pScreen->ClipNotify = ScreenPriv->ClipNotify;
  pScreen->UnrealizeWindow = ScreenPriv->UnrealizeWindow;
  pScreen->CopyWindow = ScreenPriv->CopyWindow;

  pScrn->EnterVT = ScreenPriv->EnterVT; 
  pScrn->LeaveVT = ScreenPriv->LeaveVT; 

  for(c = 0, pa = pxvs->pAdaptors; c < pxvs->nAdaptors; c++, pa++) { 
       xf86XVFreeAdaptor(pa);
  }

  if(pxvs->pAdaptors)
    xfree(pxvs->pAdaptors);

  xfree(ScreenPriv);


  return TRUE;
}


static int
xf86XVQueryAdaptors(
   ScreenPtr pScreen,
   XvAdaptorPtr *p_pAdaptors,
   int *p_nAdaptors
){
  XvScreenPtr pxvs = (XvScreenPtr) pScreen->devPrivates[XF86XvScreenIndex].ptr;

  *p_nAdaptors = pxvs->nAdaptors;
  *p_pAdaptors = pxvs->pAdaptors;

  return (Success);
}


/**** ScrnInfoRec fields ****/

static Bool 
xf86XVEnterVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XvScreenPtr pxvs = 
	(XvScreenPtr) pScreen->devPrivates[XF86XvScreenIndex].ptr;
    XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
    Bool ret;

    ret = (*ScreenPriv->EnterVT)(index, flags);

    if(ret) WalkTree(pScreen, xf86XVReputAllVideo, 0); 
 
    return ret;
}

static void 
xf86XVLeaveVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XvScreenPtr pxvs = 
	(XvScreenPtr) pScreen->devPrivates[XF86XvScreenIndex].ptr;
    XF86XVScreenPtr ScreenPriv = (XF86XVScreenPtr)pxvs->devPriv.ptr;
    XvAdaptorPtr pAdaptor;
    XvPortPtr pPort;
    XvPortRecPrivatePtr pPriv;
    int i, j;

    for(i = 0; i < pxvs->nAdaptors; i++) {
	pAdaptor = &pxvs->pAdaptors[i];
	for(j = 0; j < pAdaptor->nPorts; j++) {
	    pPort = &pAdaptor->pPorts[j];
	    pPriv = (XvPortRecPrivatePtr)pPort->devPriv.ptr;
	    if(pPriv->isOn) {
		(*pPriv->AdaptorRec->StopVideo)(
			pPriv->pScrn, pPriv->DevPriv.ptr, TRUE);
		pPriv->isOn = FALSE;
	    }
	}
    }

    (*ScreenPriv->LeaveVT)(index, flags);
}


/**** XvAdaptorRec fields ****/

static int
xf86XVAllocatePort(
   unsigned long port,
   XvPortPtr pPort,
   XvPortPtr *ppPort
){
  *ppPort = pPort;
  return Success;
}



static int
xf86XVFreePort(XvPortPtr pPort)
{
  return Success;
}


static int
xf86XVPutVideo(
   ClientPtr client,
   DrawablePtr pDraw,
   XvPortPtr pPort,
   GCPtr pGC,
   INT16 vid_x, INT16 vid_y, 
   CARD16 vid_w, CARD16 vid_h, 
   INT16 drw_x, INT16 drw_y,
   CARD16 drw_w, CARD16 drw_h
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
  int result;

  /* No dumping video to pixmaps... For now anyhow */
  if(pDraw->type != DRAWABLE_WINDOW) {
      pPort->pDraw = (DrawablePtr)NULL;
      return BadAlloc;
  }
  
  /* If we are changing windows, unregister our port in the old window */
  if(portPriv->pDraw && (portPriv->pDraw != pDraw))
     xf86XVRemovePortFromWindow((WindowPtr)(portPriv->pDraw), portPriv);

  portPriv->pDraw = NULL;

  /* Register our port with the new window */
  result =  xf86XVEnlistPortInWindow((WindowPtr)pDraw, portPriv);
  if(result != Success) return result;

  portPriv->pDraw = pDraw;
  portPriv->type = XvInputMask;

  /* save a copy of these parameters */
  portPriv->vid_x = vid_x;  portPriv->vid_y = vid_y;
  portPriv->vid_w = vid_w;  portPriv->vid_h = vid_h;
  portPriv->drw_x = drw_x;  portPriv->drw_y = drw_y;
  portPriv->drw_w = drw_w;  portPriv->drw_h = drw_h;

  /* make sure we have the most recent copy of the clientClip */
  xf86XVCopyClip(portPriv, pGC);

  /* To indicate to the DI layer that we were successful */
  pPort->pDraw = pDraw;
  
  if(!portPriv->pScrn->vtSema) return Success; /* Success ? */

  return(xf86XVReputVideo(portPriv));
}

static int
xf86XVPutStill(
   ClientPtr client,
   DrawablePtr pDraw,
   XvPortPtr pPort,
   GCPtr pGC,
   INT16 vid_x, INT16 vid_y, 
   CARD16 vid_w, CARD16 vid_h, 
   INT16 drw_x, INT16 drw_y,
   CARD16 drw_w, CARD16 drw_h
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
  ScreenPtr pScreen = pDraw->pScreen;
  RegionRec WinRegion;
  RegionRec ClipRegion;
  BoxRec WinBox;
  Bool WasOn = FALSE;
  int ret = Success;

  if (pDraw->type != DRAWABLE_WINDOW)
      return BadAlloc;

  if(!portPriv->pScrn->vtSema) return Success; /* Success ? */

  WinBox.x1 = pDraw->x + drw_x;
  WinBox.y1 = pDraw->y + drw_y;
  WinBox.x2 = WinBox.x1 + drw_w;
  WinBox.y2 = WinBox.y1 + drw_h;
  
  REGION_INIT(pScreen, &WinRegion, &WinBox, 1);
  REGION_INIT(pScreen, &ClipRegion, NullBox, 1);
  REGION_INTERSECT(pScreen, &ClipRegion, &WinRegion, pGC->pCompositeClip);   

  if(!REGION_NOTEMPTY(pScreen, &ClipRegion))
	goto PUT_STILL_BAILOUT; 

  if(portPriv->isOn) { 
     WasOn = TRUE;
     (*portPriv->AdaptorRec->StopVideo)(
	portPriv->pScrn, portPriv->DevPriv.ptr, FALSE);
     portPriv->isOn = FALSE;
  }

  if(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING) {
     BoxPtr clipBox = REGION_RECTS(&ClipRegion);
     if(  (REGION_NUM_RECTS(&ClipRegion) != 1) ||
	  (clipBox->x1 != WinBox.x1) || (clipBox->x2 != WinBox.x2) || 
	  (clipBox->y1 != WinBox.y1) || (clipBox->y2 != WinBox.y2))
		goto PUT_STILL_BAILOUT;
  }


  if(portPriv->AdaptorRec->flags & VIDEO_INVERT_CLIPLIST) {
     REGION_SUBTRACT(pScreen, &ClipRegion, &WinRegion, &ClipRegion);
  }

  if(!(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING))
	(*portPriv->AdaptorRec->ReclipVideo)(
			portPriv->pScrn, &ClipRegion, portPriv->DevPriv.ptr);
 
  ret = (*portPriv->AdaptorRec->PutStill)(portPriv->pScrn, vid_x, vid_y, 
		pDraw->x  + drw_x, pDraw->y + drw_y,
		vid_w, vid_h, drw_w, drw_h, portPriv->DevPriv.ptr);

PUT_STILL_BAILOUT:

  REGION_UNINIT(pScreen, &WinRegion);
  REGION_UNINIT(pScreen, &ClipRegion);

  /* If this port was in use, reinstate it */  
  if(portPriv->pDraw && WasOn) {
	if(portPriv->type == XvInputMask)
	    xf86XVReputVideo(portPriv);
	else 
	    xf86XVRegetVideo(portPriv);
  }	

  return ret;
}

static int
xf86XVGetVideo(
   ClientPtr client,
   DrawablePtr pDraw,
   XvPortPtr pPort,
   GCPtr pGC,
   INT16 vid_x, INT16 vid_y, 
   CARD16 vid_w, CARD16 vid_h, 
   INT16 drw_x, INT16 drw_y,
   CARD16 drw_w, CARD16 drw_h
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
  int result;

  /* No pixmaps... For now anyhow */
  if(pDraw->type != DRAWABLE_WINDOW) {
      pPort->pDraw = (DrawablePtr)NULL;
      return BadAlloc;
  }
  
  /* If we are changing windows, unregister our port in the old window */
  if(portPriv->pDraw && (portPriv->pDraw != pDraw))
     xf86XVRemovePortFromWindow((WindowPtr)(portPriv->pDraw), portPriv);

  portPriv->pDraw = NULL;

  /* Register our port with the new window */
  result =  xf86XVEnlistPortInWindow((WindowPtr)pDraw, portPriv);
  if(result != Success) return result;

  portPriv->pDraw = pDraw;
  portPriv->type = XvOutputMask;

  /* save a copy of these parameters */
  portPriv->vid_x = vid_x;  portPriv->vid_y = vid_y;
  portPriv->vid_w = vid_w;  portPriv->vid_h = vid_h;
  portPriv->drw_x = drw_x;  portPriv->drw_y = drw_y;
  portPriv->drw_w = drw_w;  portPriv->drw_h = drw_h;

  /* make sure we have the most recent copy of the clientClip */
  xf86XVCopyClip(portPriv, pGC);

  /* To indicate to the DI layer that we were successful */
  pPort->pDraw = pDraw;
  
  if(!portPriv->pScrn->vtSema) return Success; /* Success ? */

  return(xf86XVRegetVideo(portPriv));
}

static int
xf86XVGetStill(
   ClientPtr client,
   DrawablePtr pDraw,
   XvPortPtr pPort,
   GCPtr pGC,
   INT16 vid_x, INT16 vid_y, 
   CARD16 vid_w, CARD16 vid_h, 
   INT16 drw_x, INT16 drw_y,
   CARD16 drw_w, CARD16 drw_h
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
  ScreenPtr pScreen = pDraw->pScreen;
  RegionRec WinRegion;
  RegionRec ClipRegion;
  BoxRec WinBox;
  Bool WasOn = FALSE;
  int ret = Success;

  if (pDraw->type != DRAWABLE_WINDOW)
      return BadAlloc;

  if(!portPriv->pScrn->vtSema) return Success; /* Success ? */

  WinBox.x1 = pDraw->x + drw_x;
  WinBox.y1 = pDraw->y + drw_y;
  WinBox.x2 = WinBox.x1 + drw_w;
  WinBox.y2 = WinBox.y1 + drw_h;
  
  REGION_INIT(pScreen, &WinRegion, &WinBox, 1);
  REGION_INIT(pScreen, &ClipRegion, NullBox, 1);
  REGION_INTERSECT(pScreen, &ClipRegion, &WinRegion, pGC->pCompositeClip);   

  if(!REGION_NOTEMPTY(pScreen, &ClipRegion))
	goto GET_STILL_BAILOUT; 

  if(portPriv->isOn) { 
     WasOn = TRUE;
     (*portPriv->AdaptorRec->StopVideo)(
	portPriv->pScrn, portPriv->DevPriv.ptr, FALSE);
     portPriv->isOn = FALSE;
  }


#if 1
  /* if you wanted VIDEO_NO_CLIPPING hardware to grab the window area
     if part of it was visible rather than just failing, you could 
     comment out this section.  There may be security problems
     with that though. */

  if(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING) {
     BoxPtr clipBox = REGION_RECTS(&ClipRegion);
     if(  (REGION_NUM_RECTS(&ClipRegion) != 1) ||
	  (clipBox->x1 != WinBox.x1) || (clipBox->x2 != WinBox.x2) || 
	  (clipBox->y1 != WinBox.y1) || (clipBox->y2 != WinBox.y2))
		goto GET_STILL_BAILOUT;
  }
#endif

  if(portPriv->AdaptorRec->flags & VIDEO_INVERT_CLIPLIST) {
     REGION_SUBTRACT(pScreen, &ClipRegion, &WinRegion, &ClipRegion);
  }

  if(!(portPriv->AdaptorRec->flags & VIDEO_NO_CLIPPING))
	(*portPriv->AdaptorRec->ReclipVideo)(
			portPriv->pScrn, &ClipRegion, portPriv->DevPriv.ptr);
 
  ret = (*portPriv->AdaptorRec->GetStill)(portPriv->pScrn, vid_x, vid_y, 
		pDraw->x  + drw_x, pDraw->y + drw_y,
		vid_w, vid_h, drw_w, drw_h, portPriv->DevPriv.ptr);

GET_STILL_BAILOUT:

  REGION_UNINIT(pScreen, &WinRegion);
  REGION_UNINIT(pScreen, &ClipRegion);

  /* If this port was in use, reinstate it */  
  if(portPriv->pDraw && WasOn) {
	if(portPriv->type == XvInputMask)
	    xf86XVReputVideo(portPriv);
	else 
	    xf86XVRegetVideo(portPriv);
  }	

  return ret;
}



static int
xf86XVStopVideo(
   ClientPtr client,
   XvPortPtr pPort,
   DrawablePtr pDraw
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);

  if(pDraw->type != DRAWABLE_WINDOW)
      return BadAlloc;
  
  xf86XVRemovePortFromWindow((WindowPtr)pDraw, portPriv);

  portPriv->pDraw = NULL;
  portPriv->type = 0;

  if(!portPriv->pScrn->vtSema) return Success; /* Success ? */

  if(portPriv->isOn) { 
     (*portPriv->AdaptorRec->StopVideo)(
	portPriv->pScrn, portPriv->DevPriv.ptr, TRUE);
     portPriv->isOn = FALSE;
  }

  return Success;
}

static int
xf86XVSetPortAttribute(
   ClientPtr client,
   XvPortPtr pPort,
   Atom attribute,
   INT32 value
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
     
  return((*portPriv->AdaptorRec->SetPortAttribute)(portPriv->pScrn, 
		attribute, value, portPriv->DevPriv.ptr));
}


static int
xf86XVGetPortAttribute(
   ClientPtr client,
   XvPortPtr pPort,
   Atom attribute,
   INT32 *p_value
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
     
  return((*portPriv->AdaptorRec->GetPortAttribute)(portPriv->pScrn, 
		attribute, p_value, portPriv->DevPriv.ptr));
}



static int
xf86XVQueryBestSize(
   ClientPtr client,
   XvPortPtr pPort,
   CARD8 motion,
   CARD16 vid_w, CARD16 vid_h,
   CARD16 drw_w, CARD16 drw_h,
   unsigned int *p_w, unsigned int *p_h
){
  XvPortRecPrivatePtr portPriv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);
     
  (*portPriv->AdaptorRec->QueryBestSize)(portPriv->pScrn, 
		(Bool)motion, vid_w, vid_h, drw_w, drw_h,
		p_w, p_h, portPriv->DevPriv.ptr);

  return Success;
}




