/*
 * $XFree86$
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include    "layerstr.h"

int layerScrPrivateIndex;
int layerGCPrivateIndex;
int layerWinPrivateIndex;
int layerGeneration;

/*
 * Call this before wrapping stuff for acceleration, it
 * gives layer pointers to the raw frame buffer functions
 */

extern const GCFuncs fbGCFuncs;
extern GCFuncs shadowGCFuncs;

Bool
LayerStartInit (ScreenPtr pScreen)
{
    LayerScreenPtr  pScrPriv;
    
    if (layerGeneration != serverGeneration)
    {
	layerScrPrivateIndex = AllocateScreenPrivateIndex ();
	if (layerScrPrivateIndex == -1)
	    return FALSE;
	layerGCPrivateIndex = AllocateGCPrivateIndex ();
	if (layerGCPrivateIndex == -1)
	    return FALSE;
	layerWinPrivateIndex = AllocateWindowPrivateIndex ();
	if (layerWinPrivateIndex == -1)
	    return FALSE;
	layerGeneration = serverGeneration;
    }
    if (!AllocateGCPrivate (pScreen, layerGCPrivateIndex, sizeof (LayerGCRec)))
	return FALSE;
    if (!AllocateWindowPrivate (pScreen, layerWinPrivateIndex, sizeof (LayerWinRec)))
	return FALSE;
    pScrPriv = (LayerScreenPtr) xalloc (sizeof (LayerScreenRec));
    if (!pScrPriv)
	return FALSE;
    pScrPriv->nkinds = 0;
    pScrPriv->kinds = 0;
    pScrPriv->pLayers = 0;
    pScreen->devPrivates[layerScrPrivateIndex].ptr = (pointer) pScrPriv;
    /*
     * Add fb kind -- always 0
     */
    if (LayerNewKind (pScreen) < 0)
    {
	pScreen->devPrivates[layerScrPrivateIndex].ptr = 0;
	xfree (pScrPriv);
	return FALSE;
    }
    /*
     * Add shadow kind -- always 1
     */
    if (!shadowSetup (pScreen))
	return FALSE;
    if (LayerNewKind (pScreen) < 0)
    {
	pScreen->devPrivates[layerScrPrivateIndex].ptr = 0;
	xfree (pScrPriv->kinds);
	xfree (pScrPriv);
	return FALSE;
    }
    LayerSetGCFuncs (pScreen, LAYER_FB, (GCFuncs *) &fbGCFuncs, 0, 0);
    LayerSetGCFuncs (pScreen, LAYER_SHADOW, &shadowGCFuncs, shadowWrapGC, shadowUnwrapGC);
    return TRUE;
}

/*
 * Initialize wrappers for each acceleration type and
 * call this function, it will move the needed functions
 * into a new LayerKind and replace them with the generic
 * functions.
 */

int
LayerNewKind (ScreenPtr pScreen)
{
    layerScrPriv(pScreen);
    LayerKindPtr	pLayKind, pLayKinds;
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreen (pScreen);
#endif
    
    /*
     * Allocate a new kind structure
     */
    if (pLayScr->kinds)
	pLayKinds = (LayerKindPtr) xrealloc ((pointer) pLayScr->kinds,
					     (pLayScr->nkinds + 1) * sizeof (LayerKindRec));
    else
	pLayKinds = (LayerKindPtr) xalloc (sizeof (LayerKindRec));
    if (!pLayKinds)
	return -1;
    pLayScr->kinds = pLayKinds;
    pLayKind = &pLayScr->kinds[pLayScr->nkinds];
    pLayKind->kind = pLayScr->nkinds;

    /*
     * Extract wrapped functions from screen and stick in kind
     */
    pLayKind->CloseScreen = pScreen->CloseScreen;
    
    pLayKind->CreateWindow = pScreen->CreateWindow;
    pLayKind->DestroyWindow = pScreen->DestroyWindow;
    pLayKind->ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
    pLayKind->PaintWindowBackground = pScreen->PaintWindowBackground;
    pLayKind->PaintWindowBorder = pScreen->PaintWindowBorder;
    pLayKind->CopyWindow = pScreen->CopyWindow;

    pLayKind->CreateGC = pScreen->CreateGC;

#ifdef RENDER
    if (ps)
    {
	pLayKind->Composite = ps->Composite;
	pLayKind->Glyphs = ps->Glyphs;
	pLayKind->CompositeRects = ps->CompositeRects;
    }
#endif
    /*
     * If not underlying frame buffer kind,
     * replace screen functions with those
     */
    if (pLayKind->kind != 0)
    {
	pScreen->CloseScreen = pLayKinds->CloseScreen;

	pScreen->CreateWindow = pLayKinds->CreateWindow;
	pScreen->DestroyWindow = pLayKinds->DestroyWindow;
	pScreen->ChangeWindowAttributes = pLayKinds->ChangeWindowAttributes;
	pScreen->PaintWindowBackground = pLayKinds->PaintWindowBackground;
	pScreen->PaintWindowBorder = pLayKinds->PaintWindowBorder;
	pScreen->CopyWindow = pLayKinds->CopyWindow;

	pScreen->CreateGC = pLayKinds->CreateGC;

#ifdef RENDER
	if (ps)
	{
	    ps->Composite = pLayKinds->Composite;
	    ps->Glyphs = pLayKinds->Glyphs;
	    ps->CompositeRects = pLayKinds->CompositeRects;
	}
#endif
    }

    pLayScr->nkinds++;
    return pLayKind->kind;
}

/*
 * Set the GC ops/funcs to use for this layer
 */

void
LayerSetGCFuncs (ScreenPtr pScreen, int kind, 
		 GCFuncs *funcs,
		 void (*WrapGC) (GCPtr pGC),
		 void (*UnwrapGC) (GCPtr pGC))
{
    layerScrPriv(pScreen);
    LayerKindPtr    pLayKind;

    if (0 <= kind && kind < pLayScr->nkinds)
    {
	pLayKind = &pLayScr->kinds[kind];
	pLayKind->funcs = funcs;
	pLayKind->WrapGC = WrapGC;
	pLayKind->UnwrapGC = UnwrapGC;
    }
}

/*
 * Finally, call this function and layer
 * will wrap the screen functions and prepare for execution
 */

Bool
LayerFinishInit (ScreenPtr pScreen)
{
    layerScrPriv(pScreen);
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreen (pScreen);
#endif
    
    pScreen->CloseScreen = layerCloseScreen;

    pScreen->CreateWindow = layerCreateWindow;
    pScreen->DestroyWindow = layerDestroyWindow;
    pScreen->ChangeWindowAttributes = layerChangeWindowAttributes;
    pScreen->PaintWindowBackground = layerPaintWindowBackground;
    pScreen->PaintWindowBorder = layerPaintWindowBorder;
    pScreen->CopyWindow = layerCopyWindow;

    pScreen->CreateGC = layerCreateGC;

#ifdef RENDER
    if (ps)
    {
	ps->Composite = layerComposite;
	ps->Glyphs = layerGlyphs;
	ps->CompositeRects = layerCompositeRects;
    }
#endif
    
    return TRUE;
}

/*
 * At any point after LayerFinishInit, a new layer can be created.
 */
LayerPtr
LayerCreate (ScreenPtr		pScreen, 
	     int		kind, 
	     int		depth,
	     PixmapPtr		pPixmap,
	     ShadowUpdateProc	update,
	     ShadowWindowProc	window,
	     void		*closure)
{
    layerScrPriv(pScreen);
    LayerPtr	    pLay, *pPrev;
    LayerKindPtr    pLayKind;

    if (kind < 0 || pLayScr->nkinds <= kind)
	return 0;
    pLayKind = &pLayScr->kinds[kind];
    pLay = (LayerPtr) xalloc (sizeof (LayerRec));
    if (!pLay)
	return 0;
    /*
     * Initialize the layer
     */
    pLay->pNext = 0;
    pLay->pKind = pLayKind;
    pLay->refcnt = 1;
    pLay->windows = 0;
    pLay->depth = depth;
    pLay->pPixmap = pPixmap;
    pLay->update = update;
    pLay->window = window;
    pLay->closure = closure;
    if (pPixmap && pPixmap != LAYER_SCREEN_PIXMAP)
	pPixmap->refcnt++;
    REGION_INIT (pScreen, &pLay->region, NullBox, 0);
    /*
     * Hook the layer at the end of the list
     */
    for (pPrev = &pLayScr->pLayers; *pPrev; pPrev = &(*pPrev)->pNext)
	;
    *pPrev = pLay;
    return pLay;
}

/*
 * Change a layer pixmap
 */
void
LayerSetPixmap (ScreenPtr pScreen, LayerPtr pLayer, PixmapPtr pPixmap)
{
    if (pLayer->pPixmap && pLayer->pPixmap != LAYER_SCREEN_PIXMAP)
	(*pScreen->DestroyPixmap) (pLayer->pPixmap);
    pLayer->pPixmap = pPixmap;
    if (pPixmap && pPixmap != LAYER_SCREEN_PIXMAP)
	pPixmap->refcnt++;
}

/*
 * Destroy a layer. The layer must not contain any windows.
 */
void
LayerDestroy (ScreenPtr pScreen, LayerPtr pLay)
{
    layerScrPriv(pScreen);
    LayerPtr	*pPrev;
    
    --pLay->refcnt;
    if (pLay->refcnt > 0)
	return;
    /*
     * Unhook the layer from the list
     */
    for (pPrev = &pLayScr->pLayers; *pPrev; pPrev = &(*pPrev)->pNext)
	if (*pPrev == pLay)
	{
	    *pPrev = pLay->pNext;
	    break;
	}
    /*
     * Free associated storage
     */
    if (pLay->pPixmap && pLay->pPixmap != LAYER_SCREEN_PIXMAP)
	(*pScreen->DestroyPixmap) (pLay->pPixmap);
    REGION_UNINIT (pScreen, &pLay->region);
}

/*
 * CloseScreen wrapper
 */
Bool
layerCloseScreen (int index, ScreenPtr pScreen)
{
    layerScrPriv(pScreen);
    int	    kind;

    for (kind = pLayScr->nkinds - 1; kind > 0; kind--)
    {
	pScreen->CloseScreen = pLayScr->kinds[kind].CloseScreen;
	(*pScreen->CloseScreen) (index, pScreen);
    }
    xfree (pLayScr->kinds);
    xfree (pLayScr);
    pScreen->devPrivates[layerScrPrivateIndex].ptr = 0;
    return TRUE;
}
