/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dga.c,v 1.6 1999/04/11 13:10:58 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xaa.h"
#include "xaalocal.h"
#include "mga_bios.h"
#include "mga.h"
#include "mga_reg.h"
#include "mga_macros.h"
#include "dgaproc.h"


static Bool MGA_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
					int *, int *, int *);
static Bool MGA_SetMode(ScrnInfoPtr, DGAModePtr);
static int  MGA_GetViewport(ScrnInfoPtr);
static void MGA_SetViewport(ScrnInfoPtr, int, int, int);
static void MGA_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void MGA_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);
static void MGA_BlitTransRect(ScrnInfoPtr, int, int, int, int, int, int, 
					unsigned long);

static
DGAFunctionRec MGA_DGAFuncs = {
   MGA_OpenFramebuffer,
   NULL,
   MGA_SetMode,
   MGA_SetViewport,
   MGA_GetViewport,
   MGAStormSync,
   MGA_FillRect,
   MGA_BlitRect,
   MGA_BlitTransRect
};


Bool
MGADGAInit(ScreenPtr pScreen)
{   
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   MGAPtr pMga = MGAPTR(pScrn);
   DGAModePtr modes = NULL, newmodes = NULL, currentMode;
   DisplayModePtr pMode, firstMode;
   int Bpp = pScrn->bitsPerPixel >> 3;
   int num = 0;
   Bool oneMore;

   pMode = firstMode = pScrn->modes;

   while(pMode) {
	/* The MGA driver wasn't designed with switching depths in
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
	if(!pMga->NoAccel)
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
	currentMode->viewportWidth = pMode->HDisplay;
	currentMode->viewportHeight = pMode->VDisplay;
	currentMode->xViewportStep = 1;
	currentMode->yViewportStep = 1;
	currentMode->viewportFlags = DGA_FLIP_RETRACE;
	currentMode->offset = pMga->YDstOrg * (pScrn->bitsPerPixel / 8);
	currentMode->address = pMga->FbStart;

	if(oneMore) { /* first one is narrow width */
	    currentMode->bytesPerScanline = ((pMode->HDisplay * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pMode->HDisplay;
	    currentMode->imageHeight =  pMga->FbUsableSize /
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
	    currentMode->imageHeight =  pMga->FbUsableSize /
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

   pMga->numDGAModes = num;
   pMga->DGAModes = modes;

   return DGAInit(pScreen, &MGA_DGAFuncs, modes, num);  
}


static Bool
MGA_SetMode(
   ScrnInfoPtr pScrn,
   DGAModePtr pMode
){
   static int OldDisplayWidth[MAXSCREENS];
   int index = pScrn->pScreen->myNum;

   MGAPtr pMga = MGAPTR(pScrn);

   if(!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
	
	pScrn->displayWidth = OldDisplayWidth[index];
	
        MGASwitchMode(index, pScrn->currentMode, 0);
	pMga->DGAactive = FALSE;
   } else {
	if(!pMga->DGAactive) {  /* save the old parameters */
	    OldDisplayWidth[index] = pScrn->displayWidth;

	    pMga->DGAactive = TRUE;
	}

	pScrn->displayWidth = pMode->bytesPerScanline / 
			      (pMode->bitsPerPixel >> 3);

        MGASwitchMode(index, pMode->mode, 0);
   }
   
   return TRUE;
}



static int  
MGA_GetViewport(
  ScrnInfoPtr pScrn
){
    MGAPtr pMga = MGAPTR(pScrn);

    return pMga->DGAViewportStatus;
}

static void 
MGA_SetViewport(
   ScrnInfoPtr pScrn, 
   int x, int y, 
   int flags
){
   MGAPtr pMga = MGAPTR(pScrn);

   MGAAdjustFrame(pScrn->pScreen->myNum, x, y, flags);
   pMga->DGAViewportStatus = 0;  /* MGAAdjustFrame loops until finished */
}

static void 
MGA_FillRect (
   ScrnInfoPtr pScrn, 
   int x, int y, int w, int h, 
   unsigned long color
){
    MGAPtr pMga = MGAPTR(pScrn);

    if(pMga->AccelInfoRec) {
	(*pMga->AccelInfoRec->SetupForSolidFill)(pScrn, color, GXcopy, ~0);
	(*pMga->AccelInfoRec->SubsequentSolidFillRect)(pScrn, x, y, w, h);
	SET_SYNC_FLAG(pMga->AccelInfoRec);
    }
}

static void 
MGA_BlitRect(
   ScrnInfoPtr pScrn, 
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty
){
    MGAPtr pMga = MGAPTR(pScrn);

    if(pMga->AccelInfoRec) {
	int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
	int ydir = (srcy < dsty) ? -1 : 1;

	(*pMga->AccelInfoRec->SetupForScreenToScreenCopy)(
		pScrn, xdir, ydir, GXcopy, ~0, -1);
	(*pMga->AccelInfoRec->SubsequentScreenToScreenCopy)(
		pScrn, srcx, srcy, dstx, dsty, w, h);
	SET_SYNC_FLAG(pMga->AccelInfoRec);
    }
}


static void 
MGA_BlitTransRect(
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
MGA_OpenFramebuffer(
   ScrnInfoPtr pScrn, 
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    MGAPtr pMga = MGAPTR(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)pMga->FbAddress;
    *size = pMga->FbMapSize;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
