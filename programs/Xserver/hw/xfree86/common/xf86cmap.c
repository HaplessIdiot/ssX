/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86cmap.c,v 1.2 1998/11/22 10:37:16 dawes Exp $ */

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
#define LOAD_PALETTE(pmap, index) ((pmap == miInstalledMaps[index]) && \
 				xf86Screens[index]->vtSema) 
#else
#define LOAD_PALETTE(pmap, index) ((pmap == miInstalledMaps[index]) && \
 				xf86Screens[index]->vtSema) 
#endif


typedef struct _CMapLink {
  ColormapPtr		cmap;
  struct _CMapLink	*next;
} CMapLink, *CMapLinkPtr;

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
  CMapLinkPtr			maps;
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
static void UnwrapScreen(ScreenPtr pScreen);



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
    pScreenPriv->maps = NULL;
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
        UnwrapScreen(pScreen);
	return FALSE;
    }

    /* Force the initial map to be loaded */
    miInstalledMaps[pScreen->myNum] = NULL;
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
    UnwrapScreen(pScreen);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static Bool
CMapAllocateColormapPrivate(ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pmap->pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr pColPriv;
    CMapLinkPtr pLink;
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

    /* add map to list */
    pLink = (CMapLinkPtr)xalloc(sizeof(CMapLink));
    if(pLink) {
	pLink->cmap = pmap;
	pLink->next = pScreenPriv->maps;
	pScreenPriv->maps = pLink;
    }

    return TRUE;
}

static Bool 
CMapCreateColormap (ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr)pScreen->devPrivates[CMapScreenIndex].ptr;
    Bool ret = FALSE;

    pScreen->CreateColormap = pScreenPriv->CreateColormap;
    if((*pScreen->CreateColormap)(pmap)) { 
	if(CMapAllocateColormapPrivate(pmap)) 
	   ret = TRUE;
    }
    pScreen->CreateColormap = CMapCreateColormap;

    return ret;
}

static void
CMapDestroyColormap (ColormapPtr cmap)
{
    ScreenPtr pScreen = cmap->pScreen;
    CMapScreenPtr pScreenPriv = 
        (CMapScreenPtr) pScreen->devPrivates[CMapScreenIndex].ptr;
    CMapColormapPtr pColPriv = 
	(CMapColormapPtr) cmap->devPrivates[CMapColormapIndex].ptr;
    CMapLinkPtr prevLink = NULL, pLink = pScreenPriv->maps;

    if(pColPriv) {
	if(pColPriv->colors) xfree(pColPriv->colors);
	xfree(pColPriv);
    }
   
    /* remove map from list */
    while(pLink) {
	if(pLink->cmap == cmap) {
	   if(prevLink)
		prevLink->next = pLink->next;
	   else
		pScreenPriv->maps = pLink->next;
	   xfree(pLink);
	   break;
	}
	prevLink = pLink;
	pLink = pLink->next;
    }

    if(pScreenPriv->DestroyColormap) {
    	pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
	(*pScreen->DestroyColormap)(cmap);
    	pScreen->DestroyColormap = CMapDestroyColormap;
    }
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

    /* At the moment this isn't necessary since there's nobody below us */
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

    if (pmap == miInstalledMaps[index])
	return;

    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    (*pScreen->InstallColormap)(pmap);
    pScreen->InstallColormap = CMapInstallColormap;

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
	if(miInstalledMaps[index])
	    CMapReinstallMap(miInstalledMaps[index]);
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
	if(miInstalledMaps[index])
	    CMapReinstallMap(miInstalledMaps[index]);
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

    cmapPriv->recalculate = FALSE;
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
	for(i = 0; i < numColors; i++) {
	    colors[i].red   = gamma[((i >> pVisual->offsetRed) & reds) * 
					maxValue / reds].red;
	    colors[i].green = gamma[((i >> pVisual->offsetGreen) & greens) * 
					maxValue / greens].green;
	    colors[i].blue  = gamma[((i >> pVisual->offsetBlue) & blues) * 
					maxValue / blues].blue;
	}
	break;
    case StaticColor:
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

}

static void
UnwrapScreen(ScreenPtr pScreen)
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
}


static void 
ComputeGamma(CMapScreenPtr priv)
{
    int elements = priv->gammaElements - 1;
    double RedGamma = 1.0 / (double)priv->pScrn->gamma.red;
    double GreenGamma = 1.0 / (double)priv->pScrn->gamma.green;
    double BlueGamma = 1.0 / (double)priv->pScrn->gamma.blue;
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


int
xf86ChangeGamma(
   ScreenPtr pScreen,
   Gamma gamma
){
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CMapColormapPtr pColPriv;
    CMapScreenPtr pScreenPriv;
    CMapLinkPtr pLink;
        
    /* Is this sufficient checking ? */
    if(CMapScreenIndex == -1)
	return BadImplementation;

    pScreenPriv = (CMapScreenPtr)pScreen->devPrivates[CMapScreenIndex].ptr;
    if(!pScreenPriv)
	return BadImplementation;
 
    if (gamma.red < GAMMA_MIN || gamma.red > GAMMA_MAX ||
	gamma.green < GAMMA_MIN || gamma.green > GAMMA_MAX ||
	gamma.blue < GAMMA_MIN || gamma.blue > GAMMA_MAX)
	return BadValue;

    pScrn->gamma.red = gamma.red;
    pScrn->gamma.green = gamma.green;
    pScrn->gamma.blue = gamma.blue;

    ComputeGamma(pScreenPriv);

    /* mark all colormaps on this screen */
    pLink = pScreenPriv->maps;
    while(pLink) {
    	pColPriv = 
	 (CMapColormapPtr) pLink->cmap->devPrivates[CMapColormapIndex].ptr;
	pColPriv->recalculate = TRUE;
	pLink = pLink->next;
    }

    if(miInstalledMaps[pScreen->myNum] && pScrn->vtSema)
	CMapReinstallMap(miInstalledMaps[pScreen->myNum]);

    return Success;
} 


