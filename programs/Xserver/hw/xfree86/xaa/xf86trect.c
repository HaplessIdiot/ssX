
#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86local.h"
#include "xf86pcache.h"
#include "xf86Priv.h"

static void
FillTiledColumn(src, srcwidth, dwords, h, srcy, pheight)
    CARD32 *src;
    int srcwidth, dwords, h, srcy;
{
    CARD32 *srcpntr = src + (srcy * srcwidth);
    register CARD32 *srcp;
    register CARD32 *dest;
    register int count;
	
    while(h--) {
	dest = (CARD32*)xf86AccelInfoRec.ImageWriteBase;
	srcp = srcpntr;
	count = dwords;
     	while(count >= 4) {
	    dest[0] = srcp[0];
	    dest[1] = srcp[1];
	    dest[2] = srcp[2];
	    dest[3] = srcp[3];
	    srcp += 4;
	    dest += 4;
	    count -= 4;
     	}		
     	switch(count) {
	   case 0:	break;
	   case 1: 	dest[0] = srcp[0];
			break;
	   case 2: 	dest[0] = srcp[0];
			dest[1] = srcp[1];
			break;
	   case 3: 	dest[0] = srcp[0];
			dest[1] = srcp[1];
			dest[2] = srcp[2];
			break;
    	}
	
	srcy++;
	srcpntr += srcwidth;
	if (srcy >= pheight) {
	    	srcy = 0;
	    	srcpntr = src;
	}
    }
}

static void
FillTiledColumn_FixedBase(src, srcwidth, dwords, h, srcy, pheight)
    CARD32 *src;
    int srcwidth, dwords, h, srcy;
{
    CARD32 *srcpntr = src + (srcy * srcwidth);
    register CARD32 *srcp;
    register CARD32 *dest = (CARD32*)xf86AccelInfoRec.ImageWriteBase;
    register int count;
	
    while(h--) {
	srcp = srcpntr;
	count = dwords;
     	while(count >= 4) {
	    *dest = srcp[0];
	    *dest = srcp[1];
	    *dest = srcp[2];
	    *dest = srcp[3];
	    srcp += 4;
	    count -= 4;
     	}		
     	switch(count) {
	   case 0:	break;
	   case 1: 	*dest = srcp[0];
			break;
	   case 2: 	*dest = srcp[0];
			*dest = srcp[1];
			break;
	   case 3: 	*dest = srcp[0];
			*dest = srcp[1];
			*dest = srcp[2];
			break;
    	}
	
	srcy++;
	srcpntr += srcwidth;
	if (srcy >= pheight) {
	    	srcy = 0;
	    	srcpntr = src;
	}
    }
}


static void
FillTiledRect(x, y, w, h, src, srcwidth, pwidth, pheight, srcx, srcy)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int pwidth, pheight;
    int srcx, srcy;
{
    int Bpp = xf86bpp >> 3;
    int width, MaxDWORDS, dwords;
    void (*FillColumn)();

    MaxDWORDS = ((pwidth * Bpp) + 3) >> 2;
    if(MaxDWORDS <= xf86AccelInfoRec.ImageWriteRange)
	FillColumn = FillTiledColumn;
    else
	FillColumn = FillTiledColumn_FixedBase;

    srcwidth >>= 2;	/* pixmaps must be DWORD padded */

    if(srcx) {
	unsigned char* srcPntr = src + (srcx * Bpp);
	int skipleft = 0;

	if((skipleft = (int)srcPntr & 0x03)) {
	    if(!(xf86AccelInfoRec.ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }
	    if(Bpp == 3)
		skipleft = 4 - skipleft;
	    else
	    	skipleft /= Bpp;

	    if((x < skipleft) && !(xf86AccelInfoRec.ImageWriteFlags &
				 LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
		skipleft = 0;
		goto BAD_ALIGNMENT;
	    }
	
	    if(Bpp == 3)
	    	srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    else   
	    	srcPntr = (unsigned char*)((int)srcPntr & ~0x03);    

	}
BAD_ALIGNMENT:

	width = pwidth - srcx;
	if(w <= width) {
	    width = w;
	    w = 0;
	} else w -= width;

	xf86AccelInfoRec.SubsequentImageWrite(x - skipleft, y, 
		width + skipleft, h, skipleft);

	(*FillColumn)((CARD32*)srcPntr, srcwidth, 
				(((width + skipleft) * Bpp) + 3) >> 2, 
				h, srcy, pheight);
	x += width;
    }

    while(w) {
	if(w >= pwidth) {
	    width = pwidth;
	    w -= pwidth;
	    dwords = MaxDWORDS;
	} else {
	    width = w;
	    w = 0;
    	    dwords = ((width * Bpp) + 3) >> 2;
	}

	xf86AccelInfoRec.SubsequentImageWrite(x, y, width, h, 0);

	(*FillColumn)((CARD32*)src, srcwidth, dwords, h, srcy, pheight);

	x += width;
    }
}

#ifndef GNUC
#define __inline__ /**/
#endif

static __inline__ void SetDWORDS(dest, value, dwords)
   register CARD32* dest;
   register CARD32 value;
   register int dwords;
{
    while(dwords & ~0x03) {
	dest[0] = value;
	dest[1] = value;
	dest[2] = value;
	dest[3] = value;
	dest += 4;
	dwords -= 4;
    }	
    switch(dwords) {
	case 1: dest[0] = value; 
		break;
	case 2: dest[0] = value;
		dest[1] = value; 
		break;
	case 3: dest[0] = value;
		dest[1] = value;
		dest[2] = value; 
		break;
    }
}


/* this one's for 1, 2 and 4 byte tiles */

static void
FillTiledRect_Fast(x, y, w, h, src, srcwidth, pwidth, pheight, srcx, srcy)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int pwidth, pheight;
    int srcx, srcy;
{
    register CARD32 pattern;
    unsigned char* srcp = (srcwidth * srcy) + src;
    int Bpp = xf86bpp >> 3;
    int dwords;

    srcx *= xf86bpp;
    dwords = ((w * Bpp) + 3) >> 2;
    pwidth *= Bpp;
	
    xf86AccelInfoRec.SubsequentImageWrite(x, y, w, h, 0);

    while(h--) {
	switch(pwidth) {
	    case 4: 	pattern = *((CARD32*)srcp);
			break;
	    case 2:	pattern = *((CARD32*)srcp) & 0xFFFF;
			pattern |= pattern << 16;
			break;
	    default:	pattern = *((CARD32*)srcp) & 0xFF;
			pattern |= pattern << 8;
			pattern |= pattern << 16;
			break;
	}

	if(srcx) 
	    pattern = (pattern >> srcx) | (pattern << (32 - srcx)); 
          
        if(dwords <= xf86AccelInfoRec.ImageWriteRange)
  	    SetDWORDS((CARD32*)xf86AccelInfoRec.ImageWriteBase, 
			pattern, dwords); 
	else {
	    register int count = dwords;
	    while(count--)
		*((CARD32*)xf86AccelInfoRec.ImageWriteBase) = pattern;
	}

	srcy++;
	srcp += srcwidth;
	if (srcy >= pheight) {
	     srcy = 0;
	     srcp = src;
	}
    }
}
 

void
xf86FillRectTiledImageWrite(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nBoxInit;	/* number of rectangles to fill */
    BoxPtr	pBoxInit;  	/* Pointer to first rectangle to fill */
{
    BoxPtr pBox;
    int nBox;
    unsigned char* srcPntr;  
    PixmapPtr pPixmap;	
    int w, h, xoffset, yoffset;
    void (*TileRectFunc)() = FillTiledRect;

    if(	!CHECKPLANEMASK(xf86AccelInfoRec.ImageWriteFlags) ||
	!CHECKROP(xf86AccelInfoRec.ImageWriteFlags) ||
	!CHECKRGBEQUAL(xf86AccelInfoRec.ImageWriteFlags) ||
	((xf86AccelInfoRec.ImageWriteFlags & NO_GXCOPY) && 
	       	 (pGC->alu == GXcopy))) {
	(*xf86AccelInfoRec.FillRectTiledFallBack)(pDrawable, pGC, nBoxInit, 
		pBoxInit);
	return;
    }

    pPixmap = pGC->tile.pixmap;

    switch(pPixmap->drawable.width * (xf86bpp >> 3)) {
	case 4:
	case 2:
	case 1: 	TileRectFunc = FillTiledRect_Fast;
	default: 	break;
    }

    xf86AccelInfoRec.SetupForImageWrite(pGC->alu, pGC->planemask, -1);

    for (nBox = nBoxInit, pBox = pBoxInit; nBox > 0; nBox--, pBox++) {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;

	if ((w > 0) && (h > 0)) {
	    xoffset = (pBox->x1 - (pGC->patOrg.x + pDrawable->x))
	        % pPixmap->drawable.width;
	    if (xoffset < 0)
	        xoffset += pPixmap->drawable.width;
	    yoffset = (pBox->y1 - (pGC->patOrg.y + pDrawable->y))
	        % pPixmap->drawable.height;
	    if (yoffset < 0)
	        yoffset += pPixmap->drawable.height;

	    (*TileRectFunc)(pBox->x1, pBox->y1, w, h,
	        pPixmap->devPrivate.ptr, pPixmap->devKind,
	        pPixmap->drawable.width, pPixmap->drawable.height,
	        xoffset, yoffset);
	}
    }	/* end for loop through each rectangle to draw */
    
    if(!(xf86AccelInfoRec.Flags & NO_SYNC_AFTER_CPU_COLOR_EXPAND))
    	xf86AccelInfoRec.Sync();
}







