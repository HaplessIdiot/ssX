/* $XFree86$ */

#include "vmware.h"

static __inline void
vmwarePictToBox(BoxPtr BB, DrawablePtr pDrawable, int x, int y, int w, int h)
{
    BB->x2 = (BB->x1 = pDrawable->x + x) + w;
    BB->y2 = (BB->y1 = pDrawable->y + y) + h;
}

static __inline void
computeBBpictureRect(DrawablePtr pDrawable, int nrectFill,
		     xRectangle * prectInit, BoxPtr pBB)
{
    if (nrectFill <= 0) {
	pBB->x1 = pBB->x2 = pBB->y1 = pBB->y2 = 0;
	return;
    }
    
    pBB->x1 = prectInit->x;
    pBB->y1 = prectInit->y;
    pBB->x2 = prectInit->x + prectInit->width + 1;
    pBB->y2 = prectInit->y + prectInit->height + 1;
    while (--nrectFill) {
	prectInit++;
	if (prectInit->x < pBB->x1)
	    pBB->x1 = prectInit->x;
	if (prectInit->y < pBB->y1)
	    pBB->y1 = prectInit->y;
	if ((prectInit->x + prectInit->width + 1) > pBB->x2)
	    pBB->x2 = prectInit->x + prectInit->width + 1;
	if ((prectInit->y + prectInit->height + 1) > pBB->y2)
	    pBB->y2 = prectInit->y + prectInit->height + 1;
    }
    pBB->x1 =
	MAX(pDrawable->x + pBB->x1,
	(REGION_EXTENTS(pDrawable->pScreen,
			&((WindowPtr) pDrawable)->winSize))->x1);
    pBB->y1 =
	MAX(pDrawable->y + pBB->y1,
	(REGION_EXTENTS(pDrawable->pScreen,
			&((WindowPtr) pDrawable)->winSize))->y1);
    pBB->x2 =
	MIN(pDrawable->x + pBB->x2,
	(REGION_EXTENTS(pDrawable->pScreen,
			&((WindowPtr) pDrawable)->winSize))->x2);
    pBB->y2 =
	MIN(pDrawable->y + pBB->y2,
	(REGION_EXTENTS(pDrawable->pScreen,
			&((WindowPtr) pDrawable)->winSize))->y2);
}

void vmwareComposite(CARD8 op, PicturePtr pSrc,  PicturePtr pMask,
		     PicturePtr pDst, INT16 xSrc, INT16 ySrc,
		     INT16 xMask, INT16 yMask, INT16 xDst,
		     INT16 yDst, CARD16 width, CARD16 height) 
{
    DrawablePtr         pDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDrawable->pScreen;
    VMWAREPtr pVMWARE = VMWAREPTR(infoFromScreen(pScreen));
    
    VM_FUNC_WRAPPER(pDrawable->type == DRAWABLE_WINDOW,
		    vmwarePictToBox(&BB, pDrawable, xDst, yDst, width, height),
		    pVMWARE->Picture.Composite(op,pSrc,pMask,pDst,xSrc,ySrc,
					       xMask,yMask,xDst,yDst,width,
					       height))
}

void vmwareGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
		  PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
		  int nlist, GlyphListPtr list, GlyphPtr *glyphs)
{
    DrawablePtr         pDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDrawable->pScreen;
    VMWAREPtr		pVMWARE = VMWAREPTR(infoFromScreen(pScreen));
    int			width = 0, height = 0;
    BoxRec		extents;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	
	miGlyphExtents (nlist, list, glyphs, &extents);
	
	if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
	    return;
    
	width = extents.x2 - extents.x1;
	height = extents.y2 - extents.y1;
    }
    
    VM_FUNC_WRAPPER(pDrawable->type == DRAWABLE_WINDOW,
		    vmwarePictToBox(&BB, pDrawable, extents.x1, extents.y1,
				    width, height),
		    pVMWARE->Picture.Glyphs(op,pSrc,pDst,maskFormat,xSrc,ySrc,
					    nlist,list,glyphs))
}

void vmwareRects(CARD8 op, PicturePtr pDst, xRenderColor *color, int nRect,
		  xRectangle *rects)
{
    DrawablePtr         pDrawable = pDst->pDrawable;
    ScreenPtr		pScreen = pDrawable->pScreen;
    VMWAREPtr		pVMWARE = VMWAREPTR(infoFromScreen(pScreen));

    VM_FUNC_WRAPPER(pDrawable->type == DRAWABLE_WINDOW,
		    computeBBpictureRect(pDrawable, nRect, rects, &BB),
		    pVMWARE->Picture.Rects(op,pDst,color, nRect,rects))
}
