/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaInit.c,v 1.4 1998/09/27 04:43:44 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "mi.h"
#include "miline.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xaawrap.h"
#include "xf86fbman.h"

#define MAX_PREALLOC_BOXES	128
#define MAX_PREALLOC_POINTS	256
#define MAX_PREALLOC_INTS	MAX_PREALLOC_POINTS  /* must be >= 255 */
#define MAX_PREALLOC_POINTERS   (255 + 7)

static Bool XAACloseScreen(int i, ScreenPtr pScreen);
static void XAAGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h,
			unsigned int format, unsigned long planemask,
			char *pdstLine);
static void XAAGetSpans(DrawablePtr pDrawable, int wMax, DDXPointPtr ppt,
			int *pwidth, int nspans, char *pdstStart);
static void XAASourceValidate(DrawablePtr pDrawable, int x, int y, int width,
			int height);
static void XAAClearToBackground(WindowPtr pWin, int x, int y, int w, int h,
			Bool generateExposures);
static PixmapPtr XAACreatePixmap(ScreenPtr pScreen, int w, int h, int depth);
static void XAARestoreAreas (PixmapPtr pPixmap, RegionPtr prgnRestore, 
			int xorg, int yorg, WindowPtr pWin);
static void XAASaveAreas (PixmapPtr pPixmap, RegionPtr prgnSave, 
			int xorg, int yorg, WindowPtr pWin);
static Bool XAAEnterVT (int index, int flags);
static void XAALeaveVT (int index, int flags);


int XAAScreenIndex = -1;
int XAAGCIndex = -1;
int XAAPixmapIndex = -1;
static unsigned long XAAGeneration = 0;


#ifdef XFree86LOADER
MODULEINITPROTO(xaaModuleInit);

static XF86ModuleVersionInfo xaaVersRec =
{
	"xaa",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,			/* 1.1 */
	ABI_CLASS_VIDEODRV,		/* a video driver module */
	ABI_VIDEODRV_VERSION,
	{0,0,0,0}
};


void
xaaModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &xaaVersRec;
    *setup = NULL;
    *teardown = NULL;
}
#endif


XAAInfoRecPtr
XAACreateInfoRec()
{
    XAAInfoRecPtr infoRec;

    infoRec = (XAAInfoRecPtr)xcalloc(1, sizeof(XAAInfoRec));

    return infoRec;
}

void 
XAADestroyInfoRec(XAAInfoRecPtr infoRec)
{
    if(!infoRec) return;

    if(infoRec->ClosePixmapCache)
	(*infoRec->ClosePixmapCache)(infoRec->pScrn->pScreen);
   
    if(infoRec->PreAllocBoxes)
	xfree(infoRec->PreAllocBoxes);

    if(infoRec->PreAllocDDXPointRecs)
	xfree(infoRec->PreAllocDDXPointRecs);

    if(infoRec->PreAllocInts)
	xfree(infoRec->PreAllocInts);

    if(infoRec->PixmapCachePrivate)
	xfree(infoRec->PixmapCachePrivate);

    xfree(infoRec);
}


Bool 
XAAInit(ScreenPtr pScreen, XAAInfoRecPtr infoRec)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XAAScreenPtr pScreenPriv;

    if (XAAGeneration != serverGeneration) {
	if (	((XAAScreenIndex = AllocateScreenPrivateIndex()) < 0) ||
		((XAAGCIndex = AllocateGCPrivateIndex()) < 0) ||
		((XAAPixmapIndex = AllocatePixmapPrivateIndex()) < 0))
		return FALSE;

	XAAGeneration = serverGeneration;
    }

    if (!AllocateGCPrivate(pScreen, XAAGCIndex, sizeof(XAAGCRec)))
	return FALSE;

    if (!AllocatePixmapPrivate(pScreen, XAAPixmapIndex, sizeof(XAAPixmapRec)))
	return FALSE;

    if (!(pScreenPriv = (XAAScreenPtr)xalloc(sizeof(XAAScreenRec))))
	return FALSE;

    pScreen->devPrivates[XAAScreenIndex].ptr = (pointer)pScreenPriv;


    if(!XAAInitAccel(pScreen, infoRec)) return FALSE;
    pScreenPriv->AccelInfoRec = infoRec;
    infoRec->ScratchGC.pScreen = pScreen;

    pScreenPriv->CreateGC = pScreen->CreateGC;
    pScreen->CreateGC = XAACreateGC;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = XAACloseScreen;
    pScreenPriv->GetImage = pScreen->GetImage;
    pScreen->GetImage = XAAGetImage;
    pScreenPriv->GetSpans = pScreen->GetSpans;
    pScreen->GetSpans = XAAGetSpans;
    pScreenPriv->SourceValidate = pScreen->SourceValidate;
#if 0
    /* shouldn't need this now that we wrap pixmaps */
    pScreen->SourceValidate = XAASourceValidate;
#endif
    pScreenPriv->PaintWindowBackground = pScreen->PaintWindowBackground;
    pScreen->PaintWindowBackground =  XAAPaintWindow;
    pScreenPriv->PaintWindowBorder = pScreen->PaintWindowBorder;
    pScreen->PaintWindowBorder =  XAAPaintWindow;
    pScreenPriv->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = XAACopyWindow;
    pScreenPriv->ClearToBackground = pScreen->ClearToBackground;
#if 0
    /* Is really just a wrapper for PaintWindowBackground */
    pScreen->ClearToBackground = XAAClearToBackground;
#endif
    pScreenPriv->CreatePixmap = pScreen->CreatePixmap;
    pScreen->CreatePixmap = XAACreatePixmap;
    pScreenPriv->BackingStoreFuncs.RestoreAreas = 
			pScreen->BackingStoreFuncs.RestoreAreas;
    pScreen->BackingStoreFuncs.RestoreAreas = XAARestoreAreas;
    pScreenPriv->BackingStoreFuncs.SaveAreas = 
			pScreen->BackingStoreFuncs.SaveAreas;
    pScreen->BackingStoreFuncs.SaveAreas = XAASaveAreas;

    pScreenPriv->EnterVT = pScrn->EnterVT;
    pScrn->EnterVT = XAAEnterVT; 
    pScreenPriv->LeaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = XAALeaveVT;


    infoRec->PreAllocBoxes = 
		(BoxPtr)xalloc(MAX_PREALLOC_BOXES * sizeof(BoxRec));
    if(infoRec->PreAllocBoxes)
    	infoRec->NumPreAllocBoxes = MAX_PREALLOC_BOXES;


    infoRec->PreAllocDDXPointRecs = 
		(DDXPointPtr)xalloc(MAX_PREALLOC_POINTS * sizeof(DDXPointRec));
    if(infoRec->PreAllocDDXPointRecs)
    	infoRec->NumPreAllocDDXPointRecs = MAX_PREALLOC_POINTS;


    infoRec->PreAllocInts = 
		(int*)xalloc(MAX_PREALLOC_INTS * sizeof(int));
    if(infoRec->PreAllocInts)
    	infoRec->NumPreAllocInts = MAX_PREALLOC_INTS;

    infoRec->PreAllocPointers = 
		(pointer)xalloc(MAX_PREALLOC_INTS * sizeof(pointer));
    if(infoRec->PreAllocPointers)
    	infoRec->NumPreAllocPointers = MAX_PREALLOC_POINTERS;


    if(infoRec->Flags & PIXMAP_CACHE) 
	xf86RegisterFreeBoxCallback(pScreen, infoRec->InitPixmapCache,
						(pointer)infoRec);

    if(infoRec->Flags & MICROSOFT_ZERO_LINE_BIAS)
	miSetZeroLineBias(pScreen, OCTANT1 | OCTANT2 | OCTANT3 | OCTANT4);

    return TRUE;
}



static Bool
XAACloseScreen (int i, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;

    pScrn->EnterVT = pScreenPriv->EnterVT; 
    pScrn->LeaveVT = pScreenPriv->LeaveVT; 
   
    pScreen->CreateGC = pScreenPriv->CreateGC;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->GetImage = pScreenPriv->GetImage;
    pScreen->GetSpans = pScreenPriv->GetSpans;
    pScreen->SourceValidate = pScreenPriv->SourceValidate;
    pScreen->PaintWindowBackground = pScreenPriv->PaintWindowBackground;
    pScreen->PaintWindowBorder = pScreenPriv->PaintWindowBorder;
    pScreen->CopyWindow = pScreenPriv->CopyWindow;
    pScreen->ClearToBackground = pScreenPriv->ClearToBackground;
    pScreen->BackingStoreFuncs.RestoreAreas = 
			pScreenPriv->BackingStoreFuncs.RestoreAreas;
    pScreen->BackingStoreFuncs.SaveAreas = 
			pScreenPriv->BackingStoreFuncs.SaveAreas;

    /* We leave it up to the client to free the XAAInfoRec */

    xfree ((pointer) pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static void
XAAGetImage (
    DrawablePtr pDrawable,
    int	sx, int sy, int w, int h,
    unsigned int    format,
    unsigned long   planemask,
    char	    *pdstLine 
)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    XAA_SCREEN_PROLOGUE (pScreen, GetImage);
    if(xf86Screens[pScreen->myNum]->vtSema && 
		(pDrawable->type == DRAWABLE_WINDOW)) {
	SYNC_CHECK(pDrawable);
    }
    (*pScreen->GetImage) (pDrawable, sx, sy, w, h,
			  format, planemask, pdstLine);
    XAA_SCREEN_EPILOGUE (pScreen, GetImage, XAAGetImage);
}

static void
XAAGetSpans (
    DrawablePtr	pDrawable,
    int		wMax,
    DDXPointPtr	ppt,
    int		*pwidth,
    int		nspans,
    char	*pdstStart
)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    XAA_SCREEN_PROLOGUE (pScreen, GetSpans);
    if(xf86Screens[pScreen->myNum]->vtSema && 
		(pDrawable->type == DRAWABLE_WINDOW)) {
	SYNC_CHECK(pDrawable);
    }
    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
    XAA_SCREEN_EPILOGUE (pScreen, GetSpans, XAAGetSpans);
}

static void
XAASourceValidate (
    DrawablePtr	pDrawable,
    int	x, int y, int width, int height )
{
    ScreenPtr	pScreen = pDrawable->pScreen;
    XAA_SCREEN_PROLOGUE (pScreen, SourceValidate);
    if(xf86Screens[pScreen->myNum]->vtSema) {
	SYNC_CHECK(pDrawable);
    }
    if (pScreen->SourceValidate)
	(*pScreen->SourceValidate) (pDrawable, x, y, width, height);
    XAA_SCREEN_EPILOGUE (pScreen, SourceValidate, XAASourceValidate);
}


static void
XAAClearToBackground (
    WindowPtr pWin,
    int x, int y,
    int w, int h,
    Bool generateExposures )
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    XAA_SCREEN_PROLOGUE (pScreen, ClearToBackground);
    if(xf86Screens[pScreen->myNum]->vtSema) {
	SYNC_CHECK(&pWin->drawable);
    }
    (*pScreen->ClearToBackground) (pWin, x, y, w, h, generateExposures);
    XAA_SCREEN_EPILOGUE (pScreen, ClearToBackground, XAAClearToBackground);

}

static void
XAASaveAreas (
    PixmapPtr pPixmap,
    RegionPtr prgnSave,
    int       xorg,
    int       yorg,
    WindowPtr pWin
){
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);

    if(xf86Screens[pScreen->myNum]->vtSema && infoRec->ReadPixmap) {
	BoxPtr pbox = REGION_RECTS(prgnSave);
	int nboxes = REGION_NUM_RECTS(prgnSave);
	int Bpp =  pPixmap->drawable.bitsPerPixel >> 3;
	unsigned char *dstp = (unsigned char*)pPixmap->devPrivate.ptr;

	while(nboxes--) {
	    (*infoRec->ReadPixmap)(infoRec->pScrn,
		pbox->x1 + xorg, pbox->y1 + yorg, 
		pbox->x2 - pbox->x1, pbox->y2 - pbox->y1, 
		dstp + (pPixmap->devKind * pbox->y1) + (pbox->x1 * Bpp),
		pPixmap->devKind,
		pPixmap->drawable.bitsPerPixel, pPixmap->drawable.depth);
	    pbox++;
	}
	return;
    }
    
    XAA_SCREEN_PROLOGUE (pScreen, BackingStoreFuncs.SaveAreas);
    if(xf86Screens[pScreen->myNum]->vtSema) {
	SYNC_CHECK(&pWin->drawable);
    }
    (*pScreen->BackingStoreFuncs.SaveAreas) (
		pPixmap, prgnSave, xorg, yorg, pWin);

    XAA_SCREEN_EPILOGUE (pScreen, BackingStoreFuncs.SaveAreas,
			 XAASaveAreas);
}

static void
XAARestoreAreas (    
    PixmapPtr pPixmap,
    RegionPtr prgnRestore,
    int       xorg,
    int       yorg,
    WindowPtr pWin 
){
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);

    if(xf86Screens[pScreen->myNum]->vtSema && infoRec->WritePixmap &&
			!(infoRec->WritePixmapFlags & NO_GXCOPY)) {
	BoxPtr pbox = REGION_RECTS(prgnRestore);
	int nboxes = REGION_NUM_RECTS(prgnRestore);
	int Bpp =  pPixmap->drawable.bitsPerPixel >> 3;
	unsigned char *srcp = (unsigned char*)pPixmap->devPrivate.ptr;

	while(nboxes--) {
	    (*infoRec->WritePixmap)(infoRec->pScrn, pbox->x1, pbox->y1, 
		pbox->x2 - pbox->x1, pbox->y2 - pbox->y1, 
		srcp + (pPixmap->devKind * (pbox->y1 - yorg)) + 
				((pbox->x1 - xorg) * Bpp), 
		pPixmap->devKind, GXcopy, ~0, -1, 
		pPixmap->drawable.bitsPerPixel, pPixmap->drawable.depth);
	    pbox++;
	}
	return;
    }


    XAA_SCREEN_PROLOGUE (pScreen, BackingStoreFuncs.RestoreAreas);
    if(xf86Screens[pScreen->myNum]->vtSema) {
	SYNC_CHECK(&pWin->drawable);
    }
    (*pScreen->BackingStoreFuncs.RestoreAreas) (
		pPixmap, prgnRestore, xorg, yorg, pWin);

    XAA_SCREEN_EPILOGUE (pScreen, BackingStoreFuncs.RestoreAreas,
				 XAARestoreAreas);
}


static PixmapPtr 
XAACreatePixmap(ScreenPtr pScreen, int w, int h, int depth)
{
    XAAPixmapPtr pPriv;
    PixmapPtr pPix;
    
    XAA_SCREEN_PROLOGUE (pScreen, CreatePixmap);
    pPix = (*pScreen->CreatePixmap) (pScreen, w, h, depth);
    XAA_SCREEN_EPILOGUE (pScreen, CreatePixmap, XAACreatePixmap);

    if(pPix) {
	pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	pPriv->flags = 0;
    }

    return pPix;
}


static Bool 
XAAEnterVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;

    if(pScreenPriv->AccelInfoRec->Flags & PIXMAP_CACHE)
	XAAInvalidatePixmapCache(pScreen);

    return (*pScreenPriv->EnterVT)(index, flags);
}

static void 
XAALeaveVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;

    /* nothing for now */

    (*pScreenPriv->LeaveVT)(index, flags);
}
