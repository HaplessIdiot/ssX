
/*
 * file: apm_dga.c
 * ported from s3virge, ported from mga
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_dga.c,v 1.1 1999/07/10 12:17:27 dawes Exp $ */


#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xaa.h"
#include "xaalocal.h"
#include "apm.h"
#include "dgaproc.h"


static Bool ApmOpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
					int *, int *, int *);
static Bool ApmSetMode(ScrnInfoPtr, DGAModePtr);
static int  ApmGetViewport(ScrnInfoPtr);
static void ApmSetViewport(ScrnInfoPtr, int, int, int);
static void ApmFillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void ApmBlitRect(ScrnInfoPtr, int, int, int, int, int, int);
static void ApmBlitTransRect(ScrnInfoPtr, int, int, int, int, int, int, 
					unsigned long);
static void ApmSync(ScrnInfoPtr);

static
DGAFunctionRec ApmDGAFuncs = {
    ApmOpenFramebuffer,
    NULL,
    ApmSetMode,
    ApmSetViewport,
    ApmGetViewport,
    ApmSync,
    ApmFillRect,
    ApmBlitRect,
    ApmBlitTransRect
};

/*
 * Placeholder
 */
void
ApmSync(ScrnInfoPtr pScrn)
{
}


Bool
ApmDGAInit(ScreenPtr pScreen)
{   
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    DGAModePtr modes = NULL, newmodes = NULL, currentMode;
    DisplayModePtr pMode, firstMode;
    int Bpp = pScrn->bitsPerPixel >> 3;
    int num = 0;
    Bool oneMore;


    
    pMode = firstMode = pScrn->modes;

    while(pMode) {
	/* The Apm driver wasn't designed with switching depths in
	   mind.  Subsequently, large chunks of it will probably need
	   to be rewritten to accommodate depth changes in DGA mode */

	if(pScrn->displayWidth != pMode->HDisplay) {
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
	currentMode->xViewportStep = 3;
	currentMode->yViewportStep = 1;
	currentMode->viewportFlags = DGA_FLIP_RETRACE;
	currentMode->offset = 0;
	currentMode->address = (pointer)(pApm->LinAddress +
			((char *)pApm->FbBase - (char *)pApm->LinMap));
/*cep*/
  xf86ErrorFVerb(4, 
	"	ApmDGAInit firstone vpWid=%d, vpHgt=%d, Bpp=%d, mdbitsPP=%d\n",
		currentMode->viewportWidth,
		currentMode->viewportHeight,
		Bpp,
		currentMode->bitsPerPixel
		 );
		

	if(oneMore) { /* first one is narrow width */
	    currentMode->bytesPerScanline = ((pMode->HDisplay * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pMode->HDisplay;
	    currentMode->imageHeight =  (pScrn->videoRam * 1024 - pApm->OffscreenReserved) /
					currentMode->bytesPerScanline; 
	    currentMode->imageHeight =  pMode->VDisplay;
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	    oneMore = FALSE;
	if(!pApm->NoAccel && Bpp * 8 == currentMode->bitsPerPixel &&
		(currentMode->imageWidth ==  640 ||
		 currentMode->imageWidth ==  800 ||
		 currentMode->imageWidth == 1024 ||
		 currentMode->imageWidth == 1152 ||
		 currentMode->imageWidth == 1280 ||
		 currentMode->imageWidth == 1600))
	   currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT |
				   DGA_BLIT_RECT_TRANS;

/*cep*/
  xf86ErrorFVerb(4, 
	"	ApmDGAInit imgHgt=%d, ram=%d, bytesPerScanl=%d\n",
		currentMode->imageHeight,
		pScrn->videoRam << 10,
		currentMode->bytesPerScanline );
		
	    goto SECOND_PASS;
	} else {
	    currentMode->bytesPerScanline = 
			((pScrn->displayWidth * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pScrn->displayWidth;
	    currentMode->imageHeight =  (pScrn->videoRam * 1024 - pApm->OffscreenReserved) /
					currentMode->bytesPerScanline; 
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	    if(!pApm->NoAccel && Bpp * 8 == currentMode->bitsPerPixel &&
		    (currentMode->imageWidth ==  640 ||
		     currentMode->imageWidth ==  800 ||
		     currentMode->imageWidth == 1024 ||
		     currentMode->imageWidth == 1152 ||
		     currentMode->imageWidth == 1280 ||
		     currentMode->imageWidth == 1600))
	       currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT |
				       DGA_BLIT_RECT_TRANS;
	}	    

	pMode = pMode->next;
	if(pMode == firstMode)
	   break;
    }

    pApm->numDGAModes = num;
    pApm->DGAModes = modes;

    if (pApm->AccelInfoRec)
	ApmDGAFuncs.Sync = pApm->AccelInfoRec->Sync;
    return DGAInit(pScreen, &ApmDGAFuncs, modes, num);  
}


static Bool
ApmSetMode(
    ScrnInfoPtr pScrn,
    DGAModePtr pMode
)
{
    static int OldDisplayWidth[MAXSCREENS];
    int index = pScrn->pScreen->myNum, c;
    APMDECL(pScrn);

    if(!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
	
	pScrn->displayWidth = OldDisplayWidth[index];
	
        pScrn->SwitchMode(index, pScrn->currentMode, 0);
	pApm->DGAactive = FALSE;
    } else {
	if(!pApm->DGAactive) {  /* save the old parameters */
	    OldDisplayWidth[index] = pScrn->displayWidth;

	    pApm->DGAactive = TRUE;
	}

	pScrn->displayWidth = (8 * pMode->bytesPerScanline) / 
			      pMode->bitsPerPixel;

        pScrn->SwitchMode(index, pMode->mode, 0);
    }

    switch(pScrn->bitsPerPixel) {
    case 8:
	c = DEC_BITDEPTH_8;
	break;

    case 16:
	c = DEC_BITDEPTH_16;
	break;

    case 24:
	c = DEC_BITDEPTH_24;
	break;

    case 32:
	c = DEC_BITDEPTH_32;
	break;

    default:
	c = 0;
	break;
    }

    switch(pScrn->displayWidth) {
    case 640:
	c |= DEC_WIDTH_640;
	break;
   
    case 800:
	c |= DEC_WIDTH_800;
	break;
   
    case 1024:
	c |= DEC_WIDTH_1024;
	break;
   
    case 1152:
	c |= DEC_WIDTH_1152;
	break;
   
    case 1280:
	c |= DEC_WIDTH_1280;
	break;
   
    case 1600:
	c |= DEC_WIDTH_1600;
	break;
    }

    pApm->Setup_DEC = (pApm->Setup_DEC & ~(DEC_BITDEPTH_MASK|DEC_WIDTH_MASK)) | c;
    pApm->displayWidth	= pScrn->display->virtualX;
    pApm->displayHeight	= pScrn->display->virtualY;
    pApm->bitsPerPixel	= pScrn->bitsPerPixel;
    pApm->bytesPerScanline= (pApm->displayWidth * pApm->bitsPerPixel) >> 3;

    return TRUE;
}



static int  
ApmGetViewport(
  ScrnInfoPtr pScrn
)
{
    return 0;
}

static void 
ApmSetViewport(
    ScrnInfoPtr pScrn, 
    int x, int y, 
    int flags
)
{
    APMDECL(pScrn);

    pScrn->AdjustFrame(pScrn->pScreen->myNum, x, y, flags);
    if (pApm->VGAMap) {
	/* Wait until vertical retrace is in progress. */
	while (APMVGAB(0x3DA) & 0x08);
	while (!(APMVGAB(0x3DA) & 0x08));
    }
    else {
	/* Wait until vertical retrace is in progress. */
	while (inb(0x3DA) & 0x08);
	while (!(inb(0x3DA) & 0x08));            
    }
}

static void 
ApmFillRect (
    ScrnInfoPtr pScrn, 
    int x, int y, int w, int h, 
    unsigned long color
)
{
    APMDECL(pScrn);

    if(pApm->AccelInfoRec) {
	(*pApm->AccelInfoRec->SetupForSolidFill)(pScrn, color, GXcopy, ~0);
	(*pApm->AccelInfoRec->SubsequentSolidFillRect)(pScrn, x, y, w, h);
	SET_SYNC_FLAG(pApm->AccelInfoRec);
    }
}

static void 
ApmBlitRect(
    ScrnInfoPtr pScrn, 
    int srcx, int srcy, 
    int w, int h, 
    int dstx, int dsty
)
{
    APMDECL(pScrn);

    if(pApm->AccelInfoRec) {
	int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
	int ydir = (srcy < dsty) ? -1 : 1;

	(*pApm->AccelInfoRec->SetupForScreenToScreenCopy)(
		pScrn, xdir, ydir, GXcopy, ~0, -1);
	(*pApm->AccelInfoRec->SubsequentScreenToScreenCopy)(
		pScrn, srcx, srcy, dstx, dsty, w, h);
	SET_SYNC_FLAG(pApm->AccelInfoRec);
    }
}

static void 
ApmBlitTransRect(
    ScrnInfoPtr pScrn, 
    int srcx, int srcy, 
    int w, int h, 
    int dstx, int dsty,
    unsigned long color
)
{
    APMDECL(pScrn);

    if(pApm->AccelInfoRec) {
	int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
	int ydir = (srcy < dsty) ? -1 : 1;

	(*pApm->AccelInfoRec->SetupForScreenToScreenCopy)(
		pScrn, xdir, ydir, GXcopy, ~0, (int)color);
	(*pApm->AccelInfoRec->SubsequentScreenToScreenCopy)(
		pScrn, srcx, srcy, dstx, dsty, w, h);
	SET_SYNC_FLAG(pApm->AccelInfoRec);
    }
}

static Bool 
ApmOpenFramebuffer(
    ScrnInfoPtr pScrn, 
    char **name,
    unsigned char **mem,
    int *size,
    int *offset,
    int *flags
)
{
    APMDECL(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)(pApm->LinAddress +
			((char *)pApm->FbBase - (char *)pApm->LinMap));
    *size = pScrn->videoRam << 10;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
