/* $XFree86$ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"

#include "mistruct.h"
#include "mibstore.h"
#include "migc.h"


static void cfb8_32ValidateGC(GCPtr, unsigned long, DrawablePtr);
static void cfb8_32DestroyGC(GCPtr pGC);

static
GCFuncs cfb8_32GCFuncs = {
    cfb8_32ValidateGC,
    miChangeGC,
    miCopyGC,
    cfb8_32DestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

static void
cfb8_32DestroyGC(GCPtr pGC)
{
    cfb8_32GCPtr pGCPriv = CFB8_32_GET_GC_PRIVATE(pGC);

    if (pGC->freeCompClip)
        REGION_DESTROY(pGC->pScreen, pGC->pCompositeClip);
    if(pGCPriv->Ops8bpp)
	miDestroyGCOps(pGCPriv->Ops8bpp);
    if(pGCPriv->Ops32bpp)
	miDestroyGCOps(pGCPriv->Ops32bpp);
}

Bool
cfb8_32CreateGC(GCPtr pGC)
{
    cfb8_32GCPtr pGCPriv;
    cfbPrivGC *pPriv;

    if (PixmapWidthPaddingInfo[pGC->depth].padPixelsLog2 == LOG2_BITMAP_PAD)
        return (mfbCreateGC(pGC));
    if (pGC->depth == 24)
	return (cfb32CreateGC(pGC));

    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
    pGC->miTranslate = 1;
    pGC->fExpose = TRUE;
    pGC->freeCompClip = FALSE;
    pGC->pRotatedPixmap = (PixmapPtr) NULL;

    pPriv = cfbGetGCPrivate(pGC);
    pPriv->rop = pGC->alu;
    pPriv->oneRect = FALSE;

    pGC->ops = NULL;
    pGC->funcs = &cfb8_32GCFuncs;

    pGCPriv = CFB8_32_GET_GC_PRIVATE(pGC);
    pGCPriv->Ops8bpp = NULL;
    pGCPriv->Ops32bpp = NULL;
    pGCPriv->OpsAre8bpp = FALSE;
    pGCPriv->changes = 0;
	
    return TRUE;
}


static void
cfb8_32ValidateGC(
    GCPtr pGC,
    unsigned long changes,
    DrawablePtr pDraw
){
    cfb8_32GCPtr pGCPriv = CFB8_32_GET_GC_PRIVATE(pGC);

    if(pDraw->bitsPerPixel == 32) {
	if(pGCPriv->OpsAre8bpp) {
	    int origChanges = changes;
	    pGC->ops = pGCPriv->Ops32bpp;
	    changes |= pGCPriv->changes;
	    pGCPriv->changes = origChanges;
	    pGCPriv->OpsAre8bpp = FALSE;
	} else 
	    pGCPriv->changes |= changes;

	cfb8_32ValidateGC32(pGC, changes, pDraw);
	pGCPriv->Ops32bpp = pGC->ops;
    } else {  /* bitsPerPixel == 8 */
	if(!pGCPriv->OpsAre8bpp) {
	    int origChanges = changes;
	    pGC->ops = pGCPriv->Ops8bpp;
	    changes |= pGCPriv->changes;
	    pGCPriv->changes = origChanges;
	    pGCPriv->OpsAre8bpp = TRUE;
	} else 
	    pGCPriv->changes |= changes;

	cfb8_32ValidateGC8(pGC, changes, pDraw);
	pGCPriv->Ops8bpp = pGC->ops;
    }
}

