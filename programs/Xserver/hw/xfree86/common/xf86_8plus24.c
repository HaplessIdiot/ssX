/*
   Copyright (C) 1998.  The XFree86 Project Inc.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)
*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_8plus24.c,v 1.1 1998/10/05 13:23:03 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "migc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "xf86_8plus24.h"

extern WindowPtr *WindowTable;

static void OverlayValidateGC(GCPtr, unsigned long, DrawablePtr);
static void OverlayChangeGC(GCPtr, unsigned long);
static void OverlayCopyGC(GCPtr, unsigned long, GCPtr);
static void OverlayDestroyGC(GCPtr);
static void OverlayChangeClip(GCPtr, int, pointer, int);
static void OverlayDestroyClip(GCPtr);
static void OverlayCopyClip(GCPtr, GCPtr);

static void UnderlayValidateGC(GCPtr, unsigned long, DrawablePtr);
static void UnderlayChangeGC(GCPtr, unsigned long);
static void UnderlayCopyGC(GCPtr, unsigned long, GCPtr);
static void UnderlayDestroyGC(GCPtr);
static void UnderlayChangeClip(GCPtr, int, pointer, int);
static void UnderlayDestroyClip(GCPtr);
static void UnderlayCopyClip(GCPtr, GCPtr);

static Bool OverlayCloseScreen (int, ScreenPtr);
static void OverlayPaintWindowBackground(WindowPtr, RegionPtr, int);
static void OverlayPaintWindowBorder(WindowPtr, RegionPtr, int);
static Bool OverlayCreateGC(GCPtr pGC);
static void OverlayWindowExposures(WindowPtr, RegionPtr, RegionPtr);
static void OverlayCopyWindow(WindowPtr, DDXPointRec, RegionPtr);

static void OverlayFillSolidRects(WindowPtr, unsigned long, 
			unsigned long, RegionPtr);
static void OverlayFillTiledRects(WindowPtr, PixmapPtr, int, int,
			unsigned long, RegionPtr);
static void OverlayCopyAreas(WindowPtr, unsigned long, DDXPointPtr, RegionPtr);


OverlayFBfuncs diOverlayFBfuncs = {
   OverlayFillSolidRects,
   OverlayFillTiledRects,
   OverlayCopyAreas
};

static void	 OverlayFillSpans(DrawablePtr, GCPtr, int, DDXPointPtr, int *,
				  int);
static void	 OverlaySetSpans(DrawablePtr, GCPtr, char *, DDXPointPtr,
				 int *, int, int);
static void	 OverlayPutImage(DrawablePtr, GCPtr, int, int, int, int, int,
				 int, int, char *);
static void	 OverlayPushPixels(GCPtr, PixmapPtr, DrawablePtr, int, int,
				   int, int);
static RegionPtr OverlayCopyArea(DrawablePtr, DrawablePtr, GCPtr, int, int,
				 int, int, int, int);
static RegionPtr OverlayCopyPlane(DrawablePtr, DrawablePtr, GCPtr, int, int,
				  int, int, int, int, unsigned long);
static void	 OverlayPolyPoint(DrawablePtr, GCPtr, int, int, xPoint *);
static void	 OverlayPolylines(DrawablePtr, GCPtr, int, int, DDXPointPtr);
static void	 OverlayPolySegment(DrawablePtr, GCPtr, int, xSegment *);
static void	 OverlayPolyRectangle(DrawablePtr, GCPtr, int, xRectangle *);
static void	 OverlayPolyArc(DrawablePtr, GCPtr, int, xArc *);
static void	 OverlayFillPolygon(DrawablePtr, GCPtr, int, int, int,
				    DDXPointPtr);
static void	 OverlayPolyFillRect(DrawablePtr, GCPtr, int, xRectangle *);
static void	 OverlayPolyFillArc(DrawablePtr, GCPtr, int, xArc *);
static int	 OverlayPolyText8(DrawablePtr, GCPtr, int, int, int, char *);
static int	 OverlayPolyText16(DrawablePtr, GCPtr, int, int, int,
				   unsigned short *);
static void	 OverlayImageText8(DrawablePtr, GCPtr, int, int, int, char *);
static void	 OverlayImageText16(DrawablePtr, GCPtr, int, int, int,
				    unsigned short *);
static void	 OverlayImageGlyphBlt(DrawablePtr, GCPtr, int, int,
				      unsigned int, CharInfoPtr *, pointer);
static void	 OverlayPolyGlyphBlt(DrawablePtr, GCPtr, int, int,
				     unsigned int, CharInfoPtr *, pointer);

static GCFuncs OverlayGCFuncs = {
   OverlayValidateGC, OverlayChangeGC, 
   OverlayCopyGC, OverlayDestroyGC,
   OverlayChangeClip, OverlayDestroyClip, 
   OverlayCopyClip
};

static GCFuncs UnderlayGCFuncs = {
   UnderlayValidateGC, UnderlayChangeGC, 
   UnderlayCopyGC, UnderlayDestroyGC,
   UnderlayChangeClip, UnderlayDestroyClip, 
   UnderlayCopyClip
};


GCOps OverlayGCOps = {
    OverlayFillSpans, OverlaySetSpans, 
    OverlayPutImage, OverlayCopyArea, 
    OverlayCopyPlane, OverlayPolyPoint, 
    OverlayPolylines, OverlayPolySegment, 
    OverlayPolyRectangle, OverlayPolyArc, 
    OverlayFillPolygon, OverlayPolyFillRect, 
    OverlayPolyFillArc, OverlayPolyText8, 
    OverlayPolyText16, OverlayImageText8, 
    OverlayImageText16, OverlayImageGlyphBlt, 
    OverlayPolyGlyphBlt, OverlayPushPixels,
#ifdef NEED_LINEHELPER
    NULL,
#endif
    {NULL}		/* devPrivate */
};


typedef struct {
   CloseScreenProcPtr		CloseScreen;
   CreateGCProcPtr		CreateGC;
   PaintWindowBackgroundProcPtr	PaintWindowBackground;
   PaintWindowBorderProcPtr	PaintWindowBorder;
   WindowExposuresProcPtr	WindowExposures;
   CopyWindowProcPtr		CopyWindow;
   OverlayFBfuncsPtr		FBfuncs;
   unsigned char		key;
   unsigned long		keyColor;
   Bool				Destructive;
   Bool				LockPrivate;
} OverlayScreenRec, *OverlayScreenPtr;

typedef struct {
   GCFuncs		*wrapFuncs;
   GCOps		*wrapOps;
   unsigned long 	fg;
   unsigned long 	bg;
   unsigned long 	pm;
} OverlayGCRec, *OverlayGCPtr;


static int OverlayGCIndex = -1;
static int OverlayScreenIndex = -1;
static unsigned long OverlayGeneration = 0;


#define OVERLAY_GC_FUNC_PROLOGUE(pGC)\
    OverlayGCPtr pGCPriv =  \
	(OverlayGCPtr) ((pGC)->devPrivates[OverlayGCIndex].ptr);\
    (pGC)->funcs = pGCPriv->wrapFuncs;\
    if(pGCPriv->wrapOps) \
	(pGC)->ops = pGCPriv->wrapOps

#define OVERLAY_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &OverlayGCFuncs;\
    if(pGCPriv->wrapOps) { \
	pGCPriv->wrapOps = (pGC)->ops;\
	(pGC)->ops = &OverlayGCOps;\
    }

#define UNDERLAY_GC_FUNC_PROLOGUE(pGC)\
    OverlayGCPtr pGCPriv =  \
	(OverlayGCPtr) ((pGC)->devPrivates[OverlayGCIndex].ptr);\
    (pGC)->funcs = pGCPriv->wrapFuncs

#define UNDERLAY_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &UnderlayGCFuncs


#define OVERLAY_GC_OP_PROLOGUE(pGC)\
    OverlayScreenPtr pScreenPriv =\
	(OverlayScreenPtr) (pGC)->pScreen->devPrivates[OverlayScreenIndex].ptr;\
    OverlayGCPtr pGCPriv = \
	(OverlayGCPtr)((pGC)->devPrivates[OverlayGCIndex].ptr);\
    unsigned long oldfg = (pGC)->fgPixel;\
    unsigned long oldbg = (pGC)->bgPixel;\
    unsigned long oldpm = (pGC)->planemask;\
    (pGC)->fgPixel = pGCPriv->fg;\
    (pGC)->bgPixel = pGCPriv->bg;\
    (pGC)->planemask = pGCPriv->pm;\
    (pGC)->funcs = pGCPriv->wrapFuncs;\
    (pGC)->ops = pGCPriv->wrapOps;\
    pScreenPriv->LockPrivate = TRUE
    

#define OVERLAY_GC_OP_EPILOGUE(pGC)\
    pGCPriv->wrapOps = (pGC)->ops;\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->fgPixel = oldfg;\
    (pGC)->bgPixel = oldbg;\
    (pGC)->planemask = oldpm;\
    (pGC)->funcs = &OverlayGCFuncs;\
    (pGC)->ops   = &OverlayGCOps;\
    pScreenPriv->LockPrivate = FALSE



Bool
xf86Overlay8Plus24Init (
   ScreenPtr pScreen,
   unsigned char key,
   unsigned long keyColor,
   Bool destructive,
   OverlayFBfuncsPtr FBfuncs
){
    OverlayScreenPtr pScreenPriv;

    if(OverlayGeneration != serverGeneration) {
	if(((OverlayScreenIndex = AllocateScreenPrivateIndex()) < 0) ||
	   ((OverlayGCIndex = AllocateGCPrivateIndex()) < 0))
		return FALSE;

	OverlayGeneration = serverGeneration;
    }

    if (!AllocateGCPrivate(pScreen, OverlayGCIndex, sizeof(OverlayGCRec)))
	return FALSE;

    if (!(pScreenPriv = (OverlayScreenPtr)xalloc(sizeof(OverlayScreenRec))))
	return FALSE;

    pScreen->devPrivates[OverlayScreenIndex].ptr = (pointer)pScreenPriv;

    pScreenPriv->CreateGC = pScreen->CreateGC;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->PaintWindowBackground = pScreen->PaintWindowBackground;
    pScreenPriv->PaintWindowBorder = pScreen->PaintWindowBorder;
    pScreenPriv->WindowExposures = pScreen->WindowExposures;
    pScreenPriv->CopyWindow = pScreen->CopyWindow;

    pScreen->CreateGC = OverlayCreateGC;
    pScreen->CloseScreen = OverlayCloseScreen;
    pScreen->PaintWindowBackground = OverlayPaintWindowBackground;
    pScreen->PaintWindowBorder = OverlayPaintWindowBorder;
    pScreen->CopyWindow = OverlayCopyWindow;
    pScreen->WindowExposures = OverlayWindowExposures; 

    pScreenPriv->key = key;
    pScreenPriv->keyColor = keyColor & 0x00ffffff;
    pScreenPriv->Destructive = destructive;
    pScreenPriv->FBfuncs = FBfuncs;
    pScreenPriv->LockPrivate = FALSE; 

    return TRUE;
}


/*********************** Screen Funcs ***********************/

Bool
OverlayCreateGC(GCPtr pGC)
{
    ScreenPtr    pScreen = pGC->pScreen;
    OverlayGCPtr pGCPriv = (OverlayGCPtr)(pGC->devPrivates[OverlayGCIndex].ptr);
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    Bool ret;

    pScreen->CreateGC = pScreenPriv->CreateGC;

    if((ret = (*pScreen->CreateGC)(pGC))) {
     if(!pScreenPriv->LockPrivate) {
	if(pGC->depth == 8) {
	    pGCPriv->wrapFuncs = pGC->funcs;
	    pGC->funcs = &OverlayGCFuncs;
	    pGCPriv->wrapOps = pGC->ops;
	    pGC->ops = &OverlayGCOps;
	} else if(pGC->depth == 24) {
	    pGCPriv->wrapFuncs = pGC->funcs;
	    pGC->funcs = &UnderlayGCFuncs;
	}
      }
    }

    pScreen->CreateGC = OverlayCreateGC;

    return ret;
}

static Bool
OverlayCloseScreen (int i, ScreenPtr pScreen)
{
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;

    pScreen->CreateGC = pScreenPriv->CreateGC;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->PaintWindowBackground = pScreenPriv->PaintWindowBackground;
    pScreen->PaintWindowBorder = pScreenPriv->PaintWindowBorder;
    pScreen->WindowExposures = pScreenPriv->WindowExposures;
    pScreen->CopyWindow = pScreenPriv->CopyWindow;

    xfree ((pointer) pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}



static void
OverlayWindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;

    if(REGION_NUM_RECTS(pReg) && 
	(pScreenPriv->Destructive || (pWin->drawable.depth == 24))) {

/*  
    If it's a destructive overlay, everyone gets an exposure. The
    depth 8 windows have the underlay forced to the keyColor and
    the depth 24 windows have the overlay forced to the key.
    For non-desctructive overlays, the depth 24 windows need the
    key set.
*/

	unsigned long planemask, pixel;

	if(pWin->drawable.depth == 24) {
	    planemask = 0xff000000;
	    pixel = pScreenPriv->key << 24;
	} else {
	    planemask = 0x00ffffff;
	    pixel = pScreenPriv->keyColor;
	}

	if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->FillSolidRects) {
	    (*pScreenPriv->FBfuncs->FillSolidRects)(
			pWin, pixel, planemask, pReg);
	} else {
	    OverlayFillSolidRects(pWin, pixel, planemask, pReg);
	}
    }

    pScreen->WindowExposures = pScreenPriv->WindowExposures;
    (*pScreen->WindowExposures)(pWin, pReg, pOtherReg);
    pScreen->WindowExposures = OverlayWindowExposures;
}


static void
OverlayPaintWindowBackground(
   WindowPtr pWin,
   RegionPtr prgn,
   int what
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    unsigned long planemask, pixel;

    if(pWin->drawable.depth == 24) {
	planemask = 0x00ffffff;
	pixel = pWin->background.pixel;
    } else {
	planemask = 0xff000000;
	pixel = pWin->background.pixel << 24;
    }

    switch (pWin->backgroundState) {
    case None:
	return;
    case ParentRelative:
	(*pScreen->PaintWindowBackground)(pWin->parent, prgn, what);
	return;
    case BackgroundPixel:
	if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->FillSolidRects) {
	    (*pScreenPriv->FBfuncs->FillSolidRects)(
			pWin, pixel, planemask, prgn);
	} else {
	    OverlayFillSolidRects(pWin, pixel, planemask, prgn);
	}
	break;
    case BackgroundPixmap:
	if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->FillTiledRects) {
	    (*pScreenPriv->FBfuncs->FillTiledRects)( pWin,
			pWin->background.pixmap, 0, 0, planemask, prgn);
	} else {
	    OverlayFillTiledRects(pWin, pWin->background.pixmap, 0, 0,
			planemask, prgn);
	}
	break;
    } 
}

static void
OverlayPaintWindowBorder(
   WindowPtr pWin,
   RegionPtr prgn,
   int what
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    RegionRec oldClip;
    DDXPointRec oldOrigin;
    BoxRec wholeBox;
    unsigned long planemask, pixel;
    Bool SelectPlane = FALSE;

    if(!wBorderWidth(pWin)) return;

    if(pWin->drawable.depth == 24) {
	SelectPlane = TRUE;
	planemask = 0x00ffffff;
	pixel = (pWin->border.pixel & 0x00ffffff) | 
		(pScreenPriv->key << 24);
    } else {
	if(pScreenPriv->Destructive) SelectPlane = TRUE;
	planemask = 0xff000000;
	pixel = (pWin->border.pixel << 24) | pScreenPriv->keyColor;
    }

    /* Hack.  Enlarge the window to include the whole screen so we
	can draw the border */

    wholeBox.x1 = wholeBox.y1 = 0;
    wholeBox.x2 = pScreen->width;
    wholeBox.y2 = pScreen->height;
    oldOrigin.x = pWin->drawable.x;
    oldOrigin.y = pWin->drawable.y;
    pWin->drawable.x = pWin->drawable.y = 0;
    oldClip = pWin->clipList;
    REGION_INIT(pScreen, &pWin->clipList, &wholeBox, 1);

    if(pWin->borderIsPixel) {
	if(SelectPlane) /* we can do the plane selection simultaneously */
	    planemask = ~0L; 
	if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->FillSolidRects) {
	    (*pScreenPriv->FBfuncs->FillSolidRects)(
			pWin, pixel, planemask, prgn);
	} else {
	    OverlayFillSolidRects(pWin, pixel, planemask, prgn);
	}
    } else {
	if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->FillTiledRects) {
	    (*pScreenPriv->FBfuncs->FillTiledRects)( pWin,
			pWin->border.pixmap, pWin->drawable.x, pWin->drawable.y,
			planemask, prgn);
	    /* select plane */
	    if(SelectPlane) {
		if(pScreenPriv->FBfuncs->FillSolidRects) {
		    (*pScreenPriv->FBfuncs->FillSolidRects)(
			pWin, pixel, ~planemask, prgn);
		} else {
		    OverlayFillSolidRects(pWin, pixel, ~planemask, prgn);
		}
	    }
	} else {
	    OverlayFillTiledRects(pWin, pWin->border.pixmap, 
			pWin->drawable.x, pWin->drawable.y, planemask, prgn);
	    /* select plane */
	    if(SelectPlane)
		OverlayFillSolidRects(pWin, pixel, ~planemask, prgn);
	}
    }


    REGION_UNINIT(pScreen, &pWin->clipList);
    pWin->drawable.x = oldOrigin.x;
    pWin->drawable.y = oldOrigin.y;
    pWin->clipList = oldClip;
}

static void
FindDepth24Kids(WindowPtr pWin, RegionPtr pReg)
{
    WindowPtr pChild;

    for(pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      if(pChild->drawable.depth == 24) 	/* got one */
	REGION_UNION(pChild->drawable.pScreen, pReg, pReg, &pChild->borderClip);

      if(pChild->firstChild)
	FindDepth24Kids(pChild, pReg);
    }
}


static void
CopyWindowHelper(
    WindowPtr pWin,
    int dx, int dy,
    RegionPtr rgnDst,
    unsigned long planemask 
) {
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    BoxPtr pbox = REGION_RECTS(rgnDst);
    int nbox = REGION_NUM_RECTS(rgnDst);
    DDXPointPtr pptSrc, ppt;
    int i;

    if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec))))
	return;

    ppt = pptSrc;

    for (i = nbox; --i >= 0; ppt++, pbox++) {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
    }

    if(xf86Screens[pScreen->myNum]->vtSema && pScreenPriv->FBfuncs
			&& pScreenPriv->FBfuncs->CopyAreas) {
	(*pScreenPriv->FBfuncs->CopyAreas)(pWin, planemask, pptSrc, rgnDst);
    } else {
	OverlayCopyAreas(pWin, planemask, pptSrc, rgnDst);
    }

    DEALLOCATE_LOCAL(pptSrc);
}


static void 
OverlayCopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc 
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    WindowPtr pRoot = WindowTable[pScreen->myNum];
    RegionRec rgnDst;
    int dx, dy;

    if(pScreenPriv->Destructive || (pWin->drawable.depth == 24)) {
	/* the framebuffer's CopyWindow is perfectly adequete */
	pScreen->CopyWindow = pScreenPriv->CopyWindow;
	(*pScreen->CopyWindow)(pWin, ptOldOrg, prgnSrc);
	pScreen->CopyWindow = OverlayCopyWindow;
	return;
    }


    REGION_INIT(pScreen, &rgnDst, NullBox, 0);
    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pScreen, &rgnDst, &pWin->borderClip, prgnSrc);
    CopyWindowHelper(pRoot, dx, dy, &rgnDst, 0xff000000);
    REGION_UNINIT(pScreen, &rgnDst);


    REGION_INIT(pScreen, &rgnDst, NullBox, 0);
    FindDepth24Kids(pWin, &rgnDst);
    if(REGION_NOTEMPTY(pScreen, &rgnDst)) {
	REGION_INTERSECT(pScreen, &rgnDst, &rgnDst, prgnSrc);
	CopyWindowHelper(pRoot, dx, dy, &rgnDst, 0x00ffffff);
    }
    REGION_UNINIT(pScreen, &rgnDst);
}



/*********************** GC Funcs *****************************/

static void
OverlayValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
){
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pGC->pScreen->devPrivates[OverlayScreenIndex].ptr;
    OVERLAY_GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);

    /* uncomment this to stop using the Overlay wrappers on Pixmaps */
    if(pScreenPriv->LockPrivate
#if 0
	|| (pDraw->type == DRAWABLE_PIXMAP)
#endif
    )
	pGCPriv->wrapOps = NULL;
    else
	pGCPriv->wrapOps = &OverlayGCOps; /* just so it's not NULL */

    OVERLAY_GC_FUNC_EPILOGUE (pGC);

    pGCPriv->fg = pGC->fgPixel << 24;
    pGCPriv->bg = pGC->bgPixel << 24;
    pGCPriv->pm = pGC->planemask << 24;
}


static void
OverlayDestroyGC(GCPtr pGC)
{
    OVERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->DestroyGC)(pGC);
    OVERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
OverlayChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    OVERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    OVERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
OverlayCopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst
){
    OVERLAY_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    OVERLAY_GC_FUNC_EPILOGUE (pGCDst);
}
static void
OverlayChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects
){
    OVERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    OVERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
OverlayCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    OVERLAY_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    OVERLAY_GC_FUNC_EPILOGUE (pgcDst);
}

static void
OverlayDestroyClip(GCPtr pGC)
{
    OVERLAY_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    OVERLAY_GC_FUNC_EPILOGUE (pGC);
}


/**/

static void
UnderlayValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
){
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pGC->pScreen->devPrivates[OverlayScreenIndex].ptr;
    UNDERLAY_GC_FUNC_PROLOGUE (pGC);

    /* just make sure the planemask is masked to the depth */
    if(!pScreenPriv->LockPrivate)
	pGC->planemask &= 0x00ffffff;

    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);

    UNDERLAY_GC_FUNC_EPILOGUE (pGC);
}


static void
UnderlayDestroyGC(GCPtr pGC)
{
    UNDERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->DestroyGC)(pGC);
    UNDERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
UnderlayChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    UNDERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    UNDERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
UnderlayCopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst
){
    UNDERLAY_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    UNDERLAY_GC_FUNC_EPILOGUE (pGCDst);
}
static void
UnderlayChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects
){
    UNDERLAY_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    UNDERLAY_GC_FUNC_EPILOGUE (pGC);
}

static void
UnderlayCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    UNDERLAY_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    UNDERLAY_GC_FUNC_EPILOGUE (pgcDst);
}

static void
UnderlayDestroyClip(GCPtr pGC)
{
    UNDERLAY_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    UNDERLAY_GC_FUNC_EPILOGUE (pGC);
}

/******************* GC ops ***********************/

static void
OverlayFillSpans(
    DrawablePtr pDraw,
    GC		*pGC,
    int		nInit,	
    DDXPointPtr pptInit,	
    int *pwidthInit,		
    int fSorted 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->FillSpans)(pDraw, pGC, nInit, pptInit, pwidthInit, fSorted);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlaySetSpans(
    DrawablePtr		pDraw,
    GCPtr		pGC,
    char		*pcharsrc,
    register DDXPointPtr ppt,
    int			*pwidth,
    int			nspans,
    int			fSorted 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->SetSpans)(pDraw, pGC, pcharsrc, ppt, pwidth, nspans, fSorted);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPutImage(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		depth, 
    int x, int y, int w, int h,
    int		leftPad,
    int		format,
    char 	*pImage 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PutImage)(pDraw, pGC, depth, x, y, w, h, 
						leftPad, format, pImage);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static RegionPtr
OverlayCopyArea(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
){
    RegionPtr ret;

    OVERLAY_GC_OP_PROLOGUE(pGC);
    ret = (*pGC->ops->CopyArea)(pSrc, pDst,
            pGC, srcx, srcy, width, height, dstx, dsty);
    OVERLAY_GC_OP_EPILOGUE(pGC);
    return ret;
}

static RegionPtr
OverlayCopyPlane(
    DrawablePtr	pSrc,
    DrawablePtr	pDst,
    GCPtr pGC,
    int	srcx, int srcy,
    int	width, int height,
    int	dstx, int dsty,
    unsigned long bitPlane 
){
    RegionPtr ret;

    OVERLAY_GC_OP_PROLOGUE(pGC);
    ret = (*pGC->ops->CopyPlane)(pSrc, pDst,
	       pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
    OVERLAY_GC_OP_EPILOGUE(pGC);
    return ret;
}

static void
OverlayPolyPoint(
    DrawablePtr pDraw,
    GCPtr pGC,
    int mode,
    int npt,
    xPoint *pptInit
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyPoint)(pDraw, pGC, mode, npt, pptInit);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}


static void
OverlayPolylines(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		mode,		
    int		npt,		
    DDXPointPtr pptInit 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->Polylines)(pDraw, pGC, mode, npt, pptInit);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void 
OverlayPolySegment(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nseg,
    xSegment	*pSeg 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolySegment)(pDraw, pGC, nseg, pSeg);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPolyRectangle(
    DrawablePtr  pDraw,
    GCPtr        pGC,
    int	         nRectsInit,
    xRectangle  *pRectsInit 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyRectangle)(pDraw, pGC, nRectsInit, pRectsInit);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPolyArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyArc)(pDraw, pGC, narcs, parcs);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayFillPolygon(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	ptsIn 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, count, ptsIn);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}


static void 
OverlayPolyFillRect(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nrectFill, 
    xRectangle	*prectInit 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyFillRect)(pDraw, pGC, nrectFill, prectInit);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPolyFillArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyFillArc)(pDraw, pGC, narcs, parcs);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static int
OverlayPolyText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int 	y,
    int 	count,
    char	*chars 
){
    int ret;

    OVERLAY_GC_OP_PROLOGUE(pGC);
    ret = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, chars);
    OVERLAY_GC_OP_EPILOGUE(pGC);
    return ret;
}

static int
OverlayPolyText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
){
    int ret;

    OVERLAY_GC_OP_PROLOGUE(pGC);
    ret = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, chars);
    OVERLAY_GC_OP_EPILOGUE(pGC);
    return ret;
}

static void
OverlayImageText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int 	count,
    char	*chars 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, chars);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}


static void
OverlayImageText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, chars);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}


static void
OverlayImageGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->ImageGlyphBlt)(pDraw, pGC, xInit, yInit, nglyph, 
					ppci, pglyphBase);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPolyGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PolyGlyphBlt)(pDraw, pGC, xInit, yInit, nglyph, 
						ppci, pglyphBase);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}

static void
OverlayPushPixels(
    GCPtr	pGC,
    PixmapPtr	pBitMap,
    DrawablePtr pDraw,
    int	dx, int dy, int xOrg, int yOrg 
){
    OVERLAY_GC_OP_PROLOGUE(pGC);
    (*pGC->ops->PushPixels)(pGC, pBitMap, pDraw, dx, dy, xOrg, yOrg);
    OVERLAY_GC_OP_EPILOGUE(pGC);
}


/**** utilities for rendering with scratch GCs ****/


static void
OverlayFillSolidRects(
    WindowPtr pWin,
    unsigned long fg,
    unsigned long planemask,
    RegionPtr pReg
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    int numRects = REGION_NUM_RECTS(pReg);
    BoxPtr pbox = REGION_RECTS(pReg);
    xRectangle *prect;
    ChangeGCVal gcval[2];
    GCPtr pGC;
    int i;

    prect = (xRectangle*)ALLOCATE_LOCAL(numRects * sizeof(xRectangle));
    if(!prect) return;

    pScreenPriv->LockPrivate = TRUE; 

    pGC = GetScratchGC(pWin->drawable.depth, pScreen);

    if(!pGC) {
	pScreenPriv->LockPrivate = FALSE; 
	DEALLOCATE_LOCAL(prect);
	return;
    }

    gcval[0].val = planemask;
    gcval[1].val = fg;

    dixChangeGC(NullClient, pGC, GCPlaneMask | GCForeground, NULL, gcval);
    
    for (i= numRects; --i >= 0; pbox++, prect++) {
        prect->x = pbox->x1 - pWin->drawable.x;
        prect->y = pbox->y1 - pWin->drawable.y;
        prect->width = pbox->x2 - pbox->x1;
        prect->height = pbox->y2 - pbox->y1;
    }
    prect -= numRects;

    ValidateGC((DrawablePtr)pWin, pGC);

    (*pGC->ops->PolyFillRect)((DrawablePtr)pWin, pGC, numRects, prect);

    FreeScratchGC(pGC);
    DEALLOCATE_LOCAL(prect);

    pScreenPriv->LockPrivate = FALSE; 
}


static void 
OverlayFillTiledRects(
    WindowPtr pWin,
    PixmapPtr pPix,
    int xorg, int yorg,
    unsigned long planemask,
    RegionPtr pReg
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    int numRects = REGION_NUM_RECTS(pReg);
    BoxPtr pbox = REGION_RECTS(pReg);
    xRectangle *prect;
    ChangeGCVal gcval[5];
    CARD32 gcmask = GCPlaneMask | GCFillStyle | GCTile;
    GCPtr pGC;
    int i;

    prect = (xRectangle*)ALLOCATE_LOCAL(numRects * sizeof(xRectangle));
    if(!prect) return;

    pScreenPriv->LockPrivate = TRUE; 

    pGC = GetScratchGC(pWin->drawable.depth, pScreen);

    if(!pGC) {
	pScreenPriv->LockPrivate = FALSE; 
	DEALLOCATE_LOCAL(prect);
	return;
    }

    gcval[0].val = planemask;
    gcval[1].val = FillTiled;
    gcval[2].ptr = (pointer)pPix;
    if(xorg) {
	gcval[3].val = xorg;
	gcmask |= GCTileStipXOrigin;
    }
    if(yorg) {
	if(xorg) gcval[4].val = yorg;
	else gcval[3].val = yorg;
	gcmask |= GCTileStipYOrigin;
    }

    dixChangeGC(NullClient, pGC, gcmask, NULL, gcval);

    for (i= numRects; --i >= 0; pbox++, prect++) {
        prect->x = pbox->x1 - pWin->drawable.x;
        prect->y = pbox->y1 - pWin->drawable.y;
        prect->width = pbox->x2 - pbox->x1;
        prect->height = pbox->y2 - pbox->y1;
    }
    prect -= numRects;

    ValidateGC((DrawablePtr)pWin, pGC);

    (*pGC->ops->PolyFillRect)((DrawablePtr)pWin, pGC, numRects, prect);

    FreeScratchGC(pGC);
    DEALLOCATE_LOCAL(prect);

    pScreenPriv->LockPrivate = FALSE; 
}


static void 
OverlayCopyAreas(
   WindowPtr pWin,
   unsigned long planemask,
   DDXPointPtr ppnts,
   RegionPtr pReg
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    OverlayScreenPtr pScreenPriv = 
	(OverlayScreenPtr) pScreen->devPrivates[OverlayScreenIndex].ptr;
    ChangeGCVal gcval[2];
    BoxPtr pbox = REGION_RECTS(pReg);
    RegionRec oldClip;
    BoxRec wholeBox;
    GCPtr pGC;
 
    pScreenPriv->LockPrivate = TRUE; 

    pGC = GetScratchGC(pWin->drawable.depth, pScreen);
    if(!pGC) {
	pScreenPriv->LockPrivate = FALSE; 
	return;
    }

    gcval[0].val = planemask;
    gcval[1].val = IncludeInferiors;
    dixChangeGC(NullClient, pGC, GCPlaneMask | GCSubwindowMode, NULL, gcval);

    pGC->clientClipType = CT_REGION;
    pGC->clientClip = pReg;

    wholeBox.x1 = wholeBox.y1 = 0;
    wholeBox.x2 = pScreen->width;
    wholeBox.y2 = pScreen->height;
    oldClip = pWin->clipList;
    REGION_INIT(pScreen, &pWin->clipList, &wholeBox, 1);

    ValidateGC((DrawablePtr)pWin, pGC); 

    pGC->fExpose = FALSE;

    (*pGC->ops->CopyArea)((DrawablePtr)pWin, (DrawablePtr)pWin, pGC,
		pReg->extents.x1 + ppnts->x - pbox->x1, 
		pReg->extents.y1 + ppnts->y - pbox->y1, 
		pReg->extents.x2 - pReg->extents.x1, 
		pReg->extents.y2 - pReg->extents.y1, 
		pReg->extents.x1, pReg->extents.y1);

    REGION_UNINIT(pScreen, &pWin->clipList);
    pWin->clipList = oldClip;

    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
    FreeScratchGC(pGC);
    pScreenPriv->LockPrivate = FALSE; 
}

