/*
   Copyright (c) 1999 -  The XFree86 Project, Inc.

   Written by Mark Vojkovich
*/

#include "xf86.h"
#include "xf86str.h"
#include "dgaproc.h"
#include "colormapst.h"

static unsigned long DGAGeneration = 0;
static int DGAScreenIndex = -1;

static Bool DGACloseScreen(int i, ScreenPtr pScreen);

static int
DGASetDGAMode(
   int index,
   int num,
   DGADevicePtr devRet
);

static void
DGACopyDeviceInfo(
   DGAModePtr mode,
   XDGADevicePtr dev
);


#define DGA_GET_SCREEN_PRIV(pScreen) \
	((DGAScreenPtr)((pScreen)->devPrivates[DGAScreenIndex].ptr))


typedef struct {
   ScrnInfoPtr 		pScrn;
   int			numModes;
   DGAModePtr		modes;
   CloseScreenProcPtr	CloseScreen;
   DGADevicePtr		current;
   DGAFunctionPtr	funcs;
   int			input;
} DGAScreenRec, *DGAScreenPtr;

Bool
DGAInit(
   ScreenPtr pScreen,
   DGAFunctionPtr funcs, 
   DGAModePtr modes,
   int num
){
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DGAScreenPtr pScreenPriv;
    int i;

    if(!funcs->Flush || !funcs->SetMode || !funcs->SetViewport ||
	!funcs->GetViewport)
	return FALSE;

    if(!modes || !num)
	return FALSE;

    if(DGAGeneration != serverGeneration) {
	if((DGAScreenIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	DGAGeneration = serverGeneration;
    }

    if(!(pScreenPriv = (DGAScreenPtr)xalloc(sizeof(DGAScreenRec))))
	return FALSE;

    pScreenPriv->pScrn = pScrn;
    pScreenPriv->numModes = num;
    pScreenPriv->modes = modes;
    pScreenPriv->current = NULL;
    pScreenPriv->input = 0;
    pScreenPriv->funcs = funcs;
    
    for(i = 0; i < num; i++)
	modes[i].num = i + 1;

    pScreen->devPrivates[DGAScreenIndex].ptr = (pointer)pScreenPriv;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = DGACloseScreen;

    pScrn->SetDGAMode = DGASetDGAMode;

    return TRUE;
}



static Bool 
DGACloseScreen(int i, ScreenPtr pScreen)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

   pScreen->CloseScreen = pScreenPriv->CloseScreen;

   /* DGAShutdown() should have ensured that no DGA
	screen were active by here */

   if(pScreenPriv->numModes && pScreenPriv->modes)
	xfree(pScreenPriv->modes);
   xfree(pScreenPriv);

   return((*pScreen->CloseScreen)(i, pScreen));
}



static int
DGASetDGAMode(
   int index,
   int num,
   DGADevicePtr devRet
){
   ScreenPtr pScreen = screenInfo.screens[index];
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
   ScrnInfoPtr pScrn = pScreenPriv->pScrn;
   DGADevicePtr device;
   PixmapPtr pPix = NULL;
   DGAModePtr pMode = NULL;

   if(!num) {
	if(pScreenPriv->current) {
	    if(pScreenPriv->current->pPix)
		(*pScreen->DestroyPixmap)(pPix);
	    xfree(pScreenPriv->current);
	    pScreenPriv->current = NULL;
	    (*pScreenPriv->funcs->SetMode)(pScreenPriv->pScrn, NULL);
	    (*pScrn->SaveRestoreImage)(index, RestoreImage);
	    pScrn->vtSema = TRUE;
	}
	return Success;
   }

   if(!pScrn->vtSema && !pScreenPriv->current) /* Really switched away */
	return BadAlloc;
      
   if((num > 0) && (num <= pScreenPriv->numModes))
	pMode = &(pScreenPriv->modes[num - 1]);
   else
	return BadValue;

   if(!(device = (DGADevicePtr)xalloc(sizeof(DGADeviceRec))))
	return BadAlloc;

   if(!pScreenPriv->current && !(*pScrn->SaveRestoreImage)(index, SaveImage)){
	xfree(device);
	return BadAlloc;
   } 

   if(!(*pScreenPriv->funcs->SetMode)(pScreenPriv->pScrn, pMode)) {
	xfree(device);
	if(!pScreenPriv->current)
	     (*pScrn->SaveRestoreImage)(index, FreeImage);
	return BadAlloc;
   }

   if(!pScreenPriv->current && !pScreenPriv->input) {
	/* if it's multihead we need to warp the cursor off of
	   our screen so it doesn't get trapped  */
   } 

   pScrn->vtSema = FALSE;

   if(pScreenPriv->current) {
	if(pScreenPriv->current->pPix)
	    (*pScreen->DestroyPixmap)(pPix);
	xfree(pScreenPriv->current);
	pScreenPriv->current = NULL;
   } 

   if(pMode->flags & DGA_PIXMAP_AVAILABLE) {
	if((pPix = (*pScreen->CreatePixmap)(pScreen, 0, 0, pMode->depth))) {
	    (*pScreen->ModifyPixmapHeader)(pPix, 
			pMode->pixmapWidth, pMode->pixmapHeight,
			pMode->depth, pMode->bitsPerPixel, 
			pMode->bytesPerScanline / (pMode->bitsPerPixel >> 3),
 			(pointer)pMode->memBase);
        }
   }

   devRet->mode = device->mode = pMode;
   devRet->pPix = device->pPix = pPix;
   pScreenPriv->current = device;

   return Success;
}


/*********** exported ones ***************/

Bool
DGAAvailable(int index) 
{
   if(DGAScreenIndex < 0)
	return FALSE;

   if(DGA_GET_SCREEN_PRIV(screenInfo.screens[index]))
	return TRUE;
   else
	return FALSE;
}

/* Called by the event code to see which events are getting redirected */

int
DGAGetInput(int index)
{
   DGAScreenPtr pScreenPriv;

   if(DGAScreenIndex < 0)
	return 0;

   if(!(pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index])))
	return 0;
	
   if(pScreenPriv->current)
	return pScreenPriv->input;

   return 0;    
}   

/* Called by the event code in case the server is abruptly terminated */

void 
DGAShutdown()
{
    ScreenPtr pScreen;
    int i;

    for(i = 0; i < screenInfo.numScreens; i++) {
      pScreen = screenInfo.screens[i];	

      if(DGAAvailable(i)) {
	DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

	if(pScreenPriv->current) {
	    if(pScreenPriv->current->pPix)
		(*pScreen->DestroyPixmap)(pScreenPriv->current->pPix);
	    xfree(pScreenPriv->current);

	    (*pScreenPriv->funcs->SetMode)(pScreenPriv->pScrn, NULL);
	    pScreenPriv->pScrn->vtSema = TRUE;
	}
      }
    }
}

/* Called by the extension to initialize a mode */

int
DGASetMode(
   int index,
   int num,
   XDGADevicePtr devRet
){
    ScrnInfoPtr pScrn = xf86Screens[index];
    DGADeviceRec device;
    int ret;

    /* We rely on the extension to check that DGA is available */ 

    ret = (*pScrn->SetDGAMode)(index, num, &device);
    if(ret == Success) {
	DGACopyDeviceInfo(&device.mode, devRet);
	devRet->pPix = device.pPix;
    }

    return ret;
}

/* Called from the extension to let the DDX know which events are requested */

void
DGASelectInput(
   int index,
   long mask
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */ 

   pScreenPriv->input = mask; 
}

int 
DGAGetViewportStatus(int index, int flags) 
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */ 
   if(!pScreenPriv->current)
	return -1;

   return((*pScreenPriv->funcs->GetViewport)(pScreenPriv->pScrn, flags));
}

int
DGASetViewport(
   int index,
   int x, int y,
   int mode
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */ 
   if(!pScreenPriv->current || 
      (pScreenPriv->current->mode->viewportFlags & mode))
	return BadMatch;

   (*pScreenPriv->funcs->SetViewport)(pScreenPriv->pScrn, x, y, mode);
   return Success;
}

/*  Called by the extension to install a colormap on DGA active screens */

void
DGAInstallColormap(ColormapPtr cmap)
{
    ScreenPtr pScreen = cmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    /* We rely on the extension to check that DGA is available */ 

    if(pScreenPriv->current) {
	pScrn->vtSema = TRUE;
	 (*pScreen->InstallColormap)(cmap);
	pScrn->vtSema = FALSE;
    }
}

int
DGAFlush(int index)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is available */

   if(pScreenPriv->current) {
	(*pScreenPriv->funcs->Flush)(pScreenPriv->pScrn);
	return Success;
   }
   return BadMatch;
}

int
DGAFillRect(
   int index,
   int x, int y, int w, int h,
   unsigned long color
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is available */

   if(pScreenPriv->funcs->FillRect && pScreenPriv->current
	&& (pScreenPriv->current->mode->flags & DGA_FILL_RECT)) {

	(*pScreenPriv->funcs->FillRect)(pScreenPriv->pScrn, x, y, w, h, color);
	return Success;
   }
   return BadMatch;
}

int
DGABlitRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is available */

   if(pScreenPriv->funcs->BlitRect && pScreenPriv->current
	&& (pScreenPriv->current->mode->flags & DGA_BLIT_RECT)) {

	(*pScreenPriv->funcs->BlitRect)(pScreenPriv->pScrn, 	
		srcx, srcy, w, h, dstx, dsty);
	return Success;
   }
   return BadMatch;
}

int
DGABlitTransRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty,
   unsigned long color
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is available */

   if(pScreenPriv->funcs->BlitTransRect && pScreenPriv->current
	&& (pScreenPriv->current->mode->flags & DGA_BLIT_RECT_TRANS)) {

	(*pScreenPriv->funcs->BlitRect)(pScreenPriv->pScrn, 	
		srcx, srcy, w, h, dstx, dsty);
	return Success;
   }
   return BadMatch;
}


int
DGAGetModes(int index)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   /* We rely on the extension to check that DGA is available */

   return pScreenPriv->numModes;
}


int
DGAGetDeviceInfo(
  int index,
  XDGADevicePtr dev,
  int mode
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   /* We rely on the extension to check that DGA is available */

   if((mode <= 0) || (mode > pScreenPriv->numModes))
	return BadValue;

   DGACopyDeviceInfo(&(pScreenPriv->modes[mode - 1]), dev);

   return Success;
}


static void
DGACopyDeviceInfo(
   DGAModePtr mode,
   XDGADevicePtr dev
){
   DisplayModePtr dmode = mode->mode;

   dev->mode.num = mode->num;
   dev->mode.name = dmode->name;
   dev->mode.verticalRefresh = ((float)dmode->Clock * 1000.0) / 
		(float)dmode->HTotal / (float)dmode->VTotal;
   dev->mode.flags = mode->flags;
   dev->mode.imageWidth = mode->imageWidth;
   dev->mode.imageHeight = mode->imageHeight;
   dev->mode.pixmapWidth = mode->pixmapWidth;
   dev->mode.pixmapHeight = mode->pixmapHeight;
   dev->mode.bytesPerScanline = mode->bytesPerScanline;
   dev->mode.byteOrder = mode->byteOrder;
   dev->mode.depth = mode->depth;
   dev->mode.bitsPerPixel = mode->bitsPerPixel;
   dev->mode.red_mask = mode->red_mask;
   dev->mode.green_mask = mode->green_mask;
   dev->mode.blue_mask = mode->blue_mask;
   dev->mode.viewportWidth = mode->viewportWidth;
   dev->mode.viewportHeight = mode->viewportHeight;
   dev->mode.xViewportStep = mode->xViewportStep;
   dev->mode.yViewportStep = mode->yViewportStep;
   dev->mode.maxViewportX = mode->maxViewportX;
   dev->mode.maxViewportY = mode->maxViewportY;
   dev->mode.viewportFlags = mode->viewportFlags;
   dev->mode.reserved1 = mode->reserved1;
   dev->mode.reserved2 = mode->reserved2;
   dev->data = mode->memBase;
   dev->pPix = NULL;

   if(dmode->Flags & V_INTERLACE) dev->mode.flags |= DGA_INTERLACED;
   if(dmode->Flags & V_DBLSCAN) dev->mode.flags |= DGA_DOUBLESCAN;
}

/*  For DGA 1.0 backwards compatibility only */

int 
DGAGetOldDGAMode(int index)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   ScrnInfoPtr pScrn = pScreenPriv->pScrn;
   DGAModePtr mode;
   int i, w, h, p;

   /* We rely on the extension to check that DGA is available */

   w = pScrn->currentMode->HDisplay;
   h = pScrn->currentMode->VDisplay;
   p = pScrn->displayWidth / (pScrn->bitsPerPixel >> 3);

   for(i = 0; i < pScreenPriv->numModes; i++) {
	mode = &(pScreenPriv->modes[i - 1]);
  	      
	if((mode->viewportWidth == w) && (mode->viewportHeight == h) &&
		(mode->bytesPerScanline == p) && 
		(mode->bitsPerPixel == pScrn->bitsPerPixel)) {

		return mode->num;
	}
   }

   return 0;
}


