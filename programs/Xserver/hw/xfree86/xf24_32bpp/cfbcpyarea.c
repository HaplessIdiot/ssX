/* $XFree86$ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"
#include "mibstore.h"


RegionPtr
cfb24_32CopyArea(
    DrawablePtr pSrcDraw,
    DrawablePtr pDstDraw,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
){

   if(pSrcDraw->bitsPerPixel == 32) {
	if(pDstDraw->bitsPerPixel == 32) {
	    return(cfb32CopyArea(pSrcDraw, pDstDraw, pGC, srcx, srcy,
		    			width, height, dstx, dsty));
	} else {
	    /* have to translate 32 -> 24 copies */
	    return cfb32BitBlt (pSrcDraw, pDstDraw,
            		pGC, srcx, srcy, width, height, dstx, dsty, 
			cfbDoBitblt32To24, 0L);
	}
   } else {
	if(pDstDraw->bitsPerPixel == 32) {
	    /* have to translate 24 -> 32 copies */
	    return cfb32BitBlt (pSrcDraw, pDstDraw,
            		pGC, srcx, srcy, width, height, dstx, dsty, 
			cfbDoBitblt24To32, 0L);
	} else {
	    return(cfb24CopyArea(pSrcDraw, pDstDraw, pGC, srcx, srcy,
		    width, height, dstx, dsty));
	}
   }
}



/* This is probably the slowest way to transfer data across the
   bus - a byte at a time.  This is temporary until somebody wants
   to optimize it */


void 
cfbDoBitblt24To32(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask,
    unsigned long bitPlane
){
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    unsigned char *ptr24, *ptr32;
    unsigned char *data24, *data32;
    int pitch24, pitch32;
    int height, width, i, j;
    unsigned char *pm;

    cfbGetByteWidthAndPointer(pSrc, pitch24, ptr24);
    cfbGetByteWidthAndPointer(pDst, pitch32, ptr32);

    planemask &= 0x00ffffff;
    pm = (unsigned char*)(&planemask);

    if((planemask == 0x00ffffff) && (rop == GXcopy)) {
	for(;nbox; pbox++, pptSrc++, nbox--) {
	    data24 = ptr24 + (pptSrc->y * pitch24) + (pptSrc->x * 3);
	    data32 = ptr32 + (pbox->y1 * pitch32) + (pbox->x1 << 2);
	    width = (pbox->x2 - pbox->x1) << 2;
	    height = pbox->y2 - pbox->y1;

	    while(height--) {
		for(i = j = 0; i < width; i += 4, j += 3) {
		    data32[i] = data24[j];
		    data32[i + 1] = data24[j + 1];
		    data32[i + 2] = data24[j + 2];
		}
		data24 += pitch24;
		data32 += pitch32;
	    }
	}
    } else {  /* it ain't pretty, but hey */
	register unsigned char tmp;
	for(;nbox; pbox++, pptSrc++, nbox--) {
	    data24 = ptr24 + (pptSrc->y * pitch24) + (pptSrc->x * 3);
	    data32 = ptr32 + (pbox->y1 * pitch32) + (pbox->x1 << 2);
	    width = (pbox->x2 - pbox->x1) << 2;
	    height = pbox->y2 - pbox->y1;

	    /* Some of these can probably be reduced more */

	    while(height--) {	
		switch(rop) {
		case GXcopy:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			data32[i] = (data24[j] & pm[0]) | (data32[i] & ~pm[0]);
			data32[i+1] = (data24[j+1] & pm[1]) | 
					(data32[i+1] & ~pm[1]);
			data32[i+2] = (data24[j+2] & pm[2]) | 
					(data32[i+2] & ~pm[2]);
		    }
		    break;
		case GXor:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			data32[i] |= data24[j] & pm[0];
			data32[i+1] |= data24[j+1] & pm[1];
			data32[i+2] |= data24[j+2] & pm[2];
		    }
		    break;
		case GXclear:
		    for(i = 0; i < width; i += 4) {
			data32[i] &= ~pm[0];
			data32[i+1] &= ~pm[1];
			data32[i+2] &= ~pm[2];
		    }
		    break;
		case GXand:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			data32[i] &= data24[j] | ~pm[0];
			data32[i+1] &= data24[j+1] | ~pm[1];
			data32[i+2] &= data24[j+2] | ~pm[2];
		    }
		    break;
		case GXandReverse:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = ((~tmp & data24[j]) & pm[0]) | 
							(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = ((~tmp & data24[j+1]) & pm[1]) | 
							(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = ((~tmp & data24[j+2]) & pm[2]) | 
							(tmp & ~pm[2]);
		    }
		    break;
		case GXandInverted:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = (tmp & ~data24[j] & pm[0]) | (tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = (tmp & ~data24[j+1] & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = (tmp & ~data24[j+2] & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXnoop:
		    return;
		case GXxor:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = ((tmp ^ data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = ((tmp ^ data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = ((tmp ^ data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXnor:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = (~(tmp | data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = (~(tmp | data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = (~(tmp | data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXequiv:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = ((tmp ^ ~data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = ((tmp ^ ~data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = ((tmp ^ ~data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXinvert:
		    for(i = 0; i < width; i += 4) {
			data32[i] ^= pm[0];
			data32[i+1] ^= pm[1];
			data32[i+2] ^= pm[2];
		    }
		    break;
		case GXorReverse:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = ((~tmp | data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = ((~tmp | data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = ((~tmp | data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXcopyInverted:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			data32[i] = (~data24[j] & pm[0]) | (data32[i] & ~pm[0]);
			data32[i+1] = (~data24[j+1] & pm[1]) | 
						(data32[i+1] & ~pm[1]);
			data32[i+2] = (~data24[j+2] & pm[2]) | 
						(data32[i+2] & ~pm[2]);
		    }
		    break;
		case GXorInverted:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = ((tmp | ~data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = ((tmp | ~data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = ((tmp | ~data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
			}
		    break;
		case GXnand:
		    for(i = j = 0; i < width; i += 4, j += 3) {
			tmp = data32[i];
			data32[i] = (~(tmp & data24[j]) & pm[0]) | 
								(tmp & ~pm[0]);
			tmp = data32[i+1];
			data32[i+1] = (~(tmp & data24[j+1]) & pm[1]) | 
								(tmp & ~pm[1]);
			tmp = data32[i+2];
			data32[i+2] = (~(tmp & data24[j+2]) & pm[2]) | 
								(tmp & ~pm[2]);
		    }
		    break;
		case GXset:
		    for(i = 0; i < width; i+=4) {
			data32[i] |= pm[0];
			data32[i+1] |= pm[1];
			data32[i+2] |= pm[2];
		    }
		    break;
		}
	        data24 += pitch24;
	        data32 += pitch32;
	    }
	}
    }
}


void 
cfbDoBitblt32To24(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask,
    unsigned long bitPlane
){
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    unsigned char *ptr24, *ptr32;
    unsigned char *data24, *data32;
    int pitch24, pitch32;
    int height, width, i, j;
    unsigned char *pm;

    cfbGetByteWidthAndPointer(pDst, pitch24, ptr24);
    cfbGetByteWidthAndPointer(pSrc, pitch32, ptr32);

    planemask &= 0x00ffffff;
    pm = (unsigned char*)(&planemask);

    if((planemask == 0x00ffffff) && (rop == GXcopy)) {
	CARD32 *src;
	CARD8 *dst;
	long phase;
	for(;nbox; pbox++, pptSrc++, nbox--) {
	    data24 = ptr24 + (pbox->y1 * pitch24) + (pbox->x1 * 3);
	    data32 = ptr32 + (pptSrc->y * pitch32) + (pptSrc->x << 2);
	    width = pbox->x2 - pbox->x1;
	    height = pbox->y2 - pbox->y1;
	    
	    phase = (long)data32 & 3L;

	    while(height--) {
		src = (CARD32*)data32;
		dst = data24;
		j = width;

#if 1
	/* Why doesn't this help performance on my machine ? */
		switch(phase) {
		case 0: break;
		case 1:
		    dst[0] = src[0];
		    *((CARD16*)(dst + 1)) = src[0] >> 8;
		    dst += 3;
		    src = (CARD32*)(data32 + 3);
		    j--;
		    break;
		case 2:
		    if(j == 1) break;
		    *((CARD16*)dst) = src[0];
		    *((CARD32*)dst) = ((src[0] >> 16) & 0x000000ff) | 
							(src[1] << 8);
		    dst += 6;
		    src = (CARD32*)(data32 + 6);
		    j -= 2;
		    break;
		default:
		    if(j < 3) break; 
		    dst[0] = src[0];
		    *((CARD32*)(dst + 1)) = ((src[0] >> 8) & 0x0000ffff) | 
							(src[1] << 16);
		    *((CARD32*)(dst + 5)) = ((src[1] >> 16) & 0x000000ff) | 
							(src[2] << 8);
		    dst += 9;
		    src = (CARD32*)(data32 + 9);
		    j -= 3;
		}
#endif

		while(j >= 4) {
		    *((CARD32*)dst) = (src[0] & 0x00ffffff) | (src[1] << 24);
		    *((CARD32*)(dst + 4)) = ((src[1] >> 8) & 0x0000ffff)| 
							(src[2] << 16);
		    *((CARD32*)(dst + 8)) = ((src[2] >> 16) & 0x000000ff) |
		 					(src[3] << 8);
		    dst += 12;
		    src += 4;
		    j -= 4;
		}
		switch(j) {
		case 0:	
		    break;
		case 1:	
		    *((CARD16*)dst) = src[0];
		    dst[2] = src[0] >> 16;
		    break;
		case 2:	
		    *((CARD32*)dst) = (src[0] & 0x00ffffff) | (src[1] << 24);
		    *((CARD16*)(dst + 4)) = src[1] >> 8;
		    break;
		default: 
		    *((CARD32*)dst) = (src[0] & 0x00ffffff) | (src[1] << 24);
		    *((CARD32*)(dst + 4)) = ((src[1] >> 8) & 0x0000ffff) | 
							(src[2] << 16);
		    dst[8] = src[2] >> 16;
		    break;
		}

		data24 += pitch24;
		data32 += pitch32;
	    }
	}
    } else {
	for(;nbox; pbox++, pptSrc++, nbox--) {
	    data24 = ptr24 + (pbox->y1 * pitch24) + (pbox->x1 * 3);
	    data32 = ptr32 + (pptSrc->y * pitch32) + (pptSrc->x << 2);
	    width = pbox->x2 - pbox->x1;
	    height = pbox->y2 - pbox->y1;

	    while(height--) {	
		switch(rop) {
		case GXcopy:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = (data32[i] & pm[0]) | (data24[j] & ~pm[0]);
			data24[j+1] = (data32[i+1] & pm[1]) | 
						(data24[j+1] & ~pm[1]);
			data24[j+2] = (data32[i+2] & pm[2]) | 
						(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXor:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] |= data32[i] & pm[0];
			data24[j+1] |= data32[i+1] & pm[1];
			data24[j+2] |= data32[i+2] & pm[2];
		    }
		    break;
		case GXclear:
		    for(j = 0; j < width; j += 3) {
			data24[j] &= ~pm[0];
			data24[j+1] &= ~pm[1];
			data24[j+2] &= ~pm[2];
		    }
		    break;
		case GXand:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] &= data32[i] | ~pm[0];
			data24[j+1] &= data32[i+1] | ~pm[1];
			data24[j+2] &= data32[i+2] | ~pm[2];
		    }
		    break;
		case GXandReverse:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = ((data32[i] & ~data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = ((data32[i+1] & ~data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = ((data32[i+2] & ~data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXandInverted:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[i] = (~data32[i] & data24[j] & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[i+1] = (~data32[i+1] & data24[j+1] & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[i+2] = (~data32[i+2] & data24[j+2] & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXnoop:
		    return;
		case GXxor:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = ((data32[i] ^ data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = ((data32[i+1] ^ data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = ((data32[i+2] ^ data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXnor:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = (~(data32[i] | data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = (~(data32[i+1] | data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = (~(data32[i+2] | data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXequiv:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = ((~data32[i] ^ data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = ((~data32[i+1] ^ data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = ((~data32[i+2] ^ data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXinvert:
		    for(j = 0; j < width; j+=3) {
			data24[j] ^= pm[0];
			data24[j+1] ^= pm[1];
			data24[j+2] ^= pm[2];
		    }
		    break;
		case GXorReverse:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = ((data32[i] | ~data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = ((data32[i+1] | ~data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = ((data32[i+2] | ~data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXcopyInverted:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = (~data32[i] & pm[0]) | (data24[j] & ~pm[0]);
			data24[j+1] = (~data32[i+1] & pm[1]) | 
						(data24[j+1] & ~pm[1]);
			data24[j+2] = (~data32[i+2] & pm[2]) | 
						(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXorInverted:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = ((~data32[i] | data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = ((~data32[i+1] | data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = ((~data32[i+2] | data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXnand:
		    for(i = j = 0; j < width; i += 4, j += 3) {
			data24[j] = (~(data32[i] & data24[j]) & pm[0]) | 
							(data24[j] & ~pm[0]);
			data24[j+1] = (~(data32[i+1] & data24[j+1]) & pm[1]) | 
							(data24[j+1] & ~pm[1]);
			data24[j+2] = (~(data32[i+2] & data24[j+2]) & pm[2]) | 
							(data24[j+2] & ~pm[2]);
		    }
		    break;
		case GXset:
		    for(j = 0; j < width; j+=3) {
			data24[j] |= pm[0];
			data24[j+1] |= pm[1];
			data24[j+2] |= pm[2];
		    }
		    break;
		}
	        data24 += pitch24;
	        data32 += pitch32;
	    }
	}
    }
}
