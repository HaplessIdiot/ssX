/* $XFree86$ */

#ifndef XFree86LOADER
#ifdef _XOPEN_SOURCE
#include <math.h>
#else
#define _XOPEN_SOURCE   /* to get prototype for pow on some systems */
#include <math.h>
#undef _XOPEN_SOURCE
#endif
#endif

#include "X.h"
#include "misc.h"
#include "Xproto.h"
#include "colormapst.h"
#include "scrnintstr.h"

#include "resource.h"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86str.h"
#include "micmap.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#include "dgaproc.h"
#endif

#include "xf86cmap.h"

#define SCREEN_PROLOGUE(pScreen, field) ((pScreen)->field = \
   ((CMapScreenPtr) (pScreen)->devPrivates[CMapScreenIndex].ptr)->field)
#define SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

#ifdef XFreeXDGA
/* need to fix this up for DGA */
#define LOAD_PALETTE(pmap, index) ((pmap == InstalledMaps[index]) && \
 				xf86Screens[index]->vtSema) 
#else
#define LOAD_PALETTE(pmap, index) ((pmap == InstalledMaps[index]) && \
 				xf86Screens[index]->vtSema) 
#endif


typedef struct {
  ScrnInfoPtr			pScrn;
  CloseScreenProcPtr		CloseScreen;
  CreateColormapProcPtr 	CreateColormap;
  DestroyColormapProcPtr	DestroyColormap;
  InstallColormapProcPtr	InstallColormap;
  StoreColorsProcPtr		StoreColors;
  LoadPaletteFuncPtr		LoadPalette;
  Bool				(*EnterVT)(int, int);
  Bool				(*SwitchMode)(int, DisplayModePtr, int);
  int				maxColors;
  int				sigRGBbits;
  int				gammaElements;
  LOCO				*gamma;
  int				*PreAllocIndicies;
  unsigned int			flags;
} CMapScreenRec, *CMapScreenPtr;

typedef struct {
  int		numColors;
  LOCO		*colors;
  Bool		recalculate;
} CMapColormapRec, *CMapColormapPtr;

static unsigned long CMapGeneration = 0;
static int CMapScreenIndex = -1;
static int CMapColormapIndex = -1;

static void CMapInstallColormap(ColormapPtr);
static void CMapStoreColors(ColormapPtr, int, xColorItem *);
static Bool CMapCloseScreen (int, ScreenPtr);
static Bool CMapCreateColormap (ColormapPtr);
static void CMapDestroyColormap (ColormapPtr);

static Bool CMapEnterVT(int, int);
static Bool CMapSwitchMode(int, DisplayModePtr, int);

static void ComputeGamma(CMapScreenPtr);
static Bool CMapAllocateColormapPrivate(ColormapPtr);
static Bool CMapInitDefMap(ColormapPtr);
static void CMapRefreshColors(ColormapPtr, int, int*);
static void CMapReinstallMap(ColormapPtr);


Bool xf86HandleColormaps(
    ScreenPtr pScreen,
    int maxColors,
    int sigRGBbits,
    LoadPaletteFuncPtr loadPalette,
    unsigned int flags
){
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    ColormapPtr pDefMap = NULL;
    CMapScreenPtr pScreenPriv;  
    LOCO *gamma; 
    int *indicies; 
    int elements;

    if(!maxColors || !sigRGBbits || !loadPalette)
	return FALSE;

    if(CMapGeneration != serverGeneration) {
	if(((CMapScreenIndex = AllocateScreenPrivateIndex()) < 0) ||
	   ((CMapColormapIndex = AllocateColormapPrivateIndex(
					CMapInitDefMap)) < 0))
		return FALSE;
	CMapGeneration = serverGeneration;
    }

    elements = 1 << sigRGBbits;

    if(!(gamma = (LOCO*)xalloc(elements * sizeof(LOCO))))
    	return FALSE;

    if(!(indicies = (int*)xalloc(maxColors * sizeof(int)))) {
	xfree(gamma);
	return FALSE;
    }
      
    if(!(pScreenPriv = (CMapScreenPtr)xalloc(sizeof(CMapScreenRec)))) {
	xfree(gamma);
	xfree(indicies);
	return FALSE;     
    }

    pScreen->devPrivates[CMapScreenIndex].ptr = (pointer)pScreenPriv;
     
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->CreateColormap = pScreen->CreateColormap;
    pScreenPriv->DestroyColormap = pScreen->DestroyColormap;
    pScreenPriv->InstallColormap = pScreen->InstallColormap;
    pScreenPriv->StoreColors = pScreen->StoreColors;
    pScreen->CloseScreen = CMapCloseScreen;
    pScreen->CreateColormap = CMapCreateColormap;
    pScreen->DestroyColormap = CMapDestroyColormap;
    pScreen->InstallColormap = CMapInstallColormap;
    pScreen->StoreColors = CMapStoreColors;

    pScreenPriv->pScrn = pScrn;
    pScreenPriv->LoadPalette = loadPalette;
    pScreenPriv->maxColors = maxColors;
    pScreenPriv->sigRGBbits = sigRGBbits;
    pScreenPriv->gammaElements = elements;
    pScreenPriv->gamma = gamma;
    pScreenPriv->PreAllocIndicies = indicies;
    pScreenPriv->flags = flags;

    pScreenPriv->EnterVT = pScrn->EnterVT;
    pScreenPriv->SwitchMode = pScrn->SwitchMode;

    pScrn->EnterVT = CMapEnterVT;
    if(flags & CMAP_RELOAD_ON_MODE_SWITCH)
	pScrn->SwitchMode = CMapSwitchMode;
 
    ComputeGamma(pScreenPriv);

    /* get the default map */

    pDefMap = (ColormapPtr) LookupIDByType(pScreen->defColormap, RT_COLORMAP);

    if(!CMapAllocateColormapPrivate(pDefMap)) {
	pScreen->CloseScreen = pScreenPriv->CloseScreen;
	pScreen->CreateColormap = pScreenPriv->CreateColormap;
	pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
	pScreen->StoreColors = pScreenPriv->StoreColors;
	xfree(gamma);
	xfree(indicies);
	xfree(pScreenPriv);
	return FALSE;
    }
    CMapInstallColormap(pDefMap);
    return TRUE;
}

static Bool 
CMapInitDefMap(ColormapPtr cmap)
{
    return TRUE;
}


/**** Screen functions ****/


static Bool
CMapCloseScreen (int i, ScreenPtr pScreen)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->CreateColormap = pScreenPriv->CreateColormap;
    pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->StoreColors = pScreenPriv->StoreColors;

    pScrn->EnterVT = pScreenPriv->EnterVT; 
    pScrn->SwitchMode = pScreenPriv->SwitchMode; 

    xfree(pScreenPriv->gamma);
    xfree(pScreenPriv->PreAllocIndicies);
    xfree(pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static Bool
CMapAllocateColormapPrivate(ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pmap->pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr pColPriv;
    int numColors;
    LOCO *colors;

    if((1 << pmap->pVisual->nplanes) > pScreenPriv->maxColors)
	numColors = pmap->pVisual->ColormapEntries;
    else 
	numColors = 1 << pmap->pVisual->nplanes; 

    if(!(colors = (LOCO*)xalloc(numColors * sizeof(LOCO))))
	return FALSE;

    if(!(pColPriv = (CMapColormapPtr)xalloc(sizeof(CMapColormapRec)))) {
	xfree(colors);
	return FALSE;
    }	

    pmap->devPrivates[CMapColormapIndex].ptr = (pointer)pColPriv;
 
    pColPriv->numColors = numColors;
    pColPriv->colors = colors;
    pColPriv->recalculate = TRUE;

    return TRUE;
}

static Bool 
CMapCreateColormap (ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pmap->pScreen->devPrivates[CMapScreenIndex].ptr;

    if(!(*pScreenPriv->CreateColormap)(pmap)) 
	return FALSE;

    if(!CMapAllocateColormapPrivate(pmap))
	return FALSE;

    return TRUE;
}

static void
CMapDestroyColormap (ColormapPtr cmap)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) cmap->pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr pColPriv = 
	(CMapColormapPtr) cmap->devPrivates[CMapColormapIndex].ptr;

    if(pColPriv) {
	if(pColPriv->colors) xfree(pColPriv->colors);
	xfree(pColPriv);
    }
   
    if(pScreenPriv->DestroyColormap)
	(*pScreenPriv->DestroyColormap)(cmap);
}



static void
CMapStoreColors(
     ColormapPtr	pmap,
     int		ndef,
     xColorItem	        *pdefs
){
    ScreenPtr 	pScreen = pmap->pScreen;
    VisualPtr	pVisual = pmap->pVisual;
    CMapScreenPtr pScreenPriv = 
        	(CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;
    int 	*indicies = pScreenPriv->PreAllocIndicies;
    int		num = ndef;

    pScreen->StoreColors = pScreenPriv->StoreColors;
    (*pScreen->StoreColors)(pmap, ndef, pdefs); 
    pScreen->StoreColors = CMapStoreColors;

    /* should never get here for these */
    if(	(pVisual->class == TrueColor) ||
	(pVisual->class == StaticColor) ||
	(pVisual->class == StaticGray))
	return;

    if(pVisual->class == DirectColor) {
	CMapColormapPtr pColPriv = 
	   (CMapColormapPtr) pmap->devPrivates[CMapColormapIndex].ptr;
	int i;

        if((1 << pVisual->nplanes) > pScreenPriv->maxColors) {
	    int index;

	    num = 0;
	    while(ndef--) {
		if(pdefs[ndef].flags & DoRed) {
		    index = (pdefs[ndef].pixel & pVisual->redMask) >>
					pVisual->offsetRed;
		    i = num;
		    while(i--)
			if(indicies[i] == index) break;
		    if(i == -1)
			indicies[num++] = index;
		}
		if(pdefs[ndef].flags & DoGreen) {
		    index = (pdefs[ndef].pixel & pVisual->greenMask) >>
					pVisual->offsetGreen;
		    i = num;
		    while(i--)
			if(indicies[i] == index) break;
		    if(i == -1)
			indicies[num++] = index;
		}
		if(pdefs[ndef].flags & DoBlue) {
		    index = (pdefs[ndef].pixel & pVisual->blueMask) >>
					pVisual->offsetBlue;
		    i = num;
		    while(i--)
			if(indicies[i] == index) break;
		    if(i == -1)
			indicies[num++] = index;
		}
	    }

	} else {
	    /* not really as overkill as it seems */
	    num = pColPriv->numColors;
	    for(i = 0; i < pColPriv->numColors; i++)
		indicies[i] = i;
	}
    } else {
	while(ndef--)
	   indicies[ndef] = pdefs[ndef].pixel;
    } 

    CMapRefreshColors(pmap, num, indicies);
}


static void
CMapInstallColormap(ColormapPtr pmap)
{
    ScreenPtr 	  pScreen = pmap->pScreen;
    int		  index = pScreen->myNum;
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;

    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    (*pScreen->InstallColormap)(pmap);
    pScreen->InstallColormap = CMapInstallColormap;

    if (pmap == InstalledMaps[index]) return;

    InstalledMaps[index] = pmap;

    if(!(pScreenPriv->flags & CMAP_PALETTED_TRUECOLOR) &&
	(pmap->pVisual->class == TrueColor) &&
	((1 << pmap->pVisual->nplanes) > pScreenPriv->maxColors))
		return;

    if(LOAD_PALETTE(pmap, index))
	CMapReinstallMap(pmap);
}


/**** ScrnInfoRec functions ****/

static Bool 
CMapEnterVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;

    if((*pScreenPriv->EnterVT)(index, flags)) {
	if(InstalledMaps[index])
	    CMapReinstallMap(InstalledMaps[index]);
	return TRUE;
    }
    return FALSE;
}


static Bool 
CMapSwitchMode(int index, DisplayModePtr mode, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;

    if((*pScreenPriv->SwitchMode)(index, mode, flags)) {
	if(InstalledMaps[index])
	    CMapReinstallMap(InstalledMaps[index]);
	return TRUE;
    }
    return FALSE;
}


/**** Utilities ****/

static void
CMapReinstallMap(ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pmap->pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr cmapPriv = 
	(CMapColormapPtr) pmap->devPrivates[CMapColormapIndex].ptr;
    int i = cmapPriv->numColors;
    int *indicies = pScreenPriv->PreAllocIndicies;

    while(i--)
	indicies[i] = i;
   
    if(cmapPriv->recalculate)
	CMapRefreshColors(pmap, cmapPriv->numColors, indicies);
    else
	(*pScreenPriv->LoadPalette)(pScreenPriv->pScrn, cmapPriv->numColors,
 			indicies, cmapPriv->colors, pmap->pVisual->class);
}


static void 
CMapRefreshColors(ColormapPtr pmap, int defs, int* indicies)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pmap->pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr pColPriv = 
	(CMapColormapPtr) pmap->devPrivates[CMapColormapIndex].ptr;
    VisualPtr pVisual = pmap->pVisual;
    int numColors, i;
    LOCO *gamma, *colors;
    EntryPtr entry;
    int reds, greens, blues, maxValue, index, shift;

    numColors = pColPriv->numColors;
    shift = 16 - pScreenPriv->sigRGBbits;
    maxValue = (1 << pScreenPriv->sigRGBbits) - 1;
    gamma = pScreenPriv->gamma;
    colors = pColPriv->colors;

    reds = pVisual->redMask >> pVisual->offsetRed;
    greens = pVisual->greenMask >> pVisual->offsetGreen;
    blues = pVisual->blueMask >> pVisual->offsetBlue;

    switch(pVisual->class) {
    case StaticGray:
	for(i = 0; i <= numColors - 1; i++) { 
	    index = i * maxValue / numColors;
	    colors[i].red   = gamma[index].red;
	    colors[i].green = gamma[index].green;
	    colors[i].blue  = gamma[index].blue;
	}
	break;
    case TrueColor:
	if((1 << pVisual->nplanes) > pScreenPriv->maxColors) {
	    for(i = 0; i <= reds; i++) 
		colors[i].red   = gamma[i * maxValue / reds].red;
	    for(i = 0; i <= greens; i++) 
		colors[i].green = gamma[i * maxValue / greens].green;
	    for(i = 0; i <= blues; i++) 
		colors[i].blue  = gamma[i * maxValue / blues].blue;
	    break;
	}
	/* fall through */
    case StaticColor:
	for(i = 0; i < numColors; i++) {
	    colors[i].red   = gamma[((i >> pVisual->offsetRed) & reds) * 
					maxValue / reds].red;
	    colors[i].green = gamma[((i >> pVisual->offsetGreen) & greens) * 
					maxValue / greens].green;
	    colors[i].blue  = gamma[((i >> pVisual->offsetBlue) & blues) * 
					maxValue / blues].blue;
	}
	break;
    case PseudoColor:
    case GrayScale:
	for(i = 0; i < defs; i++) { 
	    index = indicies[i];
	    entry = (EntryPtr)&pmap->red[index];

	    if(entry->fShared) {
		colors[index].red = 
			gamma[entry->co.shco.red->color >> shift].red;
		colors[index].green = 
			gamma[entry->co.shco.green->color >> shift].green;
		colors[index].blue = 
			gamma[entry->co.shco.blue->color >> shift].blue;
	    } else {
		colors[index].red   = 
				gamma[entry->co.local.red >> shift].red;
		colors[index].green = 
				gamma[entry->co.local.green >> shift].green;
		colors[index].blue  = 
				gamma[entry->co.local.blue >> shift].blue;
	    }
	}
	break;
    case DirectColor:
	if((1 << pVisual->nplanes) > pScreenPriv->maxColors) {
	    for(i = 0; i < defs; i++) { 
		index = indicies[i];
		if(index <= reds)
		    colors[index].red   = 
			gamma[pmap->red[index].co.local.red >> shift].red;
		if(index <= greens)
		    colors[index].green = 
			gamma[pmap->green[index].co.local.green >> shift].green;
		if(index <= blues)
		    colors[index].blue   = 
			gamma[pmap->blue[index].co.local.blue >> shift].blue;

	    }
	    break;
	}
	for(i = 0; i < defs; i++) { 
	    index = indicies[i];
		
	    colors[index].red   = gamma[pmap->red[
				(index >> pVisual->offsetRed) & reds
				].co.local.red >> shift].red;
	    colors[index].green = gamma[pmap->green[
				(index >> pVisual->offsetGreen) & greens
				].co.local.green >> shift].green;
	    colors[index].blue  = gamma[pmap->blue[
				(index >> pVisual->offsetBlue) & blues
				].co.local.blue >> shift].blue;
	}
	break;
    }


    if(LOAD_PALETTE(pmap, pmap->pScreen->myNum))
	(*pScreenPriv->LoadPalette)(pScreenPriv->pScrn, defs, indicies,
 					colors, pmap->pVisual->class);

    pColPriv->recalculate = FALSE;
}


static void 
ComputeGamma(CMapScreenPtr priv)
{
    int elements = priv->gammaElements - 1;
    double RedGamma = (double)priv->pScrn->gamma.red;
    double GreenGamma = (double)priv->pScrn->gamma.green;
    double BlueGamma = (double)priv->pScrn->gamma.blue;
    int i;

    for(i = 0; i <= elements; i++) {
	if(RedGamma == 1.0)  
	    priv->gamma[i].red = i;
	else
	    priv->gamma[i].red = (CARD16)(pow((double)i/(double)elements,
			RedGamma) * (double)elements + 0.5);

	if(GreenGamma == 1.0)  
	    priv->gamma[i].green = i;
	else
	    priv->gamma[i].green = (CARD16)(pow((double)i/(double)elements,
			GreenGamma) * (double)elements + 0.5);

	if(BlueGamma == 1.0)  
	    priv->gamma[i].blue = i;
	else
	    priv->gamma[i].blue = (CARD16)(pow((double)i/(double)elements,
			 BlueGamma) * (double)elements + 0.5);
    }
}

