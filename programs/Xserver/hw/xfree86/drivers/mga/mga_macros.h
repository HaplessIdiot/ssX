/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_macros.h,v 1.2 1998/07/25 16:55:53 dawes Exp $ */

#ifndef _MGA_MACROS_H_
#define _MGA_MACROS_H_

#if PSZ == 8
#define REPLICATE(r) r &= 0xFF; r |= r << 8; r |= r << 16
#elif PSZ == 16
#define REPLICATE(r) r &= 0xFFFF; r |= r << 16
#elif PSZ == 24
#define REPLICATE(r) r &= 0xFFFFFF; r |= r << 24
#else
#define REPLICATE(r) /* */
#endif

#define RGBEQUAL(c) (!((((c) >> 8) ^ (c)) & 0xffff)) 

#define WAITFIFO(n) if(!pMga->UsePCIRetry) \
	{while(INREG8(MGAREG_FIFOSTATUS) < (n));}

#define XYADDRESS(x,y) ((y) * pScrn->displayWidth + (x) + pMga->YDstOrg)

#define MAKEDMAINDEX(index)  ((((index) >> 2) & 0x7f) | (((index) >> 6) & 0x80))

#define DMAINDICIES(one,two,three,four)	\
	( MAKEDMAINDEX(one) | \
	 (MAKEDMAINDEX(two) << 8) | \
	 (MAKEDMAINDEX(three) << 16) | \
 	 (MAKEDMAINDEX(four) << 24) ) 

#if PSZ == 24
#define SET_PLANEMASK(p) /**/
#else
#define SET_PLANEMASK(p) \
	if((p) != pMga->PlaneMask) { \
	   pMga->PlaneMask = (p); \
	   REPLICATE((p)); \
	   OUTREG(MGAREG_PLNWT,(p)); \
	}
#endif


#define SET_FOREGROUND(c) \
	if((c) != pMga->FgColor) { \
	   pMga->FgColor = (c); \
	   REPLICATE((c)); \
	   OUTREG(MGAREG_FCOL,(c)); \
	}

#define SET_BACKGROUND(c) \
	if((c) != pMga->BgColor) { \
	   pMga->BgColor = (c); \
	   REPLICATE((c)); \
	   OUTREG(MGAREG_BCOL,(c)); \
	}

#define DISABLE_CLIP() { \
	pMga->AccelFlags &= ~CLIPPER_ON; \
	WAITFIFO(1); \
	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000); }

#endif /* _MGA_MACROS_H_ */
