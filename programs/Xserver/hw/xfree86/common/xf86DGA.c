/*
   Copyright (c) 1999 -  The XFree86 Project, Inc.

   Written by Mark Vojkovich
*/
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DGA.c,v 1.23 1999/08/01 07:57:10 dawes Exp $ */

#include "xf86.h"
#include "xf86str.h"
#include "xf86Priv.h"
#include "dgaproc.h"
#include "xf86dgastr.h"
#include "colormapst.h"
#include "pixmapstr.h"
#include "inputstr.h"
#include "XIproto.h"
#include "globals.h"
#include "servermd.h"
#include "micmap.h"
#ifdef XKB
#include "XKBsrv.h"
#endif

static unsigned long DGAGeneration = 0;
static int DGAScreenIndex = -1;

static Bool DGACloseScreen(int i, ScreenPtr pScreen);
static void DGADestroyColormap(ColormapPtr pmap);
static void DGAInstallColormap(ColormapPtr pmap);

static int
DGASetDGAMode(
   int index,
   int num,
   DGADevicePtr devRet
);

static void
DGACopyModeInfo(
   DGAModePtr mode,
   XDGAModePtr xmode
);

#ifdef XFree86LOADER
int *XDGAEventBase = NULL;
#else
int *XDGAEventBase = &DGAEventBase;
#endif

#define DGA_GET_SCREEN_PRIV(pScreen) \
	((DGAScreenPtr)((pScreen)->devPrivates[DGAScreenIndex].ptr))


typedef struct _FakedVisualList{
   Bool free;
   VisualPtr pVisual;
   struct _FakedVisualList *next;
} FakedVisualList;


typedef struct {
   ScrnInfoPtr 		pScrn;
   int			numModes;
   DGAModePtr		modes;
   CloseScreenProcPtr	CloseScreen;
   DestroyColormapProcPtr DestroyColormap;
   InstallColormapProcPtr InstallColormap;
   DGADevicePtr		current;
   DGAFunctionPtr	funcs;
   int			input;
   ClientPtr		client;
   int			pixmapMode;
   FakedVisualList	*fakedVisuals;
   ColormapPtr 		dgaColormap;
   ColormapPtr		savedColormap;
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

    if(!funcs->Sync || !funcs->SetMode || !funcs->SetViewport ||
	!funcs->GetViewport || !funcs->OpenFramebuffer)
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
    pScreenPriv->funcs = funcs;
    pScreenPriv->input = 0;
    pScreenPriv->client = NULL;
    pScreenPriv->fakedVisuals = NULL;
    pScreenPriv->dgaColormap = NULL;
    pScreenPriv->savedColormap = NULL;
    
    for(i = 0; i < num; i++)
	modes[i].num = i + 1;

#ifdef PANORAMIX
     if(!noPanoramiXExtension)
	for(i = 0; i < num; i++)
	    modes[i].flags &= ~DGA_PIXMAP_AVAILABLE;
#endif


    pScreen->devPrivates[DGAScreenIndex].ptr = (pointer)pScreenPriv;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = DGACloseScreen;
    pScreenPriv->DestroyColormap = pScreen->DestroyColormap;
    pScreen->DestroyColormap = DGADestroyColormap;
    pScreenPriv->InstallColormap = pScreen->InstallColormap;
    pScreen->InstallColormap = DGAInstallColormap;

    pScrn->SetDGAMode = DGASetDGAMode;

    return TRUE;
}


static void
FreeMarkedVisuals(ScreenPtr pScreen)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    FakedVisualList *prev, *curr, *tmp;

    if(!pScreenPriv->fakedVisuals)
	return;

    prev = NULL;
    curr = pScreenPriv->fakedVisuals;

    while(curr) {
	if(curr->free) {
	    tmp = curr;
	    curr = curr->next;
	    if(prev)
		prev->next = curr;
	    else 
		pScreenPriv->fakedVisuals = curr;
	    xfree(tmp->pVisual);
	    xfree(tmp);
	} else {
	    prev = curr;
	    curr = curr->next;
	}
    }
}


static Bool 
DGACloseScreen(int i, ScreenPtr pScreen)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

   FreeMarkedVisuals(pScreen);

   pScreen->CloseScreen = pScreenPriv->CloseScreen;
   pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
   pScreen->InstallColormap = pScreenPriv->InstallColormap;

   /* DGAShutdown() should have ensured that no DGA
	screen were active by here */

   xfree(pScreenPriv);

   return((*pScreen->CloseScreen)(i, pScreen));
}


static void 
DGADestroyColormap(ColormapPtr pmap)
{
   ScreenPtr pScreen = pmap->pScreen;
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
   VisualPtr pVisual = pmap->pVisual;

   if(pScreenPriv->fakedVisuals) {
	FakedVisualList *curr = pScreenPriv->fakedVisuals;
	
	while(curr) {
	    if(curr->pVisual == pVisual) {
		/* We can't get rid of them yet since FreeColormap
		   still needs the pVisual during the cleanup */
		curr->free = TRUE;
		break;
	    }
	    curr = curr->next;
	}
   }  

   if(pScreenPriv->DestroyColormap) {
        pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
        (*pScreen->DestroyColormap)(pmap);
        pScreen->DestroyColormap = DGADestroyColormap;
   }
}


static void 
DGAInstallColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    if(pScreenPriv->current) {
	if (pmap != pScreenPriv->dgaColormap) {
	    pScreenPriv->savedColormap = pmap;
	    pmap = pScreenPriv->dgaColormap;
	}
    }

    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    (*pScreen->InstallColormap)(pmap);
    pScreen->InstallColormap = DGAInstallColormap;
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
	    PixmapPtr oldPix = pScreenPriv->current->pPix;
	    if(oldPix) {
		if(oldPix->drawable.id)
		    FreeResource(oldPix->drawable.id, RT_NONE);
		else
		    (*pScreen->DestroyPixmap)(oldPix);
	    }
	    xfree(pScreenPriv->current);
	    pScreenPriv->current = NULL;
	    pScrn->vtSema = TRUE;
	    if(pScreenPriv->savedColormap) {
		miInstalledMaps[index] = pScreenPriv->savedColormap;
		pScreenPriv->savedColormap = NULL;
	    }
	    pScreenPriv->dgaColormap = NULL;
	    (*pScreenPriv->funcs->SetMode)(pScreenPriv->pScrn, NULL);
	    (*pScrn->SaveRestoreImage)(index, RestoreImage);

	    FreeMarkedVisuals(pScreen);
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
			pMode->bytesPerScanline,
 			(pointer)(pMode->address));
        }
   }

   devRet->mode = device->mode = pMode;
   devRet->pPix = device->pPix = pPix;
   pScreenPriv->current = device;
   pScreenPriv->pixmapMode = FALSE;

   return Success;
}


/*********** exported ones ***************/


Bool
DGAChangePixmapMode(int index, int *x, int *y, int mode)
{
   DGAScreenPtr pScreenPriv;
   DGADevicePtr pDev;
   DGAModePtr   pMode;
   PixmapPtr    pPix;

   if(DGAScreenIndex < 0)
	return FALSE;

   pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   if(!pScreenPriv || !pScreenPriv->current || !pScreenPriv->current->pPix)
	return FALSE;

   pDev = pScreenPriv->current;
   pPix = pDev->pPix;
   pMode = pDev->mode;

   if(mode) {
	int shift = 2;

	if(*x > (pMode->pixmapWidth - pMode->viewportWidth))
	    *x = pMode->pixmapWidth - pMode->viewportWidth;
	if(*y > (pMode->pixmapHeight - pMode->viewportHeight))
	    *y = pMode->pixmapHeight - pMode->viewportHeight;

	switch(xf86Screens[index]->bitsPerPixel) {
	case 16: shift = 1;  break;
	case 32: shift = 0;  break;
	default: break;
	}

	if(BITMAP_SCANLINE_PAD == 64)
	    shift++;

	*x = (*x >> shift) << shift;

	pPix->drawable.x = *x; 
	pPix->drawable.y = *y; 
	pPix->drawable.width = pMode->viewportWidth; 
	pPix->drawable.height = pMode->viewportHeight; 
   } else {
	pPix->drawable.x = 0; 
	pPix->drawable.y = 0; 
	pPix->drawable.width = pMode->pixmapWidth; 
	pPix->drawable.height = pMode->pixmapHeight; 
   }
   pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;
   pScreenPriv->pixmapMode = mode;

   return TRUE;
}

Bool
DGAAvailable(int index) 
{
   if(DGAScreenIndex < 0)
	return FALSE;

   if(DGA_GET_SCREEN_PRIV(screenInfo.screens[index]))
	return TRUE;

   return FALSE;
}

Bool
DGAActive(int index) 
{
   DGAScreenPtr pScreenPriv;

   if(DGAScreenIndex < 0)
	return FALSE;

   pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   if(pScreenPriv && pScreenPriv->current)
	return TRUE;

   return FALSE;
}



/* Called by the event code in case the server is abruptly terminated */

void 
DGAShutdown()
{
    DGAScreenPtr pScreenPriv;
    ScreenPtr pScreen;
    int i;

    if(DGAScreenIndex < 0)
	return;

    for(i = 0; i < screenInfo.numScreens; i++) {
	pScreen = screenInfo.screens[i];	
	pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

	if(pScreenPriv && pScreenPriv->current) {
	    if(pScreenPriv->current->pPix)
		(*pScreen->DestroyPixmap)(pScreenPriv->current->pPix);
	    xfree(pScreenPriv->current);

	    (*pScreenPriv->funcs->SetMode)(pScreenPriv->pScrn, NULL);
	    pScreenPriv->pScrn->vtSema = TRUE;
	}
    }
}

/* Called by the extension to initialize a mode */

int
DGASetMode(
   int index,
   int num,
   XDGAModePtr mode,
   PixmapPtr *pPix
){
    ScrnInfoPtr pScrn = xf86Screens[index];
    DGADeviceRec device;
    int ret;

    /* We rely on the extension to check that DGA is available */ 

    ret = (*pScrn->SetDGAMode)(index, num, &device);
    if((ret == Success) && num) {
	DGACopyModeInfo(device.mode, mode);
	*pPix = device.pPix;
    }

    return ret;
}

/* Called from the extension to let the DDX know which events are requested */

void
DGASelectInput(
   int index,
   ClientPtr client,
   long mask
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */
   pScreenPriv->client = client;
   pScreenPriv->input = mask;
}

int 
DGAGetViewportStatus(int index) 
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is active */ 

   return((*pScreenPriv->funcs->GetViewport)(pScreenPriv->pScrn));
}

int
DGASetViewport(
   int index,
   int x, int y,
   int mode
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   (*pScreenPriv->funcs->SetViewport)(pScreenPriv->pScrn, x, y, mode);
   return Success;
}


static int
BitsClear(CARD32 data)
{
   int bits = 0;
   CARD32 mask;

   for(mask = 1; mask; mask <<= 1) {
	if(!(data & mask)) bits++;
	else break;
   }

   return bits;
}

int
DGACreateColormap(int index, ClientPtr client, int id, int mode, int alloc)
{
   ScreenPtr pScreen = screenInfo.screens[index];
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
   FakedVisualList *fvlp;
   VisualPtr pVisual;
   DGAModePtr pMode;
   ColormapPtr pmap;

   if(!mode || (mode > pScreenPriv->numModes))
	return BadValue;

   if((alloc != AllocNone) && (alloc != AllocAll))
	return BadValue;

   pMode = &(pScreenPriv->modes[mode - 1]);

   if(!(pVisual = xalloc(sizeof(VisualRec))))
	return BadAlloc;

   pVisual->vid = FakeClientID(0);
   pVisual->class = pMode->visualClass;
   pVisual->nplanes = pMode->depth;
   pVisual->ColormapEntries = 1 << pMode->depth;
   pVisual->bitsPerRGBValue = (pMode->depth + 2) / 3;

   switch (pVisual->class) {
   case PseudoColor:
   case GrayScale:
   case StaticGray:
	pVisual->bitsPerRGBValue = 8; /* not quite */
	pVisual->redMask     = 0;
	pVisual->greenMask   = 0;
	pVisual->blueMask    = 0;
	pVisual->offsetRed   = 0;
	pVisual->offsetGreen = 0;
	pVisual->offsetBlue  = 0;
	break;
   case DirectColor:
   case TrueColor:
	pVisual->ColormapEntries = 1 << pVisual->bitsPerRGBValue;
                /* fall through */
   case StaticColor:
	pVisual->redMask = pMode->red_mask;
	pVisual->greenMask = pMode->green_mask;
	pVisual->blueMask = pMode->blue_mask;
	pVisual->offsetRed   = BitsClear(pVisual->redMask);
	pVisual->offsetGreen = BitsClear(pVisual->greenMask);
	pVisual->offsetBlue  = BitsClear(pVisual->blueMask);
   }

   if(!(fvlp = xalloc(sizeof(FakedVisualList)))) {
	xfree(pVisual);
	return BadAlloc;
   }

   fvlp->free = FALSE;
   fvlp->pVisual = pVisual;
   fvlp->next = pScreenPriv->fakedVisuals;
   pScreenPriv->fakedVisuals = fvlp;

   LEGAL_NEW_RESOURCE(id, client);

   return CreateColormap(id, pScreen, pVisual, &pmap, alloc, client->index);
}

/*  Called by the extension to install a colormap on DGA active screens */

void
DGAInstallCmap(ColormapPtr cmap)
{
    ScreenPtr pScreen = cmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    /* We rely on the extension to check that DGA is active */ 

    if(!pScreenPriv->dgaColormap) 
	pScreenPriv->savedColormap = miInstalledMaps[pScreen->myNum];
    pScreenPriv->dgaColormap = cmap;    

    (*pScreen->InstallColormap)(cmap);
}

int
DGASync(int index)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is active */

   (*pScreenPriv->funcs->Sync)(pScreenPriv->pScrn);

   return Success;
}

int
DGAFillRect(
   int index,
   int x, int y, int w, int h,
   unsigned long color
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   
   /* We rely on the extension to check that DGA is active */

   if(pScreenPriv->funcs->FillRect && 
	(pScreenPriv->current->mode->flags & DGA_FILL_RECT)) {

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
   
   /* We rely on the extension to check that DGA is active */

   if(pScreenPriv->funcs->BlitRect &&
	(pScreenPriv->current->mode->flags & DGA_BLIT_RECT)) {

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
   
   /* We rely on the extension to check that DGA is active */

   if(pScreenPriv->funcs->BlitTransRect && 
	(pScreenPriv->current->mode->flags & DGA_BLIT_RECT_TRANS)) {

	(*pScreenPriv->funcs->BlitTransRect)(pScreenPriv->pScrn, 	
		srcx, srcy, w, h, dstx, dsty, color);
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
DGAGetModeInfo(
  int index,
  XDGAModePtr mode,
  int num
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
   /* We rely on the extension to check that DGA is available */

   if((num <= 0) || (num > pScreenPriv->numModes))
	return BadValue;

   DGACopyModeInfo(&(pScreenPriv->modes[num - 1]), mode);

   return Success;
}


static void
DGACopyModeInfo(
   DGAModePtr mode,
   XDGAModePtr xmode
){
   DisplayModePtr dmode = mode->mode;

   xmode->num = mode->num;
   xmode->name = dmode->name;
   xmode->VSync_num = dmode->Clock * 1000.0; 
   xmode->VSync_den = dmode->HTotal * dmode->VTotal;
   xmode->flags = mode->flags;
   xmode->imageWidth = mode->imageWidth;
   xmode->imageHeight = mode->imageHeight;
   xmode->pixmapWidth = mode->pixmapWidth;
   xmode->pixmapHeight = mode->pixmapHeight;
   xmode->bytesPerScanline = mode->bytesPerScanline;
   xmode->byteOrder = mode->byteOrder;
   xmode->depth = mode->depth;
   xmode->bitsPerPixel = mode->bitsPerPixel;
   xmode->red_mask = mode->red_mask;
   xmode->green_mask = mode->green_mask;
   xmode->blue_mask = mode->blue_mask;
   xmode->visualClass = mode->visualClass;
   xmode->viewportWidth = mode->viewportWidth;
   xmode->viewportHeight = mode->viewportHeight;
   xmode->xViewportStep = mode->xViewportStep;
   xmode->yViewportStep = mode->yViewportStep;
   xmode->maxViewportX = mode->maxViewportX;
   xmode->maxViewportY = mode->maxViewportY;
   xmode->viewportFlags = mode->viewportFlags;
   xmode->reserved1 = mode->reserved1;
   xmode->reserved2 = mode->reserved2;
   xmode->offset = mode->offset;

   if(dmode->Flags & V_INTERLACE) xmode->flags |= DGA_INTERLACED;
   if(dmode->Flags & V_DBLSCAN) xmode->flags |= DGA_DOUBLESCAN;
}


Bool 
DGAVTSwitch(void)
{
    ScreenPtr pScreen;
    int i;

    for(i = 0; i < screenInfo.numScreens; i++) {
       pScreen = screenInfo.screens[i];	

       /* Alternatively, this could send events to DGA clients */

       if(DGAScreenIndex >= 0) {
	   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

	   if(pScreenPriv && pScreenPriv->current)
		return FALSE;
       }
    }

   return TRUE;
}


/* We have the power to steal or modify events that are about to get queued */

extern InputInfo inputInfo;

Bool
DGAStealKeyEvent(int index, xEvent *e)
{
   DGAScreenPtr pScreenPriv;
   DeviceIntPtr keybd;
   KeyClassPtr keyc;
   int key, bit, i;
   BYTE *kptr;
   CARD8 modifiers, mask;

   if(DGAScreenIndex < 0) /* no DGA */
	return FALSE;

   pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   if(!pScreenPriv || !pScreenPriv->current) /* no direct mode */
	return FALSE;

   keybd = (DeviceIntPtr)xf86Info.pKeyboard;
   keyc = keybd->key;

   e->u.keyButtonPointer.state = (keyc->state |
				  inputInfo.pointer->button->state);

   key = e->u.u.detail;
   kptr = &keyc->down[key >> 3];
   bit = 1 << (key & 7);
   modifiers = keyc->modifierMap[key];

   if(e->u.u.type == KeyPress) {
	if(!(*kptr & bit)) {
	    *kptr |= bit;
	    keyc->prev_state = keyc->state;
	    keyc->state |= modifiers;

	    for(mask = 0x80, i = 0; mask; mask >>= 1, i++) {
		if(mask & modifiers)
		    keyc->modifierKeyCount[i]++;
	    }
	}
   } else {  /* KeyRelease */
	if(!(*kptr & bit))
	    return TRUE;
	*kptr &= ~bit;
	keyc->prev_state = keyc->state;
	for(mask = 0x80, i = 0; mask; mask >>= 1, i++) {
	    if(mask & modifiers) {
		if(--keyc->modifierKeyCount[i] <= 0) {
		    keyc->state &= ~mask;
		    keyc->modifierKeyCount[i] = 0;	
		}	
	    }
	}
   }
   
   if(pScreenPriv->client && !pScreenPriv->client->clientGone) {
	Bool GrabEvent = FALSE;

        switch(e->u.u.type) {
        case KeyPress:
             if(pScreenPriv->input & KeyPressMask) 
                GrabEvent = TRUE;
             break;
        case KeyRelease:
             if(pScreenPriv->input & KeyReleaseMask) 
                GrabEvent = TRUE;
             break;
        }

        if(GrabEvent){ /* steal this event */
            dgaEvent de;
            
            de.u.u.type = e->u.u.type + *XDGAEventBase;
	    de.u.u.detail = key;
            de.u.u.sequenceNumber = pScreenPriv->client->sequence;
            de.u.event.time = e->u.keyButtonPointer.time;
            de.u.event.screen = index;
            de.u.event.state = e->u.keyButtonPointer.state;

            TryClientEvents(pScreenPriv->client, (xEvent*)&de, 1, 
                        NoEventMask, NoEventMask, NullGrab);
	}
	return TRUE;
    }


   /* Not sure how best to handle this stuff. It's only for
      DGA 1.0 compatibility.  Hopefully, we can remove this
      some day */

   if(((DeviceIntPtr)(xf86Info.pKeyboard))->grab){
	/* these would be non-sense otherwise */
	e->u.keyButtonPointer.eventX =  0;
	e->u.keyButtonPointer.eventY =  0;
	e->u.keyButtonPointer.rootX =   0;
	e->u.keyButtonPointer.rootY =   0;

	keybd->public.processInputProc(e, keybd, 1);
   }

   /* Direct mode but the client doesn't want the events.
      We have to keep them from hitting the other windows. 
    */

   return TRUE;
}


Bool
DGAStealMouseEvent(int index, xEvent *e, int dx, int dy)
{
   DGAScreenPtr pScreenPriv;
   ButtonClassPtr butc;

   if(DGAScreenIndex < 0) /* no DGA */
	return FALSE;

   pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   if(!pScreenPriv || !pScreenPriv->current) /* no direct mode */
	return FALSE;

   /* I hope this part is correct.  Motion events are different
	in the sense that the mipointer code is relied on to fill 
	out the rest of the event info.  We steal the event before 
	then so we need to fill that info out here */

   if(dx | dy) { 
	e->u.u.type = MotionNotify;
	e->u.keyButtonPointer.time = GetTimeInMillis();
   }

   butc = ((DeviceIntPtr)xf86Info.pMouse)->button;

   e->u.keyButtonPointer.state = butc->state | (
#ifdef XKB
	!noXkbExtension ? inputInfo.keyboard->key->xkbInfo->state.grab_mods :
#endif
	inputInfo.keyboard->key->state);

   if(e->u.u.type != MotionNotify) {
	ButtonClassPtr butc = ((DeviceIntPtr)xf86Info.pMouse)->button;
 	int key = e->u.u.detail;
	BYTE *kptr = &butc->down[key >> 3];
	int bit = 1 << (key & 7);

	if(e->u.u.type == ButtonPress) {
	    butc->buttonsDown++;
	    butc->motionMask = ButtonMotionMask;
	    *kptr |= bit;
	    if (!e->u.u.detail)
		return TRUE;
	    if (e->u.u.detail <= 5)
		butc->state |= (Button1Mask >> 1) << e->u.u.detail;
	} else { /* ButtonRelease */
	    if (!--butc->buttonsDown)
		butc->motionMask = 0;
	    *kptr &= ~bit;
	    if (!e->u.u.detail)
		return TRUE;
	    if (e->u.u.detail <= 5)
		butc->state &= ~((Button1Mask >> 1) << e->u.u.detail);
	}
   }
  
   if(pScreenPriv->client && !pScreenPriv->client->clientGone) {
	Bool GrabEvent = FALSE;

	switch(e->u.u.type) {
	case MotionNotify:
	     if(pScreenPriv->input & PointerMotionMask) 
		GrabEvent = TRUE;
	     break;
	case ButtonPress:
	     if(pScreenPriv->input & ButtonPressMask) 
		GrabEvent = TRUE;
	     break;
	case ButtonRelease:
	     if(pScreenPriv->input & ButtonReleaseMask) 
		GrabEvent = TRUE;
	     break;
	}

	if(GrabEvent){ /* steal this event */
	    dgaEvent de;
	    
	    de.u.u.type = e->u.u.type + *XDGAEventBase;
	    de.u.u.detail = e->u.u.detail;
            de.u.u.sequenceNumber = pScreenPriv->client->sequence;
	    de.u.event.dx = dx;
	    de.u.event.dy = dy;
	    de.u.event.time = e->u.keyButtonPointer.time;
	    de.u.event.screen = index;
	    de.u.event.state = e->u.keyButtonPointer.state;

	    TryClientEvents(pScreenPriv->client, (xEvent*)&de, 1, 
			NoEventMask, NoEventMask, NullGrab);
	}
	return TRUE;
   }
	

   /* Not sure how best to handle this stuff. It's only for
      DGA 1.0 compatibility.  Hopefully, we can remove this
      some day */

   if(((DeviceIntPtr)(xf86Info.pMouse))->grab) {
	e->u.keyButtonPointer.eventX =  dx;
	e->u.keyButtonPointer.eventY =  dy;
	e->u.keyButtonPointer.rootX =   dx;
	e->u.keyButtonPointer.rootY =   dy;
	DeliverGrabbedEvent(e, (xf86Info.pMouse), FALSE, 1);
   }

   /* Direct mode but the client doesn't want the events.
      We have to keep them from hitting the other windows. 
    */

   return TRUE;
}


Bool 
DGAOpenFramebuffer(
   int index,
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */

   return (*pScreenPriv->funcs->OpenFramebuffer)(pScreenPriv->pScrn, 
				name, mem, size, offset, flags);
}

void
DGACloseFramebuffer(int index)
{
   DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

   /* We rely on the extension to check that DGA is available */
   if(pScreenPriv->funcs->CloseFramebuffer)
	(*pScreenPriv->funcs->CloseFramebuffer)(pScreenPriv->pScrn);
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
   p = ((pScrn->displayWidth * (pScrn->bitsPerPixel >> 3)) + 3) & ~3L;

   for(i = 0; i < pScreenPriv->numModes; i++) {
	mode = &(pScreenPriv->modes[i]);
  	      
	if((mode->viewportWidth == w) && (mode->viewportHeight == h) &&
		(mode->bytesPerScanline == p) && 
		(mode->bitsPerPixel == pScrn->bitsPerPixel)) {

		return mode->num;
	}
   }

   return 0;
}

