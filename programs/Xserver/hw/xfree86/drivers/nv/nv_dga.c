/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dga.c,v 1.8 1999/06/06 08:48:50 dawes Exp $ */

/* Straight theft from mga-driver */

#include "nv_include.h"
#include "xaalocal.h"
#include "dgaproc.h"


static Bool NV_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
					int *, int *, int *);
static Bool NV_SetMode(ScrnInfoPtr, DGAModePtr);
static int  NV_GetViewport(ScrnInfoPtr);
static void NV_SetViewport(ScrnInfoPtr, int, int, int);
static void NV_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void NV_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);
static void NV_BlitTransRect(ScrnInfoPtr, int, int, int, int, int, int, 
					unsigned long);

static
DGAFunctionRec NV_DGAFuncs = {
   NV_OpenFramebuffer,
   NULL,
   NV_SetMode,
   NV_SetViewport,
   NV_GetViewport,
   NVSync,
   NV_FillRect,
   NV_BlitRect,
   NV_BlitTransRect
};


Bool
NVDGAInit(ScreenPtr pScreen)
{   
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   NVPtr pNv = NVPTR(pScrn);
   DGAModePtr modes = NULL, newmodes = NULL, currentMode;
   DisplayModePtr pMode, firstMode;
   int Bpp = pScrn->bitsPerPixel >> 3;
   int num = 0;
   Bool oneMore;

   pMode = firstMode = pScrn->modes;

   while(pMode) {
	/* The NV driver wasn't designed with switching depths in
	   mind.  Subsequently, large chunks of it will probably need
	   to be rewritten to accommodate depth changes in DGA mode */

	if(0 /*pScrn->displayWidth != pMode->HDisplay*/) {
	    newmodes = xrealloc(modes, (num + 2) * sizeof(DGAModeRec));
	    oneMore = TRUE;
	} else {
	    newmodes = xrealloc(modes, (num + 1) * sizeof(DGAModeRec));
	    oneMore = FALSE;
	}

	if(!newmodes) {
	   xfree(modes);
	   return FALSE;
	}
	modes = newmodes;

SECOND_PASS:

	currentMode = modes + num;
	num++;

	currentMode->mode = pMode;
	currentMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
	if(!pNv->NoAccel)
	   currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
	if(pMode->Flags & V_DBLSCAN)
	   currentMode->flags |= DGA_DOUBLESCAN;
	if(pMode->Flags & V_INTERLACE)
	   currentMode->flags |= DGA_INTERLACED;
	currentMode->byteOrder = pScrn->imageByteOrder;
	currentMode->depth = pScrn->depth;
	currentMode->bitsPerPixel = pScrn->bitsPerPixel;
	currentMode->red_mask = pScrn->mask.red;
	currentMode->green_mask = pScrn->mask.green;
	currentMode->blue_mask = pScrn->mask.blue;
	currentMode->visualClass = (Bpp == 1) ? PseudoColor : TrueColor;
	currentMode->viewportWidth = pMode->HDisplay;
	currentMode->viewportHeight = pMode->VDisplay;
	currentMode->xViewportStep = (3 - pNv->BppShift);
	currentMode->yViewportStep = 1;
	currentMode->viewportFlags = DGA_FLIP_RETRACE;
	currentMode->offset = pNv->YDstOrg * Bpp;
	currentMode->address = pNv->FbStart;

	if(oneMore) { /* first one is narrow width */
	    currentMode->bytesPerScanline = ((pMode->HDisplay * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pMode->HDisplay;
	    currentMode->imageHeight =  pNv->FbUsableSize /
					currentMode->bytesPerScanline; 
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	    oneMore = FALSE;
	    goto SECOND_PASS;
	} else {
	    currentMode->bytesPerScanline = 
			((pScrn->displayWidth * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pScrn->displayWidth;
	    currentMode->imageHeight =  pNv->FbUsableSize /
					currentMode->bytesPerScanline; 
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	}	    

	pMode = pMode->next;
	if(pMode == firstMode)
	   break;
   }

   pNv->numDGAModes = num;
   pNv->DGAModes = modes;

   return DGAInit(pScreen, &NV_DGAFuncs, modes, num);  
}


static Bool
NV_SetMode(
   ScrnInfoPtr pScrn,
   DGAModePtr pMode
){
   static int OldDisplayWidth[MAXSCREENS];
   int index = pScrn->pScreen->myNum;

   NVPtr pNv = NVPTR(pScrn);

   if(!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
	
	pScrn->displayWidth = OldDisplayWidth[index];
	
        NVSwitchMode(index, pScrn->currentMode, 0);
	pNv->DGAactive = FALSE;
   } else {
	if(!pNv->DGAactive) {  /* save the old parameters */
	    OldDisplayWidth[index] = pScrn->displayWidth;

	    pNv->DGAactive = TRUE;
	}

	pScrn->displayWidth = pMode->bytesPerScanline / 
			      (pMode->bitsPerPixel >> 3);

        NVSwitchMode(index, pMode->mode, 0);
   }
   
   return TRUE;
}



static int  
NV_GetViewport(
  ScrnInfoPtr pScrn
){
    NVPtr pNv = NVPTR(pScrn);

    return pNv->DGAViewportStatus;
}

static void 
NV_SetViewport(
   ScrnInfoPtr pScrn, 
   int x, int y, 
   int flags
){
   NVPtr pNv = NVPTR(pScrn);

   NVAdjustFrame(pScrn->pScreen->myNum, x, y, flags);
   pNv->DGAViewportStatus = 0;  /* NVAdjustFrame loops until finished */
}

static void 
NV_FillRect (
   ScrnInfoPtr pScrn, 
   int x, int y, int w, int h, 
   unsigned long color
){
    NVPtr pNv = NVPTR(pScrn);

    if(pNv->AccelInfoRec) {
	(*pNv->AccelInfoRec->SetupForSolidFill)(pScrn, color, GXcopy, ~0);
	(*pNv->AccelInfoRec->SubsequentSolidFillRect)(pScrn, x, y, w, h);
	SET_SYNC_FLAG(pNv->AccelInfoRec);
    }
}

static void 
NV_BlitRect(
   ScrnInfoPtr pScrn, 
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty
){
    NVPtr pNv = NVPTR(pScrn);

    if(pNv->AccelInfoRec) {
	int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
	int ydir = (srcy < dsty) ? -1 : 1;

	(*pNv->AccelInfoRec->SetupForScreenToScreenCopy)(
		pScrn, xdir, ydir, GXcopy, ~0, -1);
	(*pNv->AccelInfoRec->SubsequentScreenToScreenCopy)(
		pScrn, srcx, srcy, dstx, dsty, w, h);
	SET_SYNC_FLAG(pNv->AccelInfoRec);
    }
}


static void 
NV_BlitTransRect(
   ScrnInfoPtr pScrn, 
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty,
   unsigned long color
){
  /* this one should be separate since the XAA function would
     prohibit usage of ~0 as the key */
}


static Bool 
NV_OpenFramebuffer(
   ScrnInfoPtr pScrn, 
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    NVPtr pNv = NVPTR(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)pNv->FbAddress;
    *size = pNv->FbMapSize;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
