/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_dga.c,v 1.4 1999/08/28 14:32:46 dawes Exp $ */
/*
 * file: apm_dga.c
 * ported from s3virge, ported from mga
 *
 */


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

static __inline__ int FindSmallestPitch(ApmPtr pApm, int Bpp, int width)
{
    if (width <= 640)
	return 640;
    else if (width <= 800)
	return 800;
    else if (width <= 1024)
	return 1024;
    else if (width <= 1152)
	return 1152;
    else if (width <= 1280)
	return 1280;
    else if (width <= 1600)
	return 1600;
    return (width + 7) & ~7;
}

static DGAModePtr
ApmSetupDGAMode(ScrnInfoPtr pScrn, DGAModePtr modes, int *num,
		   int bitsPerPixel, int depth, Bool pixmap, int secondPitch,
		   unsigned long red, unsigned long green, unsigned long blue,
		   short visualClass)
{
   DisplayModePtr firstMode, pMode;
   APMDECL(pScrn);
   DGAModePtr mode, newmodes;
   int size, pitch, Bpp = bitsPerPixel >> 3;

SECOND_PASS:

   pMode = firstMode = pScrn->modes;

   while(1) {

	pitch = FindSmallestPitch(pApm, Bpp, pMode->HDisplay);
	size = pitch * Bpp * pMode->VDisplay;

	if((!secondPitch || (pitch != secondPitch)) &&
		(size <= pScrn->videoRam * 1024 - pApm->OffscreenReserved)) {

	    if(secondPitch)
		pitch = secondPitch; 

	    if(!(newmodes = xrealloc(modes, (*num + 1) * sizeof(DGAModeRec))))
		break;

	    modes = newmodes;
	    mode = modes + *num;

	    mode->mode = pMode;
	    mode->flags = DGA_CONCURRENT_ACCESS;

	    if(pixmap)
		mode->flags |= DGA_PIXMAP_AVAILABLE;
	    if(!pApm->NoAccel) {
		mode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
		if (Bpp != 3)
		    mode->flags |= DGA_BLIT_RECT_TRANS;
	    }
	    if(pMode->Flags & V_DBLSCAN)
		mode->flags |= DGA_DOUBLESCAN;
	    if(pMode->Flags & V_INTERLACE)
		mode->flags |= DGA_INTERLACED;
	    mode->byteOrder = pScrn->imageByteOrder;
	    mode->depth = depth;
	    mode->bitsPerPixel = bitsPerPixel;
	    mode->red_mask = red;
	    mode->green_mask = green;
	    mode->blue_mask = blue;
	    mode->visualClass = visualClass;
	    mode->viewportWidth = pMode->HDisplay;
	    mode->viewportHeight = pMode->VDisplay;
	    mode->xViewportStep = (bitsPerPixel == 24) ? 4 : 1;
	    mode->yViewportStep = 1;
	    mode->viewportFlags = DGA_FLIP_RETRACE;
	    mode->offset = 0;
	    mode->address = pApm->FbBase;
	    mode->bytesPerScanline = pitch * Bpp;
	    mode->imageWidth = pitch;
	    mode->imageHeight =  (pScrn->videoRam * 1024 -
			pApm->OffscreenReserved) / mode->bytesPerScanline; 
	    mode->pixmapWidth = mode->imageWidth;
	    mode->pixmapHeight = mode->imageHeight;
	    mode->maxViewportX = mode->imageWidth - mode->viewportWidth;
	   /* this might need to get clamped to some maximum */
	    mode->maxViewportY = mode->imageHeight - mode->viewportHeight;

	    (*num)++;
	}

	pMode = pMode->next;
	if(pMode == firstMode)
	   break;
    }

    if(secondPitch) {
	secondPitch = 0;
	goto SECOND_PASS;
    }

    return modes;
}

Bool
ApmDGAInit(ScreenPtr pScreen)
{   
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   APMDECL(pScrn);
   DGAModePtr modes = NULL;
   int num = 0;

   /* 8 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 8, 8, 
		(pScrn->bitsPerPixel == 8),
		(pScrn->bitsPerPixel != 8) ? 0 : pScrn->displayWidth,
		0, 0, 0, PseudoColor);

   /* 15 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 15, 
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7c00, 0x03e0, 0x001f, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 15, 
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7c00, 0x03e0, 0x001f, DirectColor);

   /* 16 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 16, 
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xf800, 0x07e0, 0x001f, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 16, 
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xf800, 0x07e0, 0x001f, DirectColor);

   /* 24 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 24, 24, 
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 24, 24, 
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, DirectColor);

   /* 32 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 32, 24, 
		(pScrn->bitsPerPixel == 32),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 32, 24, 
		(pScrn->bitsPerPixel == 32),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, DirectColor);

   pApm->numDGAModes = num;
   pApm->DGAModes = modes;

   return DGAInit(pScreen, &ApmDGAFuncs, modes, num);  
}


static Bool
ApmSetMode(ScrnInfoPtr pScrn, DGAModePtr pMode)
{
    int		index = pScrn->pScreen->myNum, c;
    APMDECL(pScrn);

    if(!pMode) { /* restore the original mode */
	if (pApm->DGAactive) {
	    memcpy(&pApm->CurrentLayout, &pApm->SavedLayout,
						sizeof pApm->CurrentLayout);
	    pApm->DGAactive = FALSE;
	}

        pScrn->SwitchMode(index, pScrn->currentMode, 0);
    } else {
	if (!pApm->DGAactive) {
	    memcpy(&pApm->SavedLayout, &pApm->CurrentLayout,
						sizeof pApm->CurrentLayout);
	    pApm->DGAactive = TRUE;
	}

	switch(pMode->bitsPerPixel) {
	case 8:
	    c = DEC_BITDEPTH_8;
	    break;

	case 16:
	    c = DEC_BITDEPTH_16;
	    break;

	case 24:
	    c = DEC_BITDEPTH_24 | DEC_SOURCE_LINEAR | DEC_DEST_LINEAR;
	    break;

	case 32:
	    c = DEC_BITDEPTH_32;
	    break;

	default:
	    c = 0;
	    break;
	}

	switch(pMode->imageWidth) {
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

	pApm->CurrentLayout.Setup_DEC = (pApm->CurrentLayout.Setup_DEC & ~(DEC_BITDEPTH_MASK|DEC_WIDTH_MASK|DEC_SOURCE_LINEAR|DEC_DEST_LINEAR)) | c;
	pApm->CurrentLayout.displayWidth	= pMode->bytesPerScanline /
						    (pMode->bitsPerPixel >> 3);
	pApm->CurrentLayout.displayHeight	= pMode->imageHeight;
	pApm->CurrentLayout.Scanlines	= pMode->imageHeight + 1;
	pApm->CurrentLayout.bitsPerPixel	= pMode->bitsPerPixel;
	pApm->CurrentLayout.bytesPerScanline= pMode->bytesPerScanline;

        pScrn->SwitchMode(index, pMode->mode, 0);
    }

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

    if (pApm->apmLock) {
	/*
	 * This is just an attempt, because Daryll is tampering with MY
	 * registers.
	 */
	if (!pApm->noLinear) {
	    WRXB(0xDB, (RDXB(0xDB) & 0xF4) |  0x0A);
	    ApmWriteSeq(0x1B, 0x20);
	    ApmWriteSeq(0x1C, 0x2F);
	}
	else {
	    WRXB_IOP(0xDB, (RDXB_IOP(0xDB) & 0xF4) |  0x0A);
	    wrinx(0x3C4, 0x1B, 0x20);
	    wrinx(0x3C4, 0x1C, 0x2F);
	}
	pApm->apmLock = FALSE;
    }
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
			0*((char *)pApm->FbBase - (char *)pApm->LinMap));
    *size = pScrn->videoRam << 10;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
