#define PSZ 8

#include "vga256.h"
#include "xf86.h"
#include "xf86xaa.h"

#define MAX_BLIT_PIXELS 	 0x40000


static __inline__ CARD32* MoveDWORDS(dest, src, dwords)
   register CARD32* dest;
   register CARD32* src;
   register int dwords;
{
     while(dwords & ~0x03) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 4;
        dest += 4;
        dwords -= 4;
     }  
     switch(dwords) {
        case 0: return(dest);
        case 1: dest[0] = src[0];
                return(dest + 1);
        case 2: dest[0] = src[0];
                dest[1] = src[1];
                return(dest + 2);
        case 3: dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                return(dest + 3);
    }
    return dest;
}


void MGAWriteBitmap(x, y, w, h, src, srcwidth, srcx, srcy, 
			bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    register unsigned char *srcp;
    int dwords, skipleft, maxlines;
    register CARD32 *destptr = 
			(CARD32*)xf86AccelInfoRec.CPUToScreenColorExpandBase;
    CARD32 *maxptr;

    xf86AccelInfoRec.SetupForCPUToScreenColorExpand(bg, fg, rop, planemask);
    
    srcp = (srcwidth * srcy) + (srcx >> 3) + src; 
    srcx &= 0x07;
    if(skipleft = (int)srcp & 0x03) {
        skipleft = (skipleft << 3) + srcx;
        maxlines = MAX_BLIT_PIXELS / (w + skipleft);
        if(maxlines < h) {
            int newmax = MAX_BLIT_PIXELS / (w + srcx);
            if(newmax >= h) { /* do byte alignment to avoid the split */
                skipleft = srcx;
                maxlines = newmax;
            } else srcp = (unsigned char*)((int)srcp & ~0x03);
        } else srcp = (unsigned char*)((int)srcp & ~0x03);
    } else {
        skipleft = srcx; 
        maxlines = MAX_BLIT_PIXELS / (w + skipleft);
    }
    
    w += skipleft; 
    x -= skipleft;
        
    dwords = (w + 31) >> 5;
    maxptr = destptr + (xf86AccelInfoRec.CPUToScreenColorExpandRange >> 2) 
    		- dwords;
    
    while(h > maxlines) {
	int numlines = maxlines;
       	xf86AccelInfoRec.SubsequentCPUToScreenColorExpand(
		x, y, w, maxlines, skipleft);  
    	while(numlines--) {
            destptr = MoveDWORDS(destptr, (CARD32*)srcp, dwords);
	    srcp += srcwidth;
	    if(destptr > maxptr)	
	    	destptr = (CARD32*)xf86AccelInfoRec.CPUToScreenColorExpandBase;
        }   
	h -= maxlines;
	y += maxlines; 
    }

    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand(x, y, w, h, skipleft);  

    while(h--) {
        destptr = MoveDWORDS(destptr, (CARD32*)srcp, dwords);
	srcp += srcwidth;
	if(destptr > maxptr)	
	    destptr = (CARD32*)xf86AccelInfoRec.CPUToScreenColorExpandBase;
    }    

    SET_SYNC_FLAG;	
}
