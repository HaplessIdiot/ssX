/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_macros.h,v 1.1.2.1 1998/05/23 09:31:31 dawes Exp $ */

#ifndef _MGA_MACROS_H_
#define _MGA_MACROS_H_

#if PSZ == 8
#define REPLICATE(r) r &= 0xFF; r |= r << 8; r |= r << 16
#elif PSZ == 16
#define REPLICATE(r) r &= 0xFFFF; r |= r << 16
#else
#define REPLICATE(r) /* */
#endif

#if PSZ == 24
#define REPLICATE24(r) r &= 0xFFFFFF; r |= r << 24
#else
#define REPLICATE24(r) REPLICATE(r)
#endif

#define RGBEQUAL(c) (!(((c >> 8) ^ c) & 0xffff)) 

#define WAITFIFO(n) if(!pMga->UsePCIRetry) \
	{while(INREG8(MGAREG_FIFOSTATUS) < (n));}

#define XYADDRESS(x,y) ((y) * pScrn->displayWidth + (x) + pMga->YDstOrg)

#define MAKEDMAINDEX(index)  ((((index) >> 2) & 0x7f) | (((index) >> 6) & 0x80))

#define DMAINDICIES(one,two,three,four)	\
	( MAKEDMAINDEX(one) | \
	 (MAKEDMAINDEX(two) << 8) | \
	 (MAKEDMAINDEX(three) << 16) | \
 	 (MAKEDMAINDEX(four) << 24) ) 


#endif /* _MGA_MACROS_H_ */
