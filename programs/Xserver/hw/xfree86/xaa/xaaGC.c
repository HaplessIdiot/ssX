/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaGC.c,v 1.3 1998/08/02 05:17:06 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "migc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "xaawrap.h"

static void XAAValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw);
static void XAAChangeGC(GCPtr pGC, unsigned long mask);
static void XAACopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
static void XAADestroyGC(GCPtr pGC);
static void XAAChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects);
static void XAADestroyClip(GCPtr pGC);
static void XAACopyClip(GCPtr pgcDst, GCPtr pgcSrc);

GCFuncs XAAGCFuncs = {
    XAAValidateGC, XAAChangeGC, XAACopyGC, XAADestroyGC,
    XAAChangeClip, XAADestroyClip, XAACopyClip
};

Bool
XAACreateGC(GCPtr pGC)
{
    ScreenPtr    pScreen = pGC->pScreen;
    XAAGCPtr     pGCPriv = (XAAGCPtr)(pGC->devPrivates[XAAGCIndex].ptr);
    Bool         ret;

    XAA_SCREEN_PROLOGUE(pScreen,CreateGC);

    ret = (*pScreen->CreateGC)(pGC);
	
    pGCPriv->wrapOps = NULL;
    pGCPriv->wrapFuncs = pGC->funcs;
    pGCPriv->XAAOps = &XAAFallbackOps;
    pGCPriv->isPixmap = TRUE;
    pGCPriv->DashLength = 0;
    pGCPriv->DashPattern = NULL;
    /* initialize any other private fields here */

    pGC->funcs = &XAAGCFuncs;
 
    XAA_SCREEN_EPILOGUE(pScreen,CreateGC,XAACreateGC);

    return ret;
}


static void
XAAValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    XAA_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);

    if(!infoRec->pScrn->vtSema || (pDraw->type != DRAWABLE_WINDOW)) {
	pGCPriv->isPixmap = TRUE;
	pGCPriv->wrapOps = NULL;
    } else {
	pGCPriv->wrapOps = pGC->ops;
    }

    XAA_GC_FUNC_EPILOGUE(pGC);

    /* keep the colors out of the overlay planes */
    if(changes & GCForeground) pGC->fgPixel &= infoRec->FullPlanemask;
    if(changes & GCBackground) pGC->bgPixel &= infoRec->FullPlanemask;


    if(!pGCPriv->wrapOps) return;

    /* if we get here, we are going to be using acceleration */

    /* If our Ops are still the default ones we need to allocate new ones */
    if(pGC->ops == &XAAFallbackOps) {
	if(!(pGCPriv->XAAOps = (GCOps*)xalloc(sizeof(GCOps)))) {	
	    pGCPriv->XAAOps = &XAAFallbackOps;
	    return;
	}
	/* make a modifiable copy of the default ops */
	memcpy(pGCPriv->XAAOps, &XAAFallbackOps, sizeof(GCOps));
	pGC->ops = pGCPriv->XAAOps;
	changes = ~0;
    }

    if(pGCPriv->isPixmap) {
	changes = ~0;
	pGCPriv->isPixmap = FALSE;
    }

    if((changes & GCDashList) && infoRec->ComputeDash)
	infoRec->ComputeDash(pGC);

    if(changes & infoRec->FillSpansMask)
	(*infoRec->ValidateFillSpans)(pGC, changes, pDraw); 	

    if(changes & infoRec->SetSpansMask)
	(*infoRec->ValidateSetSpans)(pGC, changes, pDraw); 	

    if(changes & infoRec->PutImageMask)
	(*infoRec->ValidatePutImage)(pGC, changes, pDraw); 	

    if(changes & infoRec->CopyAreaMask)
	(*infoRec->ValidateCopyArea)(pGC, changes, pDraw); 	

    if(changes & infoRec->CopyPlaneMask)
	(*infoRec->ValidateCopyPlane)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolyPointMask)
	(*infoRec->ValidatePolyPoint)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolylinesMask)
	(*infoRec->ValidatePolylines)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolySegmentMask)
	(*infoRec->ValidatePolySegment)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolyRectangleMask)
	(*infoRec->ValidatePolyRectangle)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolyArcMask)
	(*infoRec->ValidatePolyArc)(pGC, changes, pDraw); 	

    if(changes & infoRec->FillPolygonMask)
	(*infoRec->ValidateFillPolygon)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolyFillRectMask)
	(*infoRec->ValidatePolyFillRect)(pGC, changes, pDraw); 	
 
    if(changes & infoRec->PolyFillArcMask)
	(*infoRec->ValidatePolyFillArc)(pGC, changes, pDraw); 	

    if(changes & infoRec->PolyGlyphBltMask)
	(*infoRec->ValidatePolyGlyphBlt)(pGC, changes, pDraw);

    if(changes & infoRec->ImageGlyphBltMask)
	(*infoRec->ValidateImageGlyphBlt)(pGC, changes, pDraw);

    if(changes & infoRec->PolyText8Mask)
	(*infoRec->ValidatePolyText8)(pGC, changes, pDraw);
    
    if(changes & infoRec->PolyText16Mask)
	(*infoRec->ValidatePolyText16)(pGC, changes, pDraw);

    if(changes & infoRec->ImageText8Mask)
	(*infoRec->ValidateImageText8)(pGC, changes, pDraw);
    
    if(changes & infoRec->ImageText16Mask)
	(*infoRec->ValidateImageText16)(pGC, changes, pDraw);
 
    if(changes & infoRec->PushPixelsMask) 
	(*infoRec->ValidatePushPixels)(pGC, changes, pDraw); 	

}


static void
XAADestroyGC(GCPtr pGC)
{
    XAA_GC_FUNC_PROLOGUE (pGC);
     
    if(pGCPriv->XAAOps != &XAAFallbackOps)
	xfree(pGCPriv->XAAOps);

    if(pGCPriv->DashPattern)
	xfree(pGCPriv->DashPattern);    

    (*pGC->funcs->DestroyGC)(pGC);
    XAA_GC_FUNC_EPILOGUE (pGC);
}

static void
XAAChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
)
{
    XAA_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    XAA_GC_FUNC_EPILOGUE (pGC);
}

static void
XAACopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst)
{
    XAA_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    XAA_GC_FUNC_EPILOGUE (pGCDst);
}
static void
XAAChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects )
{
    XAA_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    XAA_GC_FUNC_EPILOGUE (pGC);
}

static void
XAACopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    XAA_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    XAA_GC_FUNC_EPILOGUE (pgcDst);
}

static void
XAADestroyClip(GCPtr pGC)
{
    XAA_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    XAA_GC_FUNC_EPILOGUE (pGC);
}
