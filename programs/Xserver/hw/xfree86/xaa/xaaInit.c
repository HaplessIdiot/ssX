/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaInit.c,v 1.7 1998/12/13 05:32:57 dawes Exp $ */

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
#include "servermd.h"

#define MAX_PREALLOC_MEM	65536	/* MUST be >= 1024 */

#define MIN_OFFPIX_SIZE		(320*200)

static Bool XAACloseScreen(int i, ScreenPtr pScreen);
static void XAAGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h,
			unsigned int format, unsigned long planemask,
			char *pdstLine);
static void XAAGetSpans(DrawablePtr pDrawable, int wMax, DDXPointPtr ppt,
			int *pwidth, int nspans, char *pdstStart);
static PixmapPtr XAACreatePixmap(ScreenPtr pScreen, int w, int h, int depth);
static Bool XAADestroyPixmap(PixmapPtr pPixmap);
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
	1, 0, 0,
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
   
    if(infoRec->PreAllocMem)
	xfree(infoRec->PreAllocMem);

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

    if(!xf86FBManagerRunning(pScreen))
	infoRec->Flags &= ~(PIXMAP_CACHE | OFFSCREEN_PIXMAPS);
    if(!(infoRec->Flags & LINEAR_FRAMEBUFFER))
	infoRec->Flags &= ~OFFSCREEN_PIXMAPS;
#if 0
    if(pScreen->backingStoreSupport || pScreen->saveUnderSupport)
	infoRec->Flags &= ~OFFSCREEN_PIXMAPS;
#endif

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
    pScreenPriv->PaintWindowBackground = pScreen->PaintWindowBackground;
    pScreen->PaintWindowBackground =  XAAPaintWindow;
    pScreenPriv->PaintWindowBorder = pScreen->PaintWindowBorder;
    pScreen->PaintWindowBorder =  XAAPaintWindow;
    pScreenPriv->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = XAACopyWindow;
    pScreenPriv->CreatePixmap = pScreen->CreatePixmap;
    pScreen->CreatePixmap = XAACreatePixmap;
    pScreenPriv->DestroyPixmap = pScreen->DestroyPixmap;
    pScreen->DestroyPixmap = XAADestroyPixmap;
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


    infoRec->PreAllocMem = 
		(unsigned char*)xalloc(MAX_PREALLOC_MEM);
    if(infoRec->PreAllocMem)
    	infoRec->PreAllocSize = MAX_PREALLOC_MEM;

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
    pScreen->PaintWindowBackground = pScreenPriv->PaintWindowBackground;
    pScreen->PaintWindowBorder = pScreenPriv->PaintWindowBorder;
    pScreen->CopyWindow = pScreenPriv->CopyWindow;
    pScreen->CreatePixmap = pScreenPriv->CreatePixmap;
    pScreen->DestroyPixmap = pScreenPriv->DestroyPixmap;
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
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XAAPixmapPtr pPriv;
    PixmapPtr pPix = NULL;
    int size = w * h;
    
    if((infoRec->Flags & OFFSCREEN_PIXMAPS) && pScrn->vtSema &&
	(BitsPerPixel(depth) == pScrn->bitsPerPixel) && 
	(size >= MIN_OFFPIX_SIZE)){

	PixmapPtr pScreenPix;

	XAA_SCREEN_PROLOGUE (pScreen, CreatePixmap);
	pPix = (*pScreen->CreatePixmap) (pScreen, 0, 0, depth);
	XAA_SCREEN_EPILOGUE (pScreen, CreatePixmap, XAACreatePixmap);

	if(pPix) {
	    FBAreaPtr area = NULL;
	    PixmapLinkPtr pLink = NULL;

	    pScreenPix = (*pScreen->GetScreenPixmap)(pScreen);

	    pLink = (PixmapLinkPtr)xalloc(sizeof(PixmapLink));

	    if(pLink)
	      area = xf86AllocateOffscreenArea(pScreen, w, h, 0, 0, 
				XAARemoveAreaCallback, pPix);	    
	    if(area){
		pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
#if 0	
ErrorF("Allocated one !!!!  (%i x %i) %x\n", w, h, pPix);
#endif
		pPix->drawable.x = area->box.x1;
		pPix->drawable.y = area->box.y1;
		pPix->drawable.width = w;
		pPix->drawable.height = h;
		pPix->devKind = pScreenPix->devKind;
		pPix->devPrivate.ptr = pScreenPix->devPrivate.ptr;

		pPriv->flags = OFFSCREEN;
		pPriv->offscreenArea = area;
		
		pLink->next = infoRec->OffscreenPixmaps;
		pLink->pPix = pPix;
	    } else {
		if(pLink) xfree(pLink);
		(*pScreen->DestroyPixmap)(pPix);
		pPix = NULL;
	    }
        }
    }

    if(!pPix) {
	XAA_SCREEN_PROLOGUE (pScreen, CreatePixmap);
	pPix = (*pScreen->CreatePixmap) (pScreen, w, h, depth);
	XAA_SCREEN_EPILOGUE (pScreen, CreatePixmap, XAACreatePixmap);

	if(pPix) {
	   pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	   pPriv->flags = 0;
	   pPriv->offscreenArea = NULL;
	}
    }

    return pPix;
}

static Bool 
XAADestroyPixmap(PixmapPtr pPix)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
    Bool ret;

    if((pPix->refcnt == 1) && (pPriv->flags & OFFSCREEN)) {
	if(pPriv->offscreenArea)
	    xf86FreeOffscreenArea(pPriv->offscreenArea);
	else
	    xfree(pPix->devPrivate.ptr);	   

        DELIST_OFFSCREEN_PIXMAP(pPix);
#if 0
ErrorF("Freed one !!!!  (%x)\n", pPix);
#endif
    }
    
    XAA_SCREEN_PROLOGUE (pScreen, DestroyPixmap);
    ret = (*pScreen->DestroyPixmap) (pPix);
    XAA_SCREEN_EPILOGUE (pScreen, DestroyPixmap, XAADestroyPixmap);
 
    return ret;
}


static Bool 
XAAEnterVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;
    Bool ret;

    if(pScreenPriv->AccelInfoRec->Flags & PIXMAP_CACHE)
	XAAInvalidatePixmapCache(pScreen);

    ret = (*pScreenPriv->EnterVT)(index, flags);

    if((infoRec->Flags & OFFSCREEN_PIXMAPS) && infoRec->OffscreenPixmaps)
	XAAMoveInOffscreenPixmaps(pScreen);

    return ret;
}

static void 
XAALeaveVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;

    if((infoRec->Flags & OFFSCREEN_PIXMAPS) && infoRec->OffscreenPixmaps)
	 XAAMoveOutOffscreenPixmaps(pScreen);

    (*pScreenPriv->LeaveVT)(index, flags);
}
