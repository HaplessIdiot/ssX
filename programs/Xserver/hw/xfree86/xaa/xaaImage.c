/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaImage.c,v 1.6 1998/10/05 13:23:18 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "mi.h"
#include "pixmapstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"

void XAAMoveDWORDS_FixedBase(
   register CARD32* dest,
   register CARD32* src,
   register int dwords )
{
     while(dwords & ~0x03) {
	 *dest = *src;
	 *dest = *(src + 1);
	 *dest = *(src + 2);
	 *dest = *(src + 3);	
	 dwords -= 4;
	 src += 4;
     }

     if(!dwords) return;
     *dest = *src;
     if(dwords == 1) return;
     *dest = *(src + 1);
     if(dwords == 2) return;
     *dest = *(src + 2);
}

void XAAMoveDWORDS(
   register CARD32* dest,
   register CARD32* src,
   register int dwords )
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
	*(dest + 3) = *(src + 3);
	src += 4;
	dest += 4;
	dwords -= 4;
     }	
     if(!dwords) return;
     *dest = *src;
     if(dwords == 1) return;
     *(dest + 1) = *(src + 1);
     if(dwords == 2) return;
     *(dest + 2) = *(src + 2);
}

void XAAMoveDWORDS_FixedSrc(
   register CARD32* dest,
   register CARD32* src,
   register int dwords )
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *src;
	*(dest + 2) = *src;
	*(dest + 3) = *src;
	dest += 4;
	dwords -= 4;
     }	
     if(!dwords) return;
     *dest = *src;
     if(dwords == 1) return;
     *(dest + 1) = *src;
     if(dwords == 2) return;
     *(dest + 2) = *src;
}

void 
XAAWritePixmap (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,	
   int srcwidth,	/* bytes */
   int rop,
   unsigned int planemask,
   int trans,
   int bpp, int depth
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int dwords, skipleft, Bpp = bpp >> 3; 
    Bool beCareful = FALSE;
    Bool PlusOne = FALSE;

    if((skipleft = (long)src & 0x03L)) {
	if(!(infoRec->ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
	   skipleft = 0;
	   beCareful = TRUE;
	   goto BAD_ALIGNMENT;
	}

	if(Bpp == 3)
	   skipleft = 4 - skipleft;
	else
	   skipleft /= Bpp;

	if((x < skipleft) && !(infoRec->ImageWriteFlags &
				 LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
	   skipleft = 0;
	   beCareful = TRUE;
	   goto BAD_ALIGNMENT;
	}

	x -= skipleft;	     
	w += skipleft;
	
	if(Bpp == 3)
	   src -= 3 * skipleft;  
	else   /* is this Alpha friendly ? */
	   src = (unsigned char*)((long)src & ~0x03L);     
    }

BAD_ALIGNMENT:

    dwords = ((w * Bpp) + 3) >> 2;

    if((infoRec->ImageWriteFlags & CPU_TRANSFER_PAD_QWORD) && 
						((dwords * h) & 0x01)) {
	PlusOne = TRUE;
    } 
		
	
    (*infoRec->SetupForImageWrite)(pScrn, rop, planemask, trans, bpp, depth);
    (*infoRec->SubsequentImageWriteRect)(pScrn, x, y, w, h, skipleft);

    if(beCareful) {
	/* in cases with bad alignment we have to be careful not
	   to read beyond the end of the source */
	if(((x * Bpp) + (dwords << 2)) > srcwidth) h--;
	else beCareful = FALSE;
    }

    if(dwords > infoRec->ImageWriteRange) {
	while(h--) {
	    XAAMoveDWORDS_FixedBase((CARD32*)infoRec->ImageWriteBase,
		(CARD32*)src, dwords);
	    src += srcwidth;
	}
	if(beCareful) {
	   int shift = ((long)src & 0x03L) << 3;
	   if(--dwords)
		XAAMoveDWORDS_FixedBase((CARD32*)infoRec->ImageWriteBase,
			(CARD32*)src, dwords);
	   src = (unsigned char*)((long)(src + (dwords << 2)) & ~0x03L);
	   *((CARD32*)infoRec->ImageWriteBase) = *((CARD32*)src) >> shift;
	}
    } else {
	if(srcwidth == (dwords << 2)) {
	   int decrement = infoRec->ImageWriteRange/dwords;

	   while(h > decrement) {
		XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords * decrement);
		src += (srcwidth * decrement);
		h -= decrement;
	   }
	   if(h) {
		XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords * h);
		if(beCareful) src += (srcwidth * h);
	   }
	} else {
	    while(h--) {
		XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords);
		src += srcwidth;
	    }
	}

	if(beCareful) {
	    int shift = ((long)src & 0x03L) << 3;
	    if(--dwords)
		XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
					(CARD32*)src, dwords);
	    src = (unsigned char*)((long)(src + (dwords << 2)) & ~0x03L);
     
	    ((CARD32*)infoRec->ImageWriteBase)[dwords] = 
			*((CARD32*)src) >> shift;
	}
    }

    if(PlusOne) {
	CARD32* base = (CARD32*)infoRec->ImageWriteBase;
	*base = 0x00000000;
    }

    if(infoRec->ImageWriteFlags & SYNC_AFTER_IMAGE_WRITE)
	(*infoRec->Sync)(pScrn);
    else SET_SYNC_FLAG(infoRec);
}


void 
XAAWritePixmapScanline (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,	
   int srcwidth,	/* bytes */
   int rop,
   unsigned int planemask,
   int trans,
   int bpp, int depth
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int dwords, skipleft, bufferNo = 0, Bpp = bpp >> 3; 
    Bool beCareful = FALSE;
    CARD32* base = (CARD32*)infoRec->ScanlineImageWriteBuffers[0];

    if((skipleft = (long)src & 0x03L)) {
	if(!(infoRec->ImageWriteFlags & LEFT_EDGE_CLIPPING)) {
	   skipleft = 0;
	   beCareful = TRUE;
	   goto BAD_ALIGNMENT;
	}

	if(Bpp == 3)
	   skipleft = 4 - skipleft;
	else
	   skipleft /= Bpp;

	if((x < skipleft) && !(infoRec->ImageWriteFlags &
				 LEFT_EDGE_CLIPPING_NEGATIVE_X)) {
	   skipleft = 0;
	   beCareful = TRUE;
	   goto BAD_ALIGNMENT;
	}

	x -= skipleft;	     
	w += skipleft;
	
	if(Bpp == 3)
	   src -= 3 * skipleft;  
	else
	   src = (unsigned char*)((long)src & ~0x03L);     
    }

BAD_ALIGNMENT:

    dwords = ((w * Bpp) + 3) >> 2;

    (*infoRec->SetupForScanlineImageWrite)(
				pScrn, rop, planemask, trans, bpp, depth);
    (*infoRec->SubsequentScanlineImageWriteRect)(pScrn, x, y, w, h, skipleft);

    if(beCareful) {
	/* in cases with bad alignment we have to be careful not
	   to read beyond the end of the source */
	if(((x * Bpp) + (dwords << 2)) > srcwidth) h--;
	else beCareful = FALSE;
    }

    while(h--) {
	XAAMoveDWORDS(base, (CARD32*)src, dwords);
	(*infoRec->SubsequentImageWriteScanline)(pScrn, bufferNo++);
	src += srcwidth;
	if(bufferNo >= infoRec->NumScanlineImageWriteBuffers)
	    bufferNo = 0;
	base = (CARD32*)infoRec->ScanlineImageWriteBuffers[bufferNo];
    }

    if(beCareful) {
	int shift = ((long)src & 0x03L) << 3;
	if(--dwords)
	    XAAMoveDWORDS(base,(CARD32*)src, dwords);
	src = (unsigned char*)((long)(src + (dwords << 2)) & ~0x03L);
     
	base[dwords] = *((CARD32*)src) >> shift;
	(*infoRec->SubsequentImageWriteScanline)(pScrn, bufferNo);
    }

    SET_SYNC_FLAG(infoRec);
}


void
XAAPutImage(
    DrawablePtr pDraw,
    GCPtr       pGC,
    int         depth, 
    int 	x, 
    int		y, 
    int		w, 
    int		h,
    int         leftPad,
    int         format,
    char        *pImage
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    if(!w || !h) return;

    if(((format == ZPixmap) && infoRec->WritePixmap &&
	     CHECK_ROP(pGC,infoRec->WritePixmapFlags) &&
	     CHECK_ROPSRC(pGC,infoRec->WritePixmapFlags) &&
	     CHECK_PLANEMASK(pGC,infoRec->WritePixmapFlags) &&
	     CHECK_NO_GXCOPY(pGC,infoRec->WritePixmapFlags)) ||
       ((format == XYBitmap) && infoRec->WriteBitmap &&
	     CHECK_ROP(pGC,infoRec->WriteBitmapFlags) &&
	     CHECK_ROPSRC(pGC,infoRec->WriteBitmapFlags) &&
	     CHECK_PLANEMASK(pGC,infoRec->WriteBitmapFlags) &&
	     CHECK_COLORS(pGC,infoRec->WriteBitmapFlags) &&
	     !(infoRec->WriteBitmapFlags & TRANSPARENCY_ONLY)) ||
       ((format == XYPixmap) && infoRec->WriteBitmap &&
	     CHECK_ROP(pGC,infoRec->WriteBitmapFlags) &&
	     CHECK_ROPSRC(pGC,infoRec->WriteBitmapFlags) &&
	     !(infoRec->WriteBitmapFlags & NO_PLANEMASK) &&
	     !(infoRec->WriteBitmapFlags & TRANSPARENCY_ONLY))){

	int MaxBoxes = REGION_NUM_RECTS(pGC->pCompositeClip);
	BoxPtr pbox, pClipBoxes;
	int nboxes, srcx, srcy, srcwidth;
	xRectangle TheRect;

	TheRect.x = pDraw->x + x;
	TheRect.y = pDraw->y + y;
	TheRect.width = w;
	TheRect.height = h; 

	if(MaxBoxes > infoRec->NumPreAllocBoxes) {
	   pClipBoxes = (BoxPtr)ALLOCATE_LOCAL(MaxBoxes * sizeof(BoxRec));
	   if(!pClipBoxes) return;	
	} else pClipBoxes = infoRec->PreAllocBoxes;

	nboxes = 
	  XAAGetRectClipBoxes(pGC->pCompositeClip, pClipBoxes, 1, &TheRect);
	pbox = pClipBoxes;

	if(format == XYBitmap) {
	    srcwidth = ((leftPad + w + 31) >> 5) << 2;
	    while(nboxes--) {
		srcx = pbox->x1 - TheRect.x + leftPad;
		srcy = pbox->y1 - TheRect.y;
		(*infoRec->WriteBitmap)(infoRec->pScrn, pbox->x1, pbox->y1, 
			pbox->x2 - pbox->x1, pbox->y2 - pbox->y1, 
			(unsigned char*)pImage + 
				(srcwidth * srcy) + ((srcx >> 5) << 2), 
			srcwidth, srcx & 31, pGC->fgPixel, pGC->bgPixel,
	 		pGC->alu, pGC->planemask);
		pbox++;
	    }
        } else if(format == ZPixmap) {
	    /* here we assume XImages match the framebuffer.  We will have
	       to lookup Bpp by depth when this changes */
	    int Bpp = infoRec->pScrn->bitsPerPixel >> 3;
	    srcwidth = (((leftPad + w) * Bpp) + 3) & ~0x00000003;
	    while(nboxes--) {
		srcx = pbox->x1 - TheRect.x + leftPad;
		srcy = pbox->y1 - TheRect.y;
		(*infoRec->WritePixmap)(infoRec->pScrn, pbox->x1, pbox->y1, 
			pbox->x2 - pbox->x1, pbox->y2 - pbox->y1, 
			(unsigned char*)pImage + 
				(srcwidth * srcy) + (srcx * Bpp), 
			srcwidth, pGC->alu, pGC->planemask, -1, 
			Bpp << 3, depth);
		pbox++;
	    }
	} else { /* XYPixmap */
	    int depth = pGC->depth;
	    int i, numBox, increment;
	    BoxPtr pntBox;
	    
	    srcwidth = ((leftPad + w + 31) >> 5) << 2;
	    increment = h * srcwidth;
	    for(i = 1 << (depth - 1); i ; i >>= 1, pImage += increment) {
		if(i & pGC->planemask) {
		    pntBox = pbox;
		    numBox = nboxes;
		    while(numBox--) {
			srcx = pntBox->x1 - TheRect.x + leftPad;
			srcy = pntBox->y1 - TheRect.y;
			(*infoRec->WriteBitmap)(infoRec->pScrn, 
				pntBox->x1, pntBox->y1, 
				pntBox->x2 - pntBox->x1, 
				pntBox->y2 - pntBox->y1, 
				(unsigned char*)pImage + 
				(srcwidth * srcy) + ((srcx >> 5) << 2), 
				srcwidth, srcx & 31, ~0, 0, pGC->alu, i);
			pntBox++;
	    	    }
		}
	    }

	}

	if(pClipBoxes != infoRec->PreAllocBoxes)
	   DEALLOCATE_LOCAL(pClipBoxes);
    } else 
	XAAFallbackOps.PutImage(pDraw, pGC, depth, x, y, w, h, leftPad, 
				format, pImage);
}


void 
XAAReadPixmap (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *dst,	
   int dstwidth,	/* bytes */
   int bpp, int depth
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int ReadDwords, WriteBytes, Surplus, Bpp = bpp >> 3; 
    unsigned char *tmp;
    Bool PlusOne = FALSE;
    union {
	CARD32 IntData;
	unsigned char CharData[4];
    } extra;

    WriteBytes = w * Bpp;
    ReadDwords = (WriteBytes + 3) >> 2;

    if((infoRec->ImageReadFlags & CPU_TRANSFER_PAD_QWORD) && 
				((ReadDwords * h) & 0x01)) {
	PlusOne = TRUE;
    } 


    (*infoRec->SetupForImageRead)(pScrn, bpp, depth);
    (*infoRec->SubsequentImageReadRect)(pScrn, x, y, w, h);

    if(ReadDwords > infoRec->ImageWriteRange) {
	if((Surplus = (ReadDwords << 2) - WriteBytes)) {
	    ReadDwords--;
	    while(h--) {
		if(ReadDwords)
		    XAAMoveDWORDS_FixedSrc((CARD32*)dst, 
			(CARD32*)infoRec->ImageReadBase, ReadDwords);
		extra.IntData = *((CARD32*)infoRec->ImageReadBase);
		tmp = dst + (ReadDwords << 2); 
		switch(Surplus) {
		case 3: tmp[2] = extra.CharData[2];
		case 2: tmp[1] = extra.CharData[1];
		case 1: tmp[0] = extra.CharData[0];
		}
		dst += dstwidth;
	    }
	} else {
	    while(h--) {
		XAAMoveDWORDS_FixedSrc((CARD32*)dst, 
			(CARD32*)infoRec->ImageReadBase, ReadDwords);
		dst += dstwidth;
	    }
	}
    } else {
	if((Surplus = (ReadDwords << 2) - WriteBytes)) {
	    ReadDwords--;
	    while(h--) {
		if(ReadDwords)
		   XAAMoveDWORDS((CARD32*)dst, 
			(CARD32*)infoRec->ImageReadBase, ReadDwords);
		extra.IntData = 
			*((CARD32*)(infoRec->ImageReadBase) + ReadDwords);
		tmp = dst + (ReadDwords << 2); 
		switch(Surplus) {
		case 3: tmp[2] = extra.CharData[2];
		case 2: tmp[1] = extra.CharData[1];
		case 1: tmp[0] = extra.CharData[0];
		}
		dst += dstwidth;
	    }
	} else {
	    while(h--) {
		XAAMoveDWORDS((CARD32*)dst, 
			(CARD32*)infoRec->ImageReadBase, ReadDwords);
		dst += dstwidth;
	    }
	}
    }

    if(PlusOne) 
	extra.IntData = *((CARD32*)infoRec->ImageReadBase);

    SET_SYNC_FLAG(infoRec);
}
