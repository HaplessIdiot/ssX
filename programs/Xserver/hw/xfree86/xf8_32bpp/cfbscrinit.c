/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32bpp/cfbscrinit.c,v 1.3 1999/03/21 07:35:35 dawes Exp $ */


#include "X.h"
#include "Xmd.h"
#include "misc.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "mi.h"
#include "micmap.h"
#include "mistruct.h"
#include "dix.h"
#include "mibstore.h"
#include "xf86.h"
#include "xf86str.h"

/* CAUTION:  We require that cfb8 and cfb32 were NOT 
	compiled with CFB_NEED_SCREEN_PRIVATE */

static BSFuncRec cfb8_32BSFuncRec = {
    cfb8_32SaveAreas,
    cfb8_32RestoreAreas,
    (BackingStoreSetClipmaskRgnProcPtr) 0,
    (BackingStoreGetImagePixmapProcPtr) 0,
    (BackingStoreGetSpansPixmapProcPtr) 0,
};


int cfb8_32GCPrivateIndex;
int cfb8_32ScreenPrivateIndex;

static unsigned long cfb8_32Generation = 0;

static Bool xf8_32SaveRestoreImage(int scrnIndex, SaveRestoreFlags what);

static Bool
cfb8_32AllocatePrivates(ScreenPtr pScreen)
{
   cfb8_32ScreenPtr pScreenPriv;

   if(cfb8_32Generation != serverGeneration) {
	if(((cfb8_32GCPrivateIndex = AllocateGCPrivateIndex()) < 0) ||
	    ((cfb8_32ScreenPrivateIndex = AllocateScreenPrivateIndex()) < 0))
	    return FALSE;
	cfb8_32Generation = serverGeneration;
   }

   if (!(pScreenPriv = xalloc(sizeof(cfb8_32ScreenRec))))
        return FALSE;

   pScreen->devPrivates[cfb8_32ScreenPrivateIndex].ptr = (pointer)pScreenPriv;
   
   
   /* All cfb will have the same GC and Window private indicies */
   if(!mfbAllocatePrivates(pScreen,&cfbWindowPrivateIndex, &cfbGCPrivateIndex))
	return FALSE;

   /* The cfb indicies are the mfb indicies. Reallocating them resizes them */ 
   if(!AllocateWindowPrivate(pScreen,cfbWindowPrivateIndex,sizeof(cfbPrivWin)))
	return FALSE;

   if(!AllocateGCPrivate(pScreen, cfbGCPrivateIndex, sizeof(cfbPrivGC)))
        return FALSE;

   if(!AllocateGCPrivate(pScreen, cfb8_32GCPrivateIndex, sizeof(cfb8_32GCRec)))
        return FALSE;

   return TRUE;
}

static Bool
cfb8_32SetupScreen(
    ScreenPtr pScreen,
    pointer pbits,		/* pointer to screen bitmap */
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy,		/* dots per inch */
    int width			/* pixel width of frame buffer */
){
    if (!cfb8_32AllocatePrivates(pScreen))
	return FALSE;
    pScreen->defColormap = FakeClientID(0);
    /* let CreateDefColormap do whatever it wants for pixels */ 
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
    pScreen->QueryBestSize = mfbQueryBestSize;
    /* SaveScreen */
    pScreen->GetImage = cfb8_32GetImage;
    pScreen->GetSpans = cfb8_32GetSpans;
    pScreen->CreateWindow = cfb8_32CreateWindow;
    pScreen->DestroyWindow = cfb8_32DestroyWindow;	
    pScreen->PositionWindow = cfb8_32PositionWindow;
    pScreen->ChangeWindowAttributes = cfb8_32ChangeWindowAttributes;
    pScreen->RealizeWindow = cfb32MapWindow;			/* OK */
    pScreen->UnrealizeWindow = cfb32UnmapWindow;		/* OK */
    pScreen->PaintWindowBackground = cfb8_32PaintWindow;
    pScreen->PaintWindowBorder = cfb8_32PaintWindow;
    pScreen->CopyWindow = cfb8_32CopyWindow;
    pScreen->CreatePixmap = cfb32CreatePixmap;			/* OK */
    pScreen->DestroyPixmap = cfb32DestroyPixmap; 		/* OK */
    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CreateGC = cfb8_32CreateGC;
    pScreen->CreateColormap = miInitializeColormap;
    pScreen->DestroyColormap = (void (*)())NoopDDA;
    pScreen->InstallColormap = miInstallColormap;
    pScreen->UninstallColormap = miUninstallColormap;
    pScreen->ListInstalledColormaps = miListInstalledColormaps;
    pScreen->StoreColors = (void (*)())NoopDDA;
    pScreen->ResolveColor = miResolveColor;
    pScreen->BitmapToRegion = mfbPixmapToRegion;

    mfbRegisterCopyPlaneProc (pScreen, cfb8_32CopyPlane);
    return TRUE;
}

typedef struct {
    pointer pbits; 
    int width;   
} miScreenInitParmsRec, *miScreenInitParmsPtr;

static Bool
cfb8_32CreateScreenResources(ScreenPtr pScreen)
{
    miScreenInitParmsPtr pScrInitParms;
    int pitch;
    Bool retval;

    /* get the pitch before mi destroys it */
    pScrInitParms = (miScreenInitParmsPtr)pScreen->devPrivate;
    pitch = pScrInitParms->width << 2;

    if((retval = miCreateScreenResources(pScreen))) {
	/* fix the screen pixmap */
	PixmapPtr pPix = (PixmapPtr)pScreen->devPrivate;
	pPix->drawable.bitsPerPixel = 32;
	pPix->drawable.depth = 8;
	pPix->devKind = pitch;
    }

    return retval;
}


static Bool
cfb8_32CloseScreen (int i, ScreenPtr pScreen)
{
    cfb8_32ScreenPtr pScreenPriv = CFB8_32_GET_SCREEN_PRIVATE(pScreen);

    xfree((pointer) pScreenPriv);

    return(cfb32CloseScreen(i, pScreen));
}

static Bool
cfb8_32FinishScreenInit(
    ScreenPtr pScreen,
    pointer pbits,		/* pointer to screen bitmap */
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy,		/* dots per inch */
    int width			/* pixel width of frame buffer */
){
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VisualPtr	visuals;
    DepthPtr	depths;
    int		nvisuals;
    int		ndepths;
    int		rootdepth;
    VisualID	defaultVisual;

    rootdepth = 0;
    if (!miInitVisuals (&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
			 &defaultVisual,((unsigned long)1<<(32-1)), 8, -1))
	return FALSE;
    if (! miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
			rootdepth, ndepths, depths,
			defaultVisual, nvisuals, visuals))
	return FALSE;

    pScreen->BackingStoreFuncs = cfb8_32BSFuncRec;
    pScreen->CreateScreenResources = cfb8_32CreateScreenResources;
    pScreen->CloseScreen = cfb8_32CloseScreen;
    pScreen->GetScreenPixmap = cfb32GetScreenPixmap; 	/* OK */
    pScreen->SetScreenPixmap = cfb32SetScreenPixmap;	/* OK */
    pScreen->WindowExposures = cfb8_32WindowExposures;

    pScrn->SaveRestoreImage = xf8_32SaveRestoreImage;

    return TRUE;
}

Bool
cfb8_32ScreenInit(
    ScreenPtr pScreen,
    pointer pbits,		/* pointer to screen bitmap */
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy,		/* dots per inch */
    int width			/* pixel width of frame buffer */
){
    cfb8_32ScreenPtr pScreenPriv;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    if (!cfb8_32SetupScreen(pScreen, pbits, xsize, ysize, dpix, dpiy, width))
	return FALSE;

    pScreenPriv = CFB8_32_GET_SCREEN_PRIVATE(pScreen);
    pScreenPriv->key = pScrn->colorKey;

    return cfb8_32FinishScreenInit(
		pScreen, pbits, xsize, ysize, dpix, dpiy, width);
}

/*
 * Function to save/restore the video image and replace the root drawable
 * with a pixmap.
 *
 * This is used when VT switching and when entering/leaving DGA direct mode.
 *
 * This has been rewritten compared with the older code, with the intention
 * of making it more general.  It relies on some new functions added to
 * the ScreenRec.  It has not been tested yet.
 *
 * Here, we switch the pixmap data pointers, rather than the pixmaps themselves
 * to avoid having to find and change any references to the screen pixmap
 * such as GC's, window privates etc.
 *
 * This version does not currently deal with aperture remappings.  Drivers
 * directly or indirectly calling this should not unmap the aperture on
 * LeaveVT() and remap it on EnterVT().
 */

static Bool
xf8_32SaveRestoreImage(int scrnIndex, SaveRestoreFlags what)
{
    ScreenPtr pScreen;
    static unsigned char *devPrivates[MAXSCREENS];
    static int devKinds[MAXSCREENS];
    pointer devPrivate;
    Bool ret = FALSE;
    int width, height, devKind, bitsPerPixel;
    PixmapPtr pScreenPix, pPix;

    BoxRec pixBox;
    RegionRec pixReg;

    pScreen = xf86Screens[scrnIndex]->pScreen;

    pixBox.x1 = pixBox.y1 = 0;
    pixBox.x2 = pScreen->width;
    pixBox.y2 = pScreen->height;
    REGION_INIT(pScreen, &pixReg, &pixBox, 1);

    pScreenPix = (*pScreen->GetScreenPixmap)(pScreen);

    switch (what) {
    case SaveImage:
        /*
         * Create a dummy pixmap to write to while VT is switched out, and
         * copy the screen to that pixmap.
         */
	width = pScreenPix->drawable.width;
	height = pScreenPix->drawable.height;
	bitsPerPixel = pScreenPix->drawable.bitsPerPixel;

	/* save the old data */
	devPrivates[scrnIndex] = pScreenPix->devPrivate.ptr;
	devKinds[scrnIndex] = pScreenPix->devKind;
	
	/* allocate new data */
	devKind = (((width * bitsPerPixel) + 31) >> 5) << 2; /* which macro ? */
        devPrivate = xalloc(devKind * height);

        if(devPrivate) {
	    pPix = GetScratchPixmapHeader(pScreen, width, height, 
		   pScreen->rootDepth, bitsPerPixel, devKind, devPrivate);
				
	    if(pPix) {
		(*pScreen->BackingStoreFuncs.SaveAreas)(pPix, &pixReg, 0, 0,
						WindowTable[scrnIndex]);

		FreeScratchPixmapHeader(pPix);

		/* modify the pixmap */
		pScreenPix->devPrivate.ptr = devPrivate;
		pScreenPix->devKind = devKind;

		WalkTree(xf86Screens[scrnIndex]->pScreen,xf86NewSerialNumber,0);
		ret = TRUE;
	    } else
		xfree(devPrivate);
	}
	break;
    case RestoreImage:
	/*
	 * Reinstate the screen pixmap and copy the dummy pixmap back
	 * to the screen.
	 */
	
	if (!xf86ServerIsResetting()) {
	     width = pScreenPix->drawable.width;
	     height = pScreenPix->drawable.height;
	     bitsPerPixel = pScreenPix->drawable.bitsPerPixel;
	     devPrivate = pScreenPix->devPrivate.ptr;
	     devKind = pScreenPix->devKind;

	     /* scratch pixmap for the saved screen */
	     pPix = GetScratchPixmapHeader(pScreen, width, height, 
		   pScreen->rootDepth, bitsPerPixel, devKind, devPrivate);

	     if(pPix) {
		/* restore the screen pixmap's correct values */
		pScreenPix->devPrivate.ptr = devPrivates[scrnIndex];
		pScreenPix->devKind = devKinds[scrnIndex];

		(*pScreen->BackingStoreFuncs.RestoreAreas)(pPix, &pixReg, 0, 0,
						       WindowTable[scrnIndex]);
		xfree(devPrivate);
		FreeScratchPixmapHeader(pPix);
		/* restore old values */
		WalkTree(xf86Screens[scrnIndex]->pScreen,xf86NewSerialNumber,0);
		ret = TRUE;
	     }
	     break;
	} 
	/* Fall through */
    case FreeImage:
	if (pScreenPix->devPrivate.ptr)
	    xfree(pScreenPix->devPrivate.ptr);
	ret = TRUE;
	break;
    default:
	ErrorF("xf8_32SaveRestoreImage: Invalid flag (%d)\n", what);
    }
    REGION_UNINIT(pScreen, &pixReg);
    return ret;
}
