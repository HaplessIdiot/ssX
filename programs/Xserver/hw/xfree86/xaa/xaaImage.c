/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaImage.c,v 1.1.2.2 1998/07/19 13:22:11 dawes Exp $ */

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

static void MoveDWORDS_FixedBase(
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

static void MoveDWORDS(
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

    switch(Bpp) {
    case 1:	dwords = (w + 3) >> 2;
		break;
    case 2:	dwords = (w + 1) >> 1;
		break;
    case 3:	dwords = ((w + 1) * 3) >> 2;
		break;
    case 4:	dwords = w;
		break;
    default: return; 
    }

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
	    MoveDWORDS_FixedBase((CARD32*)infoRec->ImageWriteBase,
		(CARD32*)src, dwords);
	    src += srcwidth;
	}
	if(beCareful) {
	   int shift = ((long)src & 0x03L) << 3;
	   if(--dwords)
		MoveDWORDS_FixedBase((CARD32*)infoRec->ImageWriteBase,
			(CARD32*)src, dwords);
	   src = (unsigned char*)((long)(src + (dwords << 2)) & ~0x03L);
	   *((CARD32*)infoRec->ImageWriteBase) = *((CARD32*)src) >> shift;
	}
    } else {
	if(srcwidth == (dwords << 2)) {
	   int decrement = infoRec->ImageWriteRange/dwords;

	   while(h > decrement) {
		MoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords * decrement);
		src += (srcwidth * decrement);
		h -= decrement;
	   }
	   if(h) {
		MoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords * h);
		if(beCareful) src += (srcwidth * h);
	   }
	} else {
	    while(h--) {
		MoveDWORDS((CARD32*)infoRec->ImageWriteBase,
	 		(CARD32*)src, dwords);
		src += srcwidth;
	    }
	}

	if(beCareful) {
	    int shift = ((long)src & 0x03L) << 3;
	    if(--dwords)
		MoveDWORDS((CARD32*)infoRec->ImageWriteBase,
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

    switch(Bpp) {
    case 1:	dwords = (w + 3) >> 2;
		break;
    case 2:	dwords = (w + 1) >> 1;
		break;
    case 3:	dwords = ((w + 1) * 3) >> 2;
		break;
    case 4:	dwords = w;
		break;
    default: return; 
    }

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
	MoveDWORDS(base, (CARD32*)src, dwords);
	(*infoRec->SubsequentImageWriteScanline)(pScrn, bufferNo++);
	src += srcwidth;
	if(bufferNo >= infoRec->NumScanlineImageWriteBuffers)
	    bufferNo = 0;
	base = (CARD32*)infoRec->ScanlineImageWriteBuffers[bufferNo];
    }

    if(beCareful) {
	int shift = ((long)src & 0x03L) << 3;
	if(--dwords)
	    MoveDWORDS(base,(CARD32*)src, dwords);
	src = (unsigned char*)((long)(src + (dwords << 2)) & ~0x03L);
     
	base[dwords] = *((CARD32*)src) >> shift;
    }

    SET_SYNC_FLAG(infoRec);
}
