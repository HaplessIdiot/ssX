/* $XFree86$ */

#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "mi.h"

void
cfb8_32PaintWindow(
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what
){
    cfb8_32ScreenPtr pScreenPriv;
    WindowPtr pBgWin;

    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    break;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(
						pWin, pRegion, what);
	    break;
	case BackgroundPixmap:
	    cfb32FillBoxTileOddCopy ((DrawablePtr)pWin,
			(int)REGION_NUM_RECTS(pRegion), REGION_RECTS(pRegion),
			pWin->background.pixmap,
			(int) pWin->drawable.x, (int) pWin->drawable.y,
			GXcopy, (pWin->drawable.depth == 24) ? 0x00ffffff :
				0xff000000);
	    break;
	case BackgroundPixel:
	    if(pWin->drawable.depth == 24) 
		cfb8_32FillBoxSolid32 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    else
		cfb8_32FillBoxSolid8 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    break;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel) {
	    if(pWin->drawable.depth == 24) {
	       pScreenPriv = CFB8_32_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	       cfb32FillBoxSolid ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     (pScreenPriv->key << 24) | 
			     (pWin->border.pixel & 0x00ffffff));
	    } else
	       cfb8_32FillBoxSolid8 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel);
	} else {
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);

	    cfb32FillBoxTileOddCopy ((DrawablePtr)pWin,
			(int)REGION_NUM_RECTS(pRegion), REGION_RECTS(pRegion),
			pWin->border.pixmap,
			(int) pBgWin->drawable.x, (int) pBgWin->drawable.y,
			GXcopy, (pWin->drawable.depth == 24) ? 0x00ffffff :
				0xff000000);

	    if(pWin->drawable.depth == 24) {
	       pScreenPriv = CFB8_32_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	       cfb8_32FillBoxSolid8 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pScreenPriv->key);
	    }
	}
	break;
    }

}

void
cfb8_32FillBoxSolid8(
   DrawablePtr pDraw,
   int nbox,
   BoxPtr pbox,
   unsigned long color
){
    CARD8 *ptr, *data;
    int pitch, height, width, i;
    CARD8 c = (CARD8)color;

    cfbGetByteWidthAndPointer(pDraw, pitch, ptr);
    ptr += 3; /* point to the top byte */

    while(nbox--) {
	data = ptr + (pbox->y1 * pitch) + (pbox->x1 << 2);
	width = (pbox->x2 - pbox->x1) << 2;
	height = pbox->y2 - pbox->y1;

	while(height--) {
            for(i = 0; i < width; i+=4)
		data[i] = c;
            data += pitch;
	}
	pbox++;
    }
}


void
cfb8_32FillBoxSolid32(
   DrawablePtr pDraw,
   int nbox,
   BoxPtr pbox,
   unsigned long color
){
    CARD8 *ptr, *data;
    CARD16 *ptr2, *data2;
    int pitch, pitch2;
    int height, width, i;
    CARD8 c = (CARD8)(color >> 16);
    CARD16 c2 = (CARD16)color;

    cfbGetByteWidthAndPointer(pDraw, pitch, ptr);
    cfbGetTypedWidthAndPointer(pDraw, pitch2, ptr2, CARD16, CARD16);
    ptr += 2; /* point to the third byte */

    while(nbox--) {
	data = ptr + (pbox->y1 * pitch) + (pbox->x1 << 2);
	data2 = ptr2 + (pbox->y1 * pitch2) + (pbox->x1 << 1);
	width = (pbox->x2 - pbox->x1) << 1;
	height = pbox->y2 - pbox->y1;

	while(height--) {
            for(i = 0; i < width; i+=2) {
		data[i << 1] = c;
		data2[i] = c2;
	    }
            data += pitch;
            data2 += pitch2;
	}
	pbox++;
    }
}


unsigned char
cfb8_32GetKey(ScreenPtr pScreen)
{
    cfb8_32ScreenPtr pScreenPriv = CFB8_32_GET_SCREEN_PRIVATE(pScreen);

    return pScreenPriv->key;
}
