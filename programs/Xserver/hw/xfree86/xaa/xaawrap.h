/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaawrap.h,v 1.1.2.1 1998/05/23 09:31:54 dawes Exp $ */

#define XAA_SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = \
   ((XAAScreenPtr) (pScreen)->devPrivates[XAAScreenIndex].ptr)->field)

#define XAA_SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)


#define XAA_GC_FUNC_PROLOGUE(pGC)\
    XAAGCPtr   pGCPriv = (XAAGCPtr) (pGC)->devPrivates[XAAGCIndex].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;\
    if(pGCPriv->wrapOps)\
	(pGC)->ops = pGCPriv->wrapOps

#define XAA_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XAAGCFuncs;\
    if(pGCPriv->wrapOps) {\
	pGCPriv->wrapOps = (pGC)->ops;\
	(pGC)->ops = pGCPriv->XAAOps;\
    }


#define XAA_GC_OP_PROLOGUE(pGC)\
    XAAGCPtr pGCPriv = (XAAGCPtr)(pGC->devPrivates[XAAGCIndex].ptr);\
    GCFuncs *oldFuncs = pGC->funcs;\
    pGC->funcs = pGCPriv->wrapFuncs;\
    pGC->ops = pGCPriv->wrapOps

    
#define XAA_GC_OP_EPILOGUE(pGC)\
    pGCPriv->wrapOps = pGC->ops;\
    pGC->funcs = oldFuncs;\
    pGC->ops   = pGCPriv->XAAOps


/* This also works fine for drawables */

#define SYNC_CHECK(pGC) {\
     XAAInfoRecPtr infoRec =\
((XAAScreenPtr)((pGC)->pScreen->devPrivates[XAAScreenIndex].ptr))->AccelInfoRec;\
    if(infoRec->NeedToSync) {\
	(*infoRec->Sync)(infoRec->pScrn);\
	infoRec->NeedToSync = FALSE;\
    }}
