/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/CirrusClk.h,v 3.2 1996/02/04 09:06:41 dawes Exp $ */





/* $XConsortium: CirrusClk.h /main/3 1995/11/12 19:29:50 kaleb $ */

int CirrusFindClock( 
#if NeedFunctionPrototypes
   int freq, int max_clock, int *num_out, int *den_out, int *usemclk_out
#endif
);     

int CirrusSetClock(
#if NeedFunctionPrototypes
   int freq
#endif
);
