/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.c,v 1.3 1998/08/02 05:16:56 dawes Exp $ */

#include "misc.h"
#include "xf86.h"

#include "X.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "xf86fbman.h"


static FBAreaPtr xf86AllocateFBArea (
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr callback,
   pointer privData
);
static void xf86FreeFBArea(FBAreaPtr area);
static Bool xf86ResizeFBArea(ScreenPtr pScreen, int w, int h, FBAreaPtr resize);
static Bool xf86FBCloseScreen(int i, ScreenPtr pScreen);
static unsigned long xf86FBGeneration = 0;
int xf86FBScreenIndex = -1;

Bool
xf86InitFBManager(
    ScreenPtr pScreen,  
    BoxPtr FullBox
)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   FBManagerPtr offman;
   RegionRec ScreenRegion;
   BoxRec ScreenBox;

   ScreenBox.x1 = 0;
   ScreenBox.y1 = 0;
   ScreenBox.x2 = pScrn->virtualX;
   ScreenBox.y2 = pScrn->virtualY;

   if((FullBox->x1 >  ScreenBox.x1) || (FullBox->y1 >  ScreenBox.y1) ||
      (FullBox->x2 <  ScreenBox.x2) || (FullBox->y2 <  ScreenBox.y2)) {
	return FALSE;   
   }

   if(xf86FBGeneration != serverGeneration) {
	if((xf86FBScreenIndex = AllocateScreenPrivateIndex()) < 0)
		return FALSE;
	xf86FBGeneration = serverGeneration;
   }

   offman = (FBManagerPtr)xalloc(sizeof(FBManager));
   if(!offman) return FALSE;

   pScreen->devPrivates[xf86FBScreenIndex].ptr = (pointer)offman;

   offman->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = xf86FBCloseScreen;

   offman->FullMemory = REGION_CREATE(pScreen, FullBox, 1);
   offman->InitialBoxes = REGION_CREATE(pScreen, NULL, 1);
   offman->FreeBoxes = REGION_CREATE(pScreen, NULL, 1);

   REGION_INIT(pScreen, &ScreenRegion, &ScreenBox, 1); 

   REGION_SUBTRACT(pScreen, offman->InitialBoxes, offman->FullMemory,
 						&ScreenRegion);

   REGION_COPY(pScreen, offman->FreeBoxes, offman->InitialBoxes);

   offman->UsedAreas = NULL;
   offman->NumUsedAreas = 0;  
   offman->FreeBoxesUpdateCallback = NULL;
   offman->pScreen = pScreen;
   offman->devPrivate.ptr = NULL;

   pScrn->AllocateOffscreenArea = xf86AllocateFBArea;
   pScrn->FreeOffscreenArea = xf86FreeFBArea;
   pScrn->ResizeOffscreenArea = xf86ResizeFBArea;

   REGION_UNINIT(pScreen, &ScreenRegion);

   return TRUE;
} 

void
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
)
{
   FBManagerPtr offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

   if(!offman) return;
   offman->FreeBoxesUpdateCallback = FreeBoxCallback;
   offman->devPrivate.ptr = devPriv;

   if(offman->FreeBoxesUpdateCallback)
	(*offman->FreeBoxesUpdateCallback)(
		pScreen, offman->FreeBoxes, offman->devPrivate.ptr);

}


static FBAreaPtr
xf86AllocateFBArea(
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr callback,
   pointer privData
)
{
   FBManagerPtr offman;
   FBLinkPtr link = NULL;
   FBAreaPtr area = NULL;
   RegionRec NewReg;
   BoxPtr boxp;
   int num, i, x;

   if(xf86FBScreenIndex == -1) return NULL;
   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;
   if(!offman) return NULL;   

   boxp = REGION_RECTS(offman->FreeBoxes);
   num = REGION_NUM_RECTS(offman->FreeBoxes);

   if(!num || !boxp) return NULL;

   if(granularity <= 1) granularity = 0;

   for(i = 0; i < num; i++, boxp++) {
	x = boxp->x1;
	if(granularity) {
	    int tmp = x % granularity;
	    if(tmp) x += (granularity - tmp);
	}

	if(((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w))
	   continue;

	link = (FBLinkPtr)xalloc(sizeof(FBLink));
	if(!link) return NULL;

        area = &(link->area);
	
	area->pScreen = pScreen;
	area->granularity = granularity;
	area->box.x1 = x;
	area->box.x2 = x + w;
	area->box.y1 = boxp->y1;
	area->box.y2 = boxp->y1 + h;
	area->MoveAreaCallback = callback;
	area->devPrivate.ptr = privData;

        REGION_INIT(pScreen, &NewReg, &(area->box), 1);
	REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &NewReg);
	REGION_UNINIT(pScreen, &NewReg);

	link->next = offman->UsedAreas;
	offman->UsedAreas = link;
	offman->NumUsedAreas++;
   }

   if(area && offman->FreeBoxesUpdateCallback)
	(*offman->FreeBoxesUpdateCallback)(
		pScreen, offman->FreeBoxes, offman->devPrivate.ptr);

   return area;
}


#define FREE_FBLINK(link) {			\
    if((link)->area.devPrivate.ptr)		\
	xfree((link)->area.devPrivate.ptr);	\
    xfree(link);}	
			

static void
xf86FreeFBArea(FBAreaPtr area)
{
   FBManagerPtr offman;
   FBLinkPtr pLink, pLinkPrev;
   RegionRec FreedRegion;
   ScreenPtr pScreen;


   if(!area || (xf86FBScreenIndex == -1)) return;
   pScreen = area->pScreen;
   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;
   if(!offman) return;   
       
   pLinkPrev = pLink = offman->UsedAreas;
   if(!pLink) return;  
 
   while(&(pLink->area) != area) {
	pLinkPrev = pLink;
	pLink = pLink->next;
	if(!pLink) return;
   }

   /* put the area back into the pool */
   REGION_INIT(pScreen, &FreedRegion, &(pLink->area.box), 1); 
   REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedRegion);
   REGION_UNINIT(pScreen, &FreedRegion); 

   if(pLink != pLinkPrev)
	pLinkPrev->next = pLink->next;
   else offman->UsedAreas = pLink->next;

   FREE_FBLINK(pLink); 
   offman->NumUsedAreas--;

   if(offman->FreeBoxesUpdateCallback)
	(*offman->FreeBoxesUpdateCallback)(
		pScreen, offman->FreeBoxes,  offman->devPrivate.ptr);
}
   


static Bool
xf86ResizeFBArea(
   ScreenPtr pScreen, 
   int w, int h,
   FBAreaPtr resize
){
   FBManagerPtr offman;
   BoxRec OrigArea;
   RegionRec FreedReg;
   RegionRec NewReg;
   FBLinkPtr link = NULL;
   FBAreaPtr area = NULL;
   FBLinkPtr pLink, pLinkPrev;
   BoxPtr boxp;
   int num, i, x;

   if(!resize || (xf86FBScreenIndex == -1)) return FALSE;
   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;
   if(!offman) return FALSE;   

   /* find this link */
   pLinkPrev = pLink = offman->UsedAreas;
   if(!pLink) return FALSE;  
 
   while(&(pLink->area) != resize) {
	pLinkPrev = pLink;
	pLink = pLink->next;
	if(!pLink) return FALSE;
   }

   OrigArea.x1 = resize->box.x1;
   OrigArea.x2 = resize->box.x2;
   OrigArea.y1 = resize->box.y1;
   OrigArea.y2 = resize->box.y2;

   /* if it's smaller, this is easy */

   if((w <= (resize->box.x2 - resize->box.x1)) && 
      (h <= (resize->box.y2 - resize->box.y1))) {

	resize->box.x2 = resize->box.x1 + w;
	resize->box.y2 = resize->box.y1 + h;

        if((resize->box.y2 == OrigArea.y2) &&
	   (resize->box.x2 == OrigArea.x2))
		return TRUE;

	REGION_INIT(pScreen, &FreedReg, &OrigArea, 1); 
	REGION_INIT(pScreen, &NewReg, &(resize->box), 1); 
	REGION_SUBTRACT(pScreen, &FreedReg, &FreedReg, &NewReg);
	REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
	REGION_UNINIT(pScreen, &FreedReg); 
	REGION_UNINIT(pScreen, &NewReg); 

	if(offman->FreeBoxesUpdateCallback)
	     (*offman->FreeBoxesUpdateCallback)(
		pScreen, offman->FreeBoxes, offman->devPrivate.ptr);

	return TRUE;
   }


   /* otherwise we remove the old region */

   REGION_INIT(pScreen, &FreedReg, &OrigArea, 1); 
   REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
  
   /* and try to add a new one */

   boxp = REGION_RECTS(offman->FreeBoxes);
   num = REGION_NUM_RECTS(offman->FreeBoxes);

   for(i = 0; i < num; i++, boxp++) {
	x = boxp->x1;
	if(resize->granularity) {
	    int tmp = x % resize->granularity;
	    if(tmp) x += (resize->granularity - tmp);
	}

	if(((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w))
	   continue;

	link = (FBLinkPtr)xalloc(sizeof(FBLink));
	if(!link) break;

        area = &(link->area);
	
	area->pScreen = pScreen;
	area->granularity = resize->granularity;
	area->box.x1 = x;
	area->box.x2 = x + w;
	area->box.y1 = boxp->y1;
	area->box.y2 = boxp->y1 + h;
	area->MoveAreaCallback = resize->MoveAreaCallback;
	area->devPrivate.ptr = resize->devPrivate.ptr;

        REGION_INIT(pScreen, &NewReg, &(area->box), 1);
	REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &NewReg);
	REGION_UNINIT(pScreen, &NewReg);

	/* remove the old link */
	if(pLink != pLinkPrev)
	    pLinkPrev->next = pLink->next;
	else offman->UsedAreas = pLink->next;

	FREE_FBLINK(pLink); 

	link->next = offman->UsedAreas;
	offman->UsedAreas = link;
   }

   if(!area) {
	/* reinstate the old region */
      REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
      REGION_UNINIT(pScreen, &FreedReg); 
      return FALSE;
   }

   REGION_UNINIT(pScreen, &FreedReg); 

   if(offman->FreeBoxesUpdateCallback)
	(*offman->FreeBoxesUpdateCallback)(
		pScreen, offman->FreeBoxes, offman->devPrivate.ptr);

   return TRUE;
}



static Bool
xf86FBCloseScreen (int i, ScreenPtr pScreen)
{
   FBLinkPtr pLink, tmp;
   FBManagerPtr offman = 
	(FBManagerPtr) pScreen->devPrivates[xf86FBScreenIndex].ptr;

   
   pScreen->CloseScreen = offman->CloseScreen;

   pLink = offman->UsedAreas;

   while(pLink) {
	tmp = pLink;
	pLink = pLink->next;
	FREE_FBLINK(tmp);
   }

   REGION_DESTROY(pScreen, offman->FullMemory);
   REGION_DESTROY(pScreen, offman->InitialBoxes);
   REGION_DESTROY(pScreen, offman->FreeBoxes);

   xfree(offman);

   return (*pScreen->CloseScreen) (i, pScreen);
}
